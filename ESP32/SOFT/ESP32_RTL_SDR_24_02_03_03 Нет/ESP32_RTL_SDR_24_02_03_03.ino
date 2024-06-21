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

static const char* TAG_CLASS = "CLASS";
static rtlsdr_dev_t *rtldev = NULL;

static void client_event_cb(const usb_host_client_event_msg_t* event_msg, void* arg);
static void action_open_dev(class_driver_t* driver_obj);
static void action_get_info(class_driver_t* driver_obj);
void action_get_dev_desc1(class_driver_t* driver_obj);
static void action_get_config_desc(class_driver_t* driver_obj);
static void action_get_str_desc(class_driver_t* driver_obj);
static void aciton_close_dev(class_driver_t* driver_obj);

//=============================================================================================
/* Настройка USB порта в режиме HOST */
static void host_lib_daemon_task(void* arg)
{
    SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg; // Получить состояние семафора задач

    ESP_LOGI(TAG_DAEMON, "Installing USB Host Library"); 
    usb_host_config_t host_config = {
      .skip_phy_setup = false,
      .intr_flags = ESP_INTR_FLAG_LEVEL1,  // Принять вектор прерывания уровня 1 (самый низкий приоритет)
    };


    //esp_err_t err = usb_host_install(&host_config);
    //ESP_LOGI("", "usb_host_install: %x", err);

    ESP_ERROR_CHECK(usb_host_install(&host_config));

    // Сигнал задаче драйвера класса о том, что хост-библиотека установлена
    xSemaphoreGive(signaling_sem);    // Инициализация хоста выполнена. Разблокируем семафор
    vTaskDelay(10); // Short delay to let client task spin up

    bool has_clients = true;
    bool has_devices = true;
    while (has_clients || has_devices)  // Постоянно проверяем наличие клиента или устройства
    {
        uint32_t event_flags;

        /*
           @brief Обработка событий USB-хост-библиотеки
           - Эта функция управляет всей обработкой USB-хост-библиотеки и ее следует вызывать неоднократно в цикле.
           — Проверьте event_flags_ret, чтобы узнать, установлены ли флаги, указывающие на определенные события USB-хост-библиотеки.
           - Эта функция никогда не должна вызываться несколькими потоками одновременно.
           @note Эта функция может блокировать
           @param[in] timeout_ticks Тайм-аут в тиках для ожидания возникновения события
           @param[out] event_flags_ret Флаги событий, указывающие, какое событие USB-хост-библиотеки произошло.
           @return esp_err_t
        */

  
         /**
          * Макрос, который можно использовать для проверки кода ошибки,
          * и завершить программу в случае, если код не является ESP_OK.
          * Печатает код ошибки, местоположение ошибки и сбой оператора для серийного вывода.
          *
          * Отключено, если утверждения отключены.
          */

        ESP_ERROR_CHECK(usb_host_lib_handle_events(portMAX_DELAY, &event_flags));

        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS)
        {
            has_clients = false;
            ESP_LOGI(TAG_DAEMON, "has_clients = false");
            vTaskDelay(100); // Add a short delay to let the tasks run
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE)
        {
            has_devices = false;
            ESP_LOGI(TAG_DAEMON, "has_devices = false");
            vTaskDelay(100); // Add a short delay to let the tasks run
        }

        //ESP_LOGI(TAG_DAEMON, "event_flags %d", event_flags);
        //ESP_LOGI(TAG_DAEMON, "has_clients %d", has_clients);
        //ESP_LOGI(TAG_DAEMON, "has_devices %d\n", has_devices);
    }

    vTaskDelay(100); // Add a short delay to let the tasks run
    // Клиентов нет, завершаем контроль USB хоста
    ESP_LOGI(TAG_DAEMON, "*** No more clients and devices");

    // Uninstall the USB Host Library
    ESP_ERROR_CHECK(usb_host_uninstall());
    vTaskDelay(100); // Add a short delay to let the tasks run
    // Wait to be deleted
    xSemaphoreGive(signaling_sem);   // Разблокируем семафор
    vTaskSuspend(NULL);              // Приостановить выполнение задачи.
}

//=============================================================================================


//=============================================================================================
void class_driver_task(void* arg)
{

    ESP_LOGI(TAG_CLASS, "*** Ожидание регистрации клиента");
    SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;
    class_driver_t driver_obj = { 0 };
    //Подождите, пока задача демона не установит USB-хост-библиотеку.
    xSemaphoreTake(signaling_sem, portMAX_DELAY);

    ESP_LOGI(TAG_CLASS, "Демон установил USB-хост-библиотеку");
    //ESP_LOGI(TAG_CLASS, "Waiting for client registration2");

    usb_host_client_config_t client_config = {
      .is_synchronous = false, // Синхронные клиенты в настоящее время не поддерживаются. Установите это значение на ложь
      .max_num_event_msg = CLIENT_NUM_EVENT_MSG,
      .async = {
        .client_event_callback = client_event_cb,  // Вызывается только вначале при старте
        .callback_arg = (void*)&driver_obj,
      },
    };

    /**
    * @brief Регистрация клиента USB-хост-библиотеки
    *
    * - Эта функция регистрирует клиент USB Host Library.
    * - После регистрации клиента его функция обработки usb_host_client_handle_events() должна вызываться повторно.
    *
    * @param[in] client_config Конфигурация клиента
    * @param[out] client_hdl_ret Дескриптор клиента
    * @return esp_err_t
    */

    esp_err_t r = usb_host_client_register(&client_config, &driver_obj.client_hdl);
    if (r != ESP_OK)
    {
        ESP_LOGI(TAG_CLASS, "*** !! usb_host_client_register false %d", r);
    }
    else
    {
         ESP_LOGI(TAG_CLASS, "usb_host_client_register успешно зарегистрирован %d", r);
    }


    while (1)
    {
        if (driver_obj.actions == 0)  // Ожидаем выполнение функции usb_host_client_handle_events
        {
            ESP_LOGI(TAG_CLASS, "!! Ожидаем выполнение функции usb_host_client_handle_events");
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

            /*usb_host_client_handle_events(driver_obj.client_hdl, portMAX_DELAY);*/

            esp_err_t r = usb_host_client_handle_events(driver_obj.client_hdl, portMAX_DELAY);
            if (r != ESP_OK)
            {
                ESP_LOGI(TAG_CLASS, "*** !! usb_host_client_handle_events false %d", r);
            }
            else
            {
                ESP_LOGI(TAG_CLASS, "*** usb_host_client_handle_events sucesfull %d", r);
                ESP_LOGI(TAG_CLASS, "*** driver_obj.actions %d", driver_obj.actions);
            }
        }
        else
        {
            if (driver_obj.actions & ACTION_OPEN_DEV)
            {
                ESP_LOGI(TAG_CLASS, "++ action_open_dev");
                action_open_dev(&driver_obj);
            }
            if (driver_obj.actions & ACTION_GET_DEV_INFO)
            {
                ESP_LOGI(TAG_CLASS, "++ action_get_info");
                action_get_info(&driver_obj);
            }
            if (driver_obj.actions & ACTION_GET_DEV_DESC)
            {
                ESP_LOGI(TAG_CLASS, "++ action_get_dev_desc1");
                action_get_dev_desc1(&driver_obj);
            }
            if (driver_obj.actions & ACTION_GET_CONFIG_DESC)
            {
                ESP_LOGI(TAG_CLASS, "++ action_get_config_desc");
                action_get_config_desc(&driver_obj);
            }
            if (driver_obj.actions & ACTION_GET_STR_DESC)
            {
                ESP_LOGI(TAG_CLASS, "++ action_get_str_desc");
                action_get_str_desc(&driver_obj);
            }
            if (driver_obj.actions & ACTION_CLOSE_DEV)
            {
                ESP_LOGI(TAG_CLASS, "++ aciton_close_dev");
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

  SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();

  TaskHandle_t daemon_task_hdl;
  TaskHandle_t class_driver_task_hdl;
  // Create daemon task
  xTaskCreatePinnedToCore(host_lib_daemon_task,
      "daemon",
      4096,
      (void*)signaling_sem,
      DAEMON_TASK_PRIORITY,
      &daemon_task_hdl,
      0);

  // Create the class driver task
  xTaskCreatePinnedToCore(class_driver_task,
      "class",
      4096,
      (void*)signaling_sem,
      CLASS_TASK_PRIORITY,
      &class_driver_task_hdl,
      0);

  vTaskDelay(10); // Add a short delay to let the tasks run

  // Wait for the tasks to complete
  for (int i = 0; i < 2; i++)
  {
      xSemaphoreTake(signaling_sem, portMAX_DELAY);
  }

  // Delete the tasks
  vTaskDelete(class_driver_task_hdl);
  vTaskDelete(daemon_task_hdl);


  ESP_LOGI(TAG_MAIN, "*** End setup");
  vTaskDelay(1000); // Add a short delay to let the tasks run
}

void loop()
{
    //SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();

    //TaskHandle_t daemon_task_hdl;
    //TaskHandle_t class_driver_task_hdl;
    //// Create daemon task
    //xTaskCreatePinnedToCore(host_lib_daemon_task,
    //    "daemon",
    //    4096,
    //    (void*)signaling_sem,
    //    DAEMON_TASK_PRIORITY,
    //    &daemon_task_hdl,
    //    0);
    //// Create the class driver task
    //xTaskCreatePinnedToCore(class_driver_task,
    //    "class",
    //    4096,
    //    (void*)signaling_sem,
    //    CLASS_TASK_PRIORITY,
    //    &class_driver_task_hdl,
    //    0);

    //vTaskDelay(10); // Add a short delay to let the tasks run

    //// Wait for the tasks to complete
    //for (int i = 0; i < 2; i++)
    //{
    //    xSemaphoreTake(signaling_sem, portMAX_DELAY);
    //}

    //// Delete the tasks
    //vTaskDelete(class_driver_task_hdl);
    //vTaskDelete(daemon_task_hdl);

}


static void client_event_cb(const usb_host_client_event_msg_t* event_msg, void* arg)
{
    ESP_LOGI(TAG_CLASS, "***++ client_event_cb");
    vTaskDelay(100); // Add a short delay to let the tasks run
    class_driver_t* driver_obj = (class_driver_t*)arg;
    esp_err_t err;
    switch (event_msg->event)
    {
    case USB_HOST_CLIENT_EVENT_NEW_DEV:
        if (driver_obj->dev_addr == 0)
        {
            driver_obj->dev_addr = event_msg->new_dev.address;

            //ESP_LOGI(TAG_CLASS, "*** usb_host_device_open : %x", driver_obj->dev_addr);
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

            vTaskDelay(100); // Short delay to let client task spin up

            uint32_t out_block_size = DEFAULT_BUF_LENGTH;

            uint8_t* buffer = (uint8_t*)malloc(out_block_size * sizeof(uint8_t));  // Free memory: 8513103 bytes
            int n_read = 2;
            ESP_LOGI(TAG_CLASS, "[APP] Free memory: %ld bytes", esp_get_free_heap_size()); //Free memory : 8513103 bytes
            // uint32_t bytes_to_read = 0;
            while (true)
            {
                printf("**!!dev->driver_obj1 - %d\n", rtldev);
                r = rtlsdr_read_sync(rtldev, buffer, out_block_size, &n_read);                 //!! ??Не работает этот фрагмент
                ESP_LOGI(TAG_CLASS, "*** Free memory: %ld bytes", esp_get_free_heap_size());


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
