/* Wi-Fi iperf Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "cmd_system.h"
#include "driver/gpio.h"
#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <errno.h>
#include <stdbool.h>
#include <string.h>

/*
 * Stub function for esp_wifi_enable_easy_fragment
 * This function is not supported on ESP32-C5 but is referenced by wifi-cmd
 * component. Providing a stub to fix linker error.
 */
void esp_wifi_enable_easy_fragment(bool enable) {
  ESP_LOGW("WIFI", "wifi_frag is not supported on ESP32-C5");
  (void)enable;
}

/* component manager */
#include "iperf.h"
#include "iperf_cmd.h"
#include "ping_cmd.h"
#include "wifi_cmd.h"

#if CONFIG_ESP_WIFI_ENABLE_WIFI_TX_STATS || CONFIG_ESP_WIFI_ENABLE_WIFI_RX_STATS
#include "esp_wifi_he.h"
#endif
#if CONFIG_ESP_WIFI_ENABLE_WIFI_TX_STATS
extern int wifi_cmd_get_tx_statistics(int argc, char **argv);
extern int wifi_cmd_clr_tx_statistics(int argc, char **argv);
#endif
#if CONFIG_ESP_WIFI_ENABLE_WIFI_RX_STATS
extern int wifi_cmd_get_rx_statistics(int argc, char **argv);
extern int wifi_cmd_clr_rx_statistics(int argc, char **argv);
#endif

// #define EXAMPLE_ANTENNA_TYPE "FPC"
#define EXAMPLE_ANTENNA_TYPE "ROD"

// Valid for XIAO ESP32-C6 only
// 0: Internal Antenna
// 1: External Antenna
#define USE_EXTERNAL_ANTENNA 1

#if defined(CONFIG_ESP_EXT_CONN_ENABLE) && defined(CONFIG_ESP_HOST_WIFI_ENABLED)
#include "esp_extconn.h"
#endif

static const char *get_antenna_type(void) {
#if CONFIG_IDF_TARGET_ESP32C6
#if USE_EXTERNAL_ANTENNA
  return "External (" EXAMPLE_ANTENNA_TYPE ")";
#else
  return "Internal";
#endif
#else
  return EXAMPLE_ANTENNA_TYPE;
#endif
}

static const char *get_chip_name(void) {
#if CONFIG_IDF_TARGET_ESP32S3
  return "XIAO ESP32-S3";
#elif CONFIG_IDF_TARGET_ESP32C3
  return "XIAO ESP32-C3";
#elif CONFIG_IDF_TARGET_ESP32C6
  return "XIAO ESP32-C6";
#elif CONFIG_IDF_TARGET_ESP32C5
  return "XIAO ESP32-C5";
#else
  return "Unknown Device";
#endif
}

static const char *get_wifi_band(void) {
  uint8_t primary;
  wifi_second_chan_t second;
  esp_err_t ret = esp_wifi_get_channel(&primary, &second);
  if (ret != ESP_OK) {
    return "Unknown";
  }
  // Simple logic: channel 1-14 is 2.4GHz, others are 5GHz (or 6GHz if supported
  // later)
  if (primary <= 14) {
    return "2.4GHz";
  } else {
    return "5GHz";
  }
}

static const char *get_protocol_name(iperf_traffic_type_t type) {
  switch (type) {
  case IPERF_TCP_SERVER:
  case IPERF_TCP_CLIENT:
    return "TCP";
  case IPERF_UDP_SERVER:
  case IPERF_UDP_CLIENT:
    return "UDP";
  default:
    return "Unknown";
  }
}

void iperf_hook_show_wifi_stats(iperf_traffic_type_t type,
                                iperf_status_t status) {
  if (status == IPERF_STARTED) {
    printf("\n ==================================================\n");
    printf(" |                 Test Information               |\n");
    printf(" |                                                |\n");
    printf(" |  Device : %-36s |\n", get_chip_name());
    printf(" |  Antenna: %-36s |\n", get_antenna_type());
    printf(" |  Band   : %-36s |\n", get_wifi_band());
    printf(" |  Protocol: %-35s |\n", get_protocol_name(type));
    printf(" |                                                |\n");
    printf(" ==================================================\n\n");

#if CONFIG_ESP_WIFI_ENABLE_WIFI_TX_STATS
    if (type != IPERF_UDP_SERVER) {
      wifi_cmd_clr_tx_statistics(0, NULL);
    }
#endif
#if CONFIG_ESP_WIFI_ENABLE_WIFI_RX_STATS
    if (type != IPERF_UDP_CLIENT) {
      wifi_cmd_clr_rx_statistics(0, NULL);
    }
#endif
  }

  if (status == IPERF_STOPPED) {
#if CONFIG_ESP_WIFI_ENABLE_WIFI_TX_STATS
    if (type != IPERF_UDP_SERVER) {
      wifi_cmd_get_tx_statistics(0, NULL);
    }
#endif
#if CONFIG_ESP_WIFI_ENABLE_WIFI_RX_STATS
    if (type != IPERF_UDP_CLIENT) {
      wifi_cmd_get_rx_statistics(0, NULL);
    }
#endif
  }
}

static void configure_antenna(void) {
#if CONFIG_IDF_TARGET_ESP32C6
  // GPIO 3 Low to enable antenna function
  gpio_reset_pin(3);
  gpio_set_direction(3, GPIO_MODE_OUTPUT);
  gpio_set_level(3, 0);

  vTaskDelay(pdMS_TO_TICKS(100));

  // GPIO 14 to select antenna
  gpio_reset_pin(14);
  gpio_set_direction(14, GPIO_MODE_OUTPUT);
#if USE_EXTERNAL_ANTENNA
  gpio_set_level(14, 1); // External
#else
  gpio_set_level(14, 0); // Internal
#endif
#endif
}

void app_main(void) {
  configure_antenna();
#if defined(CONFIG_ESP_EXT_CONN_ENABLE) && defined(CONFIG_ESP_HOST_WIFI_ENABLED)
  esp_extconn_config_t ext_config = ESP_EXTCONN_CONFIG_DEFAULT();
  esp_extconn_init(&ext_config);
#endif

  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  /*
  NOTE(#15855): wifi_cmd_initialize_wifi is a basic function to start wifi, set
  handlers and set wifi-cmd status. For advanced usage, please refer to
  wifi_cmd.h or the document of wifi-cmd component:
  https://components.espressif.com/components/esp-qa/wifi-cmd

  example:
      wifi_cmd_wifi_init();
      my_function();  // <---- more configs before wifi start
      wifi_cmd_wifi_start();

  Please note that some wifi commands such as "wifi start/restart" may not work
  as expected if "wifi_cmd_initialize_wifi" was not used.
  */
  /* initialise wifi and set wifi-cmd status */
  wifi_cmd_initialize_wifi(NULL);
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

#if CONFIG_ESP_WIFI_ENABLE_WIFI_RX_STATS
#if CONFIG_ESP_WIFI_ENABLE_WIFI_RX_MU_STATS
  esp_wifi_enable_rx_statistics(true, true);
#else
  esp_wifi_enable_rx_statistics(true, false);
#endif
#endif
#if CONFIG_ESP_WIFI_ENABLE_WIFI_TX_STATS
  esp_wifi_enable_tx_statistics(ESP_WIFI_ACI_BE, true);
#endif

  esp_console_repl_t *repl = NULL;
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  repl_config.prompt = "iperf>";

  // init console REPL environment
#if CONFIG_ESP_CONSOLE_UART
  esp_console_dev_uart_config_t uart_config =
      ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
#elif CONFIG_ESP_CONSOLE_USB_CDC
  esp_console_dev_usb_cdc_config_t cdc_config =
      ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(
      esp_console_new_repl_usb_cdc(&cdc_config, &repl_config, &repl));
#elif CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
  esp_console_dev_usb_serial_jtag_config_t usbjtag_config =
      ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&usbjtag_config,
                                                       &repl_config, &repl));
#endif

  /* Register commands */
  register_system();
  /* From wifi-cmd */
  wifi_cmd_register_all();
  /* From iperf-cmd */
  app_register_iperf_commands();
  app_register_iperf_hook_func(iperf_hook_show_wifi_stats);
  /* From ping-cmd */
  ping_cmd_register_ping();

  printf("\n ==================================================\n");
  printf(" |       Steps to test WiFi throughput            |\n");
  printf(" |                                                |\n");
  printf(" |  1. Print 'help' to gain overview of commands  |\n");
  printf(" |  2. Configure device to station or soft-AP     |\n");
  printf(" |  3. Setup WiFi connection                      |\n");
  printf(" |  4. Run iperf to test UDP/TCP RX/TX throughput |\n");
  printf(" |                                                |\n");
  printf(" =================================================\n\n");

  // start console REPL
  ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
