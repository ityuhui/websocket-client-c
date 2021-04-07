#include "wsclient.h"
#include <libwebsockets.h>
#include <stdlib.h>
#include <string.h>

typedef struct wsclient_t {
    char    *server_address;
    char    *path;
    int     port;
} wsclient_t;

static wsclient_t *g_wsc = NULL;

int wsclient_create(const char *server_address, const char *path, int ws_port)
{
    g_wsc = (wsclient_t *)calloc(1, sizeof(wsclient_t));
	if (!g_wsc) {
		return -1;
	}
    if (server_address) {
        g_wsc->server_address = strdup(server_address);
    }
	if (path) {
        g_wsc->path = strdup(path);
    }
    g_wsc->port = ws_port;
	return 0;
}

void wsclient_free()
{
    if (!g_wsc) {
        return ;
    }

    if (g_wsc->server_address) {
        free(g_wsc->server_address);
        g_wsc->server_address = NULL;
    }

	if (g_wsc->path) {
        free(g_wsc->path);
        g_wsc->path = NULL;
    }

    free(g_wsc);
	g_wsc = NULL;
}

/*
 * This represents your object that "contains" the client connection and has
 * the client connection bound to it
 */

static struct wsclient_connection {
	lws_sorted_usec_list_t	sul;	     /* schedule connection retry */
	struct lws		*wsi;	     /* related wsi if any */
	uint16_t		retry_count; /* count of consequetive retries */
} wscc ;

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
	struct wsclient_connection *wscc = lws_container_of(sul, struct wsclient_connection, sul);
	struct lws_client_connect_info i;

	memset(&i, 0, sizeof(i));

	i.context = context;
	i.port = g_wsc->port;
	i.address = g_wsc->server_address;
	i.path = g_wsc->path;
	i.host = i.address;
	i.origin = i.address;
	//i.ssl_connection = ssl_connection;
	//i.protocol = pro;
	i.local_protocol_name = "websocket-client";
	i.pwsi = &wscc->wsi;
	i.retry_and_idle_policy = &retry;
	i.userdata = wscc;

	if (!lws_client_connect_via_info(&i))
		/*
		 * Failed... schedule a retry... we can't use the _retry_wsi()
		 * convenience wrapper api here because no valid wsi at this
		 * point.
		 */
		if (lws_retry_sul_schedule(context, 0, sul, &retry,
					   connect_client, &wscc->retry_count)) {
			lwsl_err("%s: connection attempts exhausted\n", __func__);
			interrupted = 1;
		}
}

static int callback_wsclient(struct lws *wsi, enum lws_callback_reasons reason,
		 void *user, void *in, size_t len)
{
	struct wsclient_connection *wscc = (struct wsclient_connection *)user;

	switch (reason) {

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		lwsl_err("CLIENT_CONNECTION_ERROR: %s\n",
			 in ? (char *)in : "(null)");
		goto do_retry;
		break;

	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		lwsl_user("%s: established\n", __func__);
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		lwsl_hexdump_notice(in, len);
		break;

	case LWS_CALLBACK_CLIENT_WRITEABLE:

		break;
		
	case LWS_CALLBACK_CLIENT_CLOSED:
		goto do_retry;

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
	if (lws_retry_sul_schedule_retry_wsi(wsi, &wscc->sul, connect_client,
					     &wscc->retry_count)) {
		lwsl_err("%s: connection attempts exhausted\n", __func__);
		interrupted = 1;
	}

	return 0;
}

static const struct lws_protocols protocols[] = {
	{ "wsclient", callback_wsclient, 0, 0, },
	{ NULL, NULL, 0, 0 }
};

int wsclient_run(wsclient_t *wsc)
{
	int n = 0;
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(struct lws_context_creation_info));

    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.port = CONTEXT_PORT_NO_LISTEN; /* we do not run any server */
    info.protocols = protocols;
    //info.client_ssl_ca_filepath = "";
    info.fd_limit_per_thread = 1 + 1 + 1;

    context = lws_create_context(&info);
    if (!context) {
		lwsl_err("%s: wsclient init failed\n", __func__);
		return 1;
	}

    /* schedule the first client connection attempt to happen immediately */
	lws_sul_schedule(context, 0, &wscc.sul, connect_client, 1);

	while (n >= 0 && !interrupted) {
		n = lws_service(context, 0);
	}

	lws_context_destroy(context);
	lwsl_user("%s: wsclient completed\n", __func__);

	return 0;
}   