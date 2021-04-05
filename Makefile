INCLUDE:=
LIBS:=-lwebsockets -L/usr/local/lib
CFLAGS:=-g
BIN:= websocket_client
OBJECTS:=main.o wsclient.o

all: $(OBJECTS)
	$(CC) -o $(BIN) $(OBJECTS) $(LIBS)

$(OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@
.PHONY : clean
clean:
	rm $(BIN) $(OBJECTS)