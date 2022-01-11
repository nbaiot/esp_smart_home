
#include <memory>

#include "core/event/global_event_bus.h"
#include "core/init.h"
#include "core/led/led_indicator_wrapper.h"
#include "core/manager/sntp_manager.h"
#include "core/manager/wifi_manager.h"
#include "core/mqtt/mqtt_client_wrapper.h"
#include "core/util/board_info.h"
#include "core/util/delay.h"
#include "esp_log.h"

static const char* TAG = "main";

using namespace esp;

class TestEvent1 : public Event {
 public:
  char* Name() override { return "test_event1"; }

  int8_t Priority() override { return 0; }
};

class TestEvent2 : public Event {
 public:
  char* Name() override { return "test_event2"; }

  int8_t Priority() override { return 1; }
};

class TestEvent3 : public Event {
 public:
  char* Name() override { return "test_event3"; }

  int8_t Priority() override { return 2; }
};

class TestEventHandler : public EventHandler {
 public:
  void Process(Event* event) override {
    ESP_LOGI(TAG, "################## process event:%s", event->Name());
    DelayMS(10);
  }
};

extern "C" void app_main(void) {
  PrintBoardInfoToLog();
  if (!BaseInit()) {
    return;
  }

  PrintHeapMemInfoToLog();
  if (WifiManager::Instance()->Init()) {
    WifiManager::Instance()->SetStaLedIndicator(
        new LedIndicator(26, 0, LedIndicator::GPIO));
    if (WifiManager::Instance()->SwitchStationMode("TP-th", "tanhe123")) {
      WifiManager::Instance()->Start();
    }
  }

  PrintHeapMemInfoToLog();
  if (!SntpManager::Instance()->SyncTime()) {
    ESP_LOGE(TAG, "sntp time failed");
  }

  PrintHeapMemInfoToLog();
  auto mqtt_client = std::make_shared<MqttClient>("mqtt://192.168.0.105:1883",
                                                  "123", true, true);
  mqtt_client->SetOnReadyCallback([client = mqtt_client.get()]() {
    ESP_LOGI(TAG, "connect mqtt broker");
    client->Subscribe("topic/#", 0);
  });
  mqtt_client->SetOnReceiveMsgCallback([](std::string topic, const char* data,
                                          int32_t len) {
    std::string msg(data, len);
    ESP_LOGI(TAG, "receive msg: %s from topic:%s", msg.c_str(), topic.c_str());
  });
  PrintHeapMemInfoToLog();
  mqtt_client->Start();
  auto event_handler = std::make_shared<TestEventHandler>();
  GlobalEventBus::Instance()->Subscribe("test_event1", event_handler);
  GlobalEventBus::Instance()->Subscribe("test_event3", event_handler);
  GlobalEventBus::Instance()->Subscribe("test_event2", event_handler);
  for (;;) {
    ESP_LOGI(TAG, "hello");
    PrintHeapMemInfoToLog();
    DelayMS(1000);
    GlobalEventBus::Instance()->Publish(new TestEvent3());
    GlobalEventBus::Instance()->Publish(new TestEvent2());
    GlobalEventBus::Instance()->Publish(new TestEvent1());
    mqtt_client->AsyncPublish("topic/test", "hello123", 8, 0, 0);
  }
}
