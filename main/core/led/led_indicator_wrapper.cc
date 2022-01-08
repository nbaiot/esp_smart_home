
#include "led_indicator_wrapper.h"

extern "C" {
#include "led_indicator.h"
}

#include "esp_log.h"

#define TAG "led_indicator"

namespace esp {
LedIndicator::LedIndicator(int32_t gpio_num, uint8_t gpio_off_level, Mode mode)
    : io_num_(gpio_num) {
  led_indicator_config_t led_config = {};
  led_config.off_level = gpio_off_level;
  if (mode == GPIO) {
    led_config.mode = LED_GPIO_MODE;
  }
  handle_ = led_indicator_create(io_num_, &led_config);
  //  led_indicator_start(handle, BLINK_CONNECTED);
}

LedIndicator::~LedIndicator() {
  if (handle_) {
    led_indicator_delete(&handle_);
  }
}

bool LedIndicator::Start(Type type) {
  if (!handle_) {
    return false;
  }
  ESP_LOGI("led","######### start:::%d", type);
  auto err = led_indicator_start(handle_, (led_indicator_blink_type_t)type);
  auto success = err == ESP_OK;
  if (!success) {
    ESP_LOGE(TAG, "start indicator failed:%s, gpio:%d", esp_err_to_name(err),
             io_num_);
  }
  return success;
}

bool LedIndicator::Stop(Type type) {
  if (!handle_) {
    return false;
  }
  ESP_LOGI("led", "######### stop:::%d", (led_indicator_blink_type_t)type);
  auto err = led_indicator_stop(handle_, (led_indicator_blink_type_t)type);
  auto success = err == ESP_OK;
  if (!success) {
    ESP_LOGE(TAG, "stop indicator failed:%s, gpio:%d", esp_err_to_name(err),
             io_num_);
  }
  return success;
}

}  // namespace esp
