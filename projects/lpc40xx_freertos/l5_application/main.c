#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

static SemaphoreHandle_t switch_press_indicator;

typedef struct {
  uint8_t port;
  uint8_t pin;
} port_pin_s;

void led_task(void *pvParams) { // Task function for part 0 of the lab
  const uint32_t led18 = (1U << 18);

  LPC_IOCON->P1_18 = 0x000;

  LPC_GPIO1->DIR |= led18;

  while (true) {

    LPC_GPIO1->SET = led18;
    vTaskDelay(500);

    LPC_GPIO1->CLR = led18;
    vTaskDelay(500);
  }
}

void led_blink_task(void *parameter) { // Task function for parts 1 & 2 of the lab
  const port_pin_s *led = (port_pin_s *)(parameter);

  LPC_GPIO1->DIR |= (1 << (led->pin));

  while (true) {
    gpio1_set_high(led->pin);
    vTaskDelay(200);

    gpio1_set_low(led->pin);
    vTaskDelay(200);
  }
}

void extra_credit_task(void *parameter) { // Task function for extra credit of part 2
  const port_pin_s *e = (port_pin_s *)(parameter);
  while (1) {
    gpio_sel_set(e->port, e->pin);
    vTaskDelay(400);
    gpio_sel_clear(e->port, e->pin);
    vTaskDelay(400);
  }
}

void led_task3(void *parameter) { // LED Task function for part 3
  const port_pin_s *blink = (port_pin_s *)(parameter);
  while (1) {
    if (xSemaphoreTake(switch_press_indicator, 1000)) {
      gpio_sel_set(blink->port, blink->pin);
      vTaskDelay(200);
      gpio_sel_clear(blink->port, blink->pin);
      vTaskDelay(200);
    } else
      puts("Timeout: No switch indication for 1000 ms");
  }
}

void switch_task(void *parameter) {
  port_pin_s *sw = (port_pin_s *)(parameter);
  gpio_switch_input(sw->port, sw->pin);
  while (1) {
    if (gpio__get_level(sw->port, sw->pin))
      xSemaphoreGive(switch_press_indicator);
  }
  vTaskDelay(200);
}

int main(void) {
  // create_blinky_tasks();
  create_uart_task();
  // static port_pin_s led0 = {18};
  // static port_pin_s led1 = {24};

  switch_press_indicator = xSemaphoreCreateBinary();

  static port_pin_s switchtask = {0, 29};
  static port_pin_s ledtask = {1, 18};

  // static port_pin_s extra = {1, 18};

  // Part 0:
  // xTaskCreate(led_task, "led", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

  // Part 1 & 2
  // xTaskCreate(led_blink_task, "driver", 2048 / sizeof(void *), &led0, PRIORITY_LOW, NULL);

  // xTaskCreate(led_blink_task, "driver", 2048 / sizeof(void *), &led1, PRIORITY_LOW, NULL);

  // Extra credit for part 2
  //  xTaskCreate(extra_credit_task, "extra", 2048 / sizeof(void *), &extra, PRIORITY_LOW, NULL);

  // Part 3
  xTaskCreate(switch_task, "switch", 2048 / sizeof(void *), &switchtask, PRIORITY_LOW, NULL);
  xTaskCreate(led_task3, "led3", 2048 / sizeof(void *), &ledtask, PRIORITY_LOW, NULL);

  puts("Starting RTOS");
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
