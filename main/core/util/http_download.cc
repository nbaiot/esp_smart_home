#include "http_download.h"

#include "esp_err.h"

#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_tls.h"


static const char* TAG = "HTTP";

namespace esp {

void HttpDownload::SkipCertCommonNameCheck(bool skip) {
  skip_cert_common_name_check_ = skip;
}
void HttpDownload::SetCertPem(char* pem) {
  cert_pem_ = pem;
}
void HttpDownload::SetClientPem(char* pem) {
  client_pem_ = pem;
}
void HttpDownload::SetClientKey(char* pem) {
  client_key_pem = pem;
}

bool HttpDownload::DoDownload(const char* url, DownloadCallback callback) {
  esp_http_client_config_t config{};
  config.url = url;
  config.skip_cert_common_name_check = skip_cert_common_name_check_;

  if (cert_pem_) {
    config.cert_pem = cert_pem_;
  }
  if (client_pem_) {
    config.client_cert_pem = client_pem_;
  }
  if (client_key_pem) {
    config.client_key_pem = client_key_pem;
  }

  esp_http_client_handle_t client = esp_http_client_init(&config);
  esp_err_t err;
  if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open HTTP connection:%s", esp_err_to_name(err));
    return false;
  }

  int content_length = esp_http_client_fetch_headers(client);
  if (content_length <= 0) {
    ESP_LOGE(TAG, "Cannot fetch http headers or content_length is 0");
    return false;
  }
  int total_read_len = 0, read_len;
  bool success = true;
  while (total_read_len < content_length) {
    read_len = esp_http_client_read(client, (char*)buffer_.data(),
                                    HTTP_DOWNLOAD_BUFFER_SIZE);
    if (read_len < 0) {
      ESP_LOGE(TAG, "error download data");
      success = false;
      break;
    }
    buffer_[read_len] = 0;
    total_read_len += read_len;
    if (callback && read_len > 0) {
      auto ret = callback(buffer_.data(), read_len,
                          content_length - total_read_len, content_length);
      if (!ret) {
        success = false;
        break;
      }
    }
    ESP_LOGI(TAG, "download data len:%d", read_len);
  }

  esp_http_client_close(client);
  esp_http_client_cleanup(client);
  return success;
}

HttpDownload::HttpDownload() {
  buffer_.resize(HTTP_DOWNLOAD_BUFFER_SIZE + 1);
}

}  // namespace esp
