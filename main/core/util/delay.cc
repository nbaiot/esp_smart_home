
#include "delay.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace esp {
void DelayMS(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
}  // namespace esp
