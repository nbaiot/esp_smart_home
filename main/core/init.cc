
#include "init.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

static const char* TAG = "base init";

namespace esp {
bool BaseInit() {
  esp_err_t err = ESP_OK;
  err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGW(TAG, "nvs need erase");
#if 0
    err = nvs_flash_erase();
    err = nvs_flash_init();
#endif
  }
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "nvs init failed:%s", esp_err_to_name(err));
    return false;
  }

  // note: bin size will increase about 90k
  err = esp_netif_init();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "net init failed:%s", esp_err_to_name(err));
    return false;
  }
  ESP_LOGI(TAG, "netif init success");

  err = esp_event_loop_create_default();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "create default event loop failed:%s", esp_err_to_name(err));
    return false;
  }
  ESP_LOGI(TAG, "create default event loop success");

  return true;
}
}  // namespace esp
