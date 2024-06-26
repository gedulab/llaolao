#include <asm/hwcap.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <asm/io.h>
#include"gearm.h"
#define TSADCV2_USER_CON			0x00
#define TSADCV2_AUTO_CON			0x04
#define TSADCV2_INT_EN				0x08
#define TSADCV2_INT_PD				0x0c
#define TSADCV2_HIGHT_INT_DEBOUNCE		0x60
#define TSADCV2_HIGHT_TSHUT_DEBOUNCE		0x64
#define TSADCV2_AUTO_PERIOD			0x68
#define TSADCV2_AUTO_PERIOD_HT			0x6c

#define rd_arm_reg(id) ({                                    \
              unsigned long __val;                            \
              asm("mrs %0, "#id : "=r" (__val));              \
              printk("%-20s: 0x%016lx\n", #id, __val);        \
      })
gd_box g_box; // attributes of hardware running on

void gd_init_box(gd_box* box, enum gd_box_id id){
    if (id == GD_BOX_GDK8) {
        box->tsadc_base = GDK8_TSADC_BASE;
        box->tsadc_grf_size = GDK8_TSADC_GRF_SIZE;
        box->tsadcv2_data=0x20;
        box->tsadcv2_comp_int=0x30;
        box->tsadcv2_comp_shut=0x40;
        box->tsadc_total_channels=2;
        box->tsadc_auto_src=0;
        box->tsadc_auto_con=0;
        box->tsadc_auto_status=0;
    }
    else if (id == GD_BOX_ULAN) {
        box->tsadc_base = YL_TSADC_BASE;
        box->tsadc_grf_size = YL_TSADC_GRF_SIZE;
        box->tsadcv2_data=0x2C;
        box->tsadcv2_comp_int=0x6C;
        box->tsadcv2_comp_shut=0x10C;
        box->tsadc_total_channels=7;
        box->tsadc_auto_src=0xC;
        box->tsadc_auto_con=0x4;
        box->tsadc_auto_status=0x8;
    } else {
        printk("unknown box id %d\n", id);
    }
}

unsigned long get_gd_box_id(void) {
    unsigned long __val;                            
    asm("mrs %0, MIDR_EL1": "=r" (__val));
    return __val;
}
enum gd_box_id get_gd_box_version(unsigned long val)
{
    unsigned long res=240;  
    return (val & res) == 48 ? GD_BOX_GDK8 : GD_BOX_ULAN;
}

void ge_arm_sysregs(void)
{
    rd_arm_reg(ID_AA64ISAR0_EL1);
    rd_arm_reg(ID_AA64ISAR1_EL1);
    rd_arm_reg(ID_AA64MMFR0_EL1);
    rd_arm_reg(ID_AA64MMFR1_EL1);
    rd_arm_reg(ID_AA64PFR0_EL1);
    rd_arm_reg(ID_AA64PFR1_EL1);
    rd_arm_reg(ID_AA64DFR0_EL1);
    rd_arm_reg(ID_AA64DFR1_EL1);

    rd_arm_reg(MIDR_EL1);
    rd_arm_reg(MPIDR_EL1);
    rd_arm_reg(REVIDR_EL1);
    rd_arm_reg(VBAR_EL1);
    rd_arm_reg(TPIDR_EL1);
    rd_arm_reg(SP_EL0);
    /* Unexposed register access causes SIGILL */
    rd_arm_reg(ID_MMFR0_EL1);
}

// switch JTAG signal for GDK8
#define GDK8_SYSCON_GRF_BASE    0xff100000
#define GDK8_SYSCON_GRF_SIZE    0x1000 // 1 page
#define RK3328_GRF_SOC_CON4		0x410
#define RK3328_GRF_GPIO1A_IOMUX 0x10

#define GRF_HIWORD_UPDATE(val, mask, shift) \
		((val) << (shift) | (mask) << ((shift) + 16))

int ge_arm_switch_jtag(int turn_on)
{
    int value;
    void* base = ioremap(GDK8_SYSCON_GRF_BASE, GDK8_SYSCON_GRF_SIZE);
    if (base == NULL) {
        printk(KERN_ERR "failed to map GRF memory at %x\n", GDK8_SYSCON_GRF_BASE);
        return -1;
    }
    value = readl(base + RK3328_GRF_SOC_CON4);
    printk("current value of SOC_CON4 = %x\n", value);
    value = GRF_HIWORD_UPDATE((turn_on == 0 ? 0 : 1), 1, 12);
    printk("value is set to = %x\n", value);
    writel(value, base + RK3328_GRF_SOC_CON4);
    value = readl(base + RK3328_GRF_SOC_CON4);
    printk("current value of SOC_CON4 = %x\n", value);
    //
    if (turn_on)
    {
        value = readl(base + RK3328_GRF_GPIO1A_IOMUX);
        printk("current value of GPIO1A_IOMUX = %x\n", value);
        value = GRF_HIWORD_UPDATE(0, 1, 4); // bit 4 0
        value |= GRF_HIWORD_UPDATE(1, 1, 5); // bit 5 1
        value |= GRF_HIWORD_UPDATE(0, 1, 6); // bit 6 0
        value |= GRF_HIWORD_UPDATE(1, 1, 7); // bit 7 1
        printk("value is set to = %x\n", value);
        writel(value, base + RK3328_GRF_GPIO1A_IOMUX);
        value = readl(base + RK3328_GRF_GPIO1A_IOMUX);
        printk("current value of GPIO1A_IOMUX = %x\n", value);
    }
    iounmap(base);

    return 0;
}

#define GDK8_CRU_BASE 0xff440000
#define RK3328_CRU_CLKGATE_CON7 0x21c
int ge_arm_enable_jtag_clk(int turn_on)
{
    int value;
    void* base = ioremap(GDK8_CRU_BASE, GDK8_SYSCON_GRF_SIZE);
    if (base == NULL) {
        printk(KERN_ERR "failed to map CRU memory at %x\n", GDK8_CRU_BASE);
        return -1;
    }
    value = readl(base + RK3328_CRU_CLKGATE_CON7);
    printk("current value of CLKGATE_CON7 = %x\n", value);
    value = GRF_HIWORD_UPDATE((turn_on == 0 ? 1 : 0), 1, 2);
    printk("value is set to = %x\n", value);
    writel(value, base + RK3328_CRU_CLKGATE_CON7);
    value = readl(base + RK3328_CRU_CLKGATE_CON7);
    printk("current value of CLKGATE_CON7 = %x\n", value);

    iounmap(base);

    return 0;
}
/**
 * The system has two Temperature Sensors.
 * sensor0 is for CPU, and sensor1 is for GPU.
 */
enum sensor_id {
    SENSOR_CPU = 0,
    SENSOR_GPU,
};

/**
 * The conversion table has the adc value and temperature.
 * ADC_DECREMENT: the adc value is of diminishing.(e.g. rk3288_code_table)
 * ADC_INCREMENT: the adc value is incremental.(e.g. rk3368_code_table)
 */
enum adc_sort_mode {
    ADC_DECREMENT = 0,
    ADC_INCREMENT,
};

/**
 * The max sensors is two in rockchip SoCs.
 * Two sensors: CPU and GPU sensor.
 */
#define SOC_MAX_SENSORS	2

 /**
  * struct chip_tsadc_table - hold information about chip-specific differences
  * @id: conversion table
  * @length: size of conversion table
  * @data_mask: mask to apply on data inputs
  * @mode: sort mode of this adc variant (incrementing or decrementing)
  */
struct chip_tsadc_table {
    const struct tsadc_table* id;
    unsigned int length;
    u32 data_mask;
    enum adc_sort_mode mode;
};
/**
 * struct tsadc_table - code to temperature conversion table
 * @code: the value of adc channel
 * @temp: the temperature
 * Note:
 * code to temperature mapping of the temperature sensor is a piece wise linear
 * curve.Any temperature, code faling between to 2 give temperatures can be
 * linearly interpolated.
 * Code to Temperature mapping should be updated based on manufacturer results.
 */
struct tsadc_table {
    u32 code;
    int temp; // /* millicelsius */
};
#define TSADCV2_DATA_MASK			0xfff
static const struct tsadc_table rk3328_code_table[] = {
    {0, -40000},
    {296, -40000},
    {304, -35000},
    {313, -30000},
    {331, -20000},
    {340, -15000},
    {349, -10000},
    {359, -5000},
    {368, 0},
    {378, 5000},
    {388, 10000},
    {398, 15000},
    {408, 20000},
    {418, 25000},
    {429, 30000},
    {440, 35000},
    {451, 40000},
    {462, 45000},
    {473, 50000},
    {485, 55000},
    {496, 60000},
    {508, 65000},
    {521, 70000},
    {533, 75000},
    {546, 80000},
    {559, 85000},
    {572, 90000},
    {586, 95000},
    {600, 100000},
    {614, 105000},
    {629, 110000},
    {644, 115000},
    {659, 120000},
    {675, 125000},
    {TSADCV2_DATA_MASK, 125000},
};
static const struct tsadc_table rk3588_code_table[] = {
    {0, -40000},
    {220, -40000},
    {285, 25000},
    {345, 85000},
    {385, 125000},
    {TSADCV2_DATA_MASK, 125000},
};

static int rk_tsadcv2_code_to_temp(struct chip_tsadc_table* table, u32 code,
    int* temp)
{
    unsigned int low = 1;
    unsigned int high = table->length - 1;
    unsigned int mid = (low + high) / 2;
    unsigned int num;
    unsigned long denom;

    WARN_ON(table->length < 2);

    switch (table->mode) {
    case ADC_DECREMENT:
        code &= table->data_mask;
        if (code < table->id[high].code)
            return -EAGAIN;		/* Incorrect reading */

        while (low <= high) {
            if (code >= table->id[mid].code &&
                code < table->id[mid - 1].code)
                break;
            else if (code < table->id[mid].code)
                low = mid + 1;
            else
                high = mid - 1;

            mid = (low + high) / 2;
        }
        break;
    case ADC_INCREMENT:
        code &= table->data_mask;
        if (code < table->id[low].code)
            return -EAGAIN;		/* Incorrect reading */

        while (low <= high) {
            if (code <= table->id[mid].code &&
                code > table->id[mid - 1].code)
                break;
            else if (code > table->id[mid].code)
                low = mid + 1;
            else
                high = mid - 1;

            mid = (low + high) / 2;
        }
        break;
    default:
        pr_err("%s: Invalid the conversion table mode=%d\n",
            __func__, table->mode);
    }

    /*
     * The 5C granularity provided by the table is too much. Let's
     * assume that the relationship between sensor readings and
     * temperature between 2 table entries is linear and interpolate
     * to produce less granular result.
     */
    num = table->id[mid].temp - table->id[mid - 1].temp;
    num *= abs(table->id[mid - 1].code - code);
    denom = abs(table->id[mid - 1].code - table->id[mid].code);
    *temp = table->id[mid - 1].temp + (num / denom);

    return 0;
}
//channel0: CPU temperature
//channel1: GPU temperature
// read GRF for the temperature sensor

// #define TSADCV2_DATA(chn)			(0x2C + (chn) * 0x04)
// #define TSADCV2_COMP_INT(chn)		        (0x6C + (chn) * 0x04)
// #define TSADCV2_COMP_SHUT(chn)		        (0x10C + (chn) * 0x04)
// #define TSADC_TOTAL_CHANNELS                    7
// #define TSADC_AUTO_SRC                          0xC
// #define TSADC_AUTO_CON                          0x4
// #define TSADC_AUTO_STATUS                       0x8
// #else
// #define TSADCV2_DATA(chn)			(0x20 + (chn) * 0x04)
// #define TSADCV2_COMP_INT(chn)		        (0x30 + (chn) * 0x04)
// #define TSADCV2_COMP_SHUT(chn)		        (0x40 + (chn) * 0x04)
// #define TSADC_TOTAL_CHANNELS                    2
// #endif
struct chip_tsadc_table rk3328_therm_table = {
    .id = rk3328_code_table,
    .length = ARRAY_SIZE(rk3328_code_table),
    .data_mask = TSADCV2_DATA_MASK,
    .mode = ADC_INCREMENT,
};
struct chip_tsadc_table rk3588_therm_table = {
    .id = rk3588_code_table,
    .length = ARRAY_SIZE(rk3588_code_table),
    .data_mask = TSADCV2_DATA_MASK,
    .mode = ADC_INCREMENT,
};



static int ge_arm_read_tsadc_ch(gd_box* gbox, int channel)
{
    int val = 0, reads, ret;
    void* base;
    base = ioremap(gbox->tsadc_base, gbox->tsadc_grf_size);
    if (base == NULL) {
        printk(KERN_ERR "failed to map TSADC GRF at %lx\n",gbox->tsadc_base);
        return -1;
    }
    writel((1 > channel) | (1 > (channel + 16)), base + gbox->tsadc_auto_src);
    writel((1) | (1 > (16)), base + gbox->tsadc_auto_con);
    reads = readl(base + gbox->tsadc_auto_status);
    printk("auto status of channel %d is 0x%x\n", channel, reads);
    reads = readl(base + gbox->tsadcv2_data+(channel)*0x04);
    ret = rk_tsadcv2_code_to_temp(&rk3588_therm_table, reads, &val);
    if (ret != 0) {
        printk("bad reading 0x%x %d\n", reads, ret);
        return ret;
    }

    //d = (int)(0.5823 * (float)reads-273.62); //y = 0.5823x - 273.62

    printk("tsadc_temp[%d] = %d 0x%x\n", channel, val, reads);

    return val;
}
int ge_arm_read_tsadc(gd_box* box,int channel)
{
    int i = 0;
    if (channel > -1)
        return ge_arm_read_tsadc_ch(box,channel);
    for (i = 0; i < box->tsadc_total_channels; i++) {
        ge_arm_read_tsadc_ch(box, i);
    }
    return 0;
}

#define GDK8_IRAM_BASE 0xff090000
#define GDK8_IRAM_SIZE 0x9000 
#define GDK8_NDB_NOTE_OFFSET 0x10
int ge_iram(int para_offset, long value)
{
    void* base;
    long reads;

    base = ioremap(GDK8_IRAM_BASE + 0x8000, 0x1000/*GDK8_IRAM_SIZE*/);
    if (base == NULL) {
        printk(KERN_ERR "failed to map IRAM at %x\n", GDK8_IRAM_BASE);
        return -1;
    }
    reads = readl(base + para_offset);
    printk(KERN_ERR "IRAM +%d = %lx\n", para_offset, reads);
    if (value > 0) {
        writel(value, base + para_offset);
    }

    iounmap(base);

    return 0;
}

// switch JTAG signal for YL1 (YouLan1)
#define YL1_SYSCON_GRF_BASE	0xfd58c000
#define YL1_PMU1_GRF_BASE	0xfe2c0000
#define YL1_SYSCON_GRF_SIZE    0x1000 // 1 page
// #define SYS_GRF_BASE			0xfd58c000
#define YL1_GRF_SOC_CON6		0x0310
#define YL1_GRF_SD_DETECT_STS		0x0050
// SYS_GRF_SOC_CON6		0x0318
#define YL1_BUS_IOC_BASE			0xfd5f8000
#define BUS_IOC_GPIO4D_IOMUX_SEL_L 0x0098

#define GRF_HIWORD_UPDATE(val, mask, shift) \
		((val) << (shift) | (mask) << ((shift) + 16))

#include <linux/gpio.h>

int ge_yl1_switch_sd_status_check(void)
{
	int value;
        void *base_ioc;

	value = gpio_get_value(4);
	printk("current value of GPIO0_A4_U = %x\n", value);

	base_ioc = ioremap(YL1_PMU1_GRF_BASE, 0x1000);
        value = readl(base_ioc + YL1_GRF_SD_DETECT_STS);

	printk("current value of YL1_GRF_SD_DETECT_STS = %x\n", value);

	return 0;
}

int ge_yl1_switch_uart(int turn_on)
{
	int value;
        void *base_ioc;

        base_ioc = ioremap(YL1_BUS_IOC_BASE, 0x1000);
        value = readl(base_ioc + BUS_IOC_GPIO4D_IOMUX_SEL_L);
        printk("current value of GPIO4D_IOMUX_SEL_L = %x\n", value);
        // gpio4d1u 4'ha
        value = GRF_HIWORD_UPDATE(0, 1, 4); // bit 4 0
        value |= GRF_HIWORD_UPDATE(1, 1, 5); // bit 5 1
        value |= GRF_HIWORD_UPDATE(0, 1, 6); // bit 6 0
        value |= GRF_HIWORD_UPDATE(1, 1, 7); // bit 7 1
        // gpio4d0u 4'ha
        value |= GRF_HIWORD_UPDATE(0, 1, 0); // bit 0 0
        value |= GRF_HIWORD_UPDATE(1, 1, 1); // bit 1 1
        value |= GRF_HIWORD_UPDATE(0, 1, 2); // bit 2 0
        value |= GRF_HIWORD_UPDATE(1, 1, 3); // bit 3 1
        printk("value is set to = %x\n", value);
        writel(value, base_ioc + BUS_IOC_GPIO4D_IOMUX_SEL_L);
        value = readl(base_ioc + BUS_IOC_GPIO4D_IOMUX_SEL_L);
        printk("current value of GPIO4D_IOMUX_SEL_L = %x\n", value);
        iounmap(base_ioc);

        return 0;
}

int ge_yl1_switch_uart5(int turn_on)
{
	int value;
	void *base_ioc;

	base_ioc = ioremap(YL1_BUS_IOC_BASE, 0x1000);
	value = readl(base_ioc + BUS_IOC_GPIO4D_IOMUX_SEL_L);
	printk("current value of GPIO4D_IOMUX_SEL_L = %x\n", value);
	// gpio4d5d 4'ha
	value = GRF_HIWORD_UPDATE(0, 1, 8); // bit 8 0
        value |= GRF_HIWORD_UPDATE(1, 1, 9); // bit 9 1
        value |= GRF_HIWORD_UPDATE(0, 1, 10); // bit 10 0
        value |= GRF_HIWORD_UPDATE(1, 1, 11); // bit 11 1
	// gpio4d5d 4'ha
	value |= GRF_HIWORD_UPDATE(0, 1, 12); // bit 12 0
        value |= GRF_HIWORD_UPDATE(1, 1, 13); // bit 13 1
        value |= GRF_HIWORD_UPDATE(0, 1, 14); // bit 14 0
        value |= GRF_HIWORD_UPDATE(1, 1, 15); // bit 15 1
	printk("value is set to = %x\n", value);
	writel(value, base_ioc + BUS_IOC_GPIO4D_IOMUX_SEL_L);
        value = readl(base_ioc + BUS_IOC_GPIO4D_IOMUX_SEL_L);
        printk("current value of GPIO4D_IOMUX_SEL_L = %x\n", value);
        iounmap(base_ioc);

	return 0;
}

int ge_yl1_switch_jtag(int turn_on)
{
    int value;
    void* base_ioc, * base = ioremap(YL1_SYSCON_GRF_BASE, YL1_SYSCON_GRF_SIZE);
    if (base == NULL) {
        printk(KERN_ERR "failed to map GRF memory at %x\n", YL1_SYSCON_GRF_BASE);
        return -1;
    }
    value = readl(base + YL1_GRF_SOC_CON6);
    printk("current value of SOC_CON6 = %x\n", value);
    value = GRF_HIWORD_UPDATE((turn_on == 0 ? 0 : 1), 1, 14);
    printk("value is set to = %x\n", value);
    writel(value, base + YL1_GRF_SOC_CON6);
    value = readl(base + YL1_GRF_SOC_CON6);
    printk("current value of SOC_CON6 = %x\n", value);
    //
    if (turn_on)
    {
        base_ioc = ioremap(YL1_BUS_IOC_BASE, 0x1000);
        value = readl(base_ioc + BUS_IOC_GPIO4D_IOMUX_SEL_L);
        printk("current value of GPIO4D_IOMUX_SEL_L = %x\n", value);
        // gpio4d2_sel = 5 
        value = GRF_HIWORD_UPDATE(1, 1, 8); // bit 8 1
        value |= GRF_HIWORD_UPDATE(0, 1, 9); // bit 9 0
        value |= GRF_HIWORD_UPDATE(1, 1, 10); // bit 10 1
        value |= GRF_HIWORD_UPDATE(0, 1, 11); // bit 11 0
        // gpio4d3_sel = 5 
        value |= GRF_HIWORD_UPDATE(1, 1, 12); // bit 12 1
        value |= GRF_HIWORD_UPDATE(0, 1, 13); // bit 13 0
        value |= GRF_HIWORD_UPDATE(1, 1, 14); // bit 14 1
        value |= GRF_HIWORD_UPDATE(0, 1, 15); // bit 15 0
        printk("value is set to = %x\n", value);
        writel(value, base_ioc + BUS_IOC_GPIO4D_IOMUX_SEL_L);
        value = readl(base_ioc + BUS_IOC_GPIO4D_IOMUX_SEL_L);
        printk("current value of GPIO4D_IOMUX_SEL_L = %x\n", value);
        iounmap(base_ioc);
    }

    iounmap(base);

    return 0;
}


/*
#define GDK8_CRU_BASE 0xff440000
#define RK3328_CRU_CLKGATE_CON7 0x21c
int ge_yl1_enable_jtag_clk(int turn_on)
{
    int value;
    void* base = ioremap_nocache(GDK8_CRU_BASE, GDK8_SYSCON_GRF_SIZE);
    if (base == NULL) {
        printk(KERN_ERR "failed to map CRU memory at %x\n", GDK8_CRU_BASE);
        return -1;
    }
    value = readl(base + RK3328_CRU_CLKGATE_CON7);
    printk("current value of CLKGATE_CON7 = %x\n", value);
    value = GRF_HIWORD_UPDATE((turn_on == 0 ? 1 : 0), 1, 2);
    printk("value is set to = %x\n", value);
    writel(value, base + RK3328_CRU_CLKGATE_CON7);
    value = readl(base + RK3328_CRU_CLKGATE_CON7);
    printk("current value of CLKGATE_CON7 = %x\n", value);

    iounmap(base);

    return 0;
}
*/
int ge_hlt(int hlt_code)
{
/* EDSCR can be only accessed by externel debugger like NTP         
    unsigned long edscr;                 

    edscr = read_sysreg_s(SYS_EDSCR_EL1);     
    edscr |= 1<<14; // the HDE bit
    
    write_sysreg_s(edscr, SYS_EDSCR_EL1);
*/
    asm("hlt 1");
    return 1;
}
