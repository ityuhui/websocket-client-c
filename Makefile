INCLUDE:=-I../../kubernetes/api
LIBS:=-lwebsockets
CFLAGS:=-g
BIN:= websocket_client
OBJECTS:=main.o

all: $(OBJECTS)
	$(CC) -o $(BIN) $(OBJECTS) $(LIBS)

$(OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@
.PHONY : clean
clean:
	rm $(BIN) $(OBJECTS)