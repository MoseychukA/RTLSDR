/*

  Параметры функции:

  pvTaskCode    – указатель на функцию задачи (которую мы создали выше).
  pcName        – условное имя задачи, ни на что не влияет, нужно только для отладки в vTaskList(). Максимальная длина не может превышать 16 символов.
  usStackDepth  – длина стека задачи в байтах. Именно в байтах, а не словах (для ESP32). Я стараюсь выравнивать это значение хотя бы до 256 или 512 байт, хотя это и не обязательно.
  pvParameters  – указатель на параметры, которые можно передать в запускаемую программу. Этот указатель будет передан в функцию задачи через параметр Не обязателен к использованию.
  uxPriority    – приоритет, с которым должна выполняться задача.
  pvCreatedTask – этот параметр может содержать указатель на только что созданную задачу, который может быть
                использован для “внешнего” управления задачей (приостановки, возобновления, удаления). Не обязателен к использованию.
  xCoreID       – ядро процессора. Значения 0 или 1 указывают порядковый номер процессора, к которому должна быть привязана задача.
                Если значение равно tskNO_AFFINITY, созданная задача не привязана ни к какому процессору, и планировщик может запустить ее на любом доступном ядре.

Перечень возвращаемых ошибок:
// Определения констант ошибок.
//#define ESP_OK          0       // esp_err_t value indicating success (no error)
//#define ESP_FAIL        -1      // Generic esp_err_t code indicating failure
//
//#define ESP_ERR_NO_MEM              0x101     Out of memory
//#define ESP_ERR_INVALID_ARG         0x102     Invalid argument
//#define ESP_ERR_INVALID_STATE       0x103     Недопустимое состояние
//#define ESP_ERR_INVALID_SIZE        0x104     Invalid size
//#define ESP_ERR_NOT_FOUND           0x105     Запрошенный ресурс не найден
//#define ESP_ERR_NOT_SUPPORTED       0x106     Операция или функция не поддерживается
//#define ESP_ERR_TIMEOUT             0x107     Operation timed out
//#define ESP_ERR_INVALID_RESPONSE    0x108     Полученный ответ недействителен
//#define ESP_ERR_INVALID_CRC         0x109     CRC or checksum was invalid
//#define ESP_ERR_INVALID_VERSION     0x10A     Version was invalid
//#define ESP_ERR_INVALID_MAC         0x10B     MAC address was invalid
//#define ESP_ERR_NOT_FINISHED        0x10C     There are items remained to retrieve
//
//
//#define ESP_ERR_WIFI_BASE           0x3000    Starting number of WiFi error codes
//#define ESP_ERR_MESH_BASE           0x4000    Starting number of MESH error codes
//#define ESP_ERR_FLASH_BASE          0x6000    Starting number of flash error codes
//#define ESP_ERR_HW_CRYPTO_BASE      0xc000    Starting number of HW cryptography module error codes
//#define ESP_ERR_MEMPROT_BASE        0xd000    Starting number of Memory Protection API error codes
//
*/

#include <Arduino.h>              // define I/O functions
#include <stdint.h>
#include <stdio.h>                // define I/O functions
#include <stdlib.h>
#include "string.h"
#include "SPIFFS.h"
#include "FS.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "esp_libusb.h"
//#include "rtl-sdr.h"


#include <elapsedMillis.h>
#include <usb/usb_host.h>
#include "show_desc.hpp"


static const char* TAG_MAIN = "MAIN";
static const char* TAG_DAEMON = "DAEMON";


bool isRTLSDR = false;
bool isRTLSDRReady = false;
uint8_t RTLSDRInterval;
bool isRTLSDRPolling = false;
elapsedMillis RTLSDRTimer;

const size_t RTLSDR_IN_BUFFER_SIZE = 64;
usb_transfer_t *RTLSDRIn = NULL;

//================================================================================================

class_driver_t driver_obj;

const TickType_t HOST_EVENT_TIMEOUT = 1;
const TickType_t CLIENT_EVENT_TIMEOUT = 1;

typedef void (*usb_host_enum_cb_t)(const usb_config_desc_t* config_desc);
static usb_host_enum_cb_t _USB_host_enumerate;

void _client_event_callback(const usb_host_client_event_msg_t* event_msg, void* arg)
{

    ESP_LOGI("", "***!! driver_obj.client_hdl: %d", driver_obj.client_hdl);

    esp_err_t err;
    switch (event_msg->event)
    {
        /**< A new device has been enumerated and added to the USB Host Library */
    case USB_HOST_CLIENT_EVENT_NEW_DEV:
        ESP_LOGI("", "New device address: %d", event_msg->new_dev.address);
        err = usb_host_device_open(driver_obj.client_hdl, event_msg->new_dev.address, &driver_obj.dev_hdl);
        if (err != ESP_OK) ESP_LOGI("", "usb_host_device_open: %x", err);

        usb_device_info_t dev_info;
        err = usb_host_device_info(driver_obj.dev_hdl, &dev_info);
        if (err != ESP_OK) ESP_LOGI("", "usb_host_device_info: %x", err);
        ESP_LOGI("", "speed: %d dev_addr %d vMaxPacketSize0 %d bConfigurationValue %d",
            dev_info.speed, dev_info.dev_addr, dev_info.bMaxPacketSize0,
            dev_info.bConfigurationValue);

        const usb_device_desc_t* dev_desc;
        err = usb_host_get_device_descriptor(driver_obj.dev_hdl, &dev_desc);
        if (err != ESP_OK) ESP_LOGI("", "usb_host_get_device_desc: %x", err);
        show_dev_desc(dev_desc);

        const usb_config_desc_t* config_desc;
        err = usb_host_get_active_config_descriptor(driver_obj.dev_hdl, &config_desc);
        if (err != ESP_OK) ESP_LOGI("", "usb_host_get_config_desc: %x", err);
        (*_USB_host_enumerate)(config_desc);
        break;
        /**< A device opened by the client is now gone */
    case USB_HOST_CLIENT_EVENT_DEV_GONE:
        ESP_LOGI("", "Device Gone handle: %x", event_msg->dev_gone.dev_hdl);
        break;
    default:
        ESP_LOGI("", "Unknown value %d", event_msg->event);
        break;
    }
}

// Reference: esp-idf/examples/peripherals/usb/host/usb_host_lib/main/usb_host_lib_main.c

void usbh_setup(usb_host_enum_cb_t enumeration_cb)
{
    const usb_host_config_t config = {
      .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    esp_err_t err = usb_host_install(&config);
    ESP_LOGI("", "usb_host_install: %x", err);

    const usb_host_client_config_t client_config = {
      .is_synchronous = false,
      .max_num_event_msg = 5,
      .async = {
          .client_event_callback = _client_event_callback,
          .callback_arg = driver_obj.client_hdl
      }
    };
    err = usb_host_client_register(&client_config, &driver_obj.client_hdl);
    ESP_LOGI("", "usb_host_client_register: %x", err);


    //ESP_LOGI("", "***!! driver_obj.client_hdl: %d", driver_obj.client_hdl);
    //ESP_LOGI("", "***!! driver_obj.dev_addr: %d", driver_obj.dev_addr);
    //ESP_LOGI("", "***!! driver_obj.dev_hdl: %d", driver_obj.dev_hdl);
    //ESP_LOGI("", "***!! driver_obj.actions: %d", driver_obj.actions);

    _USB_host_enumerate = enumeration_cb;
}

void usbh_task(void)
{
    uint32_t event_flags;
    static bool all_clients_gone = false;
    static bool all_dev_free = false;
 
    esp_err_t err = usb_host_lib_handle_events(HOST_EVENT_TIMEOUT, &event_flags);
    if (err == ESP_OK)
    {
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            ESP_LOGI("", "No more clients");
            all_clients_gone = true;
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            ESP_LOGI("", "No more devices");
            all_dev_free = true;
        }
    }
    else
    {
        if (err != ESP_ERR_TIMEOUT)
        {
            ESP_LOGI("", "usb_host_lib_handle_events: %x flags: %x", err, event_flags);
        }
    }

    err = usb_host_client_handle_events(driver_obj.client_hdl, CLIENT_EVENT_TIMEOUT);
    if ((err != ESP_OK) && (err != ESP_ERR_TIMEOUT))
    {
        ESP_LOGI("", "usb_host_client_handle_events: %x", err);
    }
}

//================================================================================================
void RTLSDR_transfer_cb(usb_transfer_t *transfer)
{
    ESP_LOGI("", "***!! RTLSDR_transfer_cb");
 
  if (driver_obj.dev_hdl == transfer->device_handle)
  {
    isRTLSDRPolling = false;
    if (transfer->status == 0) {
      if (transfer->actual_num_bytes == 64) 
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
    esp_err_t err = usb_host_interface_claim(driver_obj.client_hdl, driver_obj.dev_hdl,
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
    RTLSDRIn->device_handle = driver_obj.dev_hdl;
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
    RTLSDRIn->num_bytes = 64;

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
