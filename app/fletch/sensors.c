// Copyright (c) 2015, the Fletch project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

#include <stdio.h>

#include <arch/arm/cm.h>

#include <kernel/port.h>

#include <platform/stm32.h>
#include <platform/gpio.h>

#include <dev/gpio.h>

#include <target/gpioconfig.h>

#define GPIO_EXTI_GET(line) (EXTI->PR & (line))
#define GPIO_EXTI_CLEAR(line) (EXTI->PR = (line))

port_t switch_port;
int switch_ev_count;

typedef enum {
  SWITCH_0_TOGGLE,
  SWITCH_1_TOGGLE,
  SWITCH_2_TOGGLE,
  SWITCH_3_TOGGLE,
} switch_event_t;

static void Switch_Callback(switch_event_t ev) {
  port_packet_t packet = {{(char)ev, switch_ev_count++}};
  port_write(switch_port, &packet, 1);
}

void stm32_EXTI15_10_IRQ(void) {
  arm_cm_irq_entry();
  if (GPIO_EXTI_GET(GPIO_TO_PIN_MASK(GPIO_SW100)) != RESET) {
    GPIO_EXTI_CLEAR(GPIO_TO_PIN_MASK(GPIO_SW100));
    Switch_Callback(SWITCH_0_TOGGLE);
  }
  if (GPIO_EXTI_GET(GPIO_TO_PIN_MASK(GPIO_SW101)) != RESET) {
    GPIO_EXTI_CLEAR(GPIO_TO_PIN_MASK(GPIO_SW101));
    Switch_Callback(SWITCH_1_TOGGLE);
  }
  if (GPIO_EXTI_GET(GPIO_TO_PIN_MASK(GPIO_SW102)) != RESET) {
    GPIO_EXTI_CLEAR(GPIO_TO_PIN_MASK(GPIO_SW102));
    Switch_Callback(SWITCH_2_TOGGLE);
  }
  if (GPIO_EXTI_GET(GPIO_TO_PIN_MASK(GPIO_SW103)) != RESET) {
    GPIO_EXTI_CLEAR(GPIO_TO_PIN_MASK(GPIO_SW103));
    Switch_Callback(SWITCH_3_TOGGLE);
  }
  arm_cm_irq_exit(true);
}

#if !defined(TARGET_DARTUINOP0)
void SensorsInit(void) {
  // TODO(cpu): Implement this for Disco board.
}
#else
void SensorsInit(void) {
  port_create("sys/io/sw", PORT_MODE_BROADCAST, &switch_port);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
#endif
