#pragma once

#include <memory>

#include "event_bus.h"

namespace esp {
class GlobalEventBus {
 public:
  static GlobalEventBus* Instance();

  bool Publish(Event* event, int32_t timeout_ms = 0);

  bool Subscribe(const std::string& event_name,
                 std::shared_ptr<EventHandler> handler);

  bool Unsubscribe(const std::string& event_name,
                   std::shared_ptr<EventHandler> handler);

 private:
  GlobalEventBus();

 private:
  std::unique_ptr<EventBus> event_bus_;
};
}  // namespace esp
