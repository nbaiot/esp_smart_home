#pragma once

#include <cstdint>

namespace esp {

class LedIndicator {
 public:
  enum Type {
    FACTORY_RESET, /**< restoring factory settings */
    UPDATING,      /**< updating software */
    CONNECTED,     /**< connected to AP (or Cloud) succeed */
    PROVISIONED,   /**< provision done */
    CONNECTING,    /**< connecting to AP (or Cloud) */
    RECONNECTING,  /**< reconnecting to AP (or Cloud), if lose connection */
    PROVISIONING,  /**< provisioning */
  };
  enum Mode {
    GPIO,
    PWM,
  };

  LedIndicator(int32_t gpio_num, uint8_t gpio_off_level, Mode mode);

  ~LedIndicator();

  bool Start(Type type);

  bool Stop(Type type);

 private:
  int32_t io_num_{-1};
  void* handle_{nullptr};
};

}  // namespace esp
