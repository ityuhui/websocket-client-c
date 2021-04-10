#include <stdio.h>
#include "wsclient.h"


void attach_data_callback(void **p_data_received, long *p_data_received_len)
{
    printf("Received %ld bytes: %s\n", *p_data_received_len, (char *)*p_data_received);
}

void exec(wsclient_t *wsc, const char *cmd)
{
    wsclient_run(wsc, cmd);
    printf("Received %ld bytes: %s\n", wsc->data_received_len, (char *)wsc->data_received);
}

void attach(wsclient_t *wsc)
{
    wsc->data_callback_func = attach_data_callback;
    wsclient_run(wsc, NULL);
}

int main()
{
    const char *ws_server_address = "192.168.22.117";
    const char *ws_path = "/ws";
    int ws_port = 8080;

    wsclient_t *wsc = wsclient_create(ws_server_address, ws_path, ws_port);
    if (!wsc) {
        fprintf(stderr, "Cannot create a websocket client.\n");
        return -1;
    }

    //exec(wsc, "hello");
    attach(wsc);

    wsclient_free(wsc);
    
    return 0;
}