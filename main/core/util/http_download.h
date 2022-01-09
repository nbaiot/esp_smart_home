#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace esp {

#define HTTP_DOWNLOAD_BUFFER_SIZE 1024

class HttpDownload {
 public:
  using DownloadCallback = std::function<bool(
      const uint8_t* data, size_t len, size_t left_size, size_t total_size)>;

  HttpDownload();

  void SkipCertCommonNameCheck(bool skip);

  // only save pem pointer,not copy
  void SetCertPem(char* pem);
  void SetClientPem(char* pem);
  void SetClientKey(char* pem);

  bool DoDownload(const char* url, DownloadCallback callback);

 private:
  bool skip_cert_common_name_check_{true};
  char* cert_pem_{nullptr};
  char* client_pem_{nullptr};
  char* client_key_pem{nullptr};
  std::vector<uint8_t> buffer_;
};

}  // namespace esp
