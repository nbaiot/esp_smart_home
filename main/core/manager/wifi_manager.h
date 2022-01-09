#pragma once

#include <cstdint>

namespace esp {

class LedIndicator;
// bin size will increase 350k
class WifiManager {
 public:
  enum Mode {
    kAp,
    kSta,
    kApSta,
    kUnknown,
  };
  
  class Listener {
   public:
    virtual ~Listener() = default;

    // for sta
    virtual void OnStaGotIp() = 0;
    virtual void OnStaConnectFailed() = 0;

    // TODO: add ap

    // virtual void OnModeChanged(Mode new_mode) = 0;
  };

  static WifiManager* Instance();

  bool Init();

  bool Deinit();

  bool SwitchStationMode(const char* ssid, const char* password);

  bool Start();

  bool Stop();

  bool IsReady();

  Mode CurrentMode() const;

  // TODO: support listener list, now only support one listener
  bool AddListener(Listener* listener);
  bool RemoveListener(Listener* listener);

  // Note: manager will takeover indicator lifecycle
  void SetStaLedIndicator(LedIndicator* indicator);

  void OnStaConnected();
  void OnStaConnectFailed();

 private:
  WifiManager() = default;

 private:
  Mode mode_{kUnknown};
  Listener* listener_{nullptr};

  bool init_{false};
  void *esp_netif_{nullptr};
  bool is_ready_{false};

  LedIndicator* sta_indicator_{nullptr};
};
}  // namespace esp
