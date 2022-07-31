#ifndef RP2040_CLOCKS_H
#define RP2040_CLOCKS_H

#include <stdint.h>

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_CLOCKS "rp2040_soc.clocks"

OBJECT_DECLARE_SIMPLE_TYPE(RP2040ClocksState, RP2040_CLOCKS);

struct RP2040ClocksState 
{
    SysBusDevice parent_obj;

    MemoryRegion mmio;
};

#endif /* RP2040_CLOCKS_H */