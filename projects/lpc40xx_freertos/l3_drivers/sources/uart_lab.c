
#include "uart_lab.h"
#include "FreeRTOS.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "queue.h"

#include <stdbool.h>

#include <stdint.h>
#include <stdio.h>

static QueueHandle_t my_queue;

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  const uint32_t pconp_enable = (1 << 25);
  LPC_SC->PCONP |= pconp_enable; // enable UART3 PCONP register

  const uint32_t enable_iocon_as_uart3_tx = (0b010 << 0);
  const uint32_t enable_iocon_as_uart3_rx = (0b010 << 0);

  LPC_IOCON->P4_28 |= enable_iocon_as_uart3_tx; // Enable P0_10 as TX
  LPC_IOCON->P4_29 |= enable_iocon_as_uart3_rx; // Enbale P0_11 as RX

  const uint32_t set_datasize = (0b11 << 0); // set data size as 8 bit
  LPC_UART3->LCR |= set_datasize;

  const uint32_t setup_DLAB = (1 << 7); // set DLAB bit
  LPC_UART3->LCR |= setup_DLAB;

  const uint16_t divider_16_bit = ((peripheral_clock * 1000 * 1000) / (16 * baud_rate));
  LPC_UART3->DLL = ((divider_16_bit >> 0) & 0xFF); // store lower 8 bits into DLL
  LPC_UART3->DLM = ((divider_16_bit >> 8) & 0XFF); // store upper 8 bits into DLM
  LPC_UART3->LCR &= ~setup_DLAB;                   // clear DLAB bit
}

bool uart_lab__polled_get(uart_number_e uart, char *input_byte) {
  while (!LPC_UART3->LSR & (1 << 0)) {
    ;
  }
  *input_byte = LPC_UART3->RBR;
  return true;
}

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {
  while (!(LPC_UART3->LSR & (1 << 5))) {
    ;
  }
  LPC_UART3->THR = output_byte;
  return true;
}

static void receive_interrupt(void) {
  if (!(LPC_UART3->IIR & (1 << 0))) {
    uint8_t iir_value = ((LPC_UART3->IIR) & 0xF); // read pins 1,2 and 3 of IIR register and check whether value is 0x2
    if (iir_value == 4) {
      if (LPC_UART3->LSR & (1 << 0)) {
        const char byte = LPC_UART3->RBR;
        xQueueSendFromISR(my_queue, &byte, NULL);
      }
    }
  }
}

void uart__enable_receive_interrupt(uart_number_e uart_number) {
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, receive_interrupt, NULL);
  const uint8_t enable_uart_interrupt = (1 << 0);
  LPC_UART3->IER |= enable_uart_interrupt;  // enable UART3 interrupt
  my_queue = xQueueCreate(1, sizeof(char)); // create queue
}

bool uart_lab__get_char_from_queue(char *input_byte, uint32_t timeout) {
  return xQueueReceive(my_queue, input_byte, timeout);
}