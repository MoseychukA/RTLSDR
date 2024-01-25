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
#include "Configuration_ESP32.h"  // Основные настройки программы

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "usb/usb_host.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "usb/usb_host.h"
#include <esp_task_wdt.h>
#include "rtl-sdr.h"
#include "librtlsdr.h"

#define WDT_TIMEOUT 8

#define DAEMON_TASK_PRIORITY 2
#define CLASS_TASK_PRIORITY 3

#define CLIENT_NUM_EVENT_MSG 5

#define ACTION_OPEN_DEV 0x01
#define ACTION_GET_DEV_INFO 0x02
#define ACTION_GET_DEV_DESC 0x04
#define ACTION_GET_CONFIG_DESC 0x08
#define ACTION_GET_STR_DESC 0x10
#define ACTION_CLOSE_DEV 0x20
#define ACTION_EXIT 0x40

#define DEFAULT_BUF_LENGTH (14 * 16384)

static const char* TAG_MAIN = "MAIN";
static const char* TAG_DAEMON = "DAEMON";
static const char* TAG_CLASS = "CLASS";
static const char* TAG_EVENT = "EVENT";

typedef struct
{
    usb_host_client_handle_t client_hdl;
    uint8_t dev_addr;
    usb_device_handle_t dev_hdl;
    uint32_t actions;
} class_driver_t_main;


static rtlsdr_dev_t *rtldev = NULL;

static void action_open_dev(class_driver_t_main* driver_obj);
static void action_get_info(class_driver_t_main* driver_obj);
void action_get_dev_desc1(class_driver_t_main* driver_obj);
static void action_get_config_desc(class_driver_t_main* driver_obj);
static void action_get_str_desc(class_driver_t_main* driver_obj);
static void aciton_close_dev(class_driver_t_main* driver_obj);
static void client_event_cb(const usb_host_client_event_msg_t* event_msg, void* arg);




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
          * @brief Обработка событий USB-хост-библиотеки
          * - Эта функция управляет всей обработкой USB-хост-библиотеки и ее следует вызывать неоднократно в цикле.
          * — Проверьте event_flags_ret, чтобы узнать, установлены ли флаги, указывающие на определенные события USB-хост-библиотеки.
          * - Эта функция никогда не должна вызываться несколькими потоками одновременно.
          * @note Эта функция может блокировать
          * @param[in] timeout_ticks Тайм-аут в тиках для ожидания возникновения события
          * @param[out] event_flags_ret Флаги событий, указывающие, какое событие USB-хост-библиотеки произошло.
          * @return esp_err_t
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
void class_driver_task(void* arg)
{
    SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;
    class_driver_t_main driver_obj = { 0 };

    // Подождите, пока задача демона не установит USB-хост-библиотеку.
    xSemaphoreTake(signaling_sem, portMAX_DELAY);

    ESP_LOGI(TAG_CLASS, "Registering Client");
    usb_host_client_config_t client_config = {
        .is_synchronous = false, // Synchronous clients currently not supported. Set this to false
        .max_num_event_msg = CLIENT_NUM_EVENT_MSG,
        .async = {
            .client_event_callback = client_event_cb,
            .callback_arg = (void*)&driver_obj,
        },
    };
    ESP_ERROR_CHECK(usb_host_client_register(&client_config, &driver_obj.client_hdl));

    while (1)
    {
        if (driver_obj.actions == 0)
        {
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

SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();

void setup()
{
    // поднимаем первый UART
    Serial.begin(115200);
    while (!Serial && millis() < 1000);

    esp_log_level_set("*", ESP_LOG_VERBOSE);  // Выводим отладочные сообщения
 
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
 
   /* SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();*/

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
  // Создаем задачу драйвера класса
  xTaskCreatePinnedToCore(
      class_driver_task,
      "class",
      4096,
      (void*)signaling_sem,
      CLASS_TASK_PRIORITY,
      &class_driver_task_hdl,
      0);

  vTaskDelay(10); // Add a short delay to let the tasks run

  esp_task_wdt_reset();

  ESP_LOGI(TAG_MAIN, "*** End setup");

}

void loop()
{
 
  esp_task_wdt_reset();
}


static void client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg)
{
    esp_task_wdt_reset();
    class_driver_t_main *driver_obj = (class_driver_t_main *)arg;
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

            uint8_t* buffer;
            /*uint8_t * */
            buffer = (uint8_t*) malloc(out_block_size * sizeof(uint8_t));
            int n_read = 2;
            ESP_LOGI(TAG_EVENT, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
            // uint32_t bytes_to_read = 0;
            while (true)
            {
                esp_task_wdt_reset();

                r = rtlsdr_read_sync(rtldev, buffer, out_block_size, &n_read);


                ESP_LOGI(TAG_EVENT, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
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
                ESP_LOGI(TAG_EVENT, "*** out_block_size %u bytes", out_block_size);
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
            // ESP_LOGI(TAG_MAIN, "here");
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

static void action_open_dev(class_driver_t_main *driver_obj)
{
    assert(driver_obj->dev_addr != 0);
    ESP_LOGI(TAG_MAIN, "Opening device at address %d", driver_obj->dev_addr);
    ESP_ERROR_CHECK(usb_host_device_open(driver_obj->client_hdl, driver_obj->dev_addr, &driver_obj->dev_hdl));
    // Get the device's information next
    driver_obj->actions &= ~ACTION_OPEN_DEV;
    driver_obj->actions |= ACTION_GET_DEV_INFO;
}

static void action_get_info(class_driver_t_main *driver_obj)
{
    ESP_LOGI(TAG_MAIN, "here2");
    assert(driver_obj->dev_hdl != NULL);
    ESP_LOGI(TAG_MAIN, "Getting device information");
    usb_device_info_t dev_info;
    ESP_ERROR_CHECK(usb_host_device_info(driver_obj->dev_hdl, &dev_info));
    ESP_LOGI(TAG_MAIN, "\t%s speed", (dev_info.speed == USB_SPEED_LOW) ? "Low" : "Full");
    ESP_LOGI(TAG_MAIN, "\tbConfigurationValue %d", dev_info.bConfigurationValue);
    // Todo: Print string descriptors

    // Get the device descriptor next
    driver_obj->actions &= ~ACTION_GET_DEV_INFO;
    driver_obj->actions |= ACTION_GET_DEV_DESC;
}
void action_get_dev_desc1(class_driver_t_main *driver_obj)
{
    assert(driver_obj->dev_hdl != NULL);
    ESP_LOGI(TAG_MAIN, "Getting device descriptor");
    const usb_device_desc_t *dev_desc;
    ESP_ERROR_CHECK(usb_host_get_device_descriptor(driver_obj->dev_hdl, &dev_desc));
    usb_print_device_descriptor(dev_desc);
    // Get the device's config descriptor next
    driver_obj->actions &= ~ACTION_GET_DEV_DESC;
    driver_obj->actions |= ACTION_GET_CONFIG_DESC;
}

static void action_get_config_desc(class_driver_t_main *driver_obj)
{
    assert(driver_obj->dev_hdl != NULL);
    ESP_LOGI(TAG_MAIN, "Getting config descriptor");
    const usb_config_desc_t *config_desc;
    ESP_ERROR_CHECK(usb_host_get_active_config_descriptor(driver_obj->dev_hdl, &config_desc));
    usb_print_config_descriptor(config_desc, NULL);
    // Get the device's string descriptors next
    driver_obj->actions &= ~ACTION_GET_CONFIG_DESC;
    driver_obj->actions |= ACTION_GET_STR_DESC;
}

static void action_get_str_desc(class_driver_t_main *driver_obj)
{
    assert(driver_obj->dev_hdl != NULL);
    usb_device_info_t dev_info;
    ESP_ERROR_CHECK(usb_host_device_info(driver_obj->dev_hdl, &dev_info));
    if (dev_info.str_desc_manufacturer)
    {
        ESP_LOGI(TAG_MAIN, "Getting Manufacturer string descriptor");
        usb_print_string_descriptor(dev_info.str_desc_manufacturer);
    }
    if (dev_info.str_desc_product)
    {
        ESP_LOGI(TAG_MAIN, "Getting Product string descriptor");
        usb_print_string_descriptor(dev_info.str_desc_product);
    }
    if (dev_info.str_desc_serial_num)
    {
        ESP_LOGI(TAG_MAIN, "Getting Serial Number string descriptor");
        usb_print_string_descriptor(dev_info.str_desc_serial_num);
    }
    // Nothing to do until the device disconnects
    driver_obj->actions &= ~ACTION_GET_STR_DESC;
}

static void aciton_close_dev(class_driver_t_main *driver_obj)
{
    ESP_ERROR_CHECK(usb_host_device_close(driver_obj->client_hdl, driver_obj->dev_hdl));
    driver_obj->dev_hdl = NULL;
    driver_obj->dev_addr = 0;
    // We need to exit the event handler loop
    driver_obj->actions &= ~ACTION_CLOSE_DEV;
    driver_obj->actions |= ACTION_EXIT;
}

