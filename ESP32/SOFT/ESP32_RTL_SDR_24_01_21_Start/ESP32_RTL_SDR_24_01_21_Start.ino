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


*/
#include <Arduino.h>              // define I/O functions
#include <stdint.h>
#include <stdio.h>                // define I/O functions
#include <stdlib.h>
#include "string.h"
#include "SPIFFS.h"
#include "FS.h"
#include <Wire.h>                 // 

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "usb/usb_host.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "usb/usb_host.h"

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

#define WDT_TIMEOUT 8

#define DAEMON_TASK_PRIORITY 2
#define CLASS_TASK_PRIORITY 3

//extern void class_driver_task(void* arg);

static const char* TAG_MAIN = "MAIN";
static const char* TAG_DAEMON = "DAEMON";
static const char* TAG_CLASS = "CLASS";

//=============================================================================================
static void host_lib_daemon_task(void* arg)
{
  SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg; // Получить состояние семафора задач

  ESP_LOGI(TAG_DAEMON, "Installing USB Host Library");
  usb_host_config_t host_config = {
    .skip_phy_setup = false,
    .intr_flags = ESP_INTR_FLAG_LEVEL1,
  };
  ESP_ERROR_CHECK(usb_host_install(&host_config));

  // Сигнал задаче драйвера класса о том, что хост-библиотека установлена
  xSemaphoreGive(signaling_sem);    // Инициализация хоста выполнена. Разблокируем семафор
  vTaskDelay(10); // Short delay to let client task spin up

  bool has_clients = true;
  bool has_devices = true;
  while (has_clients || has_devices)  // Периодически проверяем наличие клиента или устройства
  {
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

    uint32_t event_flags;
    ESP_ERROR_CHECK(usb_host_lib_handle_events(portMAX_DELAY, &event_flags));
    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS)
    {
      has_clients = false;
    }
    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE)
    {
      has_devices = false;
    }
  }

  // Клиентов нет, завершаем контроль USB хоста
  ESP_LOGI(TAG_DAEMON, "*** No more clients and devices");

  // Uninstall the USB Host Library
  ESP_ERROR_CHECK(usb_host_uninstall());
  // Wait to be deleted
  xSemaphoreGive(signaling_sem);   // Разблокируем семафор
  vTaskSuspend(NULL);              // Приостановить выполнение задачи.
}

//=============================================================================================

//=============================================================================================

static void _client_event_callback(const usb_host_client_event_msg_t* event_msg, void* arg)
{
    /*
        usb_host_client_handle_t client_hdl;
        uint8_t dev_addr;
        usb_device_handle_t dev_hdl;
        uint32_t actions;
    */

  
    class_driver_t* driver_obj = (class_driver_t*)arg;


    /**
      * @brief Сообщение о событии клиента
      *
      * Сообщения о клиентских событиях отправляются каждому клиенту USB-хост-библиотеки, чтобы уведомить их о различных
      * События USB-хост-библиотеки, такие как:
      * - Добавление новых устройств
      * - Удаление существующих устройств
      *
      * @note Структура сообщения о событии имеет объединение с членами, соответствующими каждому конкретному событию. Судя по событию
      * тип, доступ должен быть доступен только к соответствующему полю элемента.
      */

       
    switch (event_msg->event)
    {
    case USB_HOST_CLIENT_EVENT_NEW_DEV:
        if (driver_obj->dev_addr == 0)    // Определили что подключилось новое устройство.
        {
            driver_obj->dev_addr = event_msg->new_dev.address; // Адрес подключенного устройства.
            ESP_LOGI(TAG_CLASS, "driver_obj->dev_addr: %ld ", driver_obj->dev_addr);
            /*Здесь мы должны отправить настройки RTLSDR */
            //   rtlsdr_open(&rtldev, event_msg->new_dev.address, driver_obj->client_hdl); 
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
            //        uint8_t* buffer = malloc(out_block_size * sizeof(uint8_t));
            //        int n_read = 2;
            //        ESP_LOGI(TAG_CLASS, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
            //        // uint32_t bytes_to_read = 0;
            //        while (true)
            //        {
            //            r = rtlsdr_read_sync(rtldev, buffer, out_block_size, &n_read);
            //            ESP_LOGI(TAG_CLASS, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
            //            if (r < 0)
            //            {
            //                fprintf(stderr, "WARNING: sync read failed.\n");
            //                break;
            //            }

            //            // if ((bytes_to_read > 0) && (bytes_to_read < (uint32_t)n_read))
            //            // {
            //            //     n_read = bytes_to_read;
            //            //     do_exit = 1;
            //            // }
            //            for (int i = 0; i < out_block_size; i++)
            //                fprintf(stdout, "%02X", buffer[i]);
            //            // if (fwrite(buffer, 1, n_read, stdout) != (size_t)n_read)
            //            // {
            //            //     fprintf(stderr, "Short write, samples lost, exiting!\n");
            //            //     break;
            //            // }

            //            if ((uint32_t)n_read < out_block_size)
            //            {
            //                fprintf(stderr, "Short read, samples lost, exiting!\n");
            //                break;
            //            }

            //            // if (bytes_to_read > 0)
            //            //     bytes_to_read -= n_read;
            //        }
            //        // esp_action_get_dev_desc(rtldev);
            //        // driver_obj->dev_hdl = *(usb_device_handle_t *)get_driver_obj(rtldev);
            //        // Open the device next
            //        // driver_obj->actions |= ACTION_OPEN_DEV;
            //        // ESP_LOGI(TAG, "here");
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

//=============================================================================================




//=============================== class_driver.c ======================================================
static void action_open_dev(class_driver_t* driver_obj)
{
    if (driver_obj->dev_addr != 0)
    {
        ESP_LOGI(TAG_CLASS, "Opening device at address %d", driver_obj->dev_addr);
        ESP_ERROR_CHECK(usb_host_device_open(driver_obj->client_hdl, driver_obj->dev_addr, &driver_obj->dev_hdl));
        // Get the device's information next
        driver_obj->actions &= ~ACTION_OPEN_DEV;
        driver_obj->actions |= ACTION_GET_DEV_INFO;
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
        driver_obj->actions &= ~ACTION_GET_DEV_INFO;
        driver_obj->actions |= ACTION_GET_DEV_DESC;
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
        driver_obj->actions &= ~ACTION_GET_DEV_DESC;
        driver_obj->actions |= ACTION_GET_CONFIG_DESC;
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
        driver_obj->actions &= ~ACTION_GET_CONFIG_DESC;
        driver_obj->actions |= ACTION_GET_STR_DESC;
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
        // Nothing to do until the device disconnects
        driver_obj->actions &= ~ACTION_GET_STR_DESC;
    }
}

static void aciton_close_dev(class_driver_t* driver_obj)
{
    ESP_ERROR_CHECK(usb_host_device_close(driver_obj->client_hdl, driver_obj->dev_hdl));
    driver_obj->dev_hdl = NULL;
    driver_obj->dev_addr = 0;
    // We need to exit the event handler loop
    driver_obj->actions &= ~ACTION_CLOSE_DEV;
    driver_obj->actions |= ACTION_EXIT;
}
//===================================================================================================


/*  Постоянный контроль USB HOST */
void class_driver_task(void* arg)
{
    SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;

    class_driver_t driver_obj = { 0 };

    // Wait until daemon task has installed USB Host Library
    xSemaphoreTake(signaling_sem, portMAX_DELAY);

    ESP_LOGI(TAG_CLASS, "Waiting for client registration");
    usb_host_client_config_t client_config = {
      .is_synchronous = false, // Synchronous clients currently not supported. Set this to false
      .max_num_event_msg = CLIENT_NUM_EVENT_MSG,
      .async = {
        .client_event_callback = _client_event_callback,  // Настроить проверку ответа USB HOST
        .callback_arg = (void*)&driver_obj,
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



//=============================================================================================

void setup()
{
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

  // В этом варианте создания задачи также можно указать, на каком ядре она будет выполняться (актуально только для многоядерных ESP)

  //  xTaskCreatePinnedToCore(
  //  Task1code, /* Функция, содержащая код задачи */
  //  "Task1", /* Название задачи */
  //  10000,   /* Размер стека в словах */
  //  NULL,    /* Параметр создаваемой задачи */
  //  0,       /* Приоритет задачи */
  //  & Task1, /* Идентификатор задачи */
  //  0); /* Ядро, на котором будет выполняться задача */


  TaskHandle_t daemon_task_hdl;
  TaskHandle_t class_driver_task_hdl;


  // Create daemon task
  // Настроить и инициализировать USB HOST
  xTaskCreatePinnedToCore(
    host_lib_daemon_task,
    "daemon",
    4096,
    (void*)signaling_sem,
    DAEMON_TASK_PRIORITY,
    &daemon_task_hdl,
    0);

  // Create the class driver task
  // Создаем задачу драйвера класса. Выполнение основной программы.

  xTaskCreatePinnedToCore(
    class_driver_task,
    "class",
    4096,
    (void*)signaling_sem,
    CLASS_TASK_PRIORITY,
    &class_driver_task_hdl,
    0);

  vTaskDelay(10); // Add a short delay to let the tasks run

  ESP_LOGI(TAG_MAIN, "*** End setup");

}

void loop()
{


}


