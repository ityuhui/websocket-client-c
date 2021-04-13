#ifndef _WSCLIENT_H
#define _WSCLIENT_H

#include <libwebsockets.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef void (*data_callback_func)(void **, long *);

typedef enum wsc_mode_t {
	WSC_MODE_EXEC = 0,
	WSC_MODE_ATTACH
} wsc_mode_t;

typedef struct wsclient_t {
    char    *server_address;
    char    *path;
    int     port;
	wsc_mode_t mode;
	char    *data_to_send;
	long    data_to_send_len;
	void 	*data_received;
	long 	data_received_len;
	data_callback_func data_callback_func;
	lws_sorted_usec_list_t	sul;	     /* schedule connection retry */
	struct lws		*wsi;	     /* related wsi if any */
	uint16_t		retry_count; /* count of consequetive retries */
} wsclient_t;

wsclient_t* wsclient_create(const char *, const char *, int);
void wsclient_free(wsclient_t *);
int wsclient_run(wsclient_t *, const char *);

#ifdef  __cplusplus
}
#endif

#endif /* _WSCLIENT_H */