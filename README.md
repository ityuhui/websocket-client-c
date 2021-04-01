# websocket-client-c
A demo of websocket c client using [libwebsockets](https://libwebsockets.org/)

It can communicate with a websocket server [webserver-go](https://github.com/ityuhui/webserver-go)

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
```shell
./websocket_client
```