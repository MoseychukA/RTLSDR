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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "usb/usb_host.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "usb/usb_host.h"
#include "esp_libusb.h"
#include "rtl-sdr.h"
#include "show_desc.hpp"

#define DAEMON_TASK_PRIORITY 2
#define CLASS_TASK_PRIORITY 3


static const char* TAG_MAIN = "MAIN";
static const char* TAG_DAEMON = "DAEMON";
static const char* TAG_CLASS = "CLASS";
//=============================================================================================

#define CLIENT_NUM_EVENT_MSG 5

#define ACTION_OPEN_DEV 0x01
#define ACTION_GET_DEV_INFO 0x02
#define ACTION_GET_DEV_DESC 0x04
#define ACTION_GET_CONFIG_DESC 0x08
#define ACTION_GET_STR_DESC 0x10
#define ACTION_CLOSE_DEV 0x20
#define ACTION_EXIT 0x40

#define DEFAULT_BUF_LENGTH (14 * 16384)

const TickType_t HOST_EVENT_TIMEOUT = 1;
const TickType_t CLIENT_EVENT_TIMEOUT = 1;

static rtlsdr_dev_t *rtldev = NULL;

void _client_event_callback(const usb_host_client_event_msg_t* event_msg, void* arg);
//static void client_event_cb(const usb_host_client_event_msg_t* event_msg, void* arg);
static void action_open_dev(class_driver_t* driver_obj);
static void action_get_info(class_driver_t* driver_obj);
void action_get_dev_desc1(class_driver_t* driver_obj);
static void action_get_config_desc(class_driver_t* driver_obj);
static void action_get_str_desc(class_driver_t* driver_obj);
static void aciton_close_dev(class_driver_t* driver_obj);


//=============================================================================================

typedef void (*usb_host_enum_cb_t)(const usb_config_desc_t* config_desc);
static usb_host_enum_cb_t _USB_host_enumerate;

/*
Этот фрагмент начальной настройки 
*/
void show_config_desc_full(const usb_config_desc_t* config_desc)
{
    // Полная расшифровка описания конфигурации.
    const uint8_t* p = &config_desc->val[0];
    static uint8_t USB_Class = 0;
    uint8_t bLength;
    for (int i = 0; i < config_desc->wTotalLength; i += bLength, p += bLength)
    {
        bLength = *p;
        if ((i + bLength) <= config_desc->wTotalLength) 
        {
            const uint8_t bDescriptorType = *(p + 1);
            switch (bDescriptorType) 
            {
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

                if (USB_Class == USB_CLASS_VENDOR_SPEC)
                {
                   // check_interface_desc_boot_RTLSDR(p);
                }
                break;
            case USB_B_DESCRIPTOR_TYPE_ENDPOINT:
                show_endpoint_desc(p);
                //if (isRTLSDR && transfer == NULL) prepare_endpoint(p);
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


void host_lib_daemon_task(usb_host_enum_cb_t enumeration_cb)
{
    //SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg; // Получить состояние семафора задач

    ESP_LOGI(TAG_DAEMON, "Installing USB Host Library");
    usb_host_config_t host_config = {
      .skip_phy_setup = false,
      .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    esp_err_t err;// = usb_host_install(&host_config);
    ESP_ERROR_CHECK(usb_host_install(&host_config));

    vTaskDelay(10); // Short delay to let client task spin up

    class_driver_t driver_obj = { 0 };
    usb_host_client_config_t client_config = {
     .is_synchronous = false,                       // Синхронные клиенты в настоящее время не поддерживаются. Установите это значение на ложь
     .max_num_event_msg = CLIENT_NUM_EVENT_MSG,
     .async = {
       .client_event_callback = _client_event_callback,    // Вызывается только вначале при старте
       .callback_arg = (void*)&driver_obj           //driver_obj.client_hdl
     },
    };

    err = usb_host_client_register(&client_config, &driver_obj.client_hdl);
    ESP_LOGI(TAG_DAEMON, "usb_host_client_register: %x", err);

    _USB_host_enumerate = enumeration_cb;
}
//==================================================================================
void usbh_task(void)
{
    uint32_t event_flags;
    static bool all_clients_gone = false;
    static bool all_dev_free = false;
    class_driver_t driver_obj = { 0 };

    /**
* @brief Обработка событий USB-хост-библиотеки
*
* - Эта функция управляет всей обработкой USB-хост-библиотеки и ее следует вызывать неоднократно в цикле.
* — Проверьте event_flags_ret, чтобы узнать, установлены ли флаги, указывающие на определенные события USB-хост-библиотеки.
* - Эта функция никогда не должна вызываться несколькими потоками одновременно.
*
* @note Эта функция может блокировать
* @param[in] timeout_ticks Таймаут в тиках для ожидания возникновения события
* @param[out] event_flags_ret Флаги событий, указывающие, какое событие USB-хост-библиотеки произошло.
* @return esp_err_t
*/
    esp_err_t err = usb_host_lib_handle_events(HOST_EVENT_TIMEOUT, &event_flags); 
    if (err == ESP_OK)
    {
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) /**< Регистрация всех клиентов в USB-хост-библиотеке отменена */
        {
            ESP_LOGI("", "No more clients");
            all_clients_gone = true;
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) /**< Хост-библиотека USB освободила все устройства */
        {
            ESP_LOGI("", "No more devices");
            all_dev_free = true;
        }

       // ESP_LOGI("", "event_flags %x ", event_flags);

    }
    else
    {
        if (err != ESP_ERR_TIMEOUT)
        {
            ESP_LOGI("", "usb_host_lib_handle_events: %x flags: %x", err, event_flags);
        }
    }



    /**
    * @brief Функция обработки клиента USB Host Library
    *
    * - Эта функция обрабатывает всю обработку клиента и ее следует вызывать неоднократно в цикле.
    * - Для конкретного клиента эта функция никогда не должна вызываться несколькими потоками одновременно.
    *
    * @note Эта функция может блокировать
    * @param[in] client_hdl Дескриптор клиента
    * @param[in] timeout_ticks Таймаут в тиках для ожидания возникновения события
    * @return esp_err_t
    */

   // err = usb_host_client_handle_events(driver_obj.client_hdl, CLIENT_EVENT_TIMEOUT);
    if ((err != ESP_OK) && (err != ESP_ERR_TIMEOUT))
    {
        ESP_LOGI("", "usb_host_client_handle_events: %x", err);
    }
}


//====================================================================================================

void setup()
{
 
   esp_deep_sleep_disable_rom_logging();// Отключить ведение журнала из кода ПЗУ после глубокого сна.
  // поднимаем первый UART
  Serial.begin(115200);
  while (!Serial && millis() < 1000);

  esp_log_level_set("*", ESP_LOG_VERBOSE);  // Выводим отладочные сообщения

  String ver_soft = __FILE__;
  int val_srt = ver_soft.lastIndexOf('\\');
  ver_soft.remove(0, val_srt + 1);
  val_srt = ver_soft.lastIndexOf('.');
  ver_soft.remove(val_srt);
  Serial.println(ver_soft);

  host_lib_daemon_task(show_config_desc_full);


  ESP_LOGI(TAG_MAIN, "*** End setup");
  vTaskDelay(1000); // Add a short delay to let the tasks run
}

void loop()
{
    usbh_task();

}

void _client_event_callback(const usb_host_client_event_msg_t* event_msg, void* arg)
{
    class_driver_t* driver_obj = (class_driver_t*)arg;
    esp_err_t err;
    switch (event_msg->event)
    {
        /**< A new device has been enumerated and added to the USB Host Library */
    case USB_HOST_CLIENT_EVENT_NEW_DEV:

        driver_obj->dev_addr = event_msg->new_dev.address;
        ESP_LOGI(TAG_MAIN, "New device address: %d", driver_obj->dev_addr);

        /* Настраиваем RTLSDR свисток  */
        err = rtlsdr_open(&rtldev, event_msg->new_dev.address, driver_obj->client_hdl);

       //  err = usb_host_device_open(Client_Handle, event_msg->new_dev.address, &Device_Handle);  // Старая настройка
        if (err != ESP_OK) ESP_LOGI("", "usb_host_device_open: %x", err);

        //++++++++++++++++++++ настройка R820T2 ++++++++++++++++++++++++++++++++++++
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
            printf("Tuned to %u Hz.\n", 1090000000);
        }

        uint32_t  freq_c = rtlsdr_get_center_freq(rtldev);
        fprintf(stderr, "*** Center freq %u \n", freq_c);

        r = rtlsdr_set_tuner_gain_mode(rtldev, 0);
        if (r != 0)
        {
            fprintf(stderr, "WARNING: Failed to set tuner gain.\n");
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

      //  uint32_t out_block_size = DEFAULT_BUF_LENGTH;

        //uint8_t* buffer = (uint8_t*)malloc(out_block_size * sizeof(uint8_t));  // Free memory: 8513103 bytes
        //int n_read = 2;
        //ESP_LOGI(TAG_CLASS, "[APP] Free memory: %ld bytes", esp_get_free_heap_size()); //Free memory : 8513103 bytes

        //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        usb_device_info_t dev_info;
        err = usb_host_device_info(driver_obj->dev_hdl, &dev_info);
        if (err != ESP_OK) ESP_LOGI("", "usb_host_device_info: %x", err);
        ESP_LOGI("", "speed: %d dev_addr %d vMaxPacketSize0 %d bConfigurationValue %d",
            dev_info.speed, dev_info.dev_addr, dev_info.bMaxPacketSize0,
            dev_info.bConfigurationValue);

        const usb_device_desc_t* dev_desc;
        err = usb_host_get_device_descriptor(driver_obj->dev_hdl, &dev_desc);
        if (err != ESP_OK) ESP_LOGI("", "usb_host_get_device_desc: %x", err);
        show_dev_desc(dev_desc);

        const usb_config_desc_t* config_desc;
        err = usb_host_get_active_config_descriptor(driver_obj->dev_hdl, &config_desc);
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





//static void client_event_cb(const usb_host_client_event_msg_t* event_msg, void* arg)
//{
//    ESP_LOGI(TAG_CLASS, "***++ client_event_cb");
//    vTaskDelay(100); // Add a short delay to let the tasks run
//    class_driver_t* driver_obj = (class_driver_t*)arg;
//    esp_err_t err;
//    switch (event_msg->event)
//    {
//    case USB_HOST_CLIENT_EVENT_NEW_DEV:
//        if (driver_obj->dev_addr == 0)
//        {
//            driver_obj->dev_addr = event_msg->new_dev.address;
//
//            //ESP_LOGI(TAG_CLASS, "*** usb_host_device_open : %x", driver_obj->dev_addr);
//            err = rtlsdr_open(&rtldev, event_msg->new_dev.address, driver_obj->client_hdl);
//            if (err != ESP_OK)
//            {
//                ESP_LOGI(TAG_CLASS, "*** usb_host_device_open error: %x", err);
//            }
//            else
//            {
//
//                ESP_LOGI(TAG_CLASS, "*** usb_host_device_open OK!!");
//            }
//
//            int r;
//            r = rtlsdr_set_sample_rate(rtldev, 2000000);
//            if (r < 0)
//            {
//                fprintf(stderr, "WARNING: Failed to set sample rate.\n");
//            }
//            else
//            {
//                fprintf(stderr, "Sampling at %u S/s.\n", 2000000);
//            }
//            r = rtlsdr_set_center_freq(rtldev, 1090000000);
//            if (r < 0)
//            {
//                fprintf(stderr, "WARNING: Failed to set center freq.\n");
//            }
//            else
//            {
//                printf("Tuned to %u Hz.\n", 1090000000);
//            }
//
//            uint32_t  freq_c = rtlsdr_get_center_freq(rtldev); 
//            fprintf(stderr, "*** Center freq %u \n", freq_c);
//
//            r = rtlsdr_set_tuner_gain_mode(rtldev, 0);
//            if (r != 0)
//            {
//                fprintf(stderr, "WARNING: Failed to set tuner gain.\n");
//            }
//            else
//            {
//                fprintf(stderr, "Tuner gain set to automatic.\n");
//            }
//
//            r = rtlsdr_reset_buffer(rtldev);
//            if (r < 0)
//            {
//                fprintf(stderr, "WARNING: Failed to reset buffers.\n");
//            }
//
//            vTaskDelay(100); // Short delay to let client task spin up
//
//            uint32_t out_block_size = DEFAULT_BUF_LENGTH;
//
//            uint8_t* buffer = (uint8_t*)malloc(out_block_size * sizeof(uint8_t));  // Free memory: 8513103 bytes
//            int n_read = 2;
//            ESP_LOGI(TAG_CLASS, "[APP] Free memory: %ld bytes", esp_get_free_heap_size()); //Free memory : 8513103 bytes
//            // uint32_t bytes_to_read = 0;
//            while (true)
//            {
//                printf("**!!dev->driver_obj1 - %d\n", rtldev);
//                r = rtlsdr_read_sync(rtldev, buffer, out_block_size, &n_read);                 //!! ??Не работает этот фрагмент
//                ESP_LOGI(TAG_CLASS, "*** Free memory: %ld bytes", esp_get_free_heap_size());
//
//
//                if (r < 0)
//                {
//                    fprintf(stderr, "** WARNING: sync read failed.\n");
//                    break;
//                }
//
//                for (int i = 0; i < out_block_size; i++)
//                    printf("%02X", buffer[i]);
//
//                if ((uint32_t)n_read < out_block_size)
//                {
//                    fprintf(stderr, "Short read, samples lost, exiting!\n");
//                    break;
//                }
//            }
//        }
//        break;
//    case USB_HOST_CLIENT_EVENT_DEV_GONE:
//        if (driver_obj->dev_hdl != NULL)
//        {
//            // Cancel any other actions and close the device next
//            driver_obj->actions = ACTION_CLOSE_DEV;
//        }
//        break;
//    default:
//        // Should never occur
//        abort();
//    }
//}

static void action_open_dev(class_driver_t* driver_obj)
{
    if (driver_obj->dev_addr != 0)
    {
        ESP_LOGI(TAG_CLASS, "*** Opening device at address %d", driver_obj->dev_addr);
        ESP_ERROR_CHECK(usb_host_device_open(driver_obj->client_hdl, driver_obj->dev_addr, &driver_obj->dev_hdl));
        // Get the device's information next
        driver_obj->actions &= ~ACTION_OPEN_DEV;        // Снимаем флаг ACTION_OPEN_DEV
        driver_obj->actions |= ACTION_GET_DEV_INFO;     // Устанавливаем флаг ACTION_GET_DEV_INFO
    }
    else
    {
        ESP_LOGI(TAG_CLASS, "!!!assert1 false");
    }
}

static void action_get_info(class_driver_t* driver_obj)
{
    ESP_LOGI(TAG_CLASS, "here2");
    if (driver_obj->dev_hdl != NULL)
    {
        ESP_LOGI(TAG_CLASS, "Getting device information");
        usb_device_info_t dev_info;
        ESP_ERROR_CHECK(usb_host_device_info(driver_obj->dev_hdl, &dev_info));
        ESP_LOGI(TAG_CLASS, "\t%s speed", (dev_info.speed == USB_SPEED_LOW) ? "Low" : "Full");
        ESP_LOGI(TAG_CLASS, "\tbConfigurationValue %d", dev_info.bConfigurationValue);
        // Todo: Print string descriptors

        // Get the device descriptor next
        driver_obj->actions &= ~ACTION_GET_DEV_INFO;     // Снимаем флаг 
        driver_obj->actions |= ACTION_GET_DEV_DESC;      // Устанавливаем флаг
    }
    else
    {
        ESP_LOGI(TAG_CLASS, "!!!assert2 false");

    }
}
void action_get_dev_desc1(class_driver_t* driver_obj)
{
    if (driver_obj->dev_hdl != NULL)
    {
        ESP_LOGI(TAG_CLASS, "Getting device descriptor");
        const usb_device_desc_t* dev_desc;
        ESP_ERROR_CHECK(usb_host_get_device_descriptor(driver_obj->dev_hdl, &dev_desc));
        usb_print_device_descriptor(dev_desc);
        // Get the device's config descriptor next
        driver_obj->actions &= ~ACTION_GET_DEV_DESC;    // Снимаем флаг 
        driver_obj->actions |= ACTION_GET_CONFIG_DESC;  // Устанавливаем флаг
    }
    else
    {
        ESP_LOGI(TAG_CLASS, "!!!assert3 false");
    }
}

static void action_get_config_desc(class_driver_t* driver_obj)
{
    if (driver_obj->dev_hdl != NULL)
    {
        ESP_LOGI(TAG_CLASS, "Getting config descriptor");
        const usb_config_desc_t* config_desc;
        ESP_ERROR_CHECK(usb_host_get_active_config_descriptor(driver_obj->dev_hdl, &config_desc));
        usb_print_config_descriptor(config_desc, NULL);
        // Get the device's string descriptors next
        driver_obj->actions &= ~ACTION_GET_CONFIG_DESC;  // Снимаем флаг 
        driver_obj->actions |= ACTION_GET_STR_DESC;      // Устанавливаем флаг 
    }
    else
    {
        ESP_LOGI(TAG_CLASS, "!!!assert4 false");
    }
}

static void action_get_str_desc(class_driver_t* driver_obj)
{
    if (driver_obj->dev_hdl != NULL)
    {
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
        //Ничего не делать, пока устройство не отключится
        driver_obj->actions &= ~ACTION_GET_STR_DESC;    // Снимаем флаг. Заканчиваем сбор информации об устройстве
    }
    else
    {
        ESP_LOGI(TAG_CLASS, "!!!assert5 false");
    }
}

static void aciton_close_dev(class_driver_t* driver_obj)
{
    ESP_ERROR_CHECK(usb_host_device_close(driver_obj->client_hdl, driver_obj->dev_hdl));
    driver_obj->dev_hdl = NULL;
    driver_obj->dev_addr = 0;
    // We need to exit the event handler loop
    driver_obj->actions &= ~ACTION_CLOSE_DEV;   // Снимаем флаг 
    driver_obj->actions |= ACTION_EXIT;         // Устанавливаем флаг
}

//===========================================================================================
