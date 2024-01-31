#ifndef __ALL_CONFIG_H
#define __ALL_CONFIG_H

#include <Arduino.h>              // define I/O functions
#include <stdint.h>
#include <stdio.h>                // define I/O functions
#include <stdlib.h>
/*#include "string.h"
#include "SPIFFS.h"
#include "FS.h"
#include <Wire.h>       */          // 

//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "freertos/semphr.h"
#include "usb/usb_host.h"
//#include "esp_log.h"
//#include "esp_intr_alloc.h"
//#include "usb/usb_host.h"
////#include "rtl-sdr.h"
//#include "librtlsdr.h"





typedef struct
{
    usb_host_client_handle_t client_hdl;
    uint8_t dev_addr;
    usb_device_handle_t dev_hdl;
    uint32_t actions;
} class_driver_t;


typedef struct
{
    bool is_adsb;
    uint8_t* response_buf;
    bool is_done;
    bool is_success;
    int bytes_transferred;
    usb_transfer_t* transfer;
} class_adsb_dev;

static const char *TAG_ADSB = "ADSB";
static const char* TAG_MAIN = "MAIN";
static const char* TAG_DAEMON = "DAEMON";
static const char* TAG_CLASS = "CLASS";
static const char* TAG_RTLSDR = "RTLSDR";




#endif
