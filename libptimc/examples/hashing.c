#include "libptimc.h"
#include <assert.h>

void imc_check_main(void) {
    int counter = 0;
    for (int i = 0; i < 5; i++) {
        counter += choose(2, (counter << 8) | i);
        assert(counter < 6);
    }
}
