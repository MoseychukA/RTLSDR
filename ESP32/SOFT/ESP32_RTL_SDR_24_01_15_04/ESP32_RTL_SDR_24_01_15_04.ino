/*
 
 */
#include <Arduino.h>              // define I/O functions
#include <stdint.h>
#include <stdio.h>                // define I/O functions
#include <stdlib.h>
#include "string.h"
#include "SPIFFS.h"
#include "FS.h"
#include <Wire.h>                 // 
#include "Configuration_ESP32.h"  // Основные настройки программы

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "usb/usb_host.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "usb/usb_host.h"
#include <esp_task_wdt.h>
#include "show_desc.hpp"
#include "rtl-sdr.h"
#include "esp_libusb.h"
#include <elapsedMillis.h>


#define WDT_TIMEOUT 8
bool start_setup = false;

// Определяем на каком ядре будет выполнятся основнвя программа
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif


const TickType_t HOST_EVENT_TIMEOUT = 1;
const TickType_t CLIENT_EVENT_TIMEOUT = 1;

usb_host_client_handle_t Client_Handle;
usb_device_handle_t Device_Handle;
typedef void (*usb_host_enum_cb_t)(const usb_config_desc_t* config_desc);
static usb_host_enum_cb_t _USB_host_enumerate;



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
} class_driver_t_main;

static const char* TAG = "CLASS";
//static rtlsdr_dev_t* rtldev = NULL;

static void action_open_dev(class_driver_t_main* driver_obj);
static void action_get_info(class_driver_t_main* driver_obj);
void action_get_dev_desc1(class_driver_t_main* driver_obj);
static void action_get_config_desc(class_driver_t_main* driver_obj);
static void action_get_str_desc(class_driver_t_main* driver_obj);
static void aciton_close_dev(class_driver_t_main* driver_obj);
static void client_event_cb(const usb_host_client_event_msg_t* event_msg, void* arg);


void _client_event_callback(const usb_host_client_event_msg_t* event_msg, void* arg)
{
    esp_err_t err;
    switch (event_msg->event)
    {
          case USB_HOST_CLIENT_EVENT_NEW_DEV:  
              if (event_msg->new_dev.address == 0)
              {
                  //    ESP_LOGI("", " ***!! New device address: %d", event_msg->new_dev.address);
//    err = usb_host_device_open(Client_Handle, event_msg->new_dev.address, &Device_Handle);
//    if (err != ESP_OK) ESP_LOGI("", "usb_host_device_open: %x", err);


                 // driver_obj->dev_addr = event_msg->new_dev.address;
                  rtlsdr_open(&Client_Handle, event_msg->new_dev.address, driver_obj->client_hdl);

                 // rtlsdr_open(&rtldev, event_msg->new_dev.address, driver_obj->client_hdl);

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

                  // uint8_t* buffer;
                  // //!!buffer = (uint8_t*)malloc(out_block_size * sizeof(uint8_t));
                  //// uint8_t* buffer = malloc(out_block_size * sizeof(uint8_t));
                  // int n_read = 2;
                  // //!!ESP_LOGI(TAG, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
                  // // uint32_t bytes_to_read = 0;
                  // while (true)
                  // {
                  //     r = rtlsdr_read_sync(rtldev, buffer, out_block_size, &n_read);
                  //     ESP_LOGI(TAG, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
                  //     if (r < 0)
                  //     {
                  //         fprintf(stderr, "WARNING: sync read failed.\n");
                  //         break;
                  //     }

                  //     // if ((bytes_to_read > 0) && (bytes_to_read < (uint32_t)n_read))
                  //     // {
                  //     //     n_read = bytes_to_read;
                  //     //     do_exit = 1;
                  //     // }
                  //     for (int i = 0; i < out_block_size; i++)
                  //         fprintf(stdout, "%02X", buffer[i]);
                  //     // if (fwrite(buffer, 1, n_read, stdout) != (size_t)n_read)
                  //     // {
                  //     //     fprintf(stderr, "Short write, samples lost, exiting!\n");
                  //     //     break;
                  //     // }

                  //     if ((uint32_t)n_read < out_block_size)
                  //     {
                  //         fprintf(stderr, "Short read, samples lost, exiting!\n");
                  //         break;
                  //     }

                  //     // if (bytes_to_read > 0)
                  //     //     bytes_to_read -= n_read;
                  // }
                  // // esp_action_get_dev_desc(rtldev);
                  // // driver_obj->dev_hdl = *(usb_device_handle_t *)get_driver_obj(rtldev);
                  // // Open the device next
                  // // driver_obj->actions |= ACTION_OPEN_DEV;
                  // // ESP_LOGI(TAG, "here");
              }
              break;
          case USB_HOST_CLIENT_EVENT_DEV_GONE:
              ESP_LOGI("", "Device Gone handle: %x", event_msg->dev_gone.dev_hdl);
              break;
          //default:
          //    // Should never occur
          //    abort();
          //}





                //    // /**< Новое устройство было перечислено и добавлено в USB-хост-библиотеку */
                //case USB_HOST_CLIENT_EVENT_NEW_DEV:
                //    ESP_LOGI("", " ***!! New device address: %d", event_msg->new_dev.address);
                //    err = usb_host_device_open(Client_Handle, event_msg->new_dev.address, &Device_Handle);
                //    if (err != ESP_OK) ESP_LOGI("", "usb_host_device_open: %x", err);

                //    usb_device_info_t dev_info;
                //    err = usb_host_device_info(Device_Handle, &dev_info);
                //    if (err != ESP_OK) ESP_LOGI("", "usb_host_device_info: %x", err);
                //    ESP_LOGI("", "speed: %d dev_addr %d vMaxPacketSize0 %d bConfigurationValue %d",
                //        dev_info.speed, dev_info.dev_addr, dev_info.bMaxPacketSize0,
                //        dev_info.bConfigurationValue);

                //    const usb_device_desc_t* dev_desc;
                //    err = usb_host_get_device_descriptor(Device_Handle, &dev_desc);
                //    if (err != ESP_OK) ESP_LOGI("", "usb_host_get_device_desc: %x", err);
                //    show_dev_desc(dev_desc);

                //    const usb_config_desc_t* config_desc;
                //    err = usb_host_get_active_config_descriptor(Device_Handle, &config_desc);
                //    if (err != ESP_OK) ESP_LOGI("", "usb_host_get_config_desc: %x", err);
                //    (*_USB_host_enumerate)(config_desc);
                //    break;
                //    /**< A device opened by the client is now gone */
                //case USB_HOST_CLIENT_EVENT_DEV_GONE:
                //    ESP_LOGI("", "Device Gone handle: %x", event_msg->dev_gone.dev_hdl);
                //    break;
        default:
        ESP_LOGI("", "Unknown value %d", event_msg->event);
        break;
    }
}


//void _client_event_callback(const usb_host_client_event_msg_t* event_msg, void* arg)
//{
//    esp_err_t err;
//    switch (event_msg->event)
//    {
//        // /**< Новое устройство было перечислено и добавлено в USB-хост-библиотеку */
//      case USB_HOST_CLIENT_EVENT_NEW_DEV: 
//        ESP_LOGI("", " ***!! New device address: %d", event_msg->new_dev.address);
//        err = usb_host_device_open(Client_Handle, event_msg->new_dev.address, &Device_Handle);
//        if (err != ESP_OK) ESP_LOGI("", "usb_host_device_open: %x", err);
//
//        usb_device_info_t dev_info;
//        err = usb_host_device_info(Device_Handle, &dev_info);
//        if (err != ESP_OK) ESP_LOGI("", "usb_host_device_info: %x", err);
//        ESP_LOGI("", "speed: %d dev_addr %d vMaxPacketSize0 %d bConfigurationValue %d",
//            dev_info.speed, dev_info.dev_addr, dev_info.bMaxPacketSize0,
//            dev_info.bConfigurationValue);
//
//        const usb_device_desc_t* dev_desc;
//        err = usb_host_get_device_descriptor(Device_Handle, &dev_desc);
//        if (err != ESP_OK) ESP_LOGI("", "usb_host_get_device_desc: %x", err);
//        show_dev_desc(dev_desc);
//
//        const usb_config_desc_t* config_desc;
//        err = usb_host_get_active_config_descriptor(Device_Handle, &config_desc);
//        if (err != ESP_OK) ESP_LOGI("", "usb_host_get_config_desc: %x", err);
//        (*_USB_host_enumerate)(config_desc);
//        break;
//        /**< A device opened by the client is now gone */
//    case USB_HOST_CLIENT_EVENT_DEV_GONE:
//        ESP_LOGI("", "Device Gone handle: %x", event_msg->dev_gone.dev_hdl);
//        break;
//    default:
//        ESP_LOGI("", "Unknown value %d", event_msg->event);
//        break;
//    }
//}

// Reference: esp-idf/examples/peripherals/usb/host/usb_host_lib/main/usb_host_lib_main.c


void usbh_setup(usb_host_enum_cb_t enumeration_cb)
{

   // class_driver_t_main driver_obj = { 0 };

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
         // .callback_arg = (void*)&driver_obj,
          .callback_arg = Client_Handle
      }
    };
   // err = usb_host_client_register(&client_config, &driver_obj.client_hdl);
    err = usb_host_client_register(&client_config, &Client_Handle);
    ESP_LOGI("", "*** usb_host_client_register: %x", err);
   // ESP_LOGI("", "*** driver_obj.actions: %x", driver_obj.actions);

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

    err = usb_host_client_handle_events(Client_Handle, CLIENT_EVENT_TIMEOUT);
    if ((err != ESP_OK) && (err != ESP_ERR_TIMEOUT))
    {
        //ESP_LOGI("", "usb_host_client_handle_events: %x", err);
    }
}

bool isRTLSDR = false;



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
    /*
    * @brief Функция, позволяющая клиенту заявить права на интерфейс устройства.
  *
  * - Клиент должен заявить права на интерфейс устройства, прежде чем пытаться связаться с любой из его конечных точек.
  * - Если интерфейс заявлен клиентом, он не может быть востребован каким-либо другим клиентом.
  *
  * @note Эта функция может блокировать
  * @param[in] client_hdl Дескриптор клиента
  * @param[in] dev_hdl Дескриптор устройства
  * @param[in] bInterfaceNumber Номер интерфейса
  * @param[in] bAlternateSetting Номер альтернативной настройки интерфейса
  * @return esp_err_t
    */

    const usb_intf_desc_t* intf = (const usb_intf_desc_t*)p;

    if ((intf->bInterfaceClass == USB_CLASS_VENDOR_SPEC) &&
        (intf->bInterfaceSubClass == 0xff) &&
        (intf->bInterfaceProtocol == 0xff))
    {
        isRTLSDR = true;
        ESP_LOGI("", " ***Claiming a boot RTLSDR!");
        esp_err_t err = usb_host_interface_claim(Client_Handle, Device_Handle,
            intf->bInterfaceNumber, intf->bAlternateSetting);
        if (err != ESP_OK) ESP_LOGI("", "usb_host_interface_claim failed: %x", err);
    }
}



void prepare_endpoint(const void *p)
{
  const usb_ep_desc_t *endpoint = (const usb_ep_desc_t *)p;
  esp_err_t err;

  // должно быть прерывание для HID
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

void show_config_desc_full(const usb_config_desc_t *config_desc)
{
  // Полная расшифровка описания конфигурации.
  const uint8_t *p = &config_desc->val[0];
  static uint8_t USB_Class = 0;
  uint8_t bLength;
  for (int i = 0; i < config_desc->wTotalLength; i+=bLength, p+=bLength) 
  {
    bLength = *p;
    if ((i + bLength) <= config_desc->wTotalLength) {
      const uint8_t bDescriptorType = *(p + 1);
      switch (bDescriptorType) 
      {

        case USB_B_DESCRIPTOR_TYPE_DEVICE: //
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
          ESP_LOGI("", "*** USB_B_DESCRIPTOR_TYPE_INTERFACE: 0x%x", USB_Class);
          if (USB_Class == USB_CLASS_HID) 
          {
              check_interface_desc_boot_keyboard(p);
          }
          else if (USB_Class == USB_CLASS_VENDOR_SPEC)
          {
              ESP_LOGI("", "*** USB_B_DESCRIPTOR_TYPE_INTERFACE: RTL SDR");
              check_interface_desc_boot_RTLSDR(p);
          }
          break;
        case USB_B_DESCRIPTOR_TYPE_ENDPOINT:
          show_endpoint_desc(p);
          if (isKeyboard && KeyboardIn == NULL) prepare_endpoint(p);
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
          if (USB_Class == USB_CLASS_HID) 
          {
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

//===========================================================================
int thisByte = 33;

void  ASCII_Task(void* pvParameters);      // 

void  ASCII_Task(void* pvParameters)
{
    for (;; )
    {
        if (start_setup)
        {
            Serial.write(thisByte);

            Serial.print(", dec: ");

            Serial.print(thisByte, DEC);

            Serial.print(", hex: ");
            // prints value as string in hexadecimal (base 16):
            Serial.print(thisByte, HEX);

            Serial.print(", oct: ");
            // prints value as string in octal (base 8);
            Serial.print(thisByte, OCT);

            Serial.print(", bin: ");
            // prints value as string in binary (base 2) also prints ending line break:
            Serial.println(thisByte, BIN);

            // if printed last visible character '~' or 126, stop:
            if (thisByte == 126)
            {
                thisByte = 33;
                Serial.println("End ASCII");
                start_setup = false;
            }
            // go on to the next character
            thisByte++;

            esp_task_wdt_reset();
            vTaskDelay(10);
            yield();
        }

        esp_task_wdt_reset();
        vTaskDelay(10);
        yield();
    }
}

//=============================================================================================







void setup()
{
    // поднимаем первый UART
    Serial.begin(115200);
    while (!Serial && millis() < 1000);

    esp_log_level_set("*", ESP_LOG_VERBOSE);
    //ESP_LOGE("TAG", "Error");
    //ESP_LOGW("TAG", "Warning");
    //ESP_LOGI("TAG", "Info");
    //ESP_LOGD("TAG", "Debug");
    //ESP_LOGV("TAG", "Verbose");

    esp_task_wdt_init(WDT_TIMEOUT, false);  //enable panic so ESP32 restarts
    esp_task_wdt_add(NULL);                 //add current thread to WDT watch
    esp_task_wdt_reset();
    delay(1000);
    esp_task_wdt_reset();

    String ver_soft = __FILE__;
    int val_srt = ver_soft.lastIndexOf('\\');
    ver_soft.remove(0, val_srt + 1);
    val_srt = ver_soft.lastIndexOf('.');
    ver_soft.remove(val_srt);
    Serial.println(ver_soft);
    esp_task_wdt_reset();

    usbh_setup(show_config_desc_full);



  // В этом варианте создания задачи также можно указать, на каком ядре она будет выполняться (актуально только для многоядерных ESP)

  // xTaskCreatePinnedToCore(
  //Task1code, /* Функция, содержащая код задачи */
  // "Task1", /* Название задачи */
  //     10000, /* Размер стека в словах */
  //     NULL, /* Параметр создаваемой задачи */
  //     0, /* Приоритет задачи */
  //     & Task1, /* Идентификатор задачи */
  //     0); /* Ядро, на котором будет выполняться задача */



  xTaskCreatePinnedToCore(
      ASCII_Task            // 
      , "ASCII"
      , 2048                       // Размер стека
      , NULL                       // Когда параметр не используется, просто передайте NULL
      , 3                          // Priority
      , NULL                       // С дескриптором задачи мы сможем манипулировать этой задачей.
      , 0//ARDUINO_RUNNING_CORE       // Ядро, на котором будет выполняться задача
  );


  SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();

  //  TaskHandle_t daemon_task_hdl;
  //  TaskHandle_t class_driver_task_hdl;


  // Create daemon task
  //xTaskCreatePinnedToCore(host_lib_daemon_task,
  //    "daemon",
  //    4096,
  //    (void*)signaling_sem,
  //    DAEMON_TASK_PRIORITY,
  //    &daemon_task_hdl,
  //    0);
  // 


  //// Create the class driver task
  //xTaskCreatePinnedToCore(
  //    class_driver_task,
  //    "class",
  //    4096,
  //    (void*)signaling_sem,
  //    CLASS_TASK_PRIORITY,
  //    &class_driver_task_hdl,
  //    0);

  vTaskDelay(10); // Add a short delay to let the tasks run

  // Wait for the tasks to complete
  for (int i = 0; i < 2; i++)
  {
      esp_task_wdt_reset();
      //xSemaphoreTake(signaling_sem, portMAX_DELAY);
      xSemaphoreTake(signaling_sem, 1000);
      esp_task_wdt_reset();
  }

  // Delete the tasks
  // vTaskDelete(class_driver_task_hdl);
  // vTaskDelete(daemon_task_hdl);

  esp_task_wdt_reset();

  start_setup = false;
  Serial.println("End setup");

}

void loop()
{

  usbh_task();
  esp_task_wdt_reset();
  if (isKeyboardReady && !isKeyboardPolling && (KeyboardTimer > KeyboardInterval)) 
  {
    KeyboardIn->num_bytes = 8;
    esp_err_t err = usb_host_transfer_submit(KeyboardIn);
    if (err != ESP_OK) 
    {
      ESP_LOGI("", "usb_host_transfer_submit In fail: %x", err);
    }
    isKeyboardPolling = true;
    KeyboardTimer = 0;
  }

  esp_task_wdt_reset();
}


static void action_open_dev(class_driver_t_main* driver_obj)
{
   /**
        *@brief Открыть устройство
        *
        * -Эта функция позволяет клиенту открыть устройство
        * -Клиент должен сначала открыть устройство, прежде чем пытаться его использовать(например, отправлять переводы, запросы устройств и т.д.).
        *
        * @param[in] client_hdl Дескриптор клиента
        * @param[in] dev_addr Адрес устройства
        * @param[out] dev_hdl_ret Дескриптор устройства
        * @return esp_err_t
        */

    assert(driver_obj->dev_addr != 0);
    ESP_LOGI(TAG, "Opening device at address %d", driver_obj->dev_addr);
    ESP_ERROR_CHECK(usb_host_device_open(driver_obj->client_hdl, driver_obj->dev_addr, &driver_obj->dev_hdl));
    // Get the device's information next
    driver_obj->actions &= ~ACTION_OPEN_DEV;
    driver_obj->actions |= ACTION_GET_DEV_INFO;
}

static void action_get_info(class_driver_t_main* driver_obj)
{
   /**
  * @brief Получить информацию об устройстве
  * - Эта функция получает основную информацию об устройстве.
  * - Устройство необходимо сначала открыть, прежде чем пытаться получить его информацию.
  * @note Эта функция может блокировать
  * @param[in] dev_hdl Дескриптор устройства
  * @param[out] dev_info Информация об устройстве
  * @return esp_err_t
  */

    ESP_LOGI(TAG, "here2");
    assert(driver_obj->dev_hdl != NULL);
    ESP_LOGI(TAG, "Getting device information");
    usb_device_info_t dev_info;
    ESP_ERROR_CHECK(usb_host_device_info(driver_obj->dev_hdl, &dev_info));
    ESP_LOGI(TAG, "\t%s speed", (dev_info.speed == USB_SPEED_LOW) ? "Low" : "Full");
    ESP_LOGI(TAG, "\tbConfigurationValue %d", dev_info.bConfigurationValue);
    // Todo: Print string descriptors

    // Get the device descriptor next
    driver_obj->actions &= ~ACTION_GET_DEV_INFO;
    driver_obj->actions |= ACTION_GET_DEV_DESC;
}
void action_get_dev_desc1(class_driver_t_main* driver_obj)
{
    /**
  * @brief Получить дескриптор устройства устройства
  *
  * - Клиент должен сначала вызвать usb_host_device_open()
  * - Передача управления не отправляется. Дескриптор устройства кэшируется при перечислении.
  * - Эта функция просто возвращает указатель на кэшированный дескриптор.
  *
  * @note Передача управления не отправляется. Дескриптор устройства кэшируется при перечислении.
  * @param[in] dev_hdl Дескриптор устройства
  * @param[out] device_desc Дескриптор устройства
  * @return esp_err_t
  */

    assert(driver_obj->dev_hdl != NULL);
    ESP_LOGI(TAG, "Getting device descriptor");
    const usb_device_desc_t* dev_desc;
    ESP_ERROR_CHECK(usb_host_get_device_descriptor(driver_obj->dev_hdl, &dev_desc));

   
  /* @brief Дескриптор устройства печати
  * @param devc_desc Дескриптор устройства
  */
    usb_print_device_descriptor(dev_desc);
    // Далее получаем дескриптор конфигурации устройства
    driver_obj->actions &= ~ACTION_GET_DEV_DESC;
    driver_obj->actions |= ACTION_GET_CONFIG_DESC;
}

static void action_get_config_desc(class_driver_t_main* driver_obj)
{
    assert(driver_obj->dev_hdl != NULL);
    ESP_LOGI(TAG, "Getting config descriptor");
    const usb_config_desc_t* config_desc;
    ESP_ERROR_CHECK(usb_host_get_active_config_descriptor(driver_obj->dev_hdl, &config_desc));
    usb_print_config_descriptor(config_desc, NULL);
    // Get the device's string descriptors next
    driver_obj->actions &= ~ACTION_GET_CONFIG_DESC;
    driver_obj->actions |= ACTION_GET_STR_DESC;
}

static void action_get_str_desc(class_driver_t_main* driver_obj)
{
    assert(driver_obj->dev_hdl != NULL);
    usb_device_info_t dev_info;
    ESP_ERROR_CHECK(usb_host_device_info(driver_obj->dev_hdl, &dev_info));
    if (dev_info.str_desc_manufacturer)
    {
        ESP_LOGI(TAG, "Getting Manufacturer string descriptor");
        usb_print_string_descriptor(dev_info.str_desc_manufacturer);
    }
    if (dev_info.str_desc_product)
    {
        ESP_LOGI(TAG, "Getting Product string descriptor");
        usb_print_string_descriptor(dev_info.str_desc_product);
    }
    if (dev_info.str_desc_serial_num)
    {
        ESP_LOGI(TAG, "Getting Serial Number string descriptor");
        usb_print_string_descriptor(dev_info.str_desc_serial_num);
    }
    // Nothing to do until the device disconnects
    driver_obj->actions &= ~ACTION_GET_STR_DESC;
}

static void aciton_close_dev(class_driver_t_main* driver_obj)
{
    ESP_ERROR_CHECK(usb_host_device_close(driver_obj->client_hdl, driver_obj->dev_hdl));
    driver_obj->dev_hdl = NULL;
    driver_obj->dev_addr = 0;
    // We need to exit the event handler loop
    driver_obj->actions &= ~ACTION_CLOSE_DEV;
    driver_obj->actions |= ACTION_EXIT;
}

static void client_event_cb(const usb_host_client_event_msg_t* event_msg, void* arg)
{
    class_driver_t_main* driver_obj = (class_driver_t_main*)arg;
    //switch (event_msg->event)
    //{
    //case USB_HOST_CLIENT_EVENT_NEW_DEV:
    //    if (driver_obj->dev_addr == 0)
    //    {
    //        driver_obj->dev_addr = event_msg->new_dev.address;
    //        rtlsdr_open(&rtldev, event_msg->new_dev.address, driver_obj->client_hdl);
    //        int r;
    //        r = rtlsdr_set_sample_rate(rtldev, 2000000);
    //        if (r < 0)
    //        {
    //            fprintf(stderr, "WARNING: Failed to set sample rate.\n");
    //        }
    //        else
    //        {
    //            fprintf(stderr, "Sampling at %u S/s.\n", 2000000);
    //        }
    //        r = rtlsdr_set_center_freq(rtldev, 1090000000);
    //        if (r < 0)
    //        {
    //            fprintf(stderr, "WARNING: Failed to set center freq.\n");
    //        }
    //        else
    //        {
    //            fprintf(stderr, "Tuned to %u Hz.\n", 1090000000);
    //        }
    //        r = rtlsdr_set_tuner_gain_mode(rtldev, 0);
    //        if (r != 0)
    //        {
    //            fprintf(stderr, "WARNING: Failed to set tuner gain.\n");
    //        }
    //        else
    //        {
    //            fprintf(stderr, "Tuner gain set to automatic.\n");
    //        }
    //        // r = rtlsdr_set_freq_correction(rtldev, 0);
    //        // if (r < 0)
    //        // {
    //        //     fprintf(stderr, "WARNING: Failed to set ppm error.\n");
    //        // }
    //        // else
    //        // {
    //        //     fprintf(stderr, "Tuner error set to %i ppm.\n", 0);
    //        // }
    //        r = rtlsdr_reset_buffer(rtldev);
    //        if (r < 0)
    //        {
    //            fprintf(stderr, "WARNING: Failed to reset buffers.\n");
    //        }
    //        vTaskDelay(100); // Short delay to let client task spin up

    //        uint32_t out_block_size = DEFAULT_BUF_LENGTH;

    //        // uint8_t* buffer;
    //        // //!!buffer = (uint8_t*)malloc(out_block_size * sizeof(uint8_t));
    //        //// uint8_t* buffer = malloc(out_block_size * sizeof(uint8_t));
    //        // int n_read = 2;
    //        // //!!ESP_LOGI(TAG, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
    //        // // uint32_t bytes_to_read = 0;
    //        // while (true)
    //        // {
    //        //     r = rtlsdr_read_sync(rtldev, buffer, out_block_size, &n_read);
    //        //     ESP_LOGI(TAG, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
    //        //     if (r < 0)
    //        //     {
    //        //         fprintf(stderr, "WARNING: sync read failed.\n");
    //        //         break;
    //        //     }

    //        //     // if ((bytes_to_read > 0) && (bytes_to_read < (uint32_t)n_read))
    //        //     // {
    //        //     //     n_read = bytes_to_read;
    //        //     //     do_exit = 1;
    //        //     // }
    //        //     for (int i = 0; i < out_block_size; i++)
    //        //         fprintf(stdout, "%02X", buffer[i]);
    //        //     // if (fwrite(buffer, 1, n_read, stdout) != (size_t)n_read)
    //        //     // {
    //        //     //     fprintf(stderr, "Short write, samples lost, exiting!\n");
    //        //     //     break;
    //        //     // }

    //        //     if ((uint32_t)n_read < out_block_size)
    //        //     {
    //        //         fprintf(stderr, "Short read, samples lost, exiting!\n");
    //        //         break;
    //        //     }

    //        //     // if (bytes_to_read > 0)
    //        //     //     bytes_to_read -= n_read;
    //        // }
    //        // // esp_action_get_dev_desc(rtldev);
    //        // // driver_obj->dev_hdl = *(usb_device_handle_t *)get_driver_obj(rtldev);
    //        // // Open the device next
    //        // // driver_obj->actions |= ACTION_OPEN_DEV;
    //        // // ESP_LOGI(TAG, "here");
    //    }
    //    break;
    //case USB_HOST_CLIENT_EVENT_DEV_GONE:
    //    if (driver_obj->dev_hdl != NULL)
    //    {
    //        // Cancel any other actions and close the device next
    //        driver_obj->actions = ACTION_CLOSE_DEV;
    //    }
    //    break;
    //default:
    //    // Should never occur
    //    abort();
    //}
}
