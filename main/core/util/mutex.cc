
#include "mutex.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

namespace esp {

class MutexImpl {
 public:
  MutexImpl() { mutex_ = xSemaphoreCreateMutex(); }

  ~MutexImpl() { vSemaphoreDelete(mutex_); }

  bool Lock() { return xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE; }

  bool Lock(uint32_t ms) {
    return xSemaphoreTake(mutex_, pdMS_TO_TICKS(ms)) == pdTRUE;
  }

  bool TryLock() { return xSemaphoreTake(mutex_, 0) == pdTRUE; }

  bool Unlock() { return xSemaphoreGive(mutex_) == pdTRUE; }

 private:
  SemaphoreHandle_t mutex_;
};

Mutex::Mutex() { impl_ = new MutexImpl(); }
Mutex::~Mutex() {
  if (impl_) {
    delete impl_;
  }
}
bool Mutex::Lock() { return impl_ ? impl_->Lock() : false; }
bool Mutex::Lock(uint32_t ms) { return impl_ ? impl_->Lock(ms) : false; }
bool Mutex::TryLock() { return impl_ ? impl_->TryLock() : false; }
bool Mutex::Unlock() { return impl_ ? impl_->Unlock() : false; }

MutexLock::MutexLock(Mutex* mutex, uint32_t timeout_ms) {
  mutex_ = mutex;
  mutex_->Lock(timeout_ms);
}

MutexLock::~MutexLock() { mutex_->Unlock(); }

}  // namespace esp
