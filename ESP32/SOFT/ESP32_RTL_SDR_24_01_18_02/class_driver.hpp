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

//===============================================================================================

const size_t USB_HID_DESC_SIZE = 9;

typedef union {
    struct {
        uint8_t bLength;                    /**< Size of the descriptor in bytes */
        uint8_t bDescriptorType;            /**< Constant name specifying type of HID descriptor. */
        uint16_t bcdHID;                    /**< USB HID Specification Release Number in Binary-Coded Decimal (i.e., 2.10 is 210H) */
        uint8_t bCountryCode;               /**< Numeric expression identifying country code of the localized hardware. */
        uint8_t bNumDescriptor;             /**< Numeric expression specifying the number of class descriptors. */
        uint8_t bHIDDescriptorType;         /**< Constant name identifying type of class descriptor. See Section 7.1.2: Set_Descriptor Request for a table of class descriptor constants. */
        uint16_t wHIDDescriptorLength;      /**< Numeric expression that is the total size of the Report descriptor. */
        uint8_t bHIDDescriptorTypeOpt;      /**< Optional constant name identifying type of class descriptor. See Section 7.1.2: Set_Descriptor Request for a table of class descriptor constants. */
        uint16_t wHIDDescriptorLengthOpt;   /**< Optional numeric expression that is the total size of the Report descriptor. */
    } USB_DESC_ATTR;
    uint8_t val[USB_HID_DESC_SIZE];
} usb_hid_desc_t;

void show_dev_desc(const usb_device_desc_t* dev_desc)
{
    ESP_LOGI("", "bLength: %d", dev_desc->bLength);
    ESP_LOGI("", "bDescriptorType(device): %d", dev_desc->bDescriptorType);
    ESP_LOGI("", "bcdUSB: 0x%x", dev_desc->bcdUSB);
    ESP_LOGI("", "bDeviceClass: 0x%02x", dev_desc->bDeviceClass);
    ESP_LOGI("", "bDeviceSubClass: 0x%02x", dev_desc->bDeviceSubClass);
    ESP_LOGI("", "bDeviceProtocol: 0x%02x", dev_desc->bDeviceProtocol);
    ESP_LOGI("", "bMaxPacketSize0: %d", dev_desc->bMaxPacketSize0);
    ESP_LOGI("", "idVendor: 0x%x", dev_desc->idVendor);
    ESP_LOGI("", "idProduct: 0x%x", dev_desc->idProduct);
    ESP_LOGI("", "bcdDevice: 0x%x", dev_desc->bcdDevice);
    ESP_LOGI("", "iManufacturer: %d", dev_desc->iManufacturer);
    ESP_LOGI("", "iProduct: %d", dev_desc->iProduct);
    ESP_LOGI("", "iSerialNumber: %d", dev_desc->iSerialNumber);
    ESP_LOGI("", "bNumConfigurations: %d", dev_desc->bNumConfigurations);
}

void show_config_desc(const void* p)
{
    const usb_config_desc_t* config_desc = (const usb_config_desc_t*)p;

    ESP_LOGI("", "bLength: %d", config_desc->bLength);
    ESP_LOGI("", "bDescriptorType(config): %d", config_desc->bDescriptorType);
    ESP_LOGI("", "wTotalLength: %d", config_desc->wTotalLength);
    ESP_LOGI("", "bNumInterfaces: %d", config_desc->bNumInterfaces);
    ESP_LOGI("", "bConfigurationValue: %d", config_desc->bConfigurationValue);
    ESP_LOGI("", "iConfiguration: %d", config_desc->iConfiguration);
    ESP_LOGI("", "bmAttributes(%s%s%s): 0x%02x",
        (config_desc->bmAttributes & USB_BM_ATTRIBUTES_SELFPOWER) ? "Self Powered" : "",
        (config_desc->bmAttributes & USB_BM_ATTRIBUTES_WAKEUP) ? ", Remote Wakeup" : "",
        (config_desc->bmAttributes & USB_BM_ATTRIBUTES_BATTERY) ? ", Battery Powered" : "",
        config_desc->bmAttributes);
    ESP_LOGI("", "bMaxPower: %d = %d mA", config_desc->bMaxPower, config_desc->bMaxPower * 2);
}

uint8_t show_interface_desc(const void* p)
{
    const usb_intf_desc_t* intf = (const usb_intf_desc_t*)p;

    ESP_LOGI("", "bLength: %d", intf->bLength);
    ESP_LOGI("", "bDescriptorType (interface): %d", intf->bDescriptorType);
    ESP_LOGI("", "bInterfaceNumber: %d", intf->bInterfaceNumber);
    ESP_LOGI("", "bAlternateSetting: %d", intf->bAlternateSetting);
    ESP_LOGI("", "bNumEndpoints: %d", intf->bNumEndpoints);
    ESP_LOGI("", "bInterfaceClass: 0x%02x", intf->bInterfaceClass);
    ESP_LOGI("", "bInterfaceSubClass: 0x%02x", intf->bInterfaceSubClass);
    ESP_LOGI("", "bInterfaceProtocol: 0x%02x", intf->bInterfaceProtocol);
    ESP_LOGI("", "iInterface: %d", intf->iInterface);
    return intf->bInterfaceClass;
}

void show_endpoint_desc(const void* p)
{
    const usb_ep_desc_t* endpoint = (const usb_ep_desc_t*)p;
    const char* XFER_TYPE_NAMES[] = {
      "Control", "Isochronous", "Bulk", "Interrupt"
    };
    ESP_LOGI("", "bLength: %d", endpoint->bLength);
    ESP_LOGI("", "bDescriptorType (endpoint): %d", endpoint->bDescriptorType);
    ESP_LOGI("", "bEndpointAddress(%s): 0x%02x",
        (endpoint->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK) ? "In" : "Out",
        endpoint->bEndpointAddress);
    ESP_LOGI("", "bmAttributes(%s): 0x%02x",
        XFER_TYPE_NAMES[endpoint->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK],
        endpoint->bmAttributes);
    ESP_LOGI("", "wMaxPacketSize: %d", endpoint->wMaxPacketSize);
    ESP_LOGI("", "bInterval: %d", endpoint->bInterval);
}

void show_hid_desc(const void* p)
{
    usb_hid_desc_t* hid = (usb_hid_desc_t*)p;
    ESP_LOGI("", "bLength: %d", hid->bLength);
    ESP_LOGI("", "bDescriptorType (HID): %d", hid->bDescriptorType);
    ESP_LOGI("", "bcdHID: 0x%04x", hid->bcdHID);
    ESP_LOGI("", "bCountryCode: %d", hid->bCountryCode);
    ESP_LOGI("", "bNumDescriptor: %d", hid->bNumDescriptor);
    ESP_LOGI("", "bDescriptorType: %d", hid->bHIDDescriptorType);
    ESP_LOGI("", "wDescriptorLength: %d", hid->wHIDDescriptorLength);
    if (hid->bNumDescriptor > 1) {
        ESP_LOGI("", "bDescriptorTypeOpt: %d", hid->bHIDDescriptorTypeOpt);
        ESP_LOGI("", "wDescriptorLengthOpt: %d", hid->wHIDDescriptorLengthOpt);
    }
}

void show_interface_assoc(const void* p)
{
    usb_iad_desc_t* iad = (usb_iad_desc_t*)p;
    ESP_LOGI("", "bLength: %d", iad->bLength);
    ESP_LOGI("", "bDescriptorType: %d", iad->bDescriptorType);
    ESP_LOGI("", "bFirstInterface: %d", iad->bFirstInterface);
    ESP_LOGI("", "bInterfaceCount: %d", iad->bInterfaceCount);
    ESP_LOGI("", "bFunctionClass: 0x%02x", iad->bFunctionClass);
    ESP_LOGI("", "bFunctionSubClass: 0x%02x", iad->bFunctionSubClass);
    ESP_LOGI("", "bFunctionProtocol: 0x%02x", iad->bFunctionProtocol);
    ESP_LOGI("", "iFunction: %d", iad->iFunction);
}



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
} class_driver_t_hpp;

static const char *TAG_CLASS = "CLASS";
static rtlsdr_dev_t *rtldev = NULL;

static void client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg)
{
  class_driver_t_hpp *driver_obj = (class_driver_t_hpp *)arg;
  esp_err_t err;
  switch (event_msg->event)
  {
    case USB_HOST_CLIENT_EVENT_NEW_DEV:
      if (driver_obj->dev_addr == 0)
      {
        driver_obj->dev_addr = event_msg->new_dev.address;
        rtlsdr_open(&rtldev, event_msg->new_dev.address, driver_obj->client_hdl);

        int r;
        r = rtlsdr_set_sample_rate(rtldev, 2000000);
        if (r < 0)
        {
          fprintf(stderr, "WARNING: Failed to set sample rate.\n");
        }
        else
        {
          fprintf(stderr, "Sampling at %u S/s.\n", 2000000);
        }
        r = rtlsdr_set_center_freq(rtldev, 1090000000);
        if (r < 0)
        {
          fprintf(stderr, "WARNING: Failed to set center freq.\n");
        }
        else
        {
          fprintf(stderr, "Tuned to %u Hz.\n", 1090000000);
        }
        r = rtlsdr_set_tuner_gain_mode(rtldev, 0);
        if (r != 0)
        {
          fprintf(stderr, "WARNING: Failed to set tuner gain.\n");
        }
        else
        {
          fprintf(stderr, "Tuner gain set to automatic.\n");
        }
        // r = rtlsdr_set_freq_correction(rtldev, 0);
        // if (r < 0)
        // {
        //     fprintf(stderr, "WARNING: Failed to set ppm error.\n");
        // }
        // else
        // {
        //     fprintf(stderr, "Tuner error set to %i ppm.\n", 0);
        // }
        r = rtlsdr_reset_buffer(rtldev);
        if (r < 0)
        {
          fprintf(stderr, "WARNING: Failed to reset buffers.\n");
        }

        vTaskDelay(100); // Short delay to let client task spin up

        uint32_t out_block_size = DEFAULT_BUF_LENGTH;

        //uint8_t* buffer ;
        uint8_t* buffer = (uint8_t*)malloc(out_block_size * sizeof(uint8_t));
        int n_read = 2;
        ESP_LOGI(TAG_CLASS, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
        // uint32_t bytes_to_read = 0;
        while (true)
        {
          r = rtlsdr_read_sync(rtldev, buffer, out_block_size, &n_read);
          ESP_LOGI(TAG_CLASS, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
          if (r < 0)
          {
            fprintf(stderr, "WARNING: sync read failed.\n");
            break;
          }

          // if ((bytes_to_read > 0) && (bytes_to_read < (uint32_t)n_read))
          // {
          //     n_read = bytes_to_read;
          //     do_exit = 1;
          // }
          for (int i = 0; i < out_block_size; i++)
            fprintf(stdout, "%02X", buffer[i]);
          // if (fwrite(buffer, 1, n_read, stdout) != (size_t)n_read)
          // {
          //     fprintf(stderr, "Short write, samples lost, exiting!\n");
          //     break;
          // }

          if ((uint32_t)n_read < out_block_size)
          {
            fprintf(stderr, "Short read, samples lost, exiting!\n");
            break;
          }

          // if (bytes_to_read > 0)
          //     bytes_to_read -= n_read;
        }
        // esp_action_get_dev_desc(rtldev);
        // driver_obj->dev_hdl = *(usb_device_handle_t *)get_driver_obj(rtldev);
        // Open the device next
        // driver_obj->actions |= ACTION_OPEN_DEV;
        // ESP_LOGI(TAG_CLASS, "here");
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

static void action_open_dev(class_driver_t_hpp *driver_obj)
{
  assert(driver_obj->dev_addr != 0);
  ESP_LOGI(TAG_CLASS, "*** Opening device at address %d", driver_obj->dev_addr);
  ESP_ERROR_CHECK(usb_host_device_open(driver_obj->client_hdl, driver_obj->dev_addr, &driver_obj->dev_hdl));
  // Get the device's information next
  driver_obj->actions &= ~ACTION_OPEN_DEV;
  driver_obj->actions |= ACTION_GET_DEV_INFO;
}

static void action_get_info(class_driver_t_hpp *driver_obj)
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
void action_get_dev_desc1(class_driver_t_hpp *driver_obj)
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

static void action_get_config_desc(class_driver_t_hpp *driver_obj)
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

static void action_get_str_desc(class_driver_t_hpp *driver_obj)
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

static void aciton_close_dev(class_driver_t_hpp *driver_obj)
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
  SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;
  class_driver_t_hpp driver_obj = {0};

  // Wait until daemon task has installed USB Host Library
  xSemaphoreTake(signaling_sem, portMAX_DELAY);

  ESP_LOGI(TAG_CLASS, "Waiting for client registration");
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