/*
   rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
   Copyright (C) 2012-2013 by Steve Markgraf <steve@steve-m.de>
   Copyright (C) 2012 by Dimitri Stolnikov <horiz0n@gmx.net>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __RTL_SDR_H
#define __RTL_SDR_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "rtl-sdr_export.h"

typedef struct rtlsdr_dev rtlsdr_dev_t;

RTLSDR_API void esp_action_get_dev_desc(rtlsdr_dev_t *dev);
RTLSDR_API int rtlsdr_open(rtlsdr_dev_t **dev, uint8_t index, usb_host_client_handle_t client_hdl);

RTLSDR_API int rtlsdr_close(rtlsdr_dev_t *dev);

/* configuration functions */

/*!
    Установите частоты кварцевого генератора, используемые для RTL2832 и микросхемы тюнера.

    Обычно обе микросхемы используют одну и ту же тактовую частоту. Изменение времени может иметь смысл, если
    вы подключаете к тюнеру внешнюю тактовую частоту или для компенсации
    Ошибка частоты (и частоты дискретизации), вызванная оригинальным (дешевым) кристаллом.

    ПРИМЕЧАНИЕ. Вызовите эту функцию только в том случае, если вы полностью понимаете последствия.

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \param rtl_freq Значение частоты, используемое для синхронизации RTL2832 в Гц
    \param Tuner_freq Значение частоты, используемое для синхронизации микросхемы тюнера в Гц
    \return 0 в случае успеха
*/
RTLSDR_API int rtlsdr_set_xtal_freq(rtlsdr_dev_t *dev, uint32_t rtl_freq,
                                    uint32_t tuner_freq);

/*!
    Получите частоты кварцевого генератора, используемые для RTL2832 и микросхемы тюнера.

    Обычно обе микросхемы используют одну и ту же тактовую частоту.

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \param rtl_freq Значение частоты, используемое для синхронизации RTL2832 в Гц
    \param Tuner_freq Значение частоты, используемое для синхронизации микросхемы тюнера в Гц
    \return 0 в случае успеха
*/
RTLSDR_API int rtlsdr_get_xtal_freq(rtlsdr_dev_t *dev, uint32_t *rtl_freq,
                                    uint32_t *tuner_freq);

/*!
    Получите строки USB-устройства.
    ПРИМЕЧАНИЕ. Строковые аргументы должны обеспечивать пространство до 256 байт.

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \param имя производителя производителя, может быть NULL
    \param Product Название продукта, может быть NULL
    \param серийный серийный номер, может быть NULL
    \return 0 в случае успеха
*/
RTLSDR_API int rtlsdr_get_usb_strings(rtlsdr_dev_t *dev, char *manufact, char *product, char *serial);

/*!
    Запишите EEPROM устройства.

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \param буфер данных для записи
    \param адрес смещения, куда должны быть записаны данные
    \param len длина данных
    \return 0 в случае успеха
    \return -1, если дескриптор устройства недействителен
    \return -2, если размер EEPROM превышен
    \return -3, если EEPROM не найден
*/

RTLSDR_API int rtlsdr_write_eeprom(rtlsdr_dev_t *dev, uint8_t *data, uint8_t offset, uint16_t len);
/*!
    Прочтите EEPROM устройства.

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \param буфер данных, куда должны быть записаны данные
    \param адрес смещения, откуда следует читать данные
    \param len длина данных
    \return 0 в случае успеха
    \return -1, если дескриптор устройства недействителен
    \return -2, если размер EEPROM превышен
    \return -3, если EEPROM не найден
*/

RTLSDR_API int rtlsdr_read_eeprom(rtlsdr_dev_t *dev, uint8_t *data, uint8_t offset, uint16_t len);

RTLSDR_API int rtlsdr_set_center_freq(rtlsdr_dev_t *dev, uint32_t freq);

/*!
   Get actual frequency the device is tuned to.

   \param dev the device handle given by rtlsdr_open()
   \return 0 on error, frequency in Hz otherwise
*/
RTLSDR_API uint32_t rtlsdr_get_center_freq(rtlsdr_dev_t *dev);

/*!
   Set the frequency correction value for the device.

   \param dev the device handle given by rtlsdr_open()
   \param ppm correction value in parts per million (ppm)
   \return 0 on success
*/
RTLSDR_API int rtlsdr_set_freq_correction(rtlsdr_dev_t *dev, int ppm);

/*!
   Get actual frequency correction value of the device.

   \param dev the device handle given by rtlsdr_open()
   \return correction value in parts per million (ppm)
*/
RTLSDR_API int rtlsdr_get_freq_correction(rtlsdr_dev_t *dev);

enum rtlsdr_tuner
{
  RTLSDR_TUNER_UNKNOWN = 0,
  RTLSDR_TUNER_E4000,
  RTLSDR_TUNER_FC0012,
  RTLSDR_TUNER_FC0013,
  RTLSDR_TUNER_FC2580,
  RTLSDR_TUNER_R820T,
  RTLSDR_TUNER_R828D
};

/*!
    Узнайте тип тюнера.

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \return RTLSDR_TUNER_UNKNOWN в случае ошибки, в противном случае введите тип тюнера
*/
RTLSDR_API enum rtlsdr_tuner rtlsdr_get_tuner_type(rtlsdr_dev_t *dev);

/*!
    Получите список усилений, поддерживаемых тюнером.

    ПРИМЕЧАНИЕ. Аргумент усиления должен быть предварительно выделен вызывающей стороной. Если НУЛЬ
    вместо этого будет возвращено количество доступных значений усиления.

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \param получает массив значений усиления. В десятых долях дБ 115 означает 11,5 дБ.
    \return <= 0 в случае ошибки, в противном случае количество доступных (возвращенных) значений усиления
*/
RTLSDR_API int rtlsdr_get_tuner_gains(rtlsdr_dev_t *dev, int *gains);

/*!
    Установите усиление для устройства.
    Чтобы это работало, необходимо включить режим ручного усиления.

    Допустимые значения усиления (в десятых долях дБ) для тюнера E4000:
    -10, 15, 40, 65, 90, 115, 140, 165, 190,
    215, 240, 290, 340, 420, 430, 450, 470, 490

    Действительные значения усиления можно запросить с помощью функции \ref rtlsdr_get_tuner_gains.

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \param усиление в десятых долях дБ, 115 означает 11,5 дБ.
    \return 0 в случае успеха
*/
RTLSDR_API int rtlsdr_set_tuner_gain(rtlsdr_dev_t *dev, int gain);

/*!
    Установите пропускную способность устройства.

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \param Полоса пропускания в Гц. Ноль означает автоматический выбор полосы пропускания.
    \return 0 в случае успеха
*/
RTLSDR_API int rtlsdr_set_tuner_bandwidth(rtlsdr_dev_t *dev, uint32_t bw);

/*!
    Получите фактическое усиление, на которое настроено устройство.

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \return 0 в случае ошибки, усиление в десятых долях дБ, 115 означает 11,5 дБ.
*/
RTLSDR_API int rtlsdr_get_tuner_gain(rtlsdr_dev_t *dev);

/*!
    Установите усиление промежуточной частоты для устройства.

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \param stage Номер ступени усиления промежуточной частоты (от 1 до 6 для E4000)
    \param усиление в десятых долях дБ, -30 означает -3,0 дБ.
    \return 0 в случае успеха
*/
RTLSDR_API int rtlsdr_set_tuner_if_gain(rtlsdr_dev_t *dev, int stage, int gain);

/*!
    Установите режим усиления (автоматический/ручной) для устройства.
    Для работы функции настройки усиления необходимо включить режим ручного усиления.

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \param режим ручного усиления, 1 означает, что режим ручного усиления должен быть включен.
    \return 0 в случае успеха
*/
RTLSDR_API int rtlsdr_set_tuner_gain_mode(rtlsdr_dev_t *dev, int manual);
/*!
    Установите частоту дискретизации для устройства, а также выберите фильтры основной полосы.
    в соответствии с запрошенной частотой дискретизации для тюнеров, где это возможно.

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \param samp_rate частота дискретизации, которую необходимо установить, возможные значения:
   225001 - 300000 Гц
   900001 - 3200000 Гц
   потери выборки следует ожидать для ставок > 2400000
    \return 0 в случае успеха, -EINVAL в случае недействительной скорости
*/
RTLSDR_API int rtlsdr_set_sample_rate(rtlsdr_dev_t *dev, uint32_t rate);

/*!
   Get actual sample rate the device is configured to.

   \param dev the device handle given by rtlsdr_open()
   \return 0 on error, sample rate in Hz otherwise
*/
RTLSDR_API uint32_t rtlsdr_get_sample_rate(rtlsdr_dev_t *dev);

/*!
   Enable test mode that returns an 8 bit counter instead of the samples.
   The counter is generated inside the RTL2832.

   \param dev the device handle given by rtlsdr_open()
   \param test mode, 1 means enabled, 0 disabled
   \return 0 on success
*/
RTLSDR_API int rtlsdr_set_testmode(rtlsdr_dev_t *dev, int on);

/*!
   Enable or disable the internal digital AGC of the RTL2832.

   \param dev the device handle given by rtlsdr_open()
   \param digital AGC mode, 1 means enabled, 0 disabled
   \return 0 on success
*/
RTLSDR_API int rtlsdr_set_agc_mode(rtlsdr_dev_t *dev, int on);

/*!
   Enable or disable the direct sampling mode. When enabled, the IF mode
   of the RTL2832 is activated, and rtlsdr_set_center_freq() will control
   the IF-frequency of the DDC, which can be used to tune from 0 to 28.8 MHz
   (xtal frequency of the RTL2832).

   \param dev the device handle given by rtlsdr_open()
   \param on 0 means disabled, 1 I-ADC input enabled, 2 Q-ADC input enabled
   \return 0 on success
*/
RTLSDR_API int rtlsdr_set_direct_sampling(rtlsdr_dev_t *dev, int on);

/*!
   Get state of the direct sampling mode

   \param dev the device handle given by rtlsdr_open()
   \return -1 on error, 0 means disabled, 1 I-ADC input enabled
 	    2 Q-ADC input enabled
*/
RTLSDR_API int rtlsdr_get_direct_sampling(rtlsdr_dev_t *dev);

/*!
   Enable or disable offset tuning for zero-IF tuners, which allows to avoid
   problems caused by the DC offset of the ADCs and 1/f noise.

   \param dev the device handle given by rtlsdr_open()
   \param on 0 means disabled, 1 enabled
   \return 0 on success
*/
RTLSDR_API int rtlsdr_set_offset_tuning(rtlsdr_dev_t *dev, int on);

/*!
    Получить состояние режима настройки смещения

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \return -1 при ошибке, 0 означает отключено, 1 включено
*/
RTLSDR_API int rtlsdr_get_offset_tuning(rtlsdr_dev_t *dev);

/* функции потоковой передачи */

RTLSDR_API int rtlsdr_reset_buffer(rtlsdr_dev_t *dev);

RTLSDR_API int rtlsdr_read_sync(rtlsdr_dev_t *dev, void *buf, int len, int *n_read);

typedef void (*rtlsdr_read_async_cb_t)(unsigned char *buf, uint32_t len, void *ctx);

/*!
    Считайте образцы с устройства асинхронно. Эта функция будет заблокирована до тех пор, пока
    он отменяется с помощью rtlsdr_cancel_async()

    ПРИМЕЧАНИЕ. Эта функция устарела и может быть удалена.

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \param Функция обратного вызова cb для возврата полученных образцов
    \param ctx пользовательский контекст для передачи через функцию обратного вызова
    \return 0 в случае успеха
*/
RTLSDR_API int rtlsdr_wait_async(rtlsdr_dev_t *dev, rtlsdr_read_async_cb_t cb, void *ctx);

/*!
    Считайте образцы с устройства асинхронно. Эта функция будет заблокирована до тех пор, пока
    он отменяется с помощью rtlsdr_cancel_async()

    \param dev дескриптор устройства, заданный rtlsdr_open()
    \param Функция обратного вызова cb для возврата полученных образцов
    \param ctx пользовательский контекст для передачи через функцию обратного вызова
    \param buf_num необязательный счетчик буферов, buf_num * buf_len = общий размер буфера
  установлено значение 0 для количества буферов по умолчанию (15)
    \param buf_len необязательная длина буфера, должна быть кратна 512,
  должно быть кратно 16384 (размер URL), установлено значение 0.
  для длины буфера по умолчанию (16 * 32 * 512)
    \return 0 в случае успеха
*/
RTLSDR_API int rtlsdr_read_async(rtlsdr_dev_t *dev,
                                 rtlsdr_read_async_cb_t cb,
                                 void *ctx,
                                 uint32_t buf_num,
                                 uint32_t buf_len);

/*!
   Cancel all pending asynchronous operations on the device.

   \param dev the device handle given by rtlsdr_open()
   \return 0 on success
*/
RTLSDR_API int rtlsdr_cancel_async(rtlsdr_dev_t *dev);

/*!
   Enable or disable the bias tee on GPIO PIN 0.

   \param dev the device handle given by rtlsdr_open()
   \param on  1 for Bias T on. 0 for Bias T off.
   \return -1 if device is not initialized. 0 otherwise.
*/
RTLSDR_API int rtlsdr_set_bias_tee(rtlsdr_dev_t *dev, int on);

/*!
   Enable or disable the bias tee on the given GPIO pin.

   \param dev the device handle given by rtlsdr_open()
   \param gpio the gpio pin to configure as a Bias T control.
   \param on  1 for Bias T on. 0 for Bias T off.
   \return -1 if device is not initialized. 0 otherwise.
*/
RTLSDR_API int rtlsdr_set_bias_tee_gpio(rtlsdr_dev_t *dev, int gpio, int on);

#ifdef __cplusplus
}
#endif

#endif /* __RTL_SDR_H */
