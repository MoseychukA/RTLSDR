﻿/*
 
 */
#include <elapsedMillis.h>
#include <usb/usb_host.h>
#include "show_desc.hpp"
#include "usbhhelp.hpp"

bool isRTLSDR = false;
bool isRTLSDRReady = false;
uint8_t RTLSDRInterval;
bool isRTLSDRPolling = false;
elapsedMillis RTLSDRTimer;

const size_t RTLSDR_IN_BUFFER_SIZE = 8;
usb_transfer_t *RTLSDRIn = NULL;

void RTLSDR_transfer_cb(usb_transfer_t *transfer)
{
    ESP_LOGI("", "***!! RTLSDR_transfer_cb");

  if (Device_Handle == transfer->device_handle) 
  {
    isRTLSDRPolling = false;
    if (transfer->status == 0) {
      if (transfer->actual_num_bytes == 8) 
      {
        uint8_t *const p = transfer->data_buffer;
        ESP_LOGI("", "HID report: %02x %02x %02x %02x %02x %02x %02x %02x",
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
      }
      else 
      {
        ESP_LOGI("", "RTLSDR boot hid transfer too short or long");
      }
    }
    else 
    {
      ESP_LOGI("", "transfer->status %d", transfer->status);
    }
  }
}

void check_interface_desc_boot_RTLSDR(const void *p)
{
  const usb_intf_desc_t *intf = (const usb_intf_desc_t *)p;

  if ((intf->bInterfaceClass == USB_CLASS_HID) &&
      (intf->bInterfaceSubClass == 1) &&
      (intf->bInterfaceProtocol == 1)) 
  {
    isRTLSDR = true;
    ESP_LOGI("", "Claiming a boot RTLSDR!");
    esp_err_t err = usb_host_interface_claim(Client_Handle, Device_Handle,
        intf->bInterfaceNumber, intf->bAlternateSetting);
    if (err != ESP_OK) ESP_LOGI("", "usb_host_interface_claim failed: %x", err);
  } 
}

void prepare_endpoint(const void *p)
{
  const usb_ep_desc_t *endpoint = (const usb_ep_desc_t *)p;
  esp_err_t err;
  ESP_LOGI("", "***!!! prepare_endpoint");
  // должно быть прерывание для HID
  if ((endpoint->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK) != USB_BM_ATTRIBUTES_XFER_INT) 
  {
    ESP_LOGI("", "Not interrupt endpoint: 0x%02x", endpoint->bmAttributes);
    return;
  }
  if (endpoint->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK) 
  {
      /**
  * @brief Выделить объект передачи
  *
  * - Эта функция выделяет объект передачи
  * - Каждый объект передачи имеет буфер фиксированного размера, указанный при выделении.
  * - Переданный объект можно использовать повторно неограниченное количество раз.
  * - Передача может быть отправлена с помощью usb_host_transfer_submit() или usb_host_transfer_submit_control().
  *
  * @param[in] data_buffer_size Размер буфера данных передачи
  * @param[in] num_isoc_packets Количество изохронных пакетов при передаче (установлено в 0 для неизохронных передач) 
  * @param[out] Transfer Передача объекта
  * @return esp_err_t
  */

    err = usb_host_transfer_alloc(RTLSDR_IN_BUFFER_SIZE, 0, &RTLSDRIn);
    if (err != ESP_OK) 
    {
      RTLSDRIn = NULL;
      ESP_LOGI("", "usb_host_transfer_alloc In fail: %x", err);
      return;
    }
    RTLSDRIn->device_handle = Device_Handle;
    RTLSDRIn->bEndpointAddress = endpoint->bEndpointAddress;
    RTLSDRIn->callback = RTLSDR_transfer_cb;
    RTLSDRIn->context = NULL;
    isRTLSDRReady = true;
    RTLSDRInterval = endpoint->bInterval;
    ESP_LOGI("", "USB boot RTLSDR ready");
  }
  else 
  {
    ESP_LOGI("", "Ignoring interrupt Out endpoint");
  }
}

void show_config_desc_full(const usb_config_desc_t *config_desc)
{
    ESP_LOGI("", "***!!show_config_desc_full");

  // Полная расшифровка описания конфигурации.
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
          ESP_LOGI("", "USB Device Descriptor should not appear in config");
          break;
        case USB_B_DESCRIPTOR_TYPE_CONFIGURATION:
          show_config_desc(p);
          break;
        case USB_B_DESCRIPTOR_TYPE_STRING:
          ESP_LOGI("", "USB string desc TBD");
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
          ESP_LOGI("", "USB device qual desc TBD");
          break;
        case USB_B_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION:
          // Should not be config config?
          ESP_LOGI("", "USB Other Speed TBD");
          break;
        case USB_B_DESCRIPTOR_TYPE_INTERFACE_POWER:
          // Should not be config config?
          ESP_LOGI("", "USB Interface Power TBD");
          break;
        case 0x21:
          if (USB_Class == USB_CLASS_HID) {
            show_hid_desc(p);
          }
          break;
        default:
          ESP_LOGI("", "Unknown USB Descriptor Type: 0x%x", bDescriptorType);
          break;
      }
    }
    else 
    {
      ESP_LOGI("", "USB Descriptor invalid");
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
    RTLSDRIn->num_bytes = 8;

    /**
  * @brief Отправьте неконтролируемую передачу
  *
  * - Отправка перевода в конкретную конечную точку. Номер устройства и конечной точки указывается внутри передачи.
  * - Перед отправкой передача должна быть правильно инициализирована.
  * - По завершении будет вызван обратный вызов передачи из клиентской функции usb_host_client_handle_events().
  *
  * @param[in] Transfer Инициализированный объект передачи
  * @return esp_err_t
  */

    esp_err_t err = usb_host_transfer_submit(RTLSDRIn);
    if (err != ESP_OK) 
    {
      ESP_LOGI("", "usb_host_transfer_submit In fail: %x", err);
    }
    isRTLSDRPolling = true;
    RTLSDRTimer = 0;
  }
}
