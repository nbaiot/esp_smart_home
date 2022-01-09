
#include "esp_log.h"

#include "core/init.h"
#include "core/led/led_indicator_wrapper.h"
#include "core/manager/wifi_manager.h"
#include "core/util/board_info.h"
#include "core/util/delay.h"

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
  for (;;) {
    ESP_LOGI("main", "hello");
    DelayMS(1000);
  }
}
