build/libptimc.so: libptimc.c libimc/libimc_linux.c libimc/master.c libimc/worker.c
	@ mkdir -p build
	$(CC) -Ilibimc/ -shared $(CFLAGS) -o $@ $^ -fPIC
