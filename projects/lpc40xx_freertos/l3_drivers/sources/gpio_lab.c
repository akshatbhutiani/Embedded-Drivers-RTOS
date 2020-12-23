#include "gpio_lab.h"
#include "lpc40xx.h"

void gpio1_set_as_input(uint8_t pin_num) { LPC_GPIO1->DIR &= ~(1 << pin_num); }

void gpio1_set_as_output(uint8_t pin_num) { LPC_GPIO1->DIR |= (1 << pin_num); }

void gpio1_set_low(uint8_t pin_num) { LPC_GPIO1->CLR = (1 << pin_num); }

void gpio1_set_high(uint8_t pin_num) { LPC_GPIO1->SET = (1 << pin_num); }

void gpio1_set(uint8_t pin_num, bool high) {
  if (high) {
    LPC_GPIO1->SET = (1 << pin_num);
  } else
    LPC_GPIO1->CLR = (1 << pin_num);
}

bool gpio__get_level(uint8_t port, uint8_t pin_num) {
  if (port == 0) {
    if (LPC_GPIO0->PIN & (1 << pin_num)) {
      return true;
    } else
      return false;
  } else if (port == 1) {
    if (LPC_GPIO1->PIN & (1 << pin_num)) {
      return true;
    } else
      return false;
  } else if (port == 2) {
    if (LPC_GPIO2->PIN & (1 << pin_num)) {
      return true;
    } else
      return false;
  } else if (port == 3) {
    if (LPC_GPIO3->PIN & (1 << pin_num)) {
      return true;
    } else
      return false;
  } else if (port == 4) {
    if (LPC_GPIO4->PIN & (1 << pin_num)) {
      return true;
    } else
      return false;
  } else if (port == 5) {
    if (LPC_GPIO5->PIN & (1 << pin_num)) {
      return true;
    } else
      return false;
  }
}

// extra credit driver for part 2
void gpio_sel_set(uint8_t port, uint8_t pin_num) {
  if (port == 0) {
    LPC_GPIO0->DIR |= (1 << pin_num);
    LPC_GPIO0->SET = (1 << pin_num);
  } else if (port == 1) {
    gpio1_set_as_output(pin_num);
    gpio1_set_high(pin_num);
  } else if (port == 2) {
    LPC_GPIO2->DIR |= (1 << pin_num);
    LPC_GPIO2->SET = (1 << pin_num);
  } else if (port == 3) {
    LPC_GPIO3->DIR |= (1 << pin_num);
    LPC_GPIO3->SET = (1 << pin_num);
  } else if (port == 4) {
    LPC_GPIO4->DIR |= (1 << pin_num);
    LPC_GPIO4->SET = (1 << pin_num);
  } else if (port == 5) {
    LPC_GPIO5->DIR |= (1 << pin_num);
    LPC_GPIO5->SET = (1 << pin_num);
  }
}

// extra credit driver for part 2
void gpio_sel_clear(uint8_t port, uint8_t pin_num) {
  if ((1 << port) == 0) {
    LPC_GPIO0->DIR |= (1 << pin_num);
    LPC_GPIO0->CLR = (1 << pin_num);
  } else if (port == 1) {
    gpio1_set_as_output(pin_num);
    gpio1_set_low(pin_num);
  } else if (port == 2) {
    LPC_GPIO2->DIR |= (1 << pin_num);
    LPC_GPIO2->CLR = (1 << pin_num);
  } else if (port == 3) {
    LPC_GPIO3->DIR |= (1 << pin_num);
    LPC_GPIO3->CLR = (1 << pin_num);
  } else if (port == 4) {
    LPC_GPIO4->DIR |= (1 << pin_num);
    LPC_GPIO4->CLR = (1 << pin_num);
  } else if (port == 5) {
    LPC_GPIO5->DIR |= (1 << pin_num);
    LPC_GPIO5->CLR = (1 << pin_num);
  }
}

// driver function for part 3 switch input
void gpio_switch_input(uint8_t port, uint8_t pin) {
  if (port == 0) {
    LPC_GPIO0->PIN &= ~(1 << pin);
  } else if (port == 1) {
    LPC_GPIO1->PIN &= ~(1 << pin);
  }
}
