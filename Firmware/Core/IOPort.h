#pragma once

#include "hal.h"

struct IOPort
{
#ifdef STM32F1
	typedef GPIO_TypeDef GPIOType;
#else
	typedef stm32_gpio_t GPIOType;
#endif

	IOPort() : gpio( nullptr ), pinNum( 0 ) {}
	IOPort( GPIOType* gpio, uint32_t pinNum ) : gpio( gpio ), pinNum( pinNum ) {}

	GPIOType* gpio;
	uint16_t pinNum;
};

#define IOPNA IOPort()

#ifdef GPIOA
#define IOPA0 IOPort( GPIOA, 0 )
#define IOPA1 IOPort( GPIOA, 1 )
#define IOPA2 IOPort( GPIOA, 2 )
#define IOPA3 IOPort( GPIOA, 3 )
#define IOPA4 IOPort( GPIOA, 4 )
#define IOPA5 IOPort( GPIOA, 5 )
#define IOPA6 IOPort( GPIOA, 6 )
#define IOPA7 IOPort( GPIOA, 7 )
#define IOPA8 IOPort( GPIOA, 8 )
#define IOPA9 IOPort( GPIOA, 9 )
#define IOPA10 IOPort( GPIOA, 10 )
#define IOPA11 IOPort( GPIOA, 11 )
#define IOPA12 IOPort( GPIOA, 12 )
#define IOPA13 IOPort( GPIOA, 13 )
#define IOPA14 IOPort( GPIOA, 14 )
#define IOPA15 IOPort( GPIOA, 15 )
#endif

#ifdef GPIOB
#define IOPB0 IOPort( GPIOB, 0 )
#define IOPB1 IOPort( GPIOB, 1 )
#define IOPB2 IOPort( GPIOB, 2 )
#define IOPB3 IOPort( GPIOB, 3 )
#define IOPB4 IOPort( GPIOB, 4 )
#define IOPB5 IOPort( GPIOB, 5 )
#define IOPB6 IOPort( GPIOB, 6 )
#define IOPB7 IOPort( GPIOB, 7 )
#define IOPB8 IOPort( GPIOB, 8 )
#define IOPB9 IOPort( GPIOB, 9 )
#define IOPB10 IOPort( GPIOB, 10 )
#define IOPB11 IOPort( GPIOB, 11 )
#define IOPB12 IOPort( GPIOB, 12 )
#define IOPB13 IOPort( GPIOB, 13 )
#define IOPB14 IOPort( GPIOB, 14 )
#define IOPB15 IOPort( GPIOB, 15 )
#endif

#ifdef GPIOC
#define IOPC0 IOPort( GPIOC, 0 )
#define IOPC1 IOPort( GPIOC, 1 )
#define IOPC2 IOPort( GPIOC, 2 )
#define IOPC3 IOPort( GPIOC, 3 )
#define IOPC4 IOPort( GPIOC, 4 )
#define IOPC5 IOPort( GPIOC, 5 )
#define IOPC6 IOPort( GPIOC, 6 )
#define IOPC7 IOPort( GPIOC, 7 )
#define IOPC8 IOPort( GPIOC, 8 )
#define IOPC9 IOPort( GPIOC, 9 )
#define IOPC10 IOPort( GPIOC, 10 )
#define IOPC11 IOPort( GPIOC, 11 )
#define IOPC12 IOPort( GPIOC, 12 )
#define IOPC13 IOPort( GPIOC, 13 )
#define IOPC14 IOPort( GPIOC, 14 )
#define IOPC15 IOPort( GPIOC, 15 )
#endif

#ifdef GPIOD
#define IOPD0 IOPort( GPIOD, 0 )
#define IOPD1 IOPort( GPIOD, 1 )
#define IOPD2 IOPort( GPIOD, 2 )
#define IOPD3 IOPort( GPIOD, 3 )
#define IOPD4 IOPort( GPIOD, 4 )
#define IOPD5 IOPort( GPIOD, 5 )
#define IOPD6 IOPort( GPIOD, 6 )
#define IOPD7 IOPort( GPIOD, 7 )
#define IOPD8 IOPort( GPIOD, 8 )
#define IOPD9 IOPort( GPIOD, 9 )
#define IOPD10 IOPort( GPIOD, 10 )
#define IOPD11 IOPort( GPIOD, 11 )
#define IOPD12 IOPort( GPIOD, 12 )
#define IOPD13 IOPort( GPIOD, 13 )
#define IOPD14 IOPort( GPIOD, 14 )
#define IOPD15 IOPort( GPIOD, 15 )
#endif

#ifdef GPIOE
#define IOPE0 IOPort( GPIOE, 0 )
#define IOPE1 IOPort( GPIOE, 1 )
#define IOPE2 IOPort( GPIOE, 2 )
#define IOPE3 IOPort( GPIOE, 3 )
#define IOPE4 IOPort( GPIOE, 4 )
#define IOPE5 IOPort( GPIOE, 5 )
#define IOPE6 IOPort( GPIOE, 6 )
#define IOPE7 IOPort( GPIOE, 7 )
#define IOPE8 IOPort( GPIOE, 8 )
#define IOPE9 IOPort( GPIOE, 9 )
#define IOPE10 IOPort( GPIOE, 10 )
#define IOPE11 IOPort( GPIOE, 11 )
#define IOPE12 IOPort( GPIOE, 12 )
#define IOPE13 IOPort( GPIOE, 13 )
#define IOPE14 IOPort( GPIOE, 14 )
#define IOPE15 IOPort( GPIOE, 15 )
#endif

#ifdef GPIOF
#define IOPF0 IOPort( GPIOF, 0 )
#define IOPF1 IOPort( GPIOF, 1 )
#define IOPF2 IOPort( GPIOF, 2 )
#define IOPF3 IOPort( GPIOF, 3 )
#define IOPF4 IOPort( GPIOF, 4 )
#define IOPF5 IOPort( GPIOF, 5 )
#define IOPF6 IOPort( GPIOF, 6 )
#define IOPF7 IOPort( GPIOF, 7 )
#define IOPF8 IOPort( GPIOF, 8 )
#define IOPF9 IOPort( GPIOF, 9 )
#define IOPF10 IOPort( GPIOF, 10 )
#define IOPF11 IOPort( GPIOF, 11 )
#define IOPF12 IOPort( GPIOF, 12 )
#define IOPF13 IOPort( GPIOF, 13 )
#define IOPF14 IOPort( GPIOF, 14 )
#define IOPF15 IOPort( GPIOF, 15 )
#endif

#ifdef GPIOG
#define IOPG0 IOPort( GPIOG, 0 )
#define IOPG1 IOPort( GPIOG, 1 )
#define IOPG2 IOPort( GPIOG, 2 )
#define IOPG3 IOPort( GPIOG, 3 )
#define IOPG4 IOPort( GPIOG, 4 )
#define IOPG5 IOPort( GPIOG, 5 )
#define IOPG6 IOPort( GPIOG, 6 )
#define IOPG7 IOPort( GPIOG, 7 )
#define IOPG8 IOPort( GPIOG, 8 )
#define IOPG9 IOPort( GPIOG, 9 )
#define IOPG10 IOPort( GPIOG, 10 )
#define IOPG11 IOPort( GPIOG, 11 )
#define IOPG12 IOPort( GPIOG, 12 )
#define IOPG13 IOPort( GPIOG, 13 )
#define IOPG14 IOPort( GPIOG, 14 )
#define IOPG15 IOPort( GPIOG, 15 )
#endif

#ifdef GPIOH
#define IOPH0 IOPort( GPIOH, 0 )
#define IOPH1 IOPort( GPIOH, 1 )
#define IOPH2 IOPort( GPIOH, 2 )
#define IOPH3 IOPort( GPIOH, 3 )
#define IOPH4 IOPort( GPIOH, 4 )
#define IOPH5 IOPort( GPIOH, 5 )
#define IOPH6 IOPort( GPIOH, 6 )
#define IOPH7 IOPort( GPIOH, 7 )
#define IOPH8 IOPort( GPIOH, 8 )
#define IOPH9 IOPort( GPIOH, 9 )
#define IOPH10 IOPort( GPIOH, 10 )
#define IOPH11 IOPort( GPIOH, 11 )
#define IOPH12 IOPort( GPIOH, 12 )
#define IOPH13 IOPort( GPIOH, 13 )
#define IOPH14 IOPort( GPIOH, 14 )
#define IOPH15 IOPort( GPIOH, 15 )
#endif

#ifdef GPIOI
#define IOPI0 IOPort( GPIOI, 0 )
#define IOPI1 IOPort( GPIOI, 1 )
#define IOPI2 IOPort( GPIOI, 2 )
#define IOPI3 IOPort( GPIOI, 3 )
#define IOPI4 IOPort( GPIOI, 4 )
#define IOPI5 IOPort( GPIOI, 5 )
#define IOPI6 IOPort( GPIOI, 6 )
#define IOPI7 IOPort( GPIOI, 7 )
#define IOPI8 IOPort( GPIOI, 8 )
#define IOPI9 IOPort( GPIOI, 9 )
#define IOPI10 IOPort( GPIOI, 10 )
#define IOPI11 IOPort( GPIOI, 11 )
#define IOPI12 IOPort( GPIOI, 12 )
#define IOPI13 IOPort( GPIOI, 13 )
#define IOPI14 IOPort( GPIOI, 14 )
#define IOPI15 IOPort( GPIOI, 15 )
#endif

#ifdef GPIOJ
#define IOPJ0 IOPort( GPIOJ, 0 )
#define IOPJ1 IOPort( GPIOJ, 1 )
#define IOPJ2 IOPort( GPIOJ, 2 )
#define IOPJ3 IOPort( GPIOJ, 3 )
#define IOPJ4 IOPort( GPIOJ, 4 )
#define IOPJ5 IOPort( GPIOJ, 5 )
#define IOPJ6 IOPort( GPIOJ, 6 )
#define IOPJ7 IOPort( GPIOJ, 7 )
#define IOPJ8 IOPort( GPIOJ, 8 )
#define IOPJ9 IOPort( GPIOJ, 9 )
#define IOPJ10 IOPort( GPIOJ, 10 )
#define IOPJ11 IOPort( GPIOJ, 11 )
#define IOPJ12 IOPort( GPIOJ, 12 )
#define IOPJ13 IOPort( GPIOJ, 13 )
#define IOPJ14 IOPort( GPIOJ, 14 )
#define IOPJ15 IOPort( GPIOJ, 15 )
#endif

#ifdef GPIOK
#define IOPK0 IOPort( GPIOK, 0 )
#define IOPK1 IOPort( GPIOK, 1 )
#define IOPK2 IOPort( GPIOK, 2 )
#define IOPK3 IOPort( GPIOK, 3 )
#define IOPK4 IOPort( GPIOK, 4 )
#define IOPK5 IOPort( GPIOK, 5 )
#define IOPK6 IOPort( GPIOK, 6 )
#define IOPK7 IOPort( GPIOK, 7 )
#define IOPK8 IOPort( GPIOK, 8 )
#define IOPK9 IOPort( GPIOK, 9 )
#define IOPK10 IOPort( GPIOK, 10 )
#define IOPK11 IOPort( GPIOK, 11 )
#define IOPK12 IOPort( GPIOK, 12 )
#define IOPK13 IOPort( GPIOK, 13 )
#define IOPK14 IOPort( GPIOK, 14 )
#define IOPK15 IOPort( GPIOK, 15 )
#endif