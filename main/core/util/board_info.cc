
#include "board_info.h"

#include "sdkconfig.h"

#include "esp_spi_flash.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace esp {

void PrintBoardInfoToLog() {
  printf("############### BOARD INFO ###############\n");
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  printf("This is %s chip with %d CPU core(s) \n WiFi%s%s \n", CONFIG_IDF_TARGET,
         chip_info.cores, (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
         (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

  printf("silicon revision %d\n", chip_info.revision);

  printf(
      "%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
      (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

  printf("Minimum free heap size: %d bytes\n",
         esp_get_minimum_free_heap_size());
}

void PrintHeapMemInfoToLog() {
  printf("############### HEAP MEM INFO ###############\n");
  printf("free heap size: %d\n", esp_get_free_heap_size());
  printf("min free heap size: %d\n", esp_get_minimum_free_heap_size());
}

}  // namespace esp
