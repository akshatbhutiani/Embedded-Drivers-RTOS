#include "gpio_isr.h"
#include "lpc40xx.h"

static function_pointer_t gpio0callbacks[32];

void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {

  gpio0callbacks[pin] = callback;
  // for Falling edge
  if (interrupt_type == GPIO_INTERRUPT_FALLING_EDGE) {
    LPC_GPIOINT->IO0IntEnF |= (1 << pin);
  }
  // for Rising edge
  if (interrupt_type == GPIO_INTERRUPT_RISING_EDGE) {
    LPC_GPIOINT->IO0IntEnR |= (1 << pin);
  }
}

// Function for clearing port 0 pin
void pin_clr(int num) { LPC_GPIOINT->IO0IntClr |= (1 << num); }

int pin_check() {
  for (int i = 0; i < 32; i++) {
    if ((LPC_GPIOINT->IO0IntStatF) & (1 << i)) {
      return i;
    }
    if ((LPC_GPIOINT->IO0IntStatR) & (1 << i)) {
      return i;
    }
  }
}

void gpio0__interrupt_dispatcher(void) {
  const int pin_num = pin_check();

  function_pointer_t attached_user_handle = gpio0callbacks[pin_num];

  attached_user_handle();

  pin_clr(pin_num);
}
