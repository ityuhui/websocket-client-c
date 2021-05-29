#include <stdio.h>
#include "wsclient.h"

#define WS_PATH_BUFFER_SIZE 1024
#define URL_COMMAND_STRING_BUFFER_SIZE 1024
#define COMMAND_STRING_DELIM " "
#define COMMAND_PREFIX "&command="
#define ESC_STRING_BUFFER 8

void attach_data_callback(void **p_data_received, long *p_data_received_len)
{
    printf("%s: Received %ld bytes: %s", __func__, *p_data_received_len, (char *)*p_data_received);
}

void exec(wsclient_t *wsc, wsc_mode_t mode, const char *cmd)
{
    if ( WSC_MODE_IT == mode) {
        wsc->data_callback_func = attach_data_callback;
    }
    wsclient_run(wsc, mode, cmd);
    printf("%s: Received %ld bytes: \n%s\n", __func__, wsc->data_received_len, (char *)wsc->data_received);
}

void escapeUrlCharacter(char *escaped, int escaped_buffer_size, const char chr)
{
    if ('+' == chr) {
        strncpy(escaped, "%2B", 3);
    } else if ('"' == chr) {
        strncpy(escaped, "%22", 3);
    } else if ('%' == chr) {
        strncpy(escaped, "%25", 3);
    } else {
        snprintf(escaped, 2, "%c", chr);
    }
}

int assembleCommandUrl(char *url_command_string, int url_command_string_buffer_size, const char *original_command_string)
{
    const char *p = original_command_string;
    char esc_string[ESC_STRING_BUFFER];
    while(*p) {
        if (' ' == *p) {
            strncat(url_command_string, COMMAND_PREFIX, strlen(COMMAND_PREFIX));

        } else {
            memset(esc_string, 0, sizeof(esc_string));
            escapeUrlCharacter(esc_string, ESC_STRING_BUFFER, *p);
            strncat(url_command_string, esc_string, strlen(esc_string));
        }
        p++;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    const char *ws_path_template_without_container = "/api/v1/namespaces/default/pods/%s/exec?stdin=true&stdout=true&command=%s";
    const char *ws_path_template_with_container = "/api/v1/namespaces/default/pods/%s/exec?stdin=true&stdout=true&container=%s&command=%s";
    const char *ws_path_template_for_bash = "/api/v1/namespaces/default/pods/%s/exec?stdin=true&stdout=true&tty=true&container=%s&command=%s";

    char *pod_name = NULL;
    char *container_name = NULL;
    char *command = NULL;

    int mode = WSC_MODE_NORMAL /*WSC_MODE_IT*/;

    if ( 3 == argc) {
        pod_name = argv[1];
        command = argv[2];
    } else if ( 4 == argc ) {
        pod_name = argv[1];
        container_name = argv[2];
        command = argv[3];
    } else {
        printf("Usage sample: ./k8s_wsc test-pod-8 my-container \"ls /dev\"\n");
        printf("Usage sample: ./k8s_wsc test-pod-8 my-container \"bash\"\n");
        return -1;
    }

    char url_command_string[URL_COMMAND_STRING_BUFFER_SIZE];
    memset(url_command_string, 0, sizeof(url_command_string));

    assembleCommandUrl(url_command_string, URL_COMMAND_STRING_BUFFER_SIZE, command);
    const char *ws_server_address = "192.168.22.117";

    char ws_path[WS_PATH_BUFFER_SIZE];
    memset(ws_path, 0, sizeof(ws_path));
    if (strstr(command, "bash")) {
        mode = WSC_MODE_IT;
        snprintf(ws_path, WS_PATH_BUFFER_SIZE, ws_path_template_for_bash, pod_name, container_name, url_command_string);
    } else if ( 3 == argc) {
        snprintf(ws_path, WS_PATH_BUFFER_SIZE, ws_path_template_without_container, pod_name, url_command_string);
    } else {
        snprintf(ws_path, WS_PATH_BUFFER_SIZE, ws_path_template_with_container, pod_name, container_name, url_command_string);
    }



    printf("ws_path=%s\n", ws_path);

    int ws_port = 6443;

    sslConfig_t *sslconfig = (sslConfig_t *) calloc(sizeof(sslConfig_t), 1);
    sslconfig->CACertFile = strdup("/root/k8s_cert/kubeconfig-7ucPpD");
    sslconfig->clientCertFile = strdup("/root/k8s_cert/kubeconfig-81noCw");
    sslconfig->clientKeyFile = strdup("/root/k8s_cert/kubeconfig-Ilh604");
    sslconfig->insecureSkipTlsVerify = 0;

    int ws_log_mask = LLL_ERR | LLL_WARN /*| LLL_USER | LLL_NOTICE*/;
    wsclient_t *wsc = wsclient_create(ws_server_address, ws_path, ws_port, ws_log_mask, sslconfig);
    if (!wsc) {
        fprintf(stderr, "Cannot create a websocket client.\n");
        return -1;
    }


    exec(wsc, mode, NULL);

    wsclient_free(wsc);
    
    return 0;
}