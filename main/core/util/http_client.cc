
#include "http_client.h"

#include <stdlib.h>
#include <string.h>
#include <cstdint>
#include <cstring>

#include "esp_err.h"

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_log.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

static const char* TAG = "HTTP";

namespace esp {

static esp_err_t HttpEventHandler(esp_http_client_event_t* evt) {
  auto response = (HttpResponse*)evt->user_data;
  switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
      ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      break;
    case HTTP_EVENT_ON_HEADER:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
      // ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s",
      // evt->header_key, evt->header_value);
      response->AddHeader(evt->header_key, evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA");
      ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      if (!esp_http_client_is_chunked_response(evt->client)) {
        response->AppendResponseData((const char*)evt->data, evt->data_len);
      } else {
        // LOGT(TAG, ERROR) << "response chunk data, not support!!!" <<
        // evt->data_len;
        response->AppendResponseData((const char*)evt->data, evt->data_len);
      }

      break;
    case HTTP_EVENT_ON_FINISH:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
      break;
    case HTTP_EVENT_DISCONNECTED:
      if (response->IsAlreadyPrcessDisconnected()) {
        break;
      }
      response->SetProcessDisconnected();
      ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
      int mbedtls_err = 0;
      esp_err_t err = esp_tls_get_and_clear_last_error(
          (esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
      if (err != 0) {
        ESP_LOGE(TAG, "Last esp error code: 0x%x", err);
        ESP_LOGE(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
      }
      break;
  }
  return ESP_OK;
}

class EspHttpClient {
 public:
  explicit EspHttpClient(const esp_http_client_config_t* config) {
    client_ = esp_http_client_init(config);
  }
  bool Valid() { return client_ != nullptr; }
  ~EspHttpClient() { esp_http_client_cleanup(client_); }

  esp_http_client_handle_t Client() { return client_; }

 private:
  esp_http_client_handle_t client_;
};

void HttpClient::SkipCertCommonNameCheck(bool skip) {
  skip_cert_common_name_check_ = skip;
}
void HttpClient::SetCertPem(char* pem) { cert_pem_ = pem; }
void HttpClient::SetClientPem(char* pem) { client_pem_ = pem; }
void HttpClient::SetClientKey(char* pem) { client_key_pem = pem; }

std::shared_ptr<HttpResponse> HttpClient::DoRequest(
    const std::shared_ptr<HttpRequest>& request, int32_t timeoutMS) {
  std::shared_ptr<HttpResponse> response = std::make_shared<HttpResponse>();
  auto url = request->GetUrl();
  esp_http_client_config_t config{};
  config.url = url.c_str();
  config.event_handler = HttpEventHandler;
  config.user_data = (void*)response.get();
  config.skip_cert_common_name_check = skip_cert_common_name_check_;
  if (tx_size_ > 0) {
    // config.buffer_size_tx = 2048;  // tx_size_;
    config.buffer_size_tx = tx_size_;
  }
  if (rx_size_ > 0) {
    // config.buffer_size = 4096;  // rx_size_;
    config.buffer_size = rx_size_;
  }
  if (timeoutMS > 0) {
    config.timeout_ms = timeoutMS;
  }
  if (cert_pem_) {
    config.cert_pem = cert_pem_;
  }
  if (client_pem_) {
    config.client_cert_pem = client_pem_;
  }
  if (client_key_pem) {
    config.client_key_pem = client_key_pem;
  }
  EspHttpClient c(&config);
  if (!c.Valid()) {
    ESP_LOGE(TAG, "esp http client init failed, check request params");
    return response;
  }

  // set method
  esp_http_client_set_method(c.Client(),
                             (esp_http_client_method_t)request->GetMethod());
  // set headers
  auto headers = request->GetHeaders();
  for (const auto& header : headers) {
    esp_http_client_set_header(c.Client(), header.first.c_str(),
                               header.second.c_str());
  }
  auto raw_headers = request->GetRawHeaders();
  for (const auto& header : raw_headers) {
    esp_http_client_set_header(c.Client(), header.first, header.second);
  }

  // set body
  HttpRequest::RequestBody body;
  request->ReleaseGetRequestBody(body);
  if (!body.empty()) {
    esp_http_client_set_post_field(c.Client(), (const char*)body.data(),
                                   body.size());
  } else {
    auto raw_request_data = request->RawRequestBody();
    if (raw_request_data) {
      esp_http_client_set_post_field(c.Client(), raw_request_data,
                                     std::strlen(raw_request_data));
    }
  }

  esp_err_t err = esp_http_client_perform(c.Client());
  if (err == ESP_OK) {
    response->SetStatusCode(esp_http_client_get_status_code(c.Client()));
  } else {
    response->SetError(esp_err_to_name(err));
  }
  return response;
}

void HttpClient::SetTxBufferSize(uint32_t tx_size) { tx_size_ = tx_size; }
void HttpClient::SetRxBufferSize(uint32_t rx_size) { rx_size_ = rx_size; }
}  // namespace esp
