#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#define UART_NUM UART_NUM_1
#define TXD_PIN GPIO_NUM_17
#define RXD_PIN GPIO_NUM_16
#define UART_BAUD_RATE 115200
#define BUF_SIZE 256

#define TRIGGER_CMD 0x01
#define NO_TARGET 0xFFFD

static const char *TAG = "DYP-L08";

void app_main(void) {
  const uart_config_t uart_config = {
      .baud_rate = UART_BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };

  uart_param_config(UART_NUM, &uart_config);
  uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE);
  uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

  ESP_LOGI(TAG, "DYP-L08 sensor ready");

  uint8_t data[BUF_SIZE];
  const uint8_t cmd = TRIGGER_CMD;

  while (1) {
    uart_write_bytes(UART_NUM, &cmd, 1);

    int length = uart_read_bytes(UART_NUM, data, BUF_SIZE, pdMS_TO_TICKS(500));

    if (length >= 4 && data[0] == 0xFF) {
      uint8_t h = data[1];
      uint8_t l = data[2];
      uint8_t sum = data[3];
      uint8_t expected = (0xFF + h + l) & 0xFF;

      if (sum == expected) {
        uint16_t dist = (h << 8) | l;
        if (dist == NO_TARGET) {
          ESP_LOGW(TAG, "No target detected");
        } else {
          ESP_LOGI(TAG, "Distance: %u mm", dist);
        }
      } else {
        ESP_LOGE(TAG, "Checksum error");
      }
    } else if (length > 0) {
      ESP_LOGE(TAG, "Unexpected response (%d bytes)", length);
    } else {
      ESP_LOGW(TAG, "No response");
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
