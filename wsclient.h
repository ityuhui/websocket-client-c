#ifndef _WSCLIENT_H
#define _WSCLIENT_H

#ifdef  __cplusplus
extern "C" {
#endif

typedef void (*data_callback_func)(void **, long *);

typedef enum wsc_run_mode {
    WSC_RUN_MODE_EXEC = 0,
    WSC_RUN_MODE_ATTACH
} wsc_run_mode;

int wsclient_create(const char *, const char *, int, data_callback_func, wsc_run_mode);
void wsclient_free();
int wsclient_run(const char *);

#ifdef  __cplusplus
}
#endif

#endif /* _WSCLIENT_H */