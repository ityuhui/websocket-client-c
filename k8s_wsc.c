#include <stdio.h>
#include "wsclient.h"

#define WS_PATH_BUFFER_SIZE 1024

void attach_data_callback(void **p_data_received, long *p_data_received_len)
{
    printf("%s: Received %ld bytes: %s", __func__, *p_data_received_len, (char *)*p_data_received);
}

void exec(wsclient_t *wsc, wsc_mode_t mode, const char *cmd)
{
    wsclient_run(wsc, mode, cmd);
    printf("%s: Received %ld bytes: \n%s\n", __func__, wsc->data_received_len, (char *)wsc->data_received);
}

void attach(wsclient_t *wsc)
{
    wsc->data_callback_func = attach_data_callback;
    wsclient_run(wsc, WSC_MODE_IT, NULL);
}

int main(int argc, char *argv[])
{

    if (argc < 3) {
        printf("Usage sample: ./k8s_wsc --pod=test-pod-8 --container=my-container --command=ls\n");
        printf("Usage sample: ./k8s_wsc -it --pod=test-pod-8 --container=my-container --command=bash\n");
        return -1;
    }

    char *pod_name = NULL;
    char *container_name = NULL;
    char *command = NULL;
    int i;
    for (i = 1; i < argc; i++) {
        if (strstr(argv[i], "--pod")) {
            pod_name = argv[i] + 6;
        } else if (strstr(argv[i], "--container")) {
            container_name = argv[i] + 2;
        } else if (strstr(argv[i], "--command")) {
            command = argv[i] + 2;
        }
    }

    const char *ws_server_address = "192.168.22.117";

    //const char *ws_path_template = "/api/v1/namespaces/default/pods/%s/exec?command=%s&stdin=true&stdout=true";
    const char *ws_path_template_with_container = "/api/v1/namespaces/default/pods/%s/exec?%s&%s&stdin=true&stdout=true";
    
    char ws_path[WS_PATH_BUFFER_SIZE];
    memset(ws_path, 0, sizeof(ws_path));
    snprintf(ws_path, WS_PATH_BUFFER_SIZE, ws_path_template_with_container, pod_name, container_name, command);
    printf("ws_path=%s\n", ws_path);

    int ws_port = 6443;

    sslConfig_t *sslconfig = (sslConfig_t *) calloc(sizeof(sslConfig_t), 1);
    sslconfig->CACertFile = strdup("/root/k8s_cert/kubeconfig-7ucPpD");
    sslconfig->clientCertFile = strdup("/root/k8s_cert/kubeconfig-81noCw");
    sslconfig->clientKeyFile = strdup("/root/k8s_cert/kubeconfig-Ilh604");
    sslconfig->insecureSkipTlsVerify = 0;

    int ws_log_mask = LLL_ERR | LLL_WARN /*| LLL_USER | LLL_NOTICE | LLL_CLIENT*/;
    wsclient_t *wsc = wsclient_create(ws_server_address, ws_path, ws_port, ws_log_mask, sslconfig);
    if (!wsc) {
        fprintf(stderr, "Cannot create a websocket client.\n");
        return -1;
    }

    int mode = WSC_MODE_NORMAL;
    if ( 2 == argc &&
        0 == strcmp(argv[1], "-it")) {
        mode = WSC_MODE_IT;
    } 
    exec(wsc, mode, NULL);

    wsclient_free(wsc);
    
    return 0;
}