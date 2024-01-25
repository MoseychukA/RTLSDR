#pragma once
#ifndef _MEMORY_H
#define _MEMORY_H
//--------------------------------------------------------------------------------------------------------------------------------
#include <Arduino.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "usb/usb_host.h"
#include "rtl-sdr.h"

#define CLIENT_NUM_EVENT_MSG 5

#define ACTION_OPEN_DEV 0x01
#define ACTION_GET_DEV_INFO 0x02
#define ACTION_GET_DEV_DESC 0x04
#define ACTION_GET_CONFIG_DESC 0x08
#define ACTION_GET_STR_DESC 0x10
#define ACTION_CLOSE_DEV 0x20
#define ACTION_EXIT 0x40

#define DEFAULT_BUF_LENGTH (14 * 16384)

typedef struct
{
    usb_host_client_handle_t client_hdl;
    uint8_t dev_addr;
    usb_device_handle_t dev_hdl;
    uint32_t actions;
} class_driver_t;

static const char *TAG = "CLASS";
static rtlsdr_dev_t *rtldev = NULL;
//--------------------------------------------------------------------------------------------------------------------------------

static void client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg);
static void action_open_dev(class_driver_t *driver_obj);
static void action_get_info(class_driver_t *driver_obj);
void action_get_dev_desc1(class_driver_t *driver_obj);
static void action_get_config_desc(class_driver_t *driver_obj);
static void action_get_str_desc(class_driver_t *driver_obj)
static void aciton_close_dev(class_driver_t *driver_obj);
void class_driver_task(void *arg);


//--------------------------------------------------------------------------------------------------------------------------------

#endif