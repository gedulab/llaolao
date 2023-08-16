#ifndef __GE_ARM_H__
#define __GE_ARM_H__
#define GDK8_TSADC_BASE             0xff250000
#define GDK8_TSADC_GRF_SIZE         0x1000 // 4KB
#define YL_TSADC_BASE 0xfec00000
#define YL_TSADC_GRF_SIZE 0x10000 // 64KB
typedef struct gd_box {
    long tsadc_base;
    int tsadc_grf_size;
    int tsadcv2_data;
    int tsadcv2_comp_int;
    int tsadcv2_comp_shut;
    int tsadc_total_channels;
    int tsadc_auto_src;
    int tsadc_auto_con;       
    int tsadc_auto_status;
}gd_box;

enum gd_box_id {
    GD_BOX_UNKNOWN = -1,	
    GD_BOX_GDK8, 
    GD_BOX_ULAN,// gdk8 = 0xA53, ulan = 0xA55
};

unsigned long get_gd_box_id(void);
enum gd_box_id get_gd_box_version(unsigned long val);
void ge_arm_sysregs(void);
int ge_arm_switch_jtag(int turn_on);
int ge_arm_enable_jtag_clk(int turn_on);
int ge_arm_read_tsadc(gd_box *box,int channel);
int ge_iram(int para);
int ge_yl1_switch_jtag(int turn_on);

#endif 
