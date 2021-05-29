#include "wsclient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define WSC_ATTACH_STDIN_BUFFER_SIZE 1024

wsclient_t* wsclient_create(const char *server_address, const char *path, int ws_port, int ws_log_mask, sslConfig_t *ssl_config)
{
    wsclient_t *wsc = (wsclient_t *)calloc(1, sizeof(wsclient_t));
	if (!wsc) {
		return NULL;
	}
    if (server_address) {
        wsc->server_address = strdup(server_address);
    }
	if (path) {
        wsc->path = strdup(path);
    }
    wsc->port = ws_port;
	wsc->mode = WSC_MODE_NORMAL;
	wsc->ssl_config = ssl_config;
	wsc->log_mask = ws_log_mask;
	wsc->input_perfix = 0;
	return wsc;
}

void wsclient_free(wsclient_t *wsc)
{
    if (!wsc) {
        return ;
    }
    if (wsc->server_address) {
        free(wsc->server_address);
        wsc->server_address = NULL;
    }
	if (wsc->path) {
        free(wsc->path);
        wsc->path = NULL;
    }
	if (wsc->data_received) {
		free(wsc->data_received);
		wsc->data_received = NULL;
	}
	wsc->data_received_len = 0;
	wsc->data_callback_func = NULL;
	wsc->ssl_config = NULL;
	
    free(wsc);
	wsc = NULL;
}

static struct lws_context *context;
static int interrupted;

/*
 * The retry and backoff policy we want to use for our client connections
 */

static const uint32_t backoff_ms[] = { 1000, 2000, 3000, 4000, 5000 };

static const lws_retry_bo_t retry = {
	.retry_ms_table			= backoff_ms,
	.retry_ms_table_count		= LWS_ARRAY_SIZE(backoff_ms),
	.conceal_count			= LWS_ARRAY_SIZE(backoff_ms),

	.secs_since_valid_ping		= 3,  /* force PINGs after secs idle */
	.secs_since_valid_hangup	= 10, /* hangup after secs idle */

	.jitter_percent			= 20,
};

/*
 * Scheduled sul callback that starts the connection attempt
 */

static void connect_client(lws_sorted_usec_list_t *sul)
{
	wsclient_t *wsc = lws_container_of(sul, wsclient_t, sul);
	struct lws_client_connect_info i;

	memset(&i, 0, sizeof(i));

	i.context = context;
	i.port = wsc->port;
	i.address = wsc->server_address;
	i.path = wsc->path;
	i.host = i.address;
	i.origin = i.address;
	i.ssl_connection = wsc->ssl_config ? LCCSCF_USE_SSL : 0;
	//i.protocol = pro;
	i.local_protocol_name = "websocket-client";
	i.pwsi = &wsc->wsi;
	i.retry_and_idle_policy = &retry;
	i.userdata = wsc;

	if (!lws_client_connect_via_info(&i))
		/*
		 * Failed... schedule a retry... we can't use the _retry_wsi()
		 * convenience wrapper api here because no valid wsi at this
		 * point.
		 */
		if (lws_retry_sul_schedule(context, 0, sul, &retry,
					   connect_client, &wsc->retry_count)) {
			lwsl_err("%s: connection attempts exhausted\n", __func__);
			interrupted = 1;
		}
}

static int callback_wsclient(struct lws *wsi, enum lws_callback_reasons reason,
		 void *user, void *in, size_t len)
{
	//lwsl_user("%s: callback reason %d\n", __func__, reason);

	wsclient_t *wsc = (wsclient_t *)user;

	switch (reason) {

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		lwsl_err("CLIENT_CONNECTION_ERROR: %s\n",
			 in ? (char *)in : "(null)");
		goto do_retry;
		break;

	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		lwsl_user("%s: established\n", __func__);
		lws_callback_on_writable(wsi);
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		lwsl_hexdump_notice(in, len);
		if (wsc->data_received_len >0 &&
			wsc->data_received) {
			free(wsc->data_received);
			wsc->data_received = NULL;
			wsc->data_received_len = 0;
		}
		wsc->data_received_len = len;
		wsc->data_received = strdup((char *)in);
		if (wsc->data_callback_func) {
			wsc->data_callback_func(&wsc->data_received, &wsc->data_received_len);
		}
		break;

	case LWS_CALLBACK_CLIENT_WRITEABLE:
		if (wsc->data_to_send_len >0 && wsc->data_to_send) {
			int m = lws_write(wsi, wsc->data_to_send + LWS_PRE, wsc->data_to_send_len, LWS_WRITE_BINARY);
			if (m < wsc->data_to_send_len) {
				lwsl_err("sending message failed: %d\n", m);
				return -1;
			} else {
				lwsl_user("succeed to send message : %d\n", m);
			}
			if (wsc->data_to_send) {
				free(wsc->data_to_send);
				wsc->data_to_send = NULL;
				wsc->data_to_send_len = 0;
			}
		}
		break;
		
	case LWS_CALLBACK_CLIENT_CLOSED:
		//goto do_retry;
		interrupted = 1;
		break;
	default:
		break;
	}

	return lws_callback_http_dummy(wsi, reason, user, in, len);

do_retry:
	/*
	 * retry the connection to keep it nailed up
	 *
	 * For this example, we try to conceal any problem for one set of
	 * backoff retries and then exit the app.
	 *
	 * If you set retry.conceal_count to be larger than the number of
	 * elements in the backoff table, it will never give up and keep
	 * retrying at the last backoff delay plus the random jitter amount.
	 */
	if (lws_retry_sul_schedule_retry_wsi(wsi, &wsc->sul, connect_client,
					     &wsc->retry_count)) {
		lwsl_err("%s: connection attempts exhausted.\n", __func__);
		interrupted = 1;
	}

	return 0;
}

static const struct lws_protocols protocols[] = {
	{ "wsclient", callback_wsclient, 0, 0, },
	{ NULL, NULL, 0, 0 }
};

void read_from_stdin(wsclient_t *wsc)
{
	char wsc_attach_stdin_buffer[WSC_ATTACH_STDIN_BUFFER_SIZE];
	memset(wsc_attach_stdin_buffer, 0, sizeof(wsc_attach_stdin_buffer));
	
	int flag, newflag;
	flag = fcntl(STDIN_FILENO, F_GETFL);
	newflag = flag | O_NONBLOCK;
	fcntl(STDIN_FILENO, F_SETFL, newflag);
	fgets(wsc_attach_stdin_buffer,sizeof(wsc_attach_stdin_buffer) -1,stdin);

	if (wsc_attach_stdin_buffer && strlen(wsc_attach_stdin_buffer) >0) {
		wsc->data_to_send_len = strlen(wsc_attach_stdin_buffer) + 1 /*wsc->input_perfix*/;
		wsc->data_to_send = (char *)calloc(LWS_PRE + 1 /* wsc->input_perfix */ + wsc->data_to_send_len + 1, 1);
		wsc->data_to_send[LWS_PRE] = wsc->input_perfix;
		strncpy(wsc->data_to_send + LWS_PRE + 1, wsc_attach_stdin_buffer, strlen(wsc_attach_stdin_buffer));
	}

	fcntl(STDIN_FILENO, F_SETFL, flag);
}

int wsclient_run(wsclient_t *wsc, int mode, const char *data_send)
{
	int n = 0;
	lws_set_log_level(wsc->log_mask, NULL);

	wsc->mode = mode;

	if (data_send && strlen(data_send) >0) {
		wsc->data_to_send_len = strlen(data_send);
		wsc->data_to_send = (char *)calloc(wsc->data_to_send_len + 1 + LWS_PRE, 1);
		strncpy(wsc->data_to_send + LWS_PRE, data_send, wsc->data_to_send_len);
	}

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(struct lws_context_creation_info));

    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.port = CONTEXT_PORT_NO_LISTEN; /* we do not run any server */
    info.protocols = protocols;
    info.fd_limit_per_thread = 1 + 1 + 1;
	if (wsc->ssl_config) {
		info.client_ssl_ca_filepath =  wsc->ssl_config->CACertFile;
		info.client_ssl_private_key_filepath = wsc->ssl_config->clientKeyFile;
		info.client_ssl_cert_filepath = wsc->ssl_config->clientCertFile;
	}

    context = lws_create_context(&info);
    if (!context) {
		lwsl_err("%s: wsclient init failed.\n", __func__);
		return 1;
	}

    /* schedule the first client connection attempt to happen immediately */
	lws_sul_schedule(context, 0, &wsc->sul, connect_client, 1);

	while (n >= 0 && !interrupted) {
		if (WSC_MODE_IT == wsc->mode) {
			read_from_stdin(wsc);
		}
		n = lws_service(context, 0);
	}

	lws_context_destroy(context);
	lwsl_user("%s: wsclient completed.\n", __func__);

	return 0;
}