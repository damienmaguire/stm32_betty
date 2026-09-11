/*
 * Clock / NVIC / USART bring-up for ZombieVerter V1.3 (STM32F107).
 * Trimmed from stm32-vcu hwinit — no GS450H / SPI3 / PWM gauges.
 */
#include "hwinit.h"
#include "hwdefs.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/timer.h>

void clock_setup(void) {
  rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
  rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV6);
  SCB_AIRCR = SCB_AIRCR_VECTKEY | SCB_AIRCR_PRIGROUP_GROUP16_NOSUB;

  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);
  rcc_periph_clock_enable(RCC_GPIOD);
  rcc_periph_clock_enable(RCC_GPIOE);
  rcc_periph_clock_enable(RCC_USART3);
  rcc_periph_clock_enable(RCC_TIM4);
  rcc_periph_clock_enable(RCC_DMA1);
  rcc_periph_clock_enable(RCC_ADC1);
  rcc_periph_clock_enable(RCC_CRC);
  rcc_periph_clock_enable(RCC_AFIO);
  rcc_periph_clock_enable(RCC_CAN1);
  rcc_periph_clock_enable(RCC_CAN2);
}

void usart_setup(void) {}
void usart1_setup(void) {}
void usart2_setup(void) {}
void tim_setup(void) {}
void tim2_setup(void) {}
void tim3_setup(void) {}
void spi2_setup(void) {}
void spi3_setup(void) {}

void nvic_setup(void) {
  nvic_enable_irq(NVIC_TIM4_IRQ);
  nvic_set_priority(NVIC_TIM4_IRQ, 0);

  nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
  nvic_set_priority(NVIC_USB_LP_CAN_RX0_IRQ, 0xe << 4);
  nvic_enable_irq(NVIC_USB_HP_CAN_TX_IRQ);
  nvic_set_priority(NVIC_USB_HP_CAN_TX_IRQ, 0xe << 4);

  nvic_enable_irq(NVIC_CAN2_RX0_IRQ);
  nvic_set_priority(NVIC_CAN2_RX0_IRQ, 0xe << 4);
  nvic_enable_irq(NVIC_CAN2_TX_IRQ);
  nvic_set_priority(NVIC_CAN2_TX_IRQ, 0xe << 4);
}

void rtc_setup(void) {}
