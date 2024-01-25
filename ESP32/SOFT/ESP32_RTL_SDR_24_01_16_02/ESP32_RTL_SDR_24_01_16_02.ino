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
#include "rtl-sdr.h"
#include "esp_libusb.h"
#include "class_driver.h"
#include <elapsedMillis.h>

#define WDT_TIMEOUT 8

// Определяем на каком ядре будет выполнятся основнвя программа
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

//SemaphoreHandle_t signaling_sem = NULL;


//===========================================================================
int thisByte = 33;
bool start_setup = false;


void  ASCII_Task(void* arg)
{

    SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;

    // ?? Сигнал задаче драйвера класса о том, что хост-библиотека установлена
    xSemaphoreGive(signaling_sem);
    vTaskDelay(10); // Short delay to let client task spin up

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

        // Ожидаем удаления
        xSemaphoreGive(signaling_sem);
        vTaskSuspend(NULL);

    }
}

//=============================================================================================


//=============================================================================================



void setup()
{
    // поднимаем первый UART
    Serial.begin(115200);
    while (!Serial && millis() < 1000);

    esp_log_level_set("*", ESP_LOG_VERBOSE);
    ESP_LOGE("TAG", "Error");
    ESP_LOGW("TAG", "Warning");
    ESP_LOGI("TAG", "Info");
    ESP_LOGD("TAG", "Debug");
    ESP_LOGV("TAG", "Verbose");

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
 
    SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();

  // В этом варианте создания задачи также можно указать, на каком ядре она будет выполняться (актуально только для многоядерных ESP)

  // xTaskCreatePinnedToCore(
  //Task1code, /* Функция, содержащая код задачи */
  // "Task1", /* Название задачи */
  //     10000, /* Размер стека в словах */
  //     NULL, /* Параметр создаваемой задачи */
  //     0, /* Приоритет задачи */
  //     & Task1, /* Идентификатор задачи */
  //     0); /* Ядро, на котором будет выполняться задача */

    TaskHandle_t ASCII_task_hdl;


  xTaskCreatePinnedToCore(
      ASCII_Task                   // 
      , "ASCII"
      , 2048                       // Размер стека
      , (void*)signaling_sem       // установить семафор
      , 4                          // Priority
      , &ASCII_task_hdl            // С дескриптором задачи мы сможем манипулировать этой задачей.
      , ARDUINO_RUNNING_CORE       // Ядро, на котором будет выполняться задача
  );



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
  //for (int i = 0; i < 2; i++)
  //{
  //    esp_task_wdt_reset();
  //    //xSemaphoreTake(signaling_sem, portMAX_DELAY);
  //    xSemaphoreTake(signaling_sem, 1000);
  //    esp_task_wdt_reset();
  //}

  // Delete the tasks
  // vTaskDelete(class_driver_task_hdl);
  // vTaskDelete(daemon_task_hdl);
  vTaskDelete (ASCII_task_hdl);


  esp_task_wdt_reset();

  start_setup = true;
  Serial.println("End setup");

}

void loop()
{

 
  esp_task_wdt_reset();
}
