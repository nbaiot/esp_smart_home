
#include "global_event_bus.h"

#include <utility>

// TODO: config them
#define EVENT_CAPACITY 30
#define TASK_PRIORITY 5
#define TASK_CPU_COOR_ID 1
#define TASK_STACK_SIZE 8192
#define TASK_NAME "global_event_bus"

namespace esp {
GlobalEventBus* GlobalEventBus::Instance() {
  static GlobalEventBus INSTANCE;
  return &INSTANCE;
}

GlobalEventBus::GlobalEventBus() {
  EventBus::Config config{};
  config.max_event_cout = EVENT_CAPACITY;
  config.task_cpu_core_id = TASK_CPU_COOR_ID;
  config.task_name = TASK_NAME;
  config.task_priority = TASK_PRIORITY;
  config.task_stack_size = TASK_STACK_SIZE;
  event_bus_ = std::make_unique<EventBus>(std::move(config));
}

  bool GlobalEventBus::Publish(Event* event, int32_t timeout_ms ) {
    return event_bus_ ? event_bus_->Publish(event, timeout_ms) : false;
  }

  bool GlobalEventBus::Subscribe(const std::string& event_name,
       std::shared_ptr<EventHandler> handler) {
    return event_bus_ ? event_bus_->Subscribe(event_name, std::move(handler)) : false;
  }

  bool GlobalEventBus::Unsubscribe(const std::string& event_name,
                   std::shared_ptr<EventHandler> handler) {
    return event_bus_ ? event_bus_->Unsubscribe(event_name, std::move(handler)) : false;
  }
}  // namespace esp
