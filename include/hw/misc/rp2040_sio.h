#ifndef RP2040_SIO_H
#define RP2040_SIO_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_SIO "rp2040.sio"

OBJECT_DECLARE_SIMPLE_TYPE(RP2040SIOState, RP2040_SIO);


struct RP2040SIOState 
{
    SysBusDevice parent_obj;

    MemoryRegion mmio;
};

#endif /* RP2040_SIO_H */