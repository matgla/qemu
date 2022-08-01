#ifndef RP2040_SSI_H
#define RP2040_SSI_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_SSI "rp2040_soc.ssi"

OBJECT_DECLARE_SIMPLE_TYPE(RP2040SSIState, RP2040_SSI);

typedef union RP2040SSICtrlr0 
{
    uint32_t value;
    struct {
        uint32_t dfs : 4;
        uint32_t frf : 3;
        uint32_t scph : 1;
        uint32_t scpol : 1;
        uint32_t tmod : 2;
        uint32_t slv_oe : 1;
        uint32_t slr : 1;
        uint32_t cfs : 4;
        uint32_t dfs32 : 4;
        uint32_t spi_frf : 2;
        uint32_t _reserved : 1;
        uint32_t sste : 1;
    };
} RP2040SSICtrlr0;

struct RP2040SSIState 
{
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    RP2040SSICtrlr0 ctrlr0;
    uint32_t ctrlr1;
    uint32_t rx_fifo_count;
    uint32_t tx_fifo_count;
};

#endif /* RP2040_SIO_H */