#pragma once

#include <memory>

#include "http_request.h"
#include "http_response.h"

namespace esp {

class HttpClient {
 public:
  void SkipCertCommonNameCheck(bool skip);

  // only save pem pointer,not copy
  void SetCertPem(char* pem);
  void SetClientPem(char* pem);
  void SetClientKey(char* pem);

  void SetTxBufferSize(uint32_t tx_size);
  void SetRxBufferSize(uint32_t rx_size);

  std::shared_ptr<HttpResponse> DoRequest(
      const std::shared_ptr<HttpRequest>& request, int32_t timeoutMS);

 private:
  bool skip_cert_common_name_check_{true};
  char* cert_pem_{nullptr};
  char* client_pem_{nullptr};
  char* client_key_pem{nullptr};
  uint32_t tx_size_{0};
  uint32_t rx_size_{0};
};

}  // namespace esp
