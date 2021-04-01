#ifndef _WSCLIENT_H
#define _WSCLIENT_H

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct wsclient_t {
    char    *url;
    int     port;
} wsclient_t;

wsclient_t* wsclient_create(const char *, int);
void wsclient_free(wsclient_t *);
void wsclient_run(wsclient_t *);

#ifdef  __cplusplus
}
#endif

#endif /* _WSCLIENT_H */