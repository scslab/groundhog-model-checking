#include "libptimc.h"
#include <assert.h>

static hash_t HASH = 0;
hash_t state_hasher() { return (hash_t)HASH; }

void imc_check_main(void) {
    register_hasher(state_hasher);

    int counter = 0;
    for (int i = 0; i < 5; i++) {
        HASH = (counter << 8) | i;

        counter += choose(2);
        assert(counter < 6);
    }
}
