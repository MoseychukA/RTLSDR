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
    ���������� ������� ���������� ����������, ������������ ��� RTL2832 � ���������� ������.

    ������ ��� ���������� ���������� ���� � �� �� �������� �������. ��������� ������� ����� ����� �����, ����
    �� ����������� � ������ ������� �������� ������� ��� ��� �����������
    ������ ������� (� ������� �������������), ��������� ������������ (�������) ����������.

    ����������. �������� ��� ������� ������ � ��� ������, ���� �� ��������� ��������� �����������.

    \param dev ���������� ����������, �������� rtlsdr_open()
    \param rtl_freq �������� �������, ������������ ��� ������������� RTL2832 � ��
    \param Tuner_freq �������� �������, ������������ ��� ������������� ���������� ������ � ��
    \return 0 � ������ ������
*/
RTLSDR_API int rtlsdr_set_xtal_freq(rtlsdr_dev_t *dev, uint32_t rtl_freq,
                                    uint32_t tuner_freq);

/*!
    �������� ������� ���������� ����������, ������������ ��� RTL2832 � ���������� ������.

    ������ ��� ���������� ���������� ���� � �� �� �������� �������.

    \param dev ���������� ����������, �������� rtlsdr_open()
    \param rtl_freq �������� �������, ������������ ��� ������������� RTL2832 � ��
    \param Tuner_freq �������� �������, ������������ ��� ������������� ���������� ������ � ��
    \return 0 � ������ ������
*/
RTLSDR_API int rtlsdr_get_xtal_freq(rtlsdr_dev_t *dev, uint32_t *rtl_freq,
                                    uint32_t *tuner_freq);

/*!
    �������� ������ USB-����������.
    ����������. ��������� ��������� ������ ������������ ������������ �� 256 ����.

    \param dev ���������� ����������, �������� rtlsdr_open()
    \param ��� ������������� �������������, ����� ���� NULL
    \param Product �������� ��������, ����� ���� NULL
    \param �������� �������� �����, ����� ���� NULL
    \return 0 � ������ ������
*/
RTLSDR_API int rtlsdr_get_usb_strings(rtlsdr_dev_t *dev, char *manufact, char *product, char *serial);

/*!
    �������� EEPROM ����������.

    \param dev ���������� ����������, �������� rtlsdr_open()
    \param ����� ������ ��� ������
    \param ����� ��������, ���� ������ ���� �������� ������
    \param len ����� ������
    \return 0 � ������ ������
    \return -1, ���� ���������� ���������� ��������������
    \return -2, ���� ������ EEPROM ��������
    \return -3, ���� EEPROM �� ������
*/

RTLSDR_API int rtlsdr_write_eeprom(rtlsdr_dev_t *dev, uint8_t *data, uint8_t offset, uint16_t len);
/*!
    �������� EEPROM ����������.

    \param dev ���������� ����������, �������� rtlsdr_open()
    \param ����� ������, ���� ������ ���� �������� ������
    \param ����� ��������, ������ ������� ������ ������
    \param len ����� ������
    \return 0 � ������ ������
    \return -1, ���� ���������� ���������� ��������������
    \return -2, ���� ������ EEPROM ��������
    \return -3, ���� EEPROM �� ������
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
    ������� ��� ������.

    \param dev ���������� ����������, �������� rtlsdr_open()
    \return RTLSDR_TUNER_UNKNOWN � ������ ������, � ��������� ������ ������� ��� ������
*/
RTLSDR_API enum rtlsdr_tuner rtlsdr_get_tuner_type(rtlsdr_dev_t *dev);

/*!
    �������� ������ ��������, �������������� �������.

    ����������. �������� �������� ������ ���� �������������� ������� ���������� ��������. ���� ����
    ������ ����� ����� ���������� ���������� ��������� �������� ��������.

    \param dev ���������� ����������, �������� rtlsdr_open()
    \param �������� ������ �������� ��������. � ������� ����� �� 115 �������� 11,5 ��.
    \return <= 0 � ������ ������, � ��������� ������ ���������� ��������� (������������) �������� ��������
*/
RTLSDR_API int rtlsdr_get_tuner_gains(rtlsdr_dev_t *dev, int *gains);

/*!
    ���������� �������� ��� ����������.
    ����� ��� ��������, ���������� �������� ����� ������� ��������.

    ���������� �������� �������� (� ������� ����� ��) ��� ������ E4000:
    -10, 15, 40, 65, 90, 115, 140, 165, 190,
    215, 240, 290, 340, 420, 430, 450, 470, 490

    �������������� �������� �������� ����� ��������� � ������� ������� \ref rtlsdr_get_tuner_gains.

    \param dev ���������� ����������, �������� rtlsdr_open()
    \param �������� � ������� ����� ��, 115 �������� 11,5 ��.
    \return 0 � ������ ������
*/
RTLSDR_API int rtlsdr_set_tuner_gain(rtlsdr_dev_t *dev, int gain);

/*!
    ���������� ���������� ����������� ����������.

    \param dev ���������� ����������, �������� rtlsdr_open()
    \param ������ ����������� � ��. ���� �������� �������������� ����� ������ �����������.
    \return 0 � ������ ������
*/
RTLSDR_API int rtlsdr_set_tuner_bandwidth(rtlsdr_dev_t *dev, uint32_t bw);

/*!
    �������� ����������� ��������, �� ������� ��������� ����������.

    \param dev ���������� ����������, �������� rtlsdr_open()
    \return 0 � ������ ������, �������� � ������� ����� ��, 115 �������� 11,5 ��.
*/
RTLSDR_API int rtlsdr_get_tuner_gain(rtlsdr_dev_t *dev);

/*!
    ���������� �������� ������������� ������� ��� ����������.

    \param dev ���������� ����������, �������� rtlsdr_open()
    \param stage ����� ������� �������� ������������� ������� (�� 1 �� 6 ��� E4000)
    \param �������� � ������� ����� ��, -30 �������� -3,0 ��.
    \return 0 � ������ ������
*/
RTLSDR_API int rtlsdr_set_tuner_if_gain(rtlsdr_dev_t *dev, int stage, int gain);

/*!
    ���������� ����� �������� (��������������/������) ��� ����������.
    ��� ������ ������� ��������� �������� ���������� �������� ����� ������� ��������.

    \param dev ���������� ����������, �������� rtlsdr_open()
    \param ����� ������� ��������, 1 ��������, ��� ����� ������� �������� ������ ���� �������.
    \return 0 � ������ ������
*/
RTLSDR_API int rtlsdr_set_tuner_gain_mode(rtlsdr_dev_t *dev, int manual);
/*!
    ���������� ������� ������������� ��� ����������, � ����� �������� ������� �������� ������.
    � ������������ � ����������� �������� ������������� ��� �������, ��� ��� ��������.

    \param dev ���������� ����������, �������� rtlsdr_open()
    \param samp_rate ������� �������������, ������� ���������� ����������, ��������� ��������:
   225001 - 300000 ��
   900001 - 3200000 ��
   ������ ������� ������� ������� ��� ������ > 2400000
    \return 0 � ������ ������, -EINVAL � ������ ���������������� ��������
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
    �������� ��������� ������ ��������� ��������

    \param dev ���������� ����������, �������� rtlsdr_open()
    \return -1 ��� ������, 0 �������� ���������, 1 ��������
*/
RTLSDR_API int rtlsdr_get_offset_tuning(rtlsdr_dev_t *dev);

/* ������� ��������� �������� */

RTLSDR_API int rtlsdr_reset_buffer(rtlsdr_dev_t *dev);

RTLSDR_API int rtlsdr_read_sync(rtlsdr_dev_t *dev, void *buf, int len, int *n_read);

typedef void (*rtlsdr_read_async_cb_t)(unsigned char *buf, uint32_t len, void *ctx);

/*!
    �������� ������� � ���������� ����������. ��� ������� ����� ������������� �� ��� ���, ����
    �� ���������� � ������� rtlsdr_cancel_async()

    ����������. ��� ������� �������� � ����� ���� �������.

    \param dev ���������� ����������, �������� rtlsdr_open()
    \param ������� ��������� ������ cb ��� �������� ���������� ��������
    \param ctx ���������������� �������� ��� �������� ����� ������� ��������� ������
    \return 0 � ������ ������
*/
RTLSDR_API int rtlsdr_wait_async(rtlsdr_dev_t *dev, rtlsdr_read_async_cb_t cb, void *ctx);

/*!
    �������� ������� � ���������� ����������. ��� ������� ����� ������������� �� ��� ���, ����
    �� ���������� � ������� rtlsdr_cancel_async()

    \param dev ���������� ����������, �������� rtlsdr_open()
    \param ������� ��������� ������ cb ��� �������� ���������� ��������
    \param ctx ���������������� �������� ��� �������� ����� ������� ��������� ������
    \param buf_num �������������� ������� �������, buf_num * buf_len = ����� ������ ������
  ����������� �������� 0 ��� ���������� ������� �� ��������� (15)
    \param buf_len �������������� ����� ������, ������ ���� ������ 512,
  ������ ���� ������ 16384 (������ URL), ����������� �������� 0.
  ��� ����� ������ �� ��������� (16 * 32 * 512)
    \return 0 � ������ ������
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
