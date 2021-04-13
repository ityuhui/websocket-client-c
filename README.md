# websocket-client-c
A demo of websocket c client using [libwebsockets](https://libwebsockets.org/)

It can communicate with a websocket server [webserver-go](https://github.com/ityuhui/webserver-go)

The status is:  **Work In Progress**

## Prepare libwebsockets

```shell
git clone https://libwebsockets.org/repo/libwebsockets
cd libwebsockets/
mkdir build
cd build/
cmake ..
make
make install
```

## Build
```shell
make
```

## Run websocket server
```shell
git clone https://github.com/ityuhui/webserver-go.git
cd webserver-go/
go run main.go
```
## Run

### Export env
```
export LD_LIBRARY_PATH=/usr/local/lib/
```

### Run exec example 
```shell
./websocket_client
```

### Run attach example
```shell
./websocket_client -1
```
