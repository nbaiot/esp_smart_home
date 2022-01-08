
#include "core/led/led_indicator_wrapper.h"
#include "core/util/delay.h"
#include "core/util/board_info.h"

using namespace esp;

extern "C" void app_main(void) {
  PrintBoardInfoToLog();

  LedIndicator led(26, 0, LedIndicator::GPIO);
  led.Start(LedIndicator::CONNECTING);
  for (;;) {
    DelayMS(1000);
  }
}
