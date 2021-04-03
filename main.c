#include <stdio.h>
#include "wsclient.h"

int main()
{
    const char *ws_server_address = "192.168.22.117";
    const char *ws_path = "/ws";
    int ws_port = 8080;

    int rc = wsclient_create(ws_server_address, ws_path, ws_port);
    if ( 0 != rc) {
        fprintf(stderr, "Cannot create a websocket client.\n");
        return -1;
    }

    wsclient_run();

    wsclient_free();
    
    return 0;
}