#include <stdio.h>
#include "wsclient.h"

void data_callback(void **p_data_received, long *p_data_received_len)
{
    printf("Received %ld bytes: %s\n", *p_data_received_len, (char *)*p_data_received);
}

int main()
{
    const char *ws_server_address = "192.168.22.117";
    const char *ws_path = "/ws";
    int ws_port = 8080;

    int rc = wsclient_create(ws_server_address, ws_path, ws_port, data_callback, WSC_RUN_MODE_EXEC);
    if ( 0 != rc) {
        fprintf(stderr, "Cannot create a websocket client.\n");
        return -1;
    }

    const char *exec_cmd = "hello";
    wsclient_run(exec_cmd);


    //wsclient_attach();

    wsclient_free();
    
    return 0;
}