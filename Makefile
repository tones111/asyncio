all: main

AIO_OUT_DIR=target/debug
LIB_AIO=$(AIO_OUT_DIR)/libasyncio.a

.PHONY: $(LIB_AIO)
$(LIB_AIO):
	cargo build

main: main.cpp aio.h aio.cpp $(LIB_AIO)
	g++ -ggdb main.cpp aio.cpp -o main -L $(AIO_OUT_DIR) -lasyncio

run: main
	./main