#pragma once

namespace esp {

// TODO: config sntp server, try count
class SntpManager {
 public:
  static SntpManager* Instance();

  bool SyncTime();

  bool HasSyncTime() const;

 private:
  SntpManager() = default;

 private:
  bool sync_time_success_{false};
};

}  // namespace esp
