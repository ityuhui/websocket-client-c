# websocket-client-c
A demo of websocket c client using [libwebsockets](https://libwebsockets.org/)

It can communicate with 
* A simple websocket server [webserver-go](https://github.com/ityuhui/webserver-go) or
* Kubernetes API server.

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


## Run

### Export env
```
export LD_LIBRARY_PATH=/usr/local/lib/
```

### Run simple_wsc
#### Run websocket server
```shell
git clone https://github.com/ityuhui/webserver-go.git
cd webserver-go/
go run main.go
```
#### Exec 
```shell
./simple_wsc
```

#### Attach
```shell
./simple_wsc -a
```

### Run k8s_wsc

#### Ensure Kubernetes cluster is ready

#### Normal mode 
```shell
./k8s_wsc $pod_name "ls /"
./k8s_wsc $pod_name $container_name "ls /"
```

#### IT mode
```shell
./k8s_wsc -it $pod_name $container_name "bash"
```
