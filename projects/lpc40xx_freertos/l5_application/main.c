#include <stdio.h>

// #define PART0
// #define PART1
// #define PART2
// #define PART3
// #define PART3_EXTRA_CREDIT

#include "FreeRTOS.h"
#include "adc.h"
#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "periodic_scheduler.h"
#include "pwm1.h"
#include "queue.h"
#include "sj2_cli.h"
#include "task.h"

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

#ifdef PART0

void pin_configure_pwm_channel_as_io_pin(void) {
  const uint32_t led0 = (1U << 0);
  LPC_IOCON->P2_0 = 0b001; // set as PWM control
  LPC_GPIO2->DIR |= led0;  // set as output
}

void pwm_task(void *params) {
  pwm1__init_single_edge(1);
  pin_configure_pwm_channel_as_io_pin();
  uint8_t percent = 0;
  // pwm1__set_duty_cycle(PWM1__2_0, 50);

  while (1) {
    pwm1__set_duty_cycle(PWM1__2_0, percent);
    if (++percent > 100) {
      percent = 0;
    }
    vTaskDelay(100);
  }
}

int main(void) {
  // create_blinky_tasks();
  create_uart_task();

  xTaskCreate(pwm_task, "pwm", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  vTaskStartScheduler();
}
#endif

#ifdef PART1

void configure_adc_channel_as_io_pin(void) {
  LPC_GPIO1->DIR &= ~(1 << 25); // set P0.25 as input
  LPC_IOCON->P0_25 = 0b001;     // set P0.25 as ADC input pin
}

void adc_task(void *params) {
  adc__enable_burst_mode();
  configure_adc_channel_as_io_pin();
  while (1) {

    const uint16_t adc_value = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_2);
    fprintf(stderr, "Output: %u\n", adc_value);
    vTaskDelay(100);
  }
}

int main(void) {
  // create_blinky_tasks();
  create_uart_task();

  xTaskCreate(adc_task, "adc", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  puts("Starting RTOS");
  vTaskStartScheduler();
  return 0;
}

#endif

#ifdef PART2

static QueueHandle_t adc_to_pwm_task_queue;

void configure_adc_channel_as_io_pin(void) {
  LPC_GPIO1->DIR &= ~(1 << 25); // set P0.25 as input
  LPC_IOCON->P0_25 = 0b001;     // set P0.25 as ADC input pin
}

void adc_task_2(void *params) {
  adc__enable_burst_mode();
  configure_adc_channel_as_io_pin();
  uint16_t adc_reading = 0;
  while (1) {
    adc_reading = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_2);
    xQueueSend(adc_to_pwm_task_queue, &adc_reading, 0);
    vTaskDelay(200);
  }
}

void pwm_task_2(void *params) {
  uint16_t adc_reading = 0;
  while (1) {
    if (xQueueReceive(adc_to_pwm_task_queue, &adc_reading, 200)) {
      fprintf(stderr, "\nReceived from queue: %u", adc_reading);
    } else {
      fprintf(stderr, "\nData not received");
    }
  }
}

int main() {

  adc_to_pwm_task_queue = xQueueCreate(1, sizeof(int));

  // create_blinky_tasks();
  create_uart_task();

  xTaskCreate(adc_task_2, "adc2", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(pwm_task_2, "pwm2", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

  puts("Starting RTOS");
  vTaskStartScheduler();
  return 0;
}

#endif

#ifdef PART3

static QueueHandle_t adc3_to_pwm3_task_queue;

void pin_configure_pwm_channel_as_io_pin(void) {
  const uint32_t led0 = (1U << 0);
  LPC_IOCON->P2_0 = 0b001; // set as PWM control
  LPC_GPIO2->DIR |= led0;  // set as output
}

void configure_adc_channel_as_io_pin(void) {
  LPC_GPIO1->DIR &= ~(1 << 25); // set P0.25 as input
  LPC_IOCON->P0_25 = 0b001;     // set P0.25 as ADC input pin
}

void pwm_task_3(void *params) {
  pwm1__init_single_edge(1);
  pin_configure_pwm_channel_as_io_pin();
  uint16_t adc_reading = 0;
  while (1) {
    if (xQueueReceive(adc3_to_pwm3_task_queue, &adc_reading, 500)) {
      uint16_t val = (adc_reading * 100) / 4095;
      fprintf(stderr, "\nMR0: %u", LPC_PWM1->MR0);
      fprintf(stderr, "\nMR1: %u", LPC_PWM1->MR1);
      pwm1__set_duty_cycle(PWM1__2_0, val);
      vTaskDelay(200);
    } else {
      fprintf(stderr, "\nData not received");
    }
  }
}

void adc_task_3(void *params) {
  adc__enable_burst_mode();
  configure_adc_channel_as_io_pin();
  uint16_t adc_reading = 0;

  while (1) {
    adc_reading = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_2);
    float output = (adc_reading * 3.3) / 4095;
    fprintf(stderr, "\nVoltage value: %fV", output);
    xQueueSend(adc3_to_pwm3_task_queue, &adc_reading, 0);
    vTaskDelay(500);
  }
}

int main(void) {
  adc3_to_pwm3_task_queue = xQueueCreate(1, sizeof(int));
  // create_blinky_tasks();
  create_uart_task();

  xTaskCreate(adc_task_3, "adc3", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(pwm_task_3, "pwm3", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  puts("Starting RTOS");
  vTaskStartScheduler();
  return 0;
}
#endif

#ifdef PART3_EXTRA_CREDIT

static QueueHandle_t adc3_to_pwm3_task_queue;

void pin_configure_pwm_channel_as_io_pin(void) {
  LPC_IOCON->P2_0 = 0b001;                               // set as PWM control
  LPC_IOCON->P2_2 = 0b001;                               // set as PWM control
  LPC_IOCON->P2_5 = 0b001;                               // set as PWM control
  LPC_GPIO2->DIR |= ((1U << 0) | (1U << 2) | (1U << 5)); // set as output
}

void configure_adc_channel_as_io_pin(void) {
  LPC_GPIO1->DIR &= ~(1 << 25); // set P0.25 as input
  LPC_IOCON->P0_25 = 0b001;     // set P0.25 as ADC input pin
}

void pwm_task_3(void *params) {
  pwm1__init_single_edge(1);
  pin_configure_pwm_channel_as_io_pin();
  uint16_t adc_reading = 0;
  while (1) {
    if (xQueueReceive(adc3_to_pwm3_task_queue, &adc_reading, 500)) {
      uint16_t val = (adc_reading * 100) / 4095;
      pwm1__set_duty_cycle(PWM1__2_0, val);
      pwm1__set_duty_cycle(PWM1__2_2, val - 10);
      pwm1__set_duty_cycle(PWM1__2_5, val - 30);
      vTaskDelay(200);
    } else {
      fprintf(stderr, "\nData not received");
    }
  }
}

void adc_task_3(void *params) {
  adc__enable_burst_mode();
  configure_adc_channel_as_io_pin();
  uint16_t adc_reading = 0;

  while (1) {
    adc_reading = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_2);
    float output = (adc_reading * 3.3) / 4095;
    fprintf(stderr, "\nVoltage value: %fV", output);
    xQueueSend(adc3_to_pwm3_task_queue, &adc_reading, 0);
    vTaskDelay(500);
  }
}

int main(void) {
  adc3_to_pwm3_task_queue = xQueueCreate(1, sizeof(int));
  // create_blinky_tasks();
  create_uart_task();

  xTaskCreate(adc_task_3, "adc3", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(pwm_task_3, "pwm3", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  puts("Starting RTOS");
  vTaskStartScheduler();
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
