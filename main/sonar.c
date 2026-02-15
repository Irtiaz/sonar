#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

#define TXD_PIN GPIO_NUM_17
#define RXD_PIN GPIO_NUM_16
#define UART_NUM UART_NUM_1
#define BAUD_RATE 115200
#define BUF_SIZE 256
#define TRIGGER_CMD 0x01
#define FRAME_HEADER 0xFF
#define NO_TARGET_VALUE 0xFFFD

#define DYP_L08_NO_TARGET -1
#define DYP_L08_ERROR -2

static const char *TAG = "DYP-L08";

static esp_err_t dyp_l08_init(void) {
  const uart_config_t uart_config = {
      .baud_rate = BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };

  esp_err_t err = uart_param_config(UART_NUM, &uart_config);
  if (err != ESP_OK)
    return err;

  err = uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE,
                     UART_PIN_NO_CHANGE);
  if (err != ESP_OK)
    return err;

  return uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
}

static int32_t dyp_l08_get_distance_mm(void) {
  const uint8_t cmd = TRIGGER_CMD;
  uart_write_bytes(UART_NUM, &cmd, 1);

  uint8_t data[4];
  int len = uart_read_bytes(UART_NUM, data, 4, pdMS_TO_TICKS(500));

  if (len < 4 || data[0] != FRAME_HEADER) {
    return DYP_L08_ERROR;
  }

  uint8_t h = data[1];
  uint8_t l = data[2];
  uint8_t sum = data[3];
  uint8_t expected = (FRAME_HEADER + h + l) & 0xFF;

  if (sum != expected) {
    return DYP_L08_ERROR;
  }

  uint16_t dist = (h << 8) | l;
  if (dist == NO_TARGET_VALUE) {
    return DYP_L08_NO_TARGET;
  }

  return dist;
}

void app_main(void) {
  esp_err_t err = dyp_l08_init();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Init failed: %s", esp_err_to_name(err));
    return;
  }

  ESP_LOGI(TAG, "Sensor ready");

  while (1) {
    int32_t dist = dyp_l08_get_distance_mm();

    if (dist == DYP_L08_NO_TARGET) {
      ESP_LOGW(TAG, "No target detected");
    } else if (dist == DYP_L08_ERROR) {
      ESP_LOGE(TAG, "Read error");
    } else {
      ESP_LOGI(TAG, "Distance: %ld mm", dist);
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
