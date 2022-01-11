#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "event.h"

namespace esp {

class EventBusImpl;

class EventBus {
 public:
  struct Config {
    uint32_t max_event_cout{30};
    uint32_t task_stack_size{8192};
    uint32_t task_priority{0};
    std::string task_name;
    uint8_t task_cpu_core_id{0};
  };
  explicit EventBus(Config config);

  ~EventBus();

  bool Publish(Event* event, int32_t timeout_ms = 0);

  bool Subscribe(const std::string& event_name, std::shared_ptr<EventHandler> handler);

  bool Unsubscribe(const std::string& event_name, std::shared_ptr<EventHandler> handler);

 private:
  EventBusImpl* impl_{nullptr};
};

}  // namespace esp
