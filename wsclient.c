#include "wsclient.h"
#include <libwebsockets.h>
#include <stdlib.h>
#include <string.h>

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

void wsclient_run(wsclient_t *wsc)
{

}