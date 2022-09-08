/*
 * RP2040 Utils
 *
 * Copyright (c) 2022 Mateusz Stadnik <matgla@live.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "hw/misc/rp2040_utils.h"

RP2040AccessType rp2040_get_access_type(const hwaddr addr)
{
    if ((addr & 0x3000) == 0x0000) {
        return RP2040_NORMAL_ACCESS;
    } else if (addr & 0x1000) {
        return RP2040_XOR_ON_WRITE;
    } else if (addr & 0x2000) {
        return RP2040_SET_ON_WRITE;
    }

    return RP2040_CLEAN_ON_WRITE;
}

void rp2040_write_to_register(RP2040AccessType type, uint32_t *reg,
    uint32_t value)
{
    switch (type) {
    case RP2040_NORMAL_ACCESS:
        *reg = value;
        break;
    case RP2040_XOR_ON_WRITE:
        *reg ^= value;
        break;
    case RP2040_SET_ON_WRITE:
        *reg |= value;
        break;
    case RP2040_CLEAN_ON_WRITE:
        *reg &= ~(value);
        break;
    }
}