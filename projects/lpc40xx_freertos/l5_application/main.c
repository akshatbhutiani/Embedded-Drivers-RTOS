#include <stdio.h>

#include "FreeRTOS.h"
#include "lpc40xx.h"
#include "semphr.h"
#include "task.h"

#include "lpc_peripherals.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#include "gpio_isr.h"

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

void pin29_isr(void) { fprintf(stderr, "\nPin 29 rising interrupt received"); }

void pin30_isr(void) { fprintf(stderr, "\nPin 30 falling interrupt received"); }

// static SemaphoreHandle_t switch_pressed_signal;

/*void gpio_interrupt(void) {
  LPC_GPIOINT->IO0IntClr |= (1U << 30);       // Clear the interrupt
  fprintf(stderr, "\nInside ISR for Part 1"); // print inside the interrupt
  xSemaphoreGiveFromISR(switch_pressed_signal, NULL);
} */

/* void sleep_on_sem_task(void *params) {
  while (1) {
    if (xSemaphoreTake(switch_pressed_signal, 1000)) {
      fprintf(stderr, "\nTaken resource from semaphore for part 1");
    }
  }
} */

int main(void) {
  // create_blinky_tasks();
  create_uart_task();

  LPC_GPIO0->DIR &= ~(1 << 29);
  LPC_GPIO0->DIR &= ~(1 << 30);

  gpio0__attach_interrupt(30, GPIO_INTERRUPT_RISING_EDGE, pin30_isr);
  gpio0__attach_interrupt(29, GPIO_INTERRUPT_FALLING_EDGE, pin29_isr);

  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0__interrupt_dispatcher, NULL);

  // Part 1:
  //  switch_pressed_signal = xSemaphoreCreateBinary(); // create binary semaphore
  // Part 0:
  // const uint32_t sw1 = (1 << 30);

  // LPC_GPIO0->DIR &= ~sw1;        // set switch as input;
  // LPC_IOCON->P0_29 |= (1 << 3);  // enable pulldown registers
  // LPC_GPIOINT->IO0IntEnF |= sw1; // configure falling edge interrupt

  //  const uint32_t led18 = (1U << 18);
  //  LPC_GPIO1->DIR |= led18;

  // Part 1
  // xTaskCreate(sleep_on_sem_task, "sem", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);

  puts("Starting RTOS");

  NVIC_EnableIRQ(GPIO_IRQn);

  // Part 0
  /*while (1) {
      delay__ms(500);
      LPC_GPIO1->SET = led18;
      delay__ms(500);
      LPC_GPIO1->CLR = led18;
      delay__ms(500);
    }*/
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

static void create_blinky_tasks(void) {
  /**
   * Use '#if (1)' if you wish to observe how two tasks can blink LEDs
   * Use '#if (0)' if you wish to use the 'periodic_scheduler.h' that will spawn 4 periodic tasks, one for each LED
   */
#if (1)
  // These variables should not go out of scope because the 'blink_task' will reference this memory
  static gpio_s led0, led1;

  led0 = board_io__get_led0();
  led1 = board_io__get_led1();

  xTaskCreate(blink_task, "led0", configMINIMAL_STACK_SIZE, (void *)&led0, PRIORITY_LOW, NULL);
  xTaskCreate(blink_task, "led1", configMINIMAL_STACK_SIZE, (void *)&led1, PRIORITY_LOW, NULL);
#else
  const bool run_1000hz = true;
  const size_t stack_size_bytes = 2048 / sizeof(void *); // RTOS stack size is in terms of 32-bits for ARM M4 32-bit CPU
  periodic_scheduler__initialize(stack_size_bytes, !run_1000hz); // Assuming we do not need the high rate 1000Hz task
  UNUSED(blink_task);
#endif
}

static void create_uart_task(void) {
  // It is advised to either run the uart_task, or the SJ2 command-line (CLI), but not both
  // Change '#if (0)' to '#if (1)' and vice versa to try it out
#if (0)
  // printf() takes more stack space, size this tasks' stack higher
  xTaskCreate(uart_task, "uart", (512U * 8) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#else
  sj2_cli__init();
  UNUSED(uart_task); // uart_task is un-used in if we are doing cli init()
#endif
}

static void blink_task(void *params) {
  const gpio_s led = *((gpio_s *)params); // Parameter was input while calling xTaskCreate()

  // Warning: This task starts with very minimal stack, so do not use printf() API here to avoid stack overflow
  while (true) {
    gpio__toggle(led);
    vTaskDelay(500);
  }
}

// This sends periodic messages over printf() which uses system_calls.c to send them to UART0
static void uart_task(void *params) {
  TickType_t previous_tick = 0;
  TickType_t ticks = 0;

  while (true) {
    // This loop will repeat at precise task delay, even if the logic below takes variable amount of ticks
    vTaskDelayUntil(&previous_tick, 2000);

    /* Calls to fprintf(stderr, ...) uses polled UART driver, so this entire output will be fully
     * sent out before this function returns. See system_calls.c for actual implementation.
     *
     * Use this style print for:
     *  - Interrupts because you cannot use printf() inside an ISR
     *    This is because regular printf() leads down to xQueueSend() that might block
     *    but you cannot block inside an ISR hence the system might crash
     *  - During debugging in case system crashes before all output of printf() is sent
     */
    ticks = xTaskGetTickCount();
    fprintf(stderr, "%u: This is a polled version of printf used for debugging ... finished in", (unsigned)ticks);
    fprintf(stderr, " %lu ticks\n", (xTaskGetTickCount() - ticks));

    /* This deposits data to an outgoing queue and doesn't block the CPU
     * Data will be sent later, but this function would return earlier
     */
    ticks = xTaskGetTickCount();
    printf("This is a more efficient printf ... finished in");
    printf(" %lu ticks\n\n", (xTaskGetTickCount() - ticks));
  }
}
