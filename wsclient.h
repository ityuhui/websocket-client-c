#ifndef _WSCLIENT_H
#define _WSCLIENT_H

#ifdef  __cplusplus
extern "C" {
#endif

int wsclient_create(const char *, const char *, int);
void wsclient_free();
int wsclient_run();

#ifdef  __cplusplus
}
#endif

#endif /* _WSCLIENT_H */