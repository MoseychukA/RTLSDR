

 /*

  ��������� �������:

 pvTaskCode    � ��������� �� ������� ������ (������� �� ������� ����).
 pcName        � �������� ��� ������, �� �� ��� �� ������, ����� ������ ��� ������� � vTaskList(). ������������ ����� �� ����� ��������� 16 ��������.
 usStackDepth  � ����� ����� ������ � ������. ������ � ������, � �� ������ (��� ESP32). � �������� ����������� ��� �������� ���� �� �� 256 ��� 512 ����, ���� ��� � �� �����������.
 pvParameters  � ��������� �� ���������, ������� ����� �������� � ����������� ���������. ���� ��������� ����� ������� � ������� ������ ����� �������� �� ���������� � �������������.
 uxPriority    � ���������, � ������� ������ ����������� ������.
 pvCreatedTask � ���� �������� ����� ��������� ��������� �� ������ ��� ��������� ������, ������� ����� ����
                 ����������� ��� ��������� ���������� ������� (������������, �������������, ��������). �� ���������� � �������������.
 xCoreID       � ���� ����������. �������� 0 ��� 1 ��������� ���������� ����� ����������, � �������� ������ ���� ��������� ������.
                 ���� �������� ����� tskNO_AFFINITY, ��������� ������ �� ��������� �� � ������ ����������, � ����������� ����� ��������� �� �� ����� ��������� ����.


  */
#include <Arduino.h>              // define I/O functions
#include <stdint.h>
#include <stdio.h>                // define I/O functions
#include <stdlib.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "usb/usb_host.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include <esp_task_wdt.h>

#include <elapsedMillis.h>
#include "show_desc.hpp"
#include "usbhhelp.hpp"


#define WDT_TIMEOUT 8

  // ���������� �� ����� ���� ����� ���������� �������� ���������
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

static const char* TAG_MAIN = "MAIN";


bool isRTLSDR = false;
bool isRTLSDRReady = false;
uint8_t RTLSDRInterval;
bool isRTLSDRPolling = false;
elapsedMillis RTLSDRTimer;

const size_t RTLSDR_IN_BUFFER_SIZE = 64;
usb_transfer_t* RTLSDRIn = NULL;


bool isKeyboard = false;
bool isKeyboardReady = false;
uint8_t KeyboardInterval;
bool isKeyboardPolling = false;
elapsedMillis KeyboardTimer;

const size_t KEYBOARD_IN_BUFFER_SIZE = 8;
usb_transfer_t *KeyboardIn = NULL;

void keyboard_transfer_cb(usb_transfer_t *transfer)
{
  if (Device_Handle == transfer->device_handle) 
  {
    isKeyboardPolling = false;
    if (transfer->status == 0) {
      if (transfer->actual_num_bytes == 8) 
      {
        uint8_t *const p = transfer->data_buffer;
        ESP_LOGI("", "HID report: %02x %02x %02x %02x %02x %02x %02x %02x",
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
      }
      else  
      {
        ESP_LOGI("", "Keyboard boot hid transfer too short or long");
      }
    }
    else 
    {
      ESP_LOGI("", "transfer->status %d", transfer->status);
    }
  }
}

void RTLSDR_transfer_cb(usb_transfer_t* transfer)
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
            if (transfer->actual_num_bytes == 8)
            {
                uint8_t* const p = transfer->data_buffer;
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



void check_interface_desc_boot_keyboard(const void *p)
{
  const usb_intf_desc_t *intf = (const usb_intf_desc_t *)p;

  if ((intf->bInterfaceClass == USB_CLASS_HID) &&
      (intf->bInterfaceSubClass == 1) &&
      (intf->bInterfaceProtocol == 1)) 
  {
    isKeyboard = true;
    ESP_LOGI("", "Claiming a boot keyboard!");
    esp_err_t err = usb_host_interface_claim(Client_Handle, Device_Handle,
        intf->bInterfaceNumber, intf->bAlternateSetting);
    if (err != ESP_OK) ESP_LOGI("", "usb_host_interface_claim failed: %x", err);
  } 
}

void check_interface_desc_boot_RTLSDR(const void* p)
{
    const usb_intf_desc_t* intf = (const usb_intf_desc_t*)p;

    if ((intf->bInterfaceClass == USB_CLASS_VENDOR_SPEC) &&
        (intf->bInterfaceSubClass == 0xff) &&
        (intf->bInterfaceProtocol == 0xff))
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

  // ������ ���� ���������� ��� HID
  if ((endpoint->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK) != USB_BM_ATTRIBUTES_XFER_INT) 
  {
    ESP_LOGI("", "Not interrupt endpoint: 0x%02x", endpoint->bmAttributes);
    return;
  }
  if (endpoint->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK) 
  {
    err = usb_host_transfer_alloc(KEYBOARD_IN_BUFFER_SIZE, 0, &KeyboardIn);
    if (err != ESP_OK) 
    {
      KeyboardIn = NULL;
      ESP_LOGI("", "usb_host_transfer_alloc In fail: %x", err);
      return;
    }
    KeyboardIn->device_handle = Device_Handle;
    KeyboardIn->bEndpointAddress = endpoint->bEndpointAddress;
    KeyboardIn->callback = keyboard_transfer_cb;
    KeyboardIn->context = NULL;
    isKeyboardReady = true;
    KeyboardInterval = endpoint->bInterval;
    ESP_LOGI("", "USB boot keyboard ready");
  }
  else 
  {
    ESP_LOGI("", "Ignoring interrupt Out endpoint");
  }
}

//===================================================================

void prepare_endpointRTLSDR(const void* p)
{
    const usb_ep_desc_t* endpoint = (const usb_ep_desc_t*)p;
    esp_err_t err;

   
    // ������ ���� ���������� ��� HID
    if ((endpoint->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK) != USB_BM_ATTRIBUTES_XFER_BULK)
    {
        ESP_LOGI("", "**!! Not interrupt endpoint: 0x%02x", endpoint->bmAttributes);
        return;
    }
    if (endpoint->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK)
    {
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
          if (USB_Class == USB_CLASS_HID)
          {
              check_interface_desc_boot_keyboard(p);
          }
          else if (USB_Class == USB_CLASS_VENDOR_SPEC)
          {
              check_interface_desc_boot_RTLSDR(p);
          }
          break;
        case USB_B_DESCRIPTOR_TYPE_ENDPOINT:
          show_endpoint_desc(p);

          if (isKeyboard && KeyboardIn == NULL) prepare_endpoint(p);
          else if(isRTLSDR && RTLSDRIn == NULL) prepare_endpointRTLSDR(p);

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
    // ��������� ������ UART
    Serial.begin(115200);
    while (!Serial && millis() < 1000);

    esp_log_level_set("*", ESP_LOG_VERBOSE);  // ������� ���������� ���������

    //esp_task_wdt_init(WDT_TIMEOUT, false);  //enable panic so ESP32 restarts
    //esp_task_wdt_add(NULL);                 //add current thread to WDT watch
    //esp_task_wdt_reset();
    //delay(1000);
    //esp_task_wdt_reset();

    String ver_soft = __FILE__;
    int val_srt = ver_soft.lastIndexOf('\\');
    ver_soft.remove(0, val_srt + 1);
    val_srt = ver_soft.lastIndexOf('.');
    ver_soft.remove(val_srt);
    Serial.println(ver_soft);
    //esp_task_wdt_reset();


    usbh_setup(show_config_desc_full);

    //esp_task_wdt_reset();

    ESP_LOGI(TAG_MAIN, "*** End setup");
}

void loop()
{
  usbh_task();

  if (isKeyboardReady && !isKeyboardPolling && (KeyboardTimer > KeyboardInterval)) 
  {
    /**
    * @brief ��������� ���������������� ��������
    * - �������� �������� � ���������� �������� �����. ����� ���������� � �������� ����� ����������� ������ ��������.
    * - ����� ��������� �������� ������ ���� ��������� ����������������.
    * - �� ���������� ����� ������ �������� ����� �������� �� ���������� ������� usb_host_client_handle_events().
    *
    * @param[in] Transfer ������������������ ������ ��������
    * @return esp_err_t
    */

    KeyboardIn->num_bytes = 8;
    esp_err_t err = usb_host_transfer_submit(KeyboardIn);
    if (err != ESP_OK) 
    {
      ESP_LOGI("", "usb_host_transfer_submit In fail: %x", err);
    }
    isKeyboardPolling = true;
    KeyboardTimer = 0;
  }
}
