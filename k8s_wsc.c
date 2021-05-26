#include <stdio.h>
#include "wsclient.h"


void attach_data_callback(void **p_data_received, long *p_data_received_len)
{
    printf("%s: Received %ld bytes: %s", __func__, *p_data_received_len, (char *)*p_data_received);
}

void exec(wsclient_t *wsc, const char *cmd)
{
    wsclient_run(wsc, cmd);
    printf("%s: Received %ld bytes: %s\n", __func__, wsc->data_received_len, (char *)wsc->data_received);
}

void attach(wsclient_t *wsc)
{
    wsc->mode = WSC_MODE_ATTACH;
    wsc->data_callback_func = attach_data_callback;
    wsclient_run(wsc, NULL);
}

int main(int argc, char *argv[])
{
    const char *ws_server_address = "192.168.22.117";
    const char *ws_path = "/api/v1/namespaces/default/pods/test-pod-8/exec?command=bash&container=my-container&stdin=true&stdout=true&tty=true";
    int ws_port = 6443;

    sslConfig_t *sslconfig = (sslConfig_t *) calloc(sizeof(sslConfig_t), 1);
    sslconfig->CACertFile = strdup("/root/k8s_cert/kubeconfig-7ucPpD");
    sslconfig->clientCertFile = strdup("/root/k8s_cert/kubeconfig-81noCw");
    sslconfig->clientKeyFile = strdup("/root/k8s_cert/kubeconfig-Ilh604");
    sslconfig->insecureSkipTlsVerify = 0;

    wsclient_t *wsc = wsclient_create(ws_server_address, ws_path, ws_port, sslconfig);
    if (!wsc) {
        fprintf(stderr, "Cannot create a websocket client.\n");
        return -1;
    }

    if ( 1 == argc ) {
        exec(wsc, "ls");
    } else {
        attach(wsc);
    }

    wsclient_free(wsc);
    
    return 0;
}