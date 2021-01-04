#pragma once
#include <stdio.h>

typedef enum {
  GPIO_INTERRUPT_FALLING_EDGE,
  GPIO_INTERRUPT_RISING_EDGE,
} gpio_interrupt_e;

typedef void (*function_pointer_t)(void);
void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt, function_pointer_t callback);

void gpio0__interrupt_dispatcher(void);

int pin_check();

void pin_clr(int pin);
