all: main

#AIO_OUT_DIR=target/x86_64-unknown-linux-gnu/debug
AIO_OUT_DIR=target/x86_64-unknown-linux-gnu/release
LIB_AIO=$(AIO_OUT_DIR)/libasyncio.a

.PHONY: $(LIB_AIO)
$(LIB_AIO):
	#RUSTFLAGS=-Zsanitizer=thread cargo +nightly -Zbuild-std build --target x86_64-unknown-linux-gnu
	RUSTFLAGS=-Zsanitizer=thread cargo +nightly -Zbuild-std build --target x86_64-unknown-linux-gnu --release

main: main.cpp aio.h aio.cpp $(LIB_AIO)
	g++ -fsanitize=thread -O3 main.cpp aio.cpp -o main -L $(AIO_OUT_DIR) -lasyncio

run: main
	./main