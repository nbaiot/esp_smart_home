
#include <memory>

#include "esp_log.h"

#include "core/init.h"
#include "core/led/led_indicator_wrapper.h"
#include "core/manager/wifi_manager.h"
#include "core/mqtt/mqtt_client_wrapper.h"
#include "core/util/board_info.h"
#include "core/util/delay.h"

static const char* TAG = "main";

using namespace esp;

extern "C" void app_main(void) {
  PrintBoardInfoToLog();
  if (!BaseInit()) {
    return;
  }

  if (WifiManager::Instance()->Init()) {
    WifiManager::Instance()->SetStaLedIndicator(new LedIndicator(26, 0, LedIndicator::GPIO));
    if (WifiManager::Instance()->SwitchStationMode("TP-th", "tanhe123")) {
      WifiManager::Instance()->Start();
    }
  }

  auto mqtt_client = std::make_shared<MqttClient>("mqtt://192.168.0.105:1883", "123", true, true);
  mqtt_client->SetOnReadyCallback([client=mqtt_client.get()](){
      ESP_LOGI(TAG, "connect mqtt broker");
      client->Subscribe("topic/#", 0);
  });
  mqtt_client->SetOnReceiveMsgCallback([](std::string topic, const char* data, int32_t len){
      std::string msg(data, len);
      ESP_LOGI(TAG, "receive msg: %s from topic:%s", msg.c_str(), topic.c_str());
  });
  mqtt_client->Start();
  for (;;) {
    ESP_LOGI(TAG, "hello");
    DelayMS(1000);
    mqtt_client->AsyncPublish("topic/test", "hello123", 8, 0, 0);
  }
}
