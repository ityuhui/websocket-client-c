#include <stdio.h>
#include "wsclient.h"

int main()
{
    const char *ws_url = "192.168.22.117/ws";
    int ws_port = 8080;

    wsclient_t *wsc = wsclient_create(ws_url, ws_port);
    if (!wsc) {
        fprintf(stderr, "Cannot create a websocket client.\n");
        return -1;
    }

    wsclient_run(wsc);

    wsclient_free(wsc);
    return 0;
}