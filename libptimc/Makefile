.PHONY: clean

CFLAGS += -g
CFLAGS += -DN_WORKERS=8

all: build/examples/simple_race build/examples/global_state

build/examples/%: examples/%.c build/libptimc.a
	@ mkdir -p build/examples
	$(CC) -I./ -Ilibimc/ $(CFLAGS) -o $@ $^ -fPIC

# https://stackoverflow.com/questions/2734719/how-to-compile-a-static-library-in-linux
build/libptimc.a: build/libptimc.o build/libimc/libimc_linux.o build/libimc/master.o build/libimc/worker.o
	ar rcs $@ $^

build/%.o: %.c
	@ mkdir -p $(dir $@)
	$(CC) -Ilibimc/ -c $(CFLAGS) -o $@ $^ -fPIC

clean:
	rm -rf build
