#ifndef RP2040_VREG_H
#define RP2040_VREG_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_VREG "rp2040_soc.vreg"

OBJECT_DECLARE_SIMPLE_TYPE(RP2040VregState, RP2040_VREG);


struct RP2040VregState 
{
    SysBusDevice parent_obj;

    MemoryRegion mmio;
};

#endif /* RP2040_VREG_H */