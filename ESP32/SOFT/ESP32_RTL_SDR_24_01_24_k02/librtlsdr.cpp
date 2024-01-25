#include "librtlsdr.h"
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "usb/usb_host.h"


#ifndef _WIN32
#include <unistd.h>
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

static const char* TAG_ADSB = "ADSB";

static class_adsb_dev* adsbdev;













int rtlsdr_open(rtlsdr_dev_t** out_dev, uint8_t index, usb_host_client_handle_t client_hdl)
{
    int r;
    rtlsdr_dev_t* dev = NULL;
    uint8_t reg;

    dev = (rtlsdr_dev_t*)malloc(sizeof(rtlsdr_dev_t));
    class_driver_t* driver_obj = (class_driver_t*)calloc(1, sizeof(class_driver_t));
    if (NULL == dev)
        return -ENOMEM;

    memset(dev, 0, sizeof(rtlsdr_dev_t));
    memcpy(dev->fir, fir_default, sizeof(fir_default));

    dev->dev_lost = 1;

    driver_obj->client_hdl = client_hdl;
    ESP_ERROR_CHECK(usb_host_device_open(driver_obj->client_hdl, index, &driver_obj->dev_hdl));
    dev->driver_obj = driver_obj;
    ESP_ERROR_CHECK(usb_host_interface_claim(dev->driver_obj->client_hdl, dev->driver_obj->dev_hdl, 0, 0));
    dev->rtl_xtal = DEF_RTL_XTAL_FREQ;

//    init_adsb_dev();
//    /* perform a dummy write, if it fails, reset the device */
//    if (rtlsdr_write_reg(dev, USBB, USB_SYSCTL, 0x09, 1) < 0)
//    {
//        fprintf(stderr, "Resetting device...\n");
//        // libusb_reset_device(dev->devh);
//    }
//
//    rtlsdr_init_baseband(dev);
//    dev->dev_lost = 0;
//
//    /* Probe tuners */
//    rtlsdr_set_i2c_repeater(dev, 1);
//
//    reg = rtlsdr_i2c_read_reg(dev, R820T_I2C_ADDR, R82XX_CHECK_ADDR);
//    ESP_LOGI(TAG_ADSB, "rtl device number %d", reg);
//    fprintf(stderr, "rtlsdr_i2c_read_reg R82XX_CHECK_ADDR setting done\n");
//    if (reg == R82XX_CHECK_VAL)
//    {
//        fprintf(stderr, "Found Rafael Micro R820T tuner\n");
//        dev->tuner_type = RTLSDR_TUNER_R820T;
//        goto found;
//    }
//
//found:
//    /* use the rtl clock value by default */
//    dev->tun_xtal = dev->rtl_xtal;
//    dev->tuner = &tuners[dev->tuner_type];
//
//    switch (dev->tuner_type)
//    {
//    case RTLSDR_TUNER_R828D:
//        dev->tun_xtal = R828D_XTAL_FREQ;
//        /* fall-through */
//    case RTLSDR_TUNER_R820T:
//        /* disable Zero-IF mode */
//        rtlsdr_demod_write_reg(dev, 1, 0xb1, 0x1a, 1);
//
//        /* only enable In-phase ADC input */
//        rtlsdr_demod_write_reg(dev, 0, 0x08, 0x4d, 1);
//
//        /* the R82XX use 3.57 MHz IF for the DVB-T 6 MHz mode, and
//         * 4.57 MHz for the 8 MHz mode */
//        rtlsdr_set_if_freq(dev, R82XX_IF_FREQ);
//
//        /* enable spectrum inversion */
//        rtlsdr_demod_write_reg(dev, 1, 0x15, 0x01, 1);
//        break;
//    case RTLSDR_TUNER_UNKNOWN:
//        fprintf(stderr, "No supported tuner found\n");
//        rtlsdr_set_direct_sampling(dev, 1);
//        break;
//    default:
//        break;
//    }
//
//    if (dev->tuner->init)
//        r = dev->tuner->init(dev);
//
//    rtlsdr_set_i2c_repeater(dev, 0);

    *out_dev = dev;

    return 0;
}


