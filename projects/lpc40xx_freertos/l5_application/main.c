#include "FreeRTOS.h"
#include "cli_handlers.h"
#include "delay.h"
#include "event_groups.h"
#include "ff.h"
#include "i2c.h"
#include "queue.h"
#include "sj2_cli.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

/********************************************
 * Used for identifying the sensor we want.
 *******************************************/
uint8_t SENSORS_ADDRESS[] = {0x38, 0x64, 0x72};

typedef enum { ACCELEROMETER = 0, TEMPERATURE, LIGHT } i2c_sensor_e;

typedef struct {
  i2c_sensor_e name;
  uint8_t address;
} sj2_sensors;

uint8_t I2C_DEVICE_ID[3];

typedef struct {
  int16_t x;
  int16_t y;
  int16_t z;
  int time;
} accelerometer_data;

int16_t convert_bytes_to_int(uint8_t msb, uint8_t lsb) {
  int16_t value = (((uint16_t)msb << 4) | ((uint16_t)lsb));
  if (value & 0x0800) {
    value |= 0xF000;
  }
  return value;
}

static QueueHandle_t sensor_queue;
static EventGroupHandle_t watchdog_event_group;

static void initialize_sensors(void);
static void producer(void *p);
static void consumer(void *p);
static void watchdog(void *p);

static void initialize_sensors(void) {
  // Initialize all the sensors used for this assignment. That being, the light and accelerometer.
  uint32_t i2c_clock_speed = 200000;
  i2c__initialize(I2C__2, i2c_clock_speed, clock__get_peripheral_clock_hz()); // All sensors are on SDA2, SCL2

  sj2_sensors accelerometer_sensor = {ACCELEROMETER, SENSORS_ADDRESS[ACCELEROMETER]};
  sj2_sensors temperature_sensor = {TEMPERATURE, SENSORS_ADDRESS[TEMPERATURE]};
  sj2_sensors light_sensor = {LIGHT, SENSORS_ADDRESS[LIGHT]};
  /********************************************
   * Accelerometer Sensor (is always on)
   ******************************************/
  // Read out the accelerometer sensor data.
  uint8_t accel_sensor_device_id = 0x0D;
  I2C_DEVICE_ID[0] = i2c__read_single(I2C__2, accelerometer_sensor.address, accel_sensor_device_id);

  /********************************************
   * Temperature Sensor (is always on)
   ******************************************/
  // Read out the temperature sensor data.
  uint8_t temp_sensor_device_id = 0xC0;
  I2C_DEVICE_ID[1] = i2c__read_single(I2C__2, temperature_sensor.address, temp_sensor_device_id);

  /********************************************
   * Light Sensor
   ********************************************/
  // Power on the light sensor.
  uint8_t light_sensor_enable = 0x80;
  uint8_t power_on_sensor = 0xC0;
  i2c__write_single(I2C__2, light_sensor.address, light_sensor_enable, power_on_sensor);
  delay__ms(1);

  // Read out the Device ID of the light sensor.
  uint8_t light_sensor_device_id = 0x92;
  I2C_DEVICE_ID[2] = i2c__read_single(I2C__2, light_sensor.address, light_sensor_device_id);
}

static void producer(void *p) {
  // The Producer task will display the average value from the acceleration X,Y,Z sensor data.

  const uint8_t accelerometer_address = 0x38;
  const uint8_t accelerometer_data_register = 0x01;

  uint8_t accelerometer_output[6];
  int total_samples = 100;
  int16_t sum_x = 0;
  int16_t sum_y = 0;
  int16_t sum_z = 0;
  accelerometer_data accelerometer_output_data;

  while (1) {
    sum_x = 0;
    sum_y = 0;
    sum_z = 0;
    // Loop for 100 samples (vTaskDelay waits 1ms)
    for (int samples = 0; samples < 100; samples++) {
      /****************************************************************************************************
       *  This command starts at data register 0x01 and increments 6 times up to 0x06 while adding data to
       *  The array accelerometer_output.
       ****************************************************************************************************/
      i2c__read_slave_data(I2C__2, accelerometer_address, accelerometer_data_register, &accelerometer_output, 6);
      accelerometer_output_data.x = convert_bytes_to_int(accelerometer_output[0], accelerometer_output[1]);
      accelerometer_output_data.y = convert_bytes_to_int(accelerometer_output[2], accelerometer_output[3]);
      accelerometer_output_data.z = convert_bytes_to_int(accelerometer_output[4], accelerometer_output[5]);
      sum_x += accelerometer_output_data.x;
      sum_y += accelerometer_output_data.y;
      sum_z += accelerometer_output_data.z;
      vTaskDelay(1);
    }
    // Finally find the average and add back into the struct accel_data with average values.
    accelerometer_output_data.x = sum_x / 100; // Averaged X data over 100 samples.
    accelerometer_output_data.y = sum_y / 100; // Averaged Y data over 100 samples.
    accelerometer_output_data.z = sum_z / 100; // Averaged Z data over 100 samples.

    // Immediately add to the Queue immediately.
    printf("Accelerometer(X,Y,Z): %d,%d,%d\n", accelerometer_output_data.x, accelerometer_output_data.y,
           accelerometer_output_data.z);
    xQueueSend(sensor_queue, &accelerometer_output_data, 0);

    // WatchDog Event Bit 0 = Producer sent sensor data to the queue successfully.
    xEventGroupSetBits(watchdog_event_group, 1 << 0);
  }
}

static void consumer(void *p) {
  // Wait for the Queue to have available sensor data and then write the data to the microSD Card.
  accelerometer_data accelerometer_output_data;
  accelerometer_data accelerometer_outputs[10];

  // WARNING!!! Make sure the microSD card is pre-formatted to FAT32 before continuing.
  const char *filename = "sensor_data.txt";

  // Create the file in the microSD card and then close it.
  FIL file; // File handle
  f_open(&file, filename, (FA_WRITE | FA_CREATE_ALWAYS));
  f_close(&file);

  while (1) {
    // Wait for the Queue 10 times in order to receive 1 second worth of data.
    for (int index = 0; index < 10; ++index) {
      // Wait infinitely to receive the the accelerometer sensor data from the Queue.
      xQueueReceive(sensor_queue, &accelerometer_output_data, portMAX_DELAY);
      accelerometer_outputs[index] = accelerometer_output_data;

      // Retrieves the current tick count (1 tick = 1 ms)
      accelerometer_outputs[index].time = (int)xTaskGetTickCount();
    }

    // If the file opened successfully, write the data to the file.
    UINT bytes_written = 0;
    FRESULT file_open_successful = f_open(&file, filename, (FA_WRITE | FA_OPEN_APPEND));

    // If file is finally open, we write all the data at one time to the SD card.
    if (FR_OK == file_open_successful) {
      // WatchDog Event Bit 1 = Successfully opened a file on the microSD card.
      xEventGroupSetBits(watchdog_event_group, 1 << 1);

      // Write all the 10 samples to the
      int successful_writes = 0;
      for (int sample = 0; sample < 10; ++sample) {
        char string[40];
        sprintf(string, "%d,%d,%d,%d\n", accelerometer_outputs[sample].time, accelerometer_outputs[sample].x,
                accelerometer_outputs[sample].y, accelerometer_outputs[sample].z);

        if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
          ++successful_writes;
        } else {
          printf("ERROR: Failed to write data to file\n");
        }
      }
      if (successful_writes == 10) {
        // WatchDog Event Bit 2 = microSD Card had all data successfully written to.
        xEventGroupSetBits(watchdog_event_group, 1 << 2);
      }
      f_close(&file); // Close the file after attempting to write all samples to the SD card.
    } else {
      printf("ERROR: Failed to open: %s\n", filename);
    }
  }
}

void watchdog(void *p) {
  while (1) {
    /*******************************************************************************
     *  Reference: https://www.freertos.org/xEventGroupWaitBits.html.
     *  Wait for all 3 Event Bits to be set while not altering the bits on return
     *  from the xEventGroupWaitBits() call. Also wait for up to 1000 ticks.
     ********************************************************************************/
    vTaskDelay(200);
    // You must wait an additional 1300 ms to let the SD card finish writing every one second.
    const TickType_t xTicksToWait = 1500 / portTICK_PERIOD_MS;
    if (xEventGroupWaitBits(watchdog_event_group, 0b111, pdFALSE, pdTRUE, xTicksToWait)) {
      uint32_t event_group_bits = xEventGroupGetBits(watchdog_event_group);

      // If all the Event Bits are set, all tasks completed successfully.
      if (event_group_bits == 0b111) {
        // printf("Watchdog: Producer & Consumer completed successfully.\n");
      } else {
        // If even one bit was not set, send the errors to the serial console for debug.
        if (!(event_group_bits & (1 << 0)))
          printf("Watchdog: Producer unable to collect Accelerometer Sensor data!\n");
        if (!(event_group_bits & (1 << 1)))
          printf("Watchdog: Consumer unable to open microSD card for writing!\n");
        if (!(event_group_bits & (1 << 2)))
          printf("Watchdog: Consumer did not write all Accelerometer Data to the microSD card!\n");
      }

    } else {
      // In the even that at least one of the event bits wasn't set, explain the error.
      uint32_t event_group_bits = xEventGroupGetBits(watchdog_event_group);
      if (!(event_group_bits & (1 << 0)))
        printf("Watchdog: Producer unable to collect Accelerometer Sensor data!\n");
      if (!(event_group_bits & (1 << 1)))
        printf("Watchdog: Consumer unable to open microSD card for writing!\n");
      if (!(event_group_bits & (1 << 2)))
        printf("Watchdog: Consumer did not write all Accelerometer Data to the microSD card!\n");
    }

    // Finally, clear all the bits, to ensure correct operation.
    xEventGroupClearBits(watchdog_event_group, 1 << 0);
    xEventGroupClearBits(watchdog_event_group, 1 << 1);
    xEventGroupClearBits(watchdog_event_group, 1 << 2);
  }
}

int main(void) {
  /* Part 2: Use CLI to suspend and resume tasks currently running. */
  sensor_queue = xQueueCreate(1, sizeof(accelerometer_data)); // Accelerometer Sensor X,Y,Z data.
  watchdog_event_group = xEventGroupCreate();
  sj2_cli__init(); // Initialize the CLI.

  // Initialize all the I2C Sensors on the SJ2 Board and verify they are all working.
  initialize_sensors();
  fprintf(stderr, "Accel Device ID: %x\n", I2C_DEVICE_ID[0]);
  fprintf(stderr, "Temp Device ID: %x\n", I2C_DEVICE_ID[1]);  // Unused
  fprintf(stderr, "Light Device ID: %x\n", I2C_DEVICE_ID[2]); // Unused

  xTaskCreate(producer, "producer", 2048 / sizeof(void *), NULL, PRIORITY_MEDIUM, &producer_task);
  xTaskCreate(consumer, "consumer", 2048 / sizeof(void *), NULL, PRIORITY_MEDIUM, &consumer_task);
  xTaskCreate(watchdog, "watchdog", 2048 / sizeof(void *), NULL, PRIORITY_HIGH, &watchdog_task);

  vTaskStartScheduler();

  return 0;
}
