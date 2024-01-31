/*
   SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD

   SPDX-License-Identifier: Unlicense OR CC0-1.0
*/

#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "usb/usb_host.h"
#include "rtl-sdr.h"
#include "librtlsdr.hpp"


//#include <iostream>
//#include <cstring>
#include <string.h>
//#include <fstream>




//===============================================================================================





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

static const char* TAG_CLASS = "CLASS";
static rtlsdr_dev_t *rtldev = NULL;

static void client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg)
{
  class_driver_t *driver_obj = (class_driver_t *)arg;
  esp_err_t err;
  switch (event_msg->event)
  {
    case USB_HOST_CLIENT_EVENT_NEW_DEV:
      if (driver_obj->dev_addr == 0)
      {
        driver_obj->dev_addr = event_msg->new_dev.address;

        ESP_LOGI(TAG_CLASS, "*** usb_host_device_open : %x", driver_obj->dev_addr);
        err = rtlsdr_open(&rtldev, event_msg->new_dev.address, driver_obj->client_hdl);
        if (err != ESP_OK)
        {
            ESP_LOGI(TAG_CLASS, "*** usb_host_device_open error: %x", err);
        }
        else
        {
            ESP_LOGI(TAG_CLASS, "*** usb_host_device_open OK!!");
        }

        int r;
        r = rtlsdr_set_sample_rate(rtldev, 2000000);
        if (r < 0)
        {
          fprintf(stderr, "WARNING: Failed to set sample rate.\n");
        }
        else
        {
            printf( "Sampling at %u S/s.\n", 2000000);
          //fprintf(stderr, "Sampling at %u S/s.\n", 2000000);
        }
        r = rtlsdr_set_center_freq(rtldev, 1090000000);
        if (r < 0)
        {
            printf("WARNING: Failed to set center freq.\n");
            //fprintf(stderr, "WARNING: Failed to set center freq.\n");
        }
        else
        {
          printf("Tuned to %u Hz.\n", 1090000000);
        }
        r = rtlsdr_set_tuner_gain_mode(rtldev, 0);
        if (r != 0)
        {
          printf("WARNING: Failed to set tuner gain.\n");
        }
        else
        {
          fprintf(stderr, "Tuner gain set to automatic.\n");
        }
 
        r = rtlsdr_reset_buffer(rtldev);
        if (r < 0)
        {
          fprintf(stderr, "WARNING: Failed to reset buffers.\n");
        }

        vTaskDelay(100); // Short delay to let client task spin up

        uint32_t out_block_size = DEFAULT_BUF_LENGTH;

        uint8_t* buffer = (uint8_t*)malloc(out_block_size * sizeof(uint8_t));  // Free memory: 8513103 bytes
        int n_read = 2;
        ESP_LOGI(TAG_CLASS, "[APP] Free memory: %ld bytes", esp_get_free_heap_size()); //Free memory : 8513103 bytes
        // uint32_t bytes_to_read = 0;
        while (true)
        {
            printf("**!!dev->driver_obj1 - %d\n", rtldev);
          r = rtlsdr_read_sync(rtldev, buffer, out_block_size, &n_read);                 // ??Не работает этот фрагмент
          ESP_LOGI(TAG_CLASS, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());


          if (r < 0)
          {
            fprintf(stderr, "** WARNING: sync read failed.\n");
            break;
          }

          for (int i = 0; i < out_block_size; i++)
            printf("%02X", buffer[i]);
 
          if ((uint32_t)n_read < out_block_size)
          {
            fprintf(stderr, "Short read, samples lost, exiting!\n");
            break;
          }
        }
      }
      break;
    case USB_HOST_CLIENT_EVENT_DEV_GONE:
      if (driver_obj->dev_hdl != NULL)
      {
        // Cancel any other actions and close the device next
        driver_obj->actions = ACTION_CLOSE_DEV;
      }
      break;
    default:
      // Should never occur
      abort();
  }
}

static void action_open_dev(class_driver_t *driver_obj)
{
  assert(driver_obj->dev_addr != 0);
  ESP_LOGI(TAG_CLASS, "*** Opening device at address %d", driver_obj->dev_addr);
  ESP_ERROR_CHECK(usb_host_device_open(driver_obj->client_hdl, driver_obj->dev_addr, &driver_obj->dev_hdl));
  // Get the device's information next
  driver_obj->actions &= ~ACTION_OPEN_DEV;
  driver_obj->actions |= ACTION_GET_DEV_INFO;
}

static void action_get_info(class_driver_t *driver_obj)
{
  ESP_LOGI(TAG_CLASS, "here2");
  assert(driver_obj->dev_hdl != NULL);
  ESP_LOGI(TAG_CLASS, "Getting device information");
  usb_device_info_t dev_info;
  ESP_ERROR_CHECK(usb_host_device_info(driver_obj->dev_hdl, &dev_info));
  ESP_LOGI(TAG_CLASS, "\t%s speed", (dev_info.speed == USB_SPEED_LOW) ? "Low" : "Full");
  ESP_LOGI(TAG_CLASS, "\tbConfigurationValue %d", dev_info.bConfigurationValue);
  // Todo: Print string descriptors

  // Get the device descriptor next
  driver_obj->actions &= ~ACTION_GET_DEV_INFO;
  driver_obj->actions |= ACTION_GET_DEV_DESC;
}
void action_get_dev_desc1(class_driver_t *driver_obj)
{
  assert(driver_obj->dev_hdl != NULL);
  ESP_LOGI(TAG_CLASS, "Getting device descriptor");
  const usb_device_desc_t *dev_desc;
  ESP_ERROR_CHECK(usb_host_get_device_descriptor(driver_obj->dev_hdl, &dev_desc));
  usb_print_device_descriptor(dev_desc);
  // Get the device's config descriptor next
  driver_obj->actions &= ~ACTION_GET_DEV_DESC;
  driver_obj->actions |= ACTION_GET_CONFIG_DESC;
}

static void action_get_config_desc(class_driver_t *driver_obj)
{
  assert(driver_obj->dev_hdl != NULL);
  ESP_LOGI(TAG_CLASS, "Getting config descriptor");
  const usb_config_desc_t *config_desc;
  ESP_ERROR_CHECK(usb_host_get_active_config_descriptor(driver_obj->dev_hdl, &config_desc));
  usb_print_config_descriptor(config_desc, NULL);
  // Get the device's string descriptors next
  driver_obj->actions &= ~ACTION_GET_CONFIG_DESC;
  driver_obj->actions |= ACTION_GET_STR_DESC;
}

static void action_get_str_desc(class_driver_t *driver_obj)
{
  assert(driver_obj->dev_hdl != NULL);
  usb_device_info_t dev_info;
  ESP_ERROR_CHECK(usb_host_device_info(driver_obj->dev_hdl, &dev_info));
  if (dev_info.str_desc_manufacturer)
  {
    ESP_LOGI(TAG_CLASS, "Getting Manufacturer string descriptor");
    usb_print_string_descriptor(dev_info.str_desc_manufacturer);
  }
  if (dev_info.str_desc_product)
  {
    ESP_LOGI(TAG_CLASS, "Getting Product string descriptor");
    usb_print_string_descriptor(dev_info.str_desc_product);
  }
  if (dev_info.str_desc_serial_num)
  {
    ESP_LOGI(TAG_CLASS, "Getting Serial Number string descriptor");
    usb_print_string_descriptor(dev_info.str_desc_serial_num);
  }
  // Nothing to do until the device disconnects
  driver_obj->actions &= ~ACTION_GET_STR_DESC;
}

static void aciton_close_dev(class_driver_t *driver_obj)
{
  ESP_ERROR_CHECK(usb_host_device_close(driver_obj->client_hdl, driver_obj->dev_hdl));
  driver_obj->dev_hdl = NULL;
  driver_obj->dev_addr = 0;
  // We need to exit the event handler loop
  driver_obj->actions &= ~ACTION_CLOSE_DEV;
  driver_obj->actions |= ACTION_EXIT;
}

void class_driver_task(void *arg)
{

    ESP_LOGI(TAG_CLASS, "***  Waiting for client registration1");
  SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;
  class_driver_t driver_obj = {0};
  ESP_LOGI(TAG_CLASS, "***  Waiting for client registration2");
  // Wait until daemon task has installed USB Host Library
  xSemaphoreTake(signaling_sem, portMAX_DELAY);

  ESP_LOGI(TAG_CLASS, "Waiting for client registration3");

  usb_host_client_config_t client_config = {
    .is_synchronous = false, // Synchronous clients currently not supported. Set this to false
    .max_num_event_msg = CLIENT_NUM_EVENT_MSG,
    .async = {
      .client_event_callback = client_event_cb,
      .callback_arg = (void *)&driver_obj,
    },
  };
  ESP_ERROR_CHECK(usb_host_client_register(&client_config, &driver_obj.client_hdl));


  while (1) 
  {
    if (driver_obj.actions == 0)
    {
      ESP_LOGI(TAG_CLASS, "!!  Waiting for usb_host_client_handle_events");
      usb_host_client_handle_events(driver_obj.client_hdl, portMAX_DELAY);
    }
    else
    {
      if (driver_obj.actions & ACTION_OPEN_DEV)
      {
        action_open_dev(&driver_obj);
      }
      if (driver_obj.actions & ACTION_GET_DEV_INFO)
      {
        action_get_info(&driver_obj);
      }
      if (driver_obj.actions & ACTION_GET_DEV_DESC)
      {
        action_get_dev_desc1(&driver_obj);
      }
      if (driver_obj.actions & ACTION_GET_CONFIG_DESC)
      {
        action_get_config_desc(&driver_obj);
      }
      if (driver_obj.actions & ACTION_GET_STR_DESC)
      {
        action_get_str_desc(&driver_obj);
      }
      if (driver_obj.actions & ACTION_CLOSE_DEV)
      {
        aciton_close_dev(&driver_obj);
      }
      if (driver_obj.actions & ACTION_EXIT)
      {
        break;
      }
    }
  }

  ESP_LOGI(TAG_CLASS, "Deregistering Client");
  ESP_ERROR_CHECK(usb_host_client_deregister(driver_obj.client_hdl));

  // Wait to be deleted
  xSemaphoreGive(signaling_sem);
  vTaskSuspend(NULL);
}


//==============================================================