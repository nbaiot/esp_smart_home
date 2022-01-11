
#include "event_bus.h"

#include <atomic>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <vector>

#include "esp_event.h"
#include "esp_log.h"
#include "util/mutex.h"
#include "util/delay.h"

namespace esp {

#define HANDLER_MUTEX_TIMEOUT_MS 1000
#define EVENT_MUTEX_TIMEOUT_MS 1000

static const char* TAG = "event_bus";

class EventBusImpl {
 public:
  using Handlers = std::set<std::shared_ptr<EventHandler>>;
  explicit EventBusImpl(EventBus::Config config);

  ~EventBusImpl();

  bool Publish(Event* event, int32_t timeout_ms);

  bool Subscribe(const std::string& event_name,
                 std::shared_ptr<EventHandler> handler);

  bool Unsubscribe(const std::string& event_name,
                   std::shared_ptr<EventHandler> handler);

  void EventLoop();

  void Wakeup() { xSemaphoreGive(wakeup_sem_); }

  struct EventCmp {
    bool operator()(Event* e1, Event* e2) {
      return e1->Priority() > e2->Priority();
    }
  };

 private:
  TaskHandle_t task_handler_;
  size_t event_queue_capacity_;
  Mutex event_mutex_;
  std::priority_queue<Event*, std::vector<Event*>, EventCmp> event_queue_;
  SemaphoreHandle_t wakeup_sem_;

  Mutex handler_mutex_;
  std::map<std::string, std::shared_ptr<Handlers>> event_handlers_;
  std::atomic<bool> exit_{false};
  SemaphoreHandle_t exit_sem_;
};

static void EventBusLoop(void* args) {
  auto self = (EventBusImpl*)args;
  if (self) {
    self->EventLoop();
  }
}

EventBusImpl::EventBusImpl(EventBus::Config config) {
  wakeup_sem_ = xSemaphoreCreateBinary();
  exit_sem_ = xSemaphoreCreateBinary();
  event_queue_capacity_ = config.max_event_cout;
  auto ret = xTaskCreatePinnedToCore(EventBusLoop, 
      config.task_name.empty() ? "event_bus": config.task_name.c_str(), config.task_stack_size,
                                     (void*)this, config.task_priority, &task_handler_,
                                     config.task_cpu_core_id);
  if (ret != pdPASS) {
    ESP_LOGE(TAG, "start event bus loop failed");
  }
}

EventBusImpl::~EventBusImpl() {
  exit_ = true;
  ESP_LOGI(TAG, "start to wait event bus loop exit ...");
  xSemaphoreTake(exit_sem_, portMAX_DELAY);
  Wakeup();
  vSemaphoreDelete(exit_sem_);
  vSemaphoreDelete(wakeup_sem_);
  while(event_queue_.empty()) {
    auto ev = event_queue_.top();
    if (ev) {
      delete ev;
    }
    event_queue_.pop();
  }
}

bool EventBusImpl::Publish(Event* event, int32_t timeout_ms) {
  if (!event || exit_) {
    return false;
  }
  bool success = false;
  if (event_mutex_.Lock(EVENT_MUTEX_TIMEOUT_MS)) {
    if (event_queue_.size() < event_queue_capacity_) {
      event_queue_.push(event);
      success = true;
    } else {
      ESP_LOGW(TAG, "cannot publish event, event bus is full");
    }
    event_mutex_.Unlock();
  }
  Wakeup();
  return success;
}

bool EventBusImpl::Subscribe(const std::string& event_name,
                             std::shared_ptr<EventHandler> handler) {
  if (!handler || event_name.empty()) {
    return false;
  }
  if (handler_mutex_.Lock(HANDLER_MUTEX_TIMEOUT_MS)) {
    if (event_handlers_.find(event_name) == event_handlers_.end()) {
      event_handlers_[event_name] = std::make_shared<Handlers>();
    }
    auto handles = event_handlers_[event_name];
    if (handles->find(handler) == handles->end()) {
      handles->insert(handler);
    } else {
      ESP_LOGW(TAG, "already subscribe event:%s", event_name.c_str());
    }
    handler_mutex_.Unlock();
    return true;
  }
  ESP_LOGE(TAG, "subscribe event:%s failed, get mutex failed",
           event_name.c_str());
  return false;
}

bool EventBusImpl::Unsubscribe(const std::string& event_name,
                               std::shared_ptr<EventHandler> handler) {
  if (!handler || event_name.empty()) {
    return false;
  }
  if (handler_mutex_.Lock(HANDLER_MUTEX_TIMEOUT_MS)) {
    if (event_handlers_.find(event_name) != event_handlers_.end()) {
      auto handles = event_handlers_[event_name];
      handles->erase(handler);
    }
    handler_mutex_.Unlock();
    return true;
  }
  ESP_LOGE(TAG, "unsubscribe event:%s failed, get mutex failed",
           event_name.c_str());
  return false;
}

void EventBusImpl::EventLoop() {
  for (;;) {
    Event* event = nullptr;
    bool need_wait = false;
    if (event_mutex_.Lock(EVENT_MUTEX_TIMEOUT_MS)) {
      if (!event_queue_.empty()) {
        event = event_queue_.top();
        event_queue_.pop();
      } else {
        need_wait = true;
      }
      event_mutex_.Unlock();
    } else {
      ESP_LOGW(TAG, "event loop get event queue mutex failed");
    }
    if (event) {
      Handlers handlers;
      if (handler_mutex_.Lock(HANDLER_MUTEX_TIMEOUT_MS)) {
        if (event_handlers_.find(event->Name()) != event_handlers_.end()) {
          handlers = *event_handlers_[event->Name()];
        }
        handler_mutex_.Unlock();
      } else {
        ESP_LOGE(TAG, "drop event:%s, get mutex failed", event->Name());
      }
      for (const auto& handler : handlers) {
        handler->Process(event);
      }
      delete event;
    }
    if (need_wait) {
     xSemaphoreTake(exit_sem_, pdMS_TO_TICKS(50));     
    }
    if (exit_) {
      break;
    }
  }
  xSemaphoreGive(exit_sem_);
  vTaskDelete(NULL);
}

EventBus::EventBus(Config config) {
  impl_ = new EventBusImpl(std::move(config));
}

EventBus::~EventBus() {
  if (impl_) {
    delete impl_;
  }
}

bool EventBus::Publish(Event* event, int32_t timeout_ms) {
  return impl_ ? impl_->Publish(event, timeout_ms) : false;
}

bool EventBus::Subscribe(const std::string& event_name,
                         std::shared_ptr<EventHandler> handler) {
  return impl_ ? impl_->Subscribe(event_name, std::move(handler)) : false;
}

bool EventBus::Unsubscribe(const std::string& event_name,
                           std::shared_ptr<EventHandler> handler) {
  return impl_ ? impl_->Unsubscribe(event_name, std::move(handler)) : false;
}

}  // namespace esp
