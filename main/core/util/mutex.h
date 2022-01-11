#pragma once

#include <cstdint>

namespace esp {

class MutexImpl;

class Mutex {
 public:
  Mutex();
  ~Mutex();
  bool Lock();
  bool Lock(uint32_t timeout_ms);
  bool TryLock();
  bool Unlock();

 private:
  MutexImpl* impl_{nullptr};
};

class MutexLock {
  public:
    MutexLock(Mutex* mutex, uint32_t timeout_ms);
    ~MutexLock();
  private:
    Mutex* mutex_{nullptr};
};

}  // namespace esp
