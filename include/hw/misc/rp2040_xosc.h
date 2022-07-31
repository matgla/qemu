#ifndef RP2040_XOSC_H
#define RP2040_XOSC_H

#include <stdint.h>

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_XOSC "rp2040_soc.xosc"

OBJECT_DECLARE_SIMPLE_TYPE(RP2040XOSCState, RP2040_XOSC);

struct RP2040XOSCState 
{
    SysBusDevice parent_obj;

    MemoryRegion mmio;
};

#endif /* RP2040_XOSC_H */