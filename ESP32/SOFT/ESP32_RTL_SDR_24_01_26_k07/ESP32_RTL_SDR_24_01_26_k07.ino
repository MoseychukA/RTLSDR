/*
 
 */

#include <Arduino.h>              // define I/O functions
#include <stdint.h>
#include <stdio.h>                // define I/O functions
#include <stdlib.h>
#include "string.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"

#include <elapsedMillis.h>
#include <usb/usb_host.h>
#include "show_desc.hpp"
#include "usbhhelp.hpp"



#include "librtlsdr.h"
//#include "rtl-sdr.h"
#include "tuner_r82xx.h"


bool isRTLSDR = false;
bool isRTLSDRReady = false;
uint8_t RTLSDRInterval;
bool isRTLSDRPolling = false;
elapsedMillis RTLSDRTimer;

const size_t RTLSDR_IN_BUFFER_SIZE = 64;// 8;
usb_transfer_t *RTLSDRIn = NULL;

static const char* TAG_MAIN = "MAIN";

void RTLSDR_transfer_cb(usb_transfer_t *transfer)
{
    /**
 * @brief ��������� �������� ������ �� USB
 *
 * ��� ��������� ������������ ��� ������������� �������� �� ������������ ������� � �������� ����� �� ���� USB. ��������� ��
 * ���� ���������� ������� ������������, ��������� ��� ����������� ��� �������������. ������������� ������� ��������� � ��������������� USB-����-����������.
 * ������� ��� ��������� ��������� �������� USB ������ ��������� ���� ��������� ��������������.
 *
 * ��� �������� ������������ �������� ������, � ������� ������������ ��� ��������. � ����������� �� ���� �������� ������������
 * ������� �������� ���������:
 *
 * - �������� ��������: ��� ��������� ������������ ����� ���� ��������� ��������. ���� ���������� ������ ��������� MPS �������� �����,
 * �������� ����� ��������� �� ��������� ������� ������� MPS, �� �������� ������� �������� �����.
 * - ����������: ��� ��������� ������������ ����� ������ �������� ����������. ��� ������ 8 ���� data_buffer ������ ���� ���������.
 * � ������������ ������� (��. usb_setup_packet_t). ���� num_bytes ������ ��������� ����� ������
 * �������� (�. �. ������ ������������� ������ + wLength).
 * - ����������: ������������ ����� �������� ����������. ���� num_bytes ��������� MPS �������� �����, �������� �����
 * ����������� �� ��������� �������, � ������ ����� ���������� ����� ��������� �������� ������ ��������.
 * - ����������: ������������ ����� ������, ������� ������ ���� ������� � �������� ����� � ������������� ���������. �������
 * ����������� �� ������ � ������������ � ������ isoc_packet_desc. ����� ���������� ����� ������ ��������
 * �������� �����. ���� ���� ISOC URB ��� ������� ��� ������ (����������� ������ �� ���������
 * ������), ����� ������ URB � ������ ������� ����������� ������ ����� ���������, �
 * act_num_bytes �������� ����� ���������� ������, ���������� �� ���� �������. ���� ISOC URB ������������
 * ������, ���� URB ��������� ���������, ������� ����� �������� ������ ����� ������.
 *
 * @note ��� �������� �������� ������/����������/����� ���������� num_bytes ������ ���� ����� ������, ������� MPS �������� �����.
 * @note ��� ��������� ������ ���� �������� ����� usb_host_transfer_alloc().
 * @note ����� �������� �������� ������������ �� ������ �������� ��������� �� ���������� ��������.
 */


  if (Device_Handle == transfer->device_handle) 
  {
    isRTLSDRPolling = false;
    if (transfer->status == 0) 
    {
      if (transfer->actual_num_bytes == 64) 
      {
        uint8_t *const p = transfer->data_buffer;
        ESP_LOGI(TAG_MAIN, "HID report: %02x %02x %02x %02x %02x %02x %02x %02x",
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
      }
      else 
      {
        ESP_LOGI(TAG_MAIN, "RTLSDR boot hid transfer too short or long");
      }
    }
    else 
    {
      ESP_LOGI(TAG_MAIN, "*** Transfer->status %d", transfer->status);
    }
  }
}

void check_interface_desc_boot_RTLSDR(const void *p)
{
  const usb_intf_desc_t *intf = (const usb_intf_desc_t *)p;

  if ((intf->bInterfaceClass == USB_CLASS_VENDOR_SPEC) &&
      (intf->bInterfaceSubClass == 0xff) &&
      (intf->bInterfaceProtocol == 0xff))
  {
    isRTLSDR = true;
    ESP_LOGI(TAG_MAIN, "Claiming a boot RTLSDR!");
    esp_err_t err = usb_host_interface_claim(Client_Handle, Device_Handle,
        intf->bInterfaceNumber, intf->bAlternateSetting);
    if (err != ESP_OK) ESP_LOGI(TAG_MAIN, "usb_host_interface_claim failed: %x", err);
  } 
}

void prepare_endpoint(const void *p)
{
  const usb_ep_desc_t *endpoint = (const usb_ep_desc_t *)p;
  esp_err_t err;

  /**
  * @brief ������� �����, ������������� ���� bmAttributes ����������� �������� �����.
  */
  // ������ ���� ���������� ��� HID 
  if ((endpoint->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK) != USB_BM_ATTRIBUTES_XFER_BULK)
  {
    ESP_LOGI(TAG_MAIN, "Not interrupt endpoint: 0x%02x", endpoint->bmAttributes);
    return;
  }
  if (endpoint->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK) 
  {
    err = usb_host_transfer_alloc(RTLSDR_IN_BUFFER_SIZE, 0, &RTLSDRIn);
    if (err != ESP_OK) 
    {
      RTLSDRIn = NULL;
      ESP_LOGI(TAG_MAIN, "usb_host_transfer_alloc In fail: %x", err);
      return;
    }
    RTLSDRIn->device_handle = Device_Handle;
    RTLSDRIn->bEndpointAddress = endpoint->bEndpointAddress;
    RTLSDRIn->callback = RTLSDR_transfer_cb;
    RTLSDRIn->context = NULL;
    isRTLSDRReady = true;
    RTLSDRInterval = endpoint->bInterval;
    ESP_LOGI(TAG_MAIN, "USB boot RTLSDR ready");
  }
  else 
  {
    ESP_LOGI(TAG_MAIN, "Ignoring interrupt Out endpoint");
  }
}

void show_config_desc_full(const usb_config_desc_t *config_desc)
{
  // ������ ����������� �������� ������������.
  const uint8_t *p = &config_desc->val[0];
  static uint8_t USB_Class = 0;
  uint8_t bLength;
  for (int i = 0; i < config_desc->wTotalLength; i+=bLength, p+=bLength) 
  {
    bLength = *p;
    if ((i + bLength) <= config_desc->wTotalLength) {
      const uint8_t bDescriptorType = *(p + 1);
      switch (bDescriptorType) {
        case USB_B_DESCRIPTOR_TYPE_DEVICE:
          ESP_LOGI(TAG_MAIN, "USB Device Descriptor should not appear in config");
          break;
        case USB_B_DESCRIPTOR_TYPE_CONFIGURATION:
          show_config_desc(p);
          break;
        case USB_B_DESCRIPTOR_TYPE_STRING:
          ESP_LOGI(TAG_MAIN, "USB string desc TBD");
          break;
        case USB_B_DESCRIPTOR_TYPE_INTERFACE:
          USB_Class = show_interface_desc(p);
          check_interface_desc_boot_RTLSDR(p);
          break;
        case USB_B_DESCRIPTOR_TYPE_ENDPOINT:
          show_endpoint_desc(p);
          if (isRTLSDR && RTLSDRIn == NULL) prepare_endpoint(p);
          break;
        case USB_B_DESCRIPTOR_TYPE_DEVICE_QUALIFIER:
          // Should not be config config?
          ESP_LOGI(TAG_MAIN, "USB device qual desc TBD");
          break;
        case USB_B_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION:
          // Should not be config config?
          ESP_LOGI(TAG_MAIN, "USB Other Speed TBD");
          break;
        case USB_B_DESCRIPTOR_TYPE_INTERFACE_POWER:
          // Should not be config config?
          ESP_LOGI(TAG_MAIN, "USB Interface Power TBD");
          break;
        case 0x21:
          if (USB_Class == USB_CLASS_HID) {
            show_hid_desc(p);
          }
          break;
        default:
          ESP_LOGI(TAG_MAIN, "Unknown USB Descriptor Type: 0x%x", bDescriptorType);
          break;
      }
    }
    else 
    {
      ESP_LOGI(TAG_MAIN, "USB Descriptor invalid");
      return;
    }
  }
}

void setup()
{
  usbh_setup(show_config_desc_full);
}

void loop()
{
  usbh_task();

  if (isRTLSDRReady && !isRTLSDRPolling && (RTLSDRTimer > RTLSDRInterval)) 
  {
      RTLSDRIn->num_bytes = 64;// 8;
    esp_err_t err = usb_host_transfer_submit(RTLSDRIn);
    if (err != ESP_OK) 
    {
      ESP_LOGI(TAG_MAIN, "usb_host_transfer_submit In fail: %x", err);
    }
    isRTLSDRPolling = true;
    RTLSDRTimer = 0;
  }
}
