
#include "libptimc.h"

#include <cstdint>

#include "object/revertable_object.h"
#include "crypto/hash.h"
#include "config/yield_config.h"

#include "xdr/storage.h"
#include "xdr/storage_delta.h"

#include <memory>


static std::unique_ptr<scs::RevertableBaseObject> obj;

static void resetter() {
    obj.reset();
}

// check consistent type

void *insert_data_AB(void *ptr) {
    scs::StorageDeltaClass sdc;
    sdc.type(scs::ObjectType::RAW_MEMORY);
    sdc.data().push_back(0xAB);

    auto b = obj->try_set(sdc);
    if (b)
    {
        b->commit();
        *((bool*)ptr) = true;
    }
    return NULL;
}

void *insert_data_AC(void *ptr) {
    scs::StorageDeltaClass sdc;
    sdc.type(scs::ObjectType::RAW_MEMORY);
    sdc.data().push_back(0xAC);

    auto b = obj->try_set(sdc);
    if (b)
    {
        b->commit();
        *((bool*)ptr) = true;
    }
    return NULL;
}

void *insert_data_int(void* ptr)
{
    scs::StorageDeltaClass sdc;
    sdc.type(scs::ObjectType::NONNEGATIVE_INT64);
    sdc.nonnegative_int64() = 5;

    auto b = obj->try_set(sdc);
    if (b)
    {
        b->commit();
        *((bool*)ptr) = true;
    }
    return NULL;
}

void imc_check_main(void) {
    register_resetter(resetter);
    obj = std::make_unique<scs::RevertableBaseObject>();

    scs::yield_config.RBO_YIELD = true;
    scs::yield_config.RBO_U128_YIELD = true;

    imcthread_t t1, t2, t3;
    bool b1 = false, b2 = false, b3 = false;
    imcthread_create(&t1, NULL, insert_data_AB, &b1);
    imcthread_create(&t2, NULL, insert_data_AC, &b2);
    imcthread_create(&t3, NULL, insert_data_int, &b3);

    imcthread_join(t1, NULL);
    imcthread_join(t2, NULL);
    imcthread_join(t3, NULL);

    assert(b1 || b2 || b3);
    assert(!((!b1) && (!b2) && (!b3)));
}
