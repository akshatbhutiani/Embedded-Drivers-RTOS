#pragma once
#include <stdbool.h>
#include <stdint.h>

// Should alter the hardware registers to set the pin as input
void gpio1_set_as_input(uint8_t pin_num);

// Should alter the hardware registers to set the pin as output
void gpio1_set_as_output(uint8_t pin_num);

// Should alter the hardware registers to set the pin high
void gpio1_set_high(uint8_t pin_num);

// Should alter the hardware registers to set the pin low
void gpio1_set_low(uint8_t pin_num);

// @param {bool} high - true => set pin high, false => set pin pin_low
void gpio1__set(uint8_t pin_num, bool high);

// Should return the state of the pin (input or output, doesn't matter)
// @return {bool} level of pin high => true, low => false
bool gpio1__get_level(uint8_t pin_num);

// extra credit for part 2
void gpio_sel_set(uint8_t port, uint8_t pin_num);

// extra credit for part 2
void gpio_sel_clear(uint8_t port, uint8_t pin_num);

// part 3
void gpio_switch_input(uint8_t port, uint8_t pin_num);
