
#include "libptimc.h"

#include <cstdint>

#include "object/revertable_object.h"
#include "crypto/hash.h"
#include "config/yield_config.h"

#include "xdr/storage.h"
#include "xdr/storage_delta.h"


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
    scs::yield_config.RBO_YIELD = true;
    scs::yield_config.RBO_U128_YIELD = true;

    register_resetter(resetter);
    obj = std::make_unique<scs::RevertableBaseObject>();

    imcthread_t t1, t2;
    bool b1 = false, b2 = false;
    imcthread_create(&t1, NULL, insert_data_int, &b1);
    imcthread_create(&t2, NULL, insert_data_int, &b2);
    
    imcthread_join(t1, NULL);
    imcthread_join(t2, NULL);

    assert(b1 && b2);
}
