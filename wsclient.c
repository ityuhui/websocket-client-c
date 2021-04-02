#include "wsclient.h"
#include <libwebsockets.h>
#include <stdlib.h>
#include <string.h>

static struct lws_context *context;

wsclient_t* wsclient_create(const char *ws_url, int ws_port)
{
    wsclient_t *wsc = (wsclient_t *)calloc(1, sizeof(wsclient_t));
    if (ws_url) {
        wsc->url = strdup(ws_url);
    }
    wsc->port = ws_port;
    return wsc;
}

void wsclient_free(wsclient_t *wsc)
{
    if (!wsc) {
        return ;
    }

    if (wsc->url) {
        free(wsc->url);
        wsc->url = NULL;
    }

    free(wsc);
}

/*
 * Scheduled sul callback that starts the connection attempt
 */

static void connect_client(lws_sorted_usec_list_t *sul)
{
	struct my_conn *mco = lws_container_of(sul, struct my_conn, sul);
	struct lws_client_connect_info i;

	memset(&i, 0, sizeof(i));

	i.context = context;
	i.port = port;
	i.address = server_address;
	i.path = "/";
	i.host = i.address;
	i.origin = i.address;
	i.ssl_connection = ssl_connection;
	i.protocol = pro;
	i.local_protocol_name = "lws-minimal-client";
	i.pwsi = &mco->wsi;
	i.retry_and_idle_policy = &retry;
	i.userdata = mco;

	if (!lws_client_connect_via_info(&i))
		/*
		 * Failed... schedule a retry... we can't use the _retry_wsi()
		 * convenience wrapper api here because no valid wsi at this
		 * point.
		 */
		if (lws_retry_sul_schedule(context, 0, sul, &retry,
					   connect_client, &mco->retry_count)) {
			lwsl_err("%s: connection attempts exhausted\n", __func__);
			interrupted = 1;
		}
}

static int callback_wsclient(struct lws *wsi, enum lws_callback_reasons reason,
		 void *user, void *in, size_t len)
{
	struct my_conn *mco = (struct my_conn *)user;

	switch (reason) {

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		lwsl_err("CLIENT_CONNECTION_ERROR: %s\n",
			 in ? (char *)in : "(null)");
		goto do_retry;
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		lwsl_hexdump_notice(in, len);
		break;

	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		lwsl_user("%s: established\n", __func__);
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
	if (lws_retry_sul_schedule_retry_wsi(wsi, &mco->sul, connect_client,
					     &mco->retry_count)) {
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
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(struct lws_context_creation_info));

    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.port = CONTEXT_PORT_NO_LISTEN; /* we do not run any server */
    info.protocols = protocols;
    //info.client_ssl_ca_filepath = "";
    info.fd_limit_per_thread = 1 + 1 + 1;

    context = lws_create_context(&info);
    if (!context) {
		lwsl_err("wsclient init failed\n");
		return 1;
	}

    /* schedule the first client connection attempt to happen immediately */
	lws_sul_schedule(context, 0, &mco.sul, connect_client, 1);

}   