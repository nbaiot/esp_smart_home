
#include "wifi_manager.h"

#include <algorithm>
#include <cstring>

#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "led/led_indicator_wrapper.h"

static const char* TAG = "wifi manager";

/* The event group allows multiple bits for each event, but we only care about
 * two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

namespace esp {
static EventGroupHandle_t s_wifi_event_group;

static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;
static int s_retry_num = 0;

static void sta_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data) {
  auto manager = (WifiManager*)(arg);
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    // TODO: fixme
    if (s_retry_num < 5) {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    manager->OnStaConnectFailed();
    ESP_LOGI(TAG, "connect to the AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    manager->OnStaConnected();
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

WifiManager* WifiManager::Instance() {
  static WifiManager INSTANCE;
  return &INSTANCE;
}

bool WifiManager::Init() {
  if (init_) {
    ESP_LOGI(TAG, "wifi manager already init");
    return true;
  }
  s_wifi_event_group = xEventGroupCreate();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  auto ret = esp_wifi_init(&cfg);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "wifi manager init failed:%s", esp_err_to_name(ret));
    return false;
  }
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &sta_event_handler, this,
      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &sta_event_handler, this,
      &instance_got_ip));

  init_ = true;
  ESP_LOGI(TAG, "wifi manager init success");
  return true;
}

bool WifiManager::Deinit() {
  if (!init_) {
    return true;
  }
  esp_wifi_deinit();
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
      IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
      WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
  vEventGroupDelete(s_wifi_event_group);
  init_ = false;
  return true;
}

bool WifiManager::SwitchStationMode(const char* ssid, const char* password) {
  if (CurrentMode() == kSta) {
    wifi_config_t conf{};
    auto ret = esp_wifi_get_config(WIFI_IF_STA, &conf);
    if (ret == ESP_OK) {
      if (std::strcmp((const char*)conf.sta.ssid, ssid) == 0 &&
          std::strcmp((const char*)conf.sta.password, password) == 0) {
        ESP_LOGI(TAG, "wifi manager already on STA mode");
        return true;
      }
    }
  }

  Stop();

  esp_netif_destroy_default_wifi(esp_netif_);
  esp_netif_ = esp_netif_create_default_wifi_sta();

  auto ret = esp_wifi_set_mode(WIFI_MODE_STA);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "set wifi_set_mode failed:%s", esp_err_to_name(ret));
    return false;
  }
  wifi_config_t wifi_config{};
  std::memcpy(wifi_config.sta.ssid, ssid,
              std::min(std::strlen(ssid), (size_t)32));
  std::memcpy(wifi_config.sta.password, password,
              std::min(std::strlen(password), (size_t)64));
  /* Setting a password implies station will connect to all security
   * modes including WEP/WPA. However these modes are deprecated and
   * not advisable to be used. Incase your Access point doesn't
   * support WPA2, these mode can be enabled by commenting below
   * line */
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  wifi_config.sta.pmf_cfg.capable = true;
  wifi_config.sta.pmf_cfg.required = false;

  ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "set wifi_config failed:%s", esp_err_to_name(ret));
    return false;
  }
  mode_ = kSta;
  return true;
}

bool WifiManager::Start() {
  if (is_ready_) {
    ESP_LOGW(TAG, "wifi manager already started");
    return true;
  }
  s_retry_num = 0;
  auto ret = esp_wifi_start();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "wifi start failed:%s", esp_err_to_name(ret));
    return false;
  }
  if (sta_indicator_) {
    sta_indicator_->Start(LedIndicator::CONNECTING);
  }
  return true;
}

bool WifiManager::Stop() {
  if (!is_ready_) {
    ESP_LOGW(TAG, "wifi manager already stoped");
    return true;
  }
  auto ret = esp_wifi_stop();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "wifi stop failed:%s", esp_err_to_name(ret));
    return false;
  }
  is_ready_ = false;
  return true;
}

void WifiManager::OnStaConnected() {
  is_ready_ = true;
  wifi_config_t conf{};
  auto ret = esp_wifi_get_config(WIFI_IF_STA, &conf);
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", (char*)conf.sta.ssid,
             (char*)conf.sta.password);
  }
  if (sta_indicator_) {
    sta_indicator_->Stop(LedIndicator::CONNECTING);
    sta_indicator_->Start(LedIndicator::CONNECTED);
  }
  if (listener_) {
    listener_->OnStaGotIp();
  }
}

void WifiManager::OnStaConnectFailed() {
  is_ready_ = false;
  wifi_config_t conf{};
  auto ret = esp_wifi_get_config(WIFI_IF_STA, &conf);
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", conf.sta.ssid,
             conf.sta.password);
  }
  if (sta_indicator_) {
    sta_indicator_->Stop(LedIndicator::CONNECTING);
    sta_indicator_->Stop(LedIndicator::CONNECTED);
  }
  if (listener_) {
    listener_->OnStaConnectFailed();
  }
}

bool WifiManager::IsReady() { return is_ready_; }

WifiManager::Mode WifiManager::CurrentMode() const { return mode_; }

bool WifiManager::AddListener(Listener* listener) {
  listener_ = listener;
  return true;
}

bool WifiManager::RemoveListener(Listener* listener) {
  listener_ = nullptr;
  return true;
}

void WifiManager::SetStaLedIndicator(LedIndicator* indicator) {
  sta_indicator_ = indicator;
}

}  // namespace esp
