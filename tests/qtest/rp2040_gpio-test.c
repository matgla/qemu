#include "qemu/osdep.h"

#include "libqtest-single.h"

#include "qapi/qmp/qnum.h"
#include "qapi/qmp/qstring.h"

#define RP2040_GPIO_BASE        0x40014000
#define RP2040_GPIO_QSPI_BASE   0x40018000
#define RP2040_GPIO_PINS        30
#define RP2040_GPIO_QSPI_PINS   6

typedef union RP2040GpioStatus {
    struct {
        uint32_t __reserved0 : 8;
        uint32_t outfromperi : 1;
        uint32_t outtopad    : 1;
        uint32_t __reserved1 : 2;
        uint32_t oefromperi  : 1;
        uint32_t oetopad     : 1;
        uint32_t __reserved2 : 3;
        uint32_t infrompad   : 1;
        uint32_t __reserved3 : 1;
        uint32_t intoperi    : 1;
        uint32_t __reserved4 : 4;
        uint32_t irqfrompad  : 1;
        uint32_t __reserved5 : 1;
        uint32_t irqtoproc   : 1;
        uint32_t __reserved6 : 5;

    };

    uint32_t _value;
} RP2040GpioStatus;

typedef enum RP2040InterruptOverride 
{
    NotInvert = 0x0,
    Invert = 0x1,
    DriveLow = 0x2,
    DriveHigh = 0x3,
} RP2040InterruptOverride;

typedef RP2040InterruptOverride RP2040InputOverride; 

enum RP2040OutputEnableOverride
{
    DriveEnableFromFunction = 0x0,
    DriveEnableFromInverseFunction = 0x1,
    DisableOutput = 0x2,
    EnableOutput = 0x3,
};

enum RP2040OutputOverride
{
    DriveOutputFromFunction = 0x0,
    DriveOutputFromInverseFunction = 0x1,
    DriveOutputLow = 0x2,
    DriveOutputHigh = 0x3,
};

enum RP2040PinFunction
{
    FUN_NONE,
    SPI0_RX,
    SPI0_CSn,
    SPI0_SCK,
    SPI0_TX,
    SPI1_RX,
    SPI1_CSn,
    SPI1_SCK,
    SPI1_TX,
    UART0_TX,
    UART0_RX,
    UART0_CTS,
    UART0_RTS,
    UART1_TX,
    UART1_RX,
    UART1_CTS,
    UART1_RTS,
    I2C0_SDA,
    I2C0_SCL,
    I2C1_SDA,
    I2C1_SCL,
    PWM0_A,
    PWM0_B,
    PWM1_A,
    PWM1_B,
    PWM2_A,
    PWM2_B,
    PWM3_A,
    PWM3_B,
    PWM4_A,
    PWM4_B,
    PWM5_A,
    PWM5_B,
    PWM6_A,
    PWM6_B,
    PWM7_A,
    PWM7_B,
    SIO,
    PIO0,
    PIO1,
    CLK_GPIN0,
    CLK_GPOUT0,
    CLK_GPIN1,
    CLK_GPOUT1,
    CLK_GPOUT2,
    CLK_GPOUT3,
    USB_OVCUR_DET,
    USB_VBUS_DET,
    USB_VBUS_EN,
    XIP_SCK,
    XIP_CSn,
    XIP_SD0,
    XIP_SD1,
    XIP_SD2,
    XIP_SD3
};

static const uint8_t function_table[RP2040_GPIO_PINS][10] = {
/* Pin      |    F0    |    F1    |     F2    |    F3    |   F4   |  F5 |  F6  |  F7  |    F8      |      F9       |*/   
/* 0 */     { FUN_NONE , SPI0_RX  , UART0_TX  , I2C0_SDA , PWM0_A , SIO , PIO0 , PIO1 , FUN_NONE   , USB_OVCUR_DET },
/* 1 */     { FUN_NONE , SPI0_CSn , UART0_RX  , I2C0_SCL , PWM0_B , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_DET  },
/* 2 */     { FUN_NONE , SPI0_SCK , UART0_CTS , I2C1_SDA , PWM1_A , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_EN   },
/* 3 */     { FUN_NONE , SPI0_TX  , UART0_RTS , I2C1_SCL , PWM1_B , SIO , PIO0 , PIO1 , FUN_NONE   , USB_OVCUR_DET },
/* 4 */     { FUN_NONE , SPI0_RX  , UART1_TX  , I2C0_SDA , PWM2_A , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_DET  },
/* 5 */     { FUN_NONE , SPI0_CSn , UART1_RX  , I2C0_SCL , PWM2_B , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_EN   },
/* 6 */     { FUN_NONE , SPI0_SCK , UART1_CTS , I2C1_SDA , PWM3_A , SIO , PIO0 , PIO1 , FUN_NONE   , USB_OVCUR_DET },
/* 7 */     { FUN_NONE , SPI0_TX  , UART1_RTS , I2C1_SCL , PWM3_B , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_DET  },
/* 8 */     { FUN_NONE , SPI1_RX  , UART1_TX  , I2C0_SDA , PWM4_A , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_EN   },
/* 9 */     { FUN_NONE , SPI1_CSn , UART1_RX  , I2C0_SCL , PWM4_B , SIO , PIO0 , PIO1 , FUN_NONE   , USB_OVCUR_DET },
/* 10 */    { FUN_NONE , SPI1_SCK , UART1_CTS , I2C1_SDA , PWM5_A , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_DET  },
/* 11 */    { FUN_NONE , SPI1_TX  , UART1_RTS , I2C1_SCL , PWM5_B , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_EN   },
/* 12 */    { FUN_NONE , SPI1_RX  , UART0_TX  , I2C0_SDA , PWM6_A , SIO , PIO0 , PIO1 , FUN_NONE   , USB_OVCUR_DET },
/* 13 */    { FUN_NONE , SPI1_CSn , UART0_RX  , I2C0_SCL , PWM6_B , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_DET  },
/* 14 */    { FUN_NONE , SPI1_SCK , UART0_CTS , I2C1_SDA , PWM7_A , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_EN   },
/* 15 */    { FUN_NONE , SPI1_TX  , UART0_RTS , I2C1_SCL , PWM7_B , SIO , PIO0 , PIO1 , FUN_NONE   , USB_OVCUR_DET },
/* 16 */    { FUN_NONE , SPI0_RX  , UART0_TX  , I2C0_SDA , PWM0_A , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_DET  },
/* 17 */    { FUN_NONE , SPI0_CSn , UART0_RX  , I2C0_SCL , PWM0_B , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_EN   },
/* 18 */    { FUN_NONE , SPI0_SCK , UART0_CTS , I2C1_SDA , PWM1_A , SIO , PIO0 , PIO1 , FUN_NONE   , USB_OVCUR_DET },
/* 19 */    { FUN_NONE , SPI0_TX  , UART0_RTS , I2C1_SCL , PWM1_B , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_DET  },
/* 20 */    { FUN_NONE , SPI0_RX  , UART1_TX  , I2C0_SDA , PWM2_A , SIO , PIO0 , PIO1 , CLK_GPIN0  , USB_VBUS_EN   },
/* 21 */    { FUN_NONE , SPI0_CSn , UART1_RX  , I2C0_SCL , PWM2_B , SIO , PIO0 , PIO1 , CLK_GPOUT0 , USB_OVCUR_DET },
/* 22 */    { FUN_NONE , SPI0_SCK , UART1_CTS , I2C1_SDA , PWM3_A , SIO , PIO0 , PIO1 , CLK_GPIN1  , USB_VBUS_DET  },
/* 23 */    { FUN_NONE , SPI0_TX  , UART1_RTS , I2C1_SCL , PWM3_B , SIO , PIO0 , PIO1 , CLK_GPOUT1 , USB_VBUS_EN   },
/* 24 */    { FUN_NONE , SPI1_RX  , UART1_TX  , I2C0_SDA , PWM4_A , SIO , PIO0 , PIO1 , CLK_GPOUT2 , USB_OVCUR_DET },
/* 25 */    { FUN_NONE , SPI1_CSn , UART1_RX  , I2C0_SCL , PWM4_B , SIO , PIO0 , PIO1 , CLK_GPOUT3 , USB_VBUS_DET  },
/* 26 */    { FUN_NONE , SPI1_SCK , UART1_CTS , I2C1_SDA , PWM5_A , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_EN   },
/* 27 */    { FUN_NONE , SPI1_TX  , UART1_RTS , I2C1_SCL , PWM5_B , SIO , PIO0 , PIO1 , FUN_NONE   , USB_OVCUR_DET },
/* 28 */    { FUN_NONE , SPI1_RX  , UART0_TX  , I2C0_SDA , PWM6_A , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_DET  },
/* 29 */    { FUN_NONE , SPI1_CSn , UART0_RX  , I2C0_SCL , PWM6_B , SIO , PIO0 , PIO1 , FUN_NONE   , USB_VBUS_EN   },
};

// static void rp2040_test_set_gpio_func(int pin, uint8_t function)
// {

// }

static void test_initialization_of_gpio_registers(const void *test_state)
{
    QTestState *s = (QTestState *)test_state;
    int i = 0;
    /* GPIO Tests */
    for (i = 0; i < RP2040_GPIO_PINS; ++i)
    {
        // test status registers 
        g_assert_cmphex(qtest_readl(s, RP2040_GPIO_BASE + 0x008 * i), ==, 0x00000000);

        // test control registers
        g_assert_cmphex(qtest_readl(s, RP2040_GPIO_BASE + 0x008 * i + 0x004), ==, 0x0000001f);
    }

    /* GPIO Tests */
    for (i = 0; i < RP2040_GPIO_QSPI_PINS - 2; ++i)
    {
        // test status registers 
        g_assert_cmphex(qtest_readl(s, RP2040_GPIO_QSPI_BASE + 0x008 * i), ==, 0x00000000);

        // test control registers
        g_assert_cmphex(qtest_readl(s, RP2040_GPIO_QSPI_BASE + 0x008 * i + 0x004), ==, 0x0000001f);

    }
}

typedef union RP2040GpioControl {
    struct {
        uint32_t funcsel       : 5; /* type RW, reset 0x1f */
        uint32_t __reserved4   : 3;
        uint32_t outover       : 2; /* type RW, reset 0x0 */
        uint32_t __reserved3   : 2;
        uint32_t oeover        : 2; /* type RW, reset 0x0 */
        uint32_t __reserved2   : 2;
        uint32_t inover        : 2; /* type RW, reset 0x0 */
        uint32_t __reserved1   : 10; 
        uint32_t irqover       : 2; /* type RW, reset 0x0 */
        uint32_t __reserved0   : 2; 
    };
    uint32_t _value;
} RP2040GpioControl;

static void test_driving_of_gpio_pins(const void *test_state)
{
    QTestState *s = (QTestState *)test_state;
    int i;

    RP2040GpioControl ctrl = {
        .funcsel = 0x1f,
        .outover = 0x03,
        .oeover = 0x03,
        .inover = 0x00,
        .irqover = 0x00
    };

    qtest_writel(s, RP2040_GPIO_BASE + 0x004, ctrl._value);
    g_assert_cmphex(qtest_readl(s, RP2040_GPIO_BASE + 0x004), ==, ctrl._value);

    qtest_irq_intercept_out(s, "/machine/rp2040/gpio");
    
    for (i = 0; i < 256; ++i) 
    {
        fprintf(stderr, "%d: %d\n", i, qtest_get_irq(s, i));
    }

    // QDict *response;
    // int value;

    // response = qtest_qmp(s, "{ 'execute': 'qom-get', 'arguments': { 'path': '/machine/rp2040/gpio', 'property': 'out[0]'} }");
    // g_assert(qdict_haskey(response, "return"));
    // // value = qnum_get_uint(qobject_to(QNum, qdict_get(response, "return")));

    // type = qobject_type(qdict_get(response, "return"));
    // if (type == QTYPE_QSTRING)
    // {
    //     fprintf(stderr, "QSTRING: %s\n", qobject_to(QString, qdict_get(response, "return"))->string);
    // }
    // else 
    // {
    //     fprintf(stderr, "OTHER\n");
    // }
    // qobject_unref(response);

    // g_assert_true(qtest_qom_get_bool(s, "/machine/rp2040/gpio", "out[0]"));
}

int main(int argc, char **argv)
{
    int r; 
    QTestState *s;

    g_test_init(&argc, &argv, NULL);
    g_test_set_nonfatal_assertions();
    
    s = qtest_init("-machine raspi_pico");
    qtest_add_data_func("/rp2040/gpio/registers_initialization", s, test_initialization_of_gpio_registers);
    qtest_add_data_func("/rp2040/gpio/drive_output", s, test_driving_of_gpio_pins);

    // qtest_irq_intercept_in(global_qtest, "/machine/rp2040/gpio.out[0]");
    r = g_test_run();
    qtest_quit(s);

    (void)function_table;
    return r;
}