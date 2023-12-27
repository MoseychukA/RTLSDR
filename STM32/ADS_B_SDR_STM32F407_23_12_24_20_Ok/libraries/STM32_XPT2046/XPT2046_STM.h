#pragma once
#include "Arduino.h"
#include <SPI.h>

#define  TOUCH_SPI_PORT hspi2  // Определяем какой SPI работает с тачскрином
extern SPI_HandleTypeDef  TOUCH_SPI_PORT;

#define  TOUCH_IRQ_Pin       GPIO_PIN_5   //  Определяем вывод информирующий о нажатии на тачскрин
#define  TOUCH_IRQ_GPIO_Port GPIOC        //  Определяем порт  TOUCH_IRQ_Pin
#define  TOUCH_CS_Pin        GPIO_PIN_12  //  Определяем pin выбора тачскрина
#define  TOUCH_CS_GPIO_Port  GPIOB        //  Определяем порт выбора тачскрина

// change depending on screen orientation
#define  TOUCH_SCALE_X 220               // Рамер по горизонтали X
#define  TOUCH_SCALE_Y 176               // Рамер по вертикали  Y

// to calibrate uncomment UART_Printf line in ili9341_touch.c
//#define  TOUCH_MIN_RAW_X 3000   // Вариант экрана Б
//#define  TOUCH_MAX_RAW_X 31100  // Вариант экрана Б
#define  TOUCH_MIN_RAW_X 1650     // Реальная минимальная величина по оси X (подстроить под конкретный дисплей)
#define  TOUCH_MAX_RAW_X 30100    // Реальная максимальная величина по оси X(подстроить под конкретный дисплей)

//#define  TOUCH_MIN_RAW_Y 1950   // Вариант экрана Б
//#define  TOUCH_MAX_RAW_Y 30900  // Вариант экрана Б
#define  TOUCH_MIN_RAW_Y 1650     // Реальная минимальная величина по оси Y (подстроить под конкретный дисплей)
#define  TOUCH_MAX_RAW_Y 31000    // Реальная максимальная величина по оси Y(подстроить под конкретный дисплей)


class TS_Point {
public:
	TS_Point(void);
	TS_Point(int16_t x, int16_t y, int16_t z);

	bool operator==(TS_Point);
	bool operator!=(TS_Point);

	int16_t x, y, z;
};

class XPT2046_STM
{
public:
	XPT2046_STM(int8_t _cs_pin, int8_t _irq_pin = -1);
	void begin();

	TS_Point getPoint();
	boolean TouchPressed();
	boolean touched();
private:
	int8_t _pin_cs, _pin_irq;
	SPIClass & _spi = SPI;
	TS_Point _point = { 0, 0, 0 };
};

