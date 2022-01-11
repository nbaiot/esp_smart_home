
#pragma once

#include <cstdint>

namespace esp {

class Event {
 public:
  virtual ~Event() = default;

  // must ensure global unique
  virtual char* Name() = 0;

  virtual int8_t Priority() = 0;
};

class EventHandler {
 public:
  virtual ~EventHandler() = default;

  virtual void Process(Event* event) = 0;
};

}  // namespace esp
