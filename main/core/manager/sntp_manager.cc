
#include "sntp_manager.h"

#include <sys/time.h>
#include <time.h>

#include "esp_log.h"
#include "esp_sntp.h"

namespace esp {

static const char* TAG = "sntp_manager";

static const char* SNTP_SERVER = "ntp.ntsc.ac.cn";
#define SNTP_TIMEOUT_MS (2 * 1000)
#define SNTP_TRY_COUNT (30)

static void SetTZ() {
  // Set timezone to China Standard Time
  setenv("TZ", "CST-8", 1);
  tzset();
}

static void SntpTimeSyncNotificationCallback(struct timeval* tv) {
  ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static bool CheckTimeValid() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  // Is time set? If not, tm_year will be (1970 - 1900).
  if (timeinfo.tm_year >= (2022 - 1900)) {
    return true;
  }
  return false;
}

bool SyncSNTP(int32_t timeoutMS, uint8_t tryCount) {
  ESP_LOGI(TAG, "##### start sync SNTP");

  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, SNTP_SERVER);
  sntp_set_time_sync_notification_cb(SntpTimeSyncNotificationCallback);
  sntp_init();

  // wait for time to be set
  time_t now = 0;
  struct tm timeinfo {};
  int retry = 0;
  while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET &&
         ++retry < tryCount) {
    ESP_LOGI(TAG, "Waiting for system time to be set...(%d/%d)", retry,
             tryCount);
    vTaskDelay(timeoutMS / portTICK_PERIOD_MS);
  }
  bool success = true;
  if (retry >= tryCount && sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
    success = false;
  }
  time(&now);
  localtime_r(&now, &timeinfo);
  sntp_stop();
  return success;
}

SntpManager* SntpManager::Instance() {
  static SntpManager INSTANCE;
  return &INSTANCE;
}

bool SntpManager::HasSyncTime() const { return sync_time_success_; }

bool SntpManager::SyncTime() {
  if (!SyncSNTP(SNTP_TIMEOUT_MS, SNTP_TRY_COUNT)) {
    ESP_LOGE(TAG, "##### sync SNTP failed");
    sync_time_success_ = false;
    return false;
  }
  sync_time_success_ = CheckTimeValid();
  if (sync_time_success_) {
    ESP_LOGI(TAG, "##### sync SNTP success");
  } else {
    ESP_LOGE(TAG, "##### sync SNTP failed");
    return false;
  }
  SetTZ();
  return true;
}

}  // namespace esp
