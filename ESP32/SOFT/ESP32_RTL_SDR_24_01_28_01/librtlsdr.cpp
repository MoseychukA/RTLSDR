
#include "librtlsdr.h"


int r820t_init(void* dev)
{
    rtlsdr_dev_t* devt = (rtlsdr_dev_t*)dev;
    devt->r82xx_p.rtl_dev = dev;

    if (devt->tuner_type == RTLSDR_TUNER_R828D)
    {
        devt->r82xx_c.i2c_addr = R828D_I2C_ADDR;
        devt->r82xx_c.rafael_chip = CHIP_R828D;
    }
    else
    {
        devt->r82xx_c.i2c_addr = R820T_I2C_ADDR;
        devt->r82xx_c.rafael_chip = CHIP_R820T;
    }

    rtlsdr_get_xtal_freq(devt, NULL, &devt->r82xx_c.xtal);

    devt->r82xx_c.max_i2c_msg_len = 8;
    devt->r82xx_c.use_predetect = 0;
    devt->r82xx_p.cfg = &devt->r82xx_c;

    return r82xx_init(&devt->r82xx_p);
}

int r820t_exit(void* dev)
{
    rtlsdr_dev_t* devt = (rtlsdr_dev_t*)dev;
    return r82xx_standby(&devt->r82xx_p);
}

int r820t_set_freq(void* dev, uint32_t freq)
{
    rtlsdr_dev_t* devt = (rtlsdr_dev_t*)dev;
    return r82xx_set_freq(&devt->r82xx_p, freq);
}

int r820t_set_bw(void* dev, int bw)
{
    int r;
    rtlsdr_dev_t* devt = (rtlsdr_dev_t*)dev;

    r = r82xx_set_bandwidth(&devt->r82xx_p, bw, devt->rate);
    if (r < 0)
        return r;
    r = rtlsdr_set_if_freq(devt, r);
    if (r)
        return r;
    return rtlsdr_set_center_freq(devt, devt->freq);
}

int r820t_set_gain(void* dev, int gain)
{
    rtlsdr_dev_t* devt = (rtlsdr_dev_t*)dev;
    return r82xx_set_gain(&devt->r82xx_p, 1, gain);
}
int r820t_set_gain_mode(void* dev, int manual)
{
    rtlsdr_dev_t* devt = (rtlsdr_dev_t*)dev;
    return r82xx_set_gain(&devt->r82xx_p, manual, 0);
}