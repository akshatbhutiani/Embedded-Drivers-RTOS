// #define PART0
// #define PART1
#define PART2

#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "queue.h"
#include "sj2_cli.h"

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

#ifdef PART0
static QueueHandle_t switch_queue;

typedef enum { switch_off, switch_on } switch_e;

switch_e get_switch_input_from_switch0(void) {
  LPC_GPIO0->DIR &= ~(1 << 29); // set as input
  if (LPC_GPIO0->PIN & (1 << 29)) {
    return switch_on;
  } else {
    return switch_off;
  }
}

void producer(void *p) {
  while (1) {
    const switch_e switch_value = get_switch_input_from_switch0();
    printf("%s(), line %d, sending\n", __FUNCTION__, __LINE__);
    if (xQueueSend(switch_queue, &switch_value, 0)) {
      printf("%s(), line %d, sent\n", __FUNCTION__, __LINE__);
    } else {
      printf("\nMessage sent not sent");
    }
    vTaskDelay(1000);
  }
}

void consumer(void *p) {
  while (1) {
    switch_e switch_value;
    printf("%s(), line %d, receiving\n", __FUNCTION__, __LINE__);
    if (xQueueReceive(switch_queue, &switch_value, portMAX_DELAY)) {
      printf("%s(), line %d, received\n", __FUNCTION__, __LINE__);
      if (switch_value == switch_off) {
        printf("\nSwitch is off\n");
      } else {
        printf("\nSwitch is pressed\n");
      }
    } else {
      printf("\nItem not received from queue");
    }
  }
}

int main(void) {
  create_blinky_tasks();
  create_uart_task();

  switch_queue = xQueueCreate(1, sizeof(switch_e));
  puts("Starting RTOS");
  puts("\n Producer is at low priority");
  puts("\n Consumer is at high priority");
  xTaskCreate(producer, "producer", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(consumer, "consumer", 2048 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

#endif

#ifdef PART1
static QueueHandle_t switch_queue;

typedef enum { switch_off, switch_on } switch_e;

switch_e get_switch_input_from_switch0(void) {
  LPC_GPIO0->DIR &= ~(1 << 29); // set as input
  if (LPC_GPIO0->PIN & (1 << 29)) {
    return switch_on;
  } else {
    return switch_off;
  }
}

void producer(void *p) {
  while (1) {
    const switch_e switch_value = get_switch_input_from_switch0();
    printf("%s(), line %d, sending\n", __FUNCTION__, __LINE__);
    if (xQueueSend(switch_queue, &switch_value, 0)) {
      printf("%s(), line %d, sent\n", __FUNCTION__, __LINE__);
    } else {
      printf("\nMessage sent not sent");
    }
    vTaskDelay(1000);
  }
}

void consumer(void *p) {
  while (1) {
    switch_e switch_value;
    printf("%s(), line %d, receiving\n", __FUNCTION__, __LINE__);
    if (xQueueReceive(switch_queue, &switch_value, portMAX_DELAY)) {
      printf("%s(), line %d, received\n", __FUNCTION__, __LINE__);
      if (switch_value == switch_off) {
        printf("\nSwitch is off\n");
      } else {
        printf("\nSwitch is pressed\n");
      }
    } else {
      printf("\nItem not received from queue");
    }
  }
}

int main(void) {
  create_blinky_tasks();
  create_uart_task();

  switch_queue = xQueueCreate(1, sizeof(switch_e));
  puts("Starting RTOS");
  puts("\n Producer is at high priority");
  puts("\n Consumer is at low priority");
  xTaskCreate(producer, "producer", 2048 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(consumer, "consumer", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}
#endif

#ifdef PART2
static QueueHandle_t switch_queue;

typedef enum { switch_off, switch_on } switch_e;

switch_e get_switch_input_from_switch0(void) {
  LPC_GPIO0->DIR &= ~(1 << 29); // set as input
  if (LPC_GPIO0->PIN & (1 << 29)) {
    return switch_on;
  } else {
    return switch_off;
  }
}

void producer(void *p) {
  while (1) {
    const switch_e switch_value = get_switch_input_from_switch0();
    printf("%s(), line %d, sending\n", __FUNCTION__, __LINE__);
    if (xQueueSend(switch_queue, &switch_value, 0)) {
      printf("%s(), line %d, sent\n", __FUNCTION__, __LINE__);
    } else {
      printf("\nMessage sent not sent");
    }
    vTaskDelay(1000);
  }
}

void consumer(void *p) {
  while (1) {
    switch_e switch_value;
    printf("%s(), line %d, receiving\n", __FUNCTION__, __LINE__);
    if (xQueueReceive(switch_queue, &switch_value, portMAX_DELAY)) {
      printf("%s(), line %d, received\n", __FUNCTION__, __LINE__);
      if (switch_value == switch_off) {
        printf("\nSwitch is off\n");
      } else {
        printf("\nSwitch is pressed\n");
      }
    } else {
      printf("\nItem not received from queue");
    }
  }
}

int main(void) {
  create_blinky_tasks();
  create_uart_task();

  switch_queue = xQueueCreate(1, sizeof(switch_e));
  puts("Starting RTOS");
  puts("\nEqual Priority");
  xTaskCreate(producer, "producer", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(consumer, "consumer", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}
#endif

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
