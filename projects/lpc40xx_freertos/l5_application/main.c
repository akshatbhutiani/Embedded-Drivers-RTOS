#include <stdio.h>

#define PART_1
// #define PART_2
// #define PART_3

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"
#include "ssp2_lab.h"

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

#ifdef PART_1
void adesto_cs(void);
void adesto_ds(void);

typedef struct {
  uint8_t manufacturer_id;
  uint8_t device_id_1;
  uint8_t device_id_2;
  uint8_t extended_device_id;
} adesto_flash_id_s;

adesto_flash_id_s adesto_read_signature(void) {
  adesto_flash_id_s data = {0};

  adesto_cs();

  const uint8_t opcode_to_read_id = 0x9F;
  const uint8_t dummy_byte = 0xAA;
  ssp2_lab__exchange_byte(opcode_to_read_id);
  data.manufacturer_id = ssp2_lab__exchange_byte(dummy_byte);
  data.device_id_1 = ssp2_lab__exchange_byte(dummy_byte);
  data.device_id_2 = ssp2_lab__exchange_byte(dummy_byte);
  data.extended_device_id = ssp2_lab__exchange_byte(dummy_byte);

  adesto_ds();

  return data;
}

void adesto_cs(void) {
  const uint32_t clear_cs_pin = (1 << 10);
  LPC_GPIO1->CLR = clear_cs_pin; // clear pin so that active low wil be enabled
  const uint32_t clear_tg_pin = (1 << 20);
  LPC_GPIO1->CLR = clear_tg_pin; // clear pin so that active low wil be enabled
}

void adesto_ds(void) {
  const uint32_t deselect_cs_pin = (1 << 10);
  LPC_GPIO1->SET = deselect_cs_pin; // set pin so that active low will be disabled
  const uint32_t set_tg_pin = (1 << 20);
  LPC_GPIO1->SET = set_tg_pin; // set pin so that active low will be disabled
}

void ssp2_pin_configure(void) {
  const uint32_t enable = (1 << 2);
  const uint32_t set_miso_as_input = (1 << 4);
  LPC_GPIO1->DIR &= ~(set_miso_as_input); // set direction register for MISO pin
  LPC_IOCON->P1_4 &= ~(0b111 << 0);       // clear the iocon pins
  LPC_IOCON->P1_4 |= enable;              // set pin functionality as MISO

  const uint32_t set_sck_as_output = (1 << 0);
  LPC_GPIO1->DIR |= (set_sck_as_output); // set direction register for SCK
  LPC_IOCON->P1_0 &= ~(0b111 << 0);      // clear the iocon pins
  LPC_IOCON->P1_0 |= enable;             // set pin functionality as SCK

  const uint32_t set_mosi_as_output = (1 << 1);
  LPC_GPIO1->DIR |= set_mosi_as_output; // set direction register for MOSI pin
  LPC_IOCON->P1_1 &= ~(0b111 << 0);     // clear the IOCON pins
  LPC_IOCON->P1_1 |= enable;            // set pin functionality as MOSI

  const uint32_t set_cs_as_output = (1 << 10);
  LPC_GPIO1->DIR |= set_cs_as_output; // set direction register for CS pin

  const uint32_t tg = (1 << 20);
  LPC_GPIO1->DIR |= tg; // set trigger pin for logic analyzer as output
}

void spi_task(void *params) {
  const uint32_t spi_clock_mhz = 1;
  ssp2__init(spi_clock_mhz);
  ssp2_pin_configure();

  while (1) {
    adesto_flash_id_s id = adesto_read_signature();
    printf("\nManufacturer ID: %x", id.manufacturer_id);
    printf("\nDevice ID_1: %x", id.device_id_1);
    printf("\nDevice ID_2: %x", id.device_id_2);
    printf("\nExtended Device ID: %x", id.extended_device_id);
    printf("\n Vlue of status register: %x", LPC_SSP2->SR);
    vTaskDelay(500);
  }
}

int main(void) {
  // create_blinky_tasks();
  create_uart_task();

  xTaskCreate(spi_task, "spi", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

#endif

#ifdef PART_2

SemaphoreHandle_t spi_bus_mutex;

void adesto_cs(void);
void adesto_ds(void);

typedef struct {
  uint8_t manufacturer_id;
  uint8_t device_id_1;
  uint8_t device_id_2;
  uint8_t extended_device_id;
} adesto_flash_id_s;

void ssp2_pin_configure(void) {
  const uint32_t enable = (1 << 2);
  const uint32_t set_miso_as_input = (1 << 4);
  LPC_GPIO1->DIR &= ~(set_miso_as_input); // set direction register for MISO pin
  LPC_IOCON->P1_4 &= ~(0b111 << 0);       // clear the iocon pins
  LPC_IOCON->P1_4 |= enable;              // set pin functionality as MISO

  const uint32_t set_sck_as_output = (1 << 0);
  LPC_GPIO1->DIR |= (set_sck_as_output); // set direction register for SCK
  LPC_IOCON->P1_0 &= ~(0b111 << 0);      // clear the iocon pins
  LPC_IOCON->P1_0 |= enable;             // set pin functionality as SCK

  const uint32_t set_mosi_as_output = (1 << 1);
  LPC_GPIO1->DIR |= set_mosi_as_output; // set direction register for MOSI pin
  LPC_IOCON->P1_1 &= ~(0b111 << 0);     // clear the IOCON pins
  LPC_IOCON->P1_1 |= enable;            // set pin functionality as MOSI

  const uint32_t set_cs_as_output = (1 << 10);
  LPC_GPIO1->DIR |= set_cs_as_output; // set direction register for CS pin

  const uint32_t tg = (1 << 20);
  LPC_GPIO1->DIR |= tg; // set trigger pin for logic analyzer as output
}

void adesto_cs(void) {
  const uint32_t clear_cs_pin = (1 << 10);
  LPC_GPIO1->CLR = clear_cs_pin; // clear pin so that active low wil be enabled

  const uint32_t clear_tg_pin = (1 << 20);
  LPC_GPIO1->CLR = clear_tg_pin; // clear pin so that active low wil be enabled
}

void adesto_ds(void) {
  const uint32_t deselect_cs_pin = (1 << 10);
  LPC_GPIO1->SET = deselect_cs_pin; // set pin so that active low will be disabled

  const uint32_t set_tg_pin = (1 << 20);
  LPC_GPIO1->SET = set_tg_pin; // set pin so that active low will be disabled
}

adesto_flash_id_s adesto_read_signature(void) {
  adesto_flash_id_s data = {0};

  adesto_cs();

  const uint8_t opcode_to_read_id = 0x9F;
  const uint8_t dummy_byte = 0xAA;
  ssp2_lab__exchange_byte(opcode_to_read_id);
  data.manufacturer_id = ssp2_lab__exchange_byte(dummy_byte);
  data.device_id_1 = ssp2_lab__exchange_byte(dummy_byte);
  data.device_id_2 = ssp2_lab__exchange_byte(dummy_byte);
  data.extended_device_id = ssp2_lab__exchange_byte(dummy_byte);

  adesto_ds();

  return data;
}

void spi_id_verification_task(void *p) {

  const uint32_t spi_clock_mhz = 24;
  ssp2__init(spi_clock_mhz);
  ssp2_pin_configure();

  while (1) {

    adesto_flash_id_s id;
    if (xSemaphoreTake(spi_bus_mutex, 1000)) {
      id = adesto_read_signature();
      printf("\nAccessed the precious resource");
      printf("\nManufacturer ID is: %x", id.manufacturer_id);

      xSemaphoreGive(spi_bus_mutex);
      printf("\nReturned the precious resource");
    }
    // When we read a manufacturer ID we do not expect, we will kill this task
    if (id.manufacturer_id != 0x1F) {
      fprintf(stderr, "\nManufacturer ID read failure");
      vTaskSuspend(NULL); // Kill this task
    }

    vTaskDelay(400);
  }
}

int main(void) {
  // create_blinky_tasks();
  create_uart_task();
  spi_bus_mutex = xSemaphoreCreateMutex();

  xTaskCreate(spi_id_verification_task, "spi", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(spi_id_verification_task, "spi2", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

#endif

#ifdef PART_3

void adesto_cs(void);
void adesto_ds(void);

typedef struct {
  uint8_t value;
} adesto_flash_id_s;

adesto_flash_id_s adesto_read_signature(void) {
  adesto_flash_id_s data = {0};

  adesto_cs();

  const uint8_t opcode_to_enable_wel = 0x06; // to enable WEL register
  ssp2_lab__exchange_byte(opcode_to_enable_wel);

  adesto_ds();

  adesto_cs();
  const uint8_t opcode_to_erase = 0x20;
  ssp2_lab__exchange_byte(opcode_to_erase);
  const uint32_t address = 0xBBBBBB; // memory adress to erase

  // begin sending address of the memory
  ssp2_lab__exchange_byte((address >> 16) & 0xFF);
  ssp2_lab__exchange_byte((address >> 8) & 0xFF);
  ssp2_lab__exchange_byte((address >> 0) & 0xFF);

  adesto_ds();

  adesto_cs();
  const uint8_t opcode_to_begin_write = 0x02; // to begin write
  const uint32_t adress = 0xBBBBBB;           // 24 bit address
  const uint8_t data_byte = 0x1A;
  ssp2_lab__exchange_byte(opcode_to_begin_write);

  // begin sending address of the memory
  ssp2_lab__exchange_byte((adress >> 16) & 0xFF);
  ssp2_lab__exchange_byte((adress >> 8) & 0xFF);
  ssp2_lab__exchange_byte((adress >> 0) & 0xFF);
  ssp2_lab__exchange_byte(data_byte); // send data byte

  adesto_ds();

  adesto_cs();

  const uint8_t opcode_to_read_data = 0x03; // to begin read
  const uint32_t memory_adress = 0xBBBBBB;  // 32 bit memory address
  const uint8_t dummy = 0xAA;

  // begin sending address of the memory to be read
  ssp2_lab__exchange_byte((memory_adress >> 16) & 0xFF);
  ssp2_lab__exchange_byte((memory_adress >> 8) & 0xFF);
  ssp2_lab__exchange_byte((memory_adress >> 0) & 0xFF);
  data.value = ssp2_lab__exchange_byte(dummy); // obtain data that is transferred into the page

  adesto_ds();

  return data;
}

void adesto_cs(void) {
  const uint32_t clear_cs_pin = (1 << 10);
  LPC_GPIO1->CLR = clear_cs_pin; // clear pin so that active low wil be enabled
  const uint32_t clear_tg_pin = (1 << 28);
  LPC_GPIO4->CLR = clear_tg_pin; // clear pin so that active low wil be enabled
}

void adesto_ds(void) {
  const uint32_t deselect_cs_pin = (1 << 10);
  LPC_GPIO1->SET = deselect_cs_pin; // set pin so that active low will be disabled
  const uint32_t set_tg_pin = (1 << 28);
  LPC_GPIO4->SET = set_tg_pin; // set pin so that active low will be disabled
}

void ssp2_pin_configure(void) {
  const uint32_t enable = (1 << 2);
  const uint32_t set_miso_as_input = (1 << 4);
  LPC_GPIO1->DIR &= ~(set_miso_as_input); // set direction register for MISO pin
  LPC_IOCON->P1_4 &= ~(0b111 << 0);       // clear the iocon pins
  LPC_IOCON->P1_4 |= enable;              // set pin functionality as MISO

  const uint32_t set_sck_as_output = (1 << 0);
  LPC_GPIO1->DIR |= (set_sck_as_output); // set direction register for SCK
  LPC_IOCON->P1_0 &= ~(0b111 << 0);      // clear the iocon pins
  LPC_IOCON->P1_0 |= enable;             // set pin functionality as SCK

  const uint32_t set_mosi_as_output = (1 << 1);
  LPC_GPIO1->DIR |= set_mosi_as_output; // set direction register for MOSI pin
  LPC_IOCON->P1_1 &= ~(0b111 << 0);     // clear the IOCON pins
  LPC_IOCON->P1_1 |= enable;            // set pin functionality as MOSI

  const uint32_t set_cs_as_output = (1 << 10);
  LPC_GPIO1->DIR |= set_cs_as_output; // set direction register for CS pin

  const uint32_t tg = (1 << 28);
  LPC_GPIO4->DIR |= tg; // set trigger pin for logic analyzer as output
}

void read_write_task(void *params) {
  const uint32_t spi_clock_mhz = 24;
  ssp2__init(spi_clock_mhz);
  ssp2_pin_configure();

  while (1) {
    adesto_flash_id_s id = adesto_read_signature();
    printf("\nData read: %x", id.value);

    vTaskDelay(500);
  }
}

int main(void) {
  // create_blinky_tasks();
  create_uart_task();

  xTaskCreate(read_write_task, "spi", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

  puts("Starting RTOS");
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
