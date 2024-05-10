CFLAGS += -g
CFLAGS += -DN_WORKERS=8

build/examples/simple_race: examples/simple_race.c build/libptimc.so
	@ mkdir -p build/examples
	$(CC) -I./ -Ilibimc/ $(CFLAGS) -o $@ $^ -fPIC

build/libptimc.so: libptimc.c libimc/libimc_linux.c libimc/master.c libimc/worker.c
	@ mkdir -p build
	$(CC) -Ilibimc/ -shared $(CFLAGS) -o $@ $^ -fPIC
