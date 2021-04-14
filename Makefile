INCLUDE:=
LIBS:=-lwebsockets -L/usr/local/lib
CFLAGS:=-g
BIN1:= simple_wsc
BIN2:= k8s_wsc
OBJECTS1:=simple_wsc.o wsclient.o
OBJECTS2:=k8s_wsc.o wsclient.o
OBJECTS:=simple_wsc.o k8s_wsc.o wsclient.o

all: $(BIN1) $(BIN2)

$(BIN1): $(OBJECTS1)
	$(CC) -o $(BIN1) $(OBJECTS1) $(LIBS)

$(BIN2): $(OBJECTS2)
	$(CC) -o $(BIN2) $(OBJECTS2) $(LIBS)

$(OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@
.PHONY : clean
clean:
	rm $(BIN1) $(BIN2) $(OBJECTS)