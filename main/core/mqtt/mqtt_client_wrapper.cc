
#include "mqtt_client_wrapper.h"

#include <stdlib.h>

#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

static const char* TAG = "mqtt_client";

namespace esp {

static void log_error_if_nonzero(const char* message, int error_code) {
  if (error_code != 0) {
    ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
  }
}

static void mqtt_event_handler(void* handler_args, esp_event_base_t base,
                               int32_t event_id, void* event_data) {
  ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base,
           event_id);
  auto mqtt_client = (MqttClient*)handler_args;
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
      ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
      mqtt_client->OnReady();
      break;
    case MQTT_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
      mqtt_client->OnDisconnect();
      break;

    case MQTT_EVENT_SUBSCRIBED:
      ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
      break;
    case MQTT_EVENT_PUBLISHED:
      ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
      break;
    case MQTT_EVENT_DATA:
      ESP_LOGI(TAG, "MQTT_EVENT_DATA");
      mqtt_client->OnReceiveMsg(event->topic, event->topic_len, event->data,
                                event->data_len);
      break;
    case MQTT_EVENT_ERROR:
      ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
      if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
        log_error_if_nonzero("reported from esp-tls",
                             event->error_handle->esp_tls_last_esp_err);
        log_error_if_nonzero("reported from tls stack",
                             event->error_handle->esp_tls_stack_err);
        log_error_if_nonzero("captured as transport's socket errno",
                             event->error_handle->esp_transport_sock_errno);
        ESP_LOGI(TAG, "Last errno string (%s)",
                 strerror(event->error_handle->esp_transport_sock_errno));
      }
      break;
    default:
      ESP_LOGI(TAG, "Other event id:%d", event->event_id);
      break;
  }
}

MqttClient::MqttClient(const char* broker_url, const char* client_id,
                       bool clean_session, bool auto_reconnect)
    : url_(broker_url), client_id_(client_id) {
  auto config =
      (esp_mqtt_client_config_t*)malloc(sizeof(esp_mqtt_client_config_t));
  memset(config, 0, sizeof(esp_mqtt_client_config_t));
  config->uri = url_.c_str();
  config->client_id = client_id_.c_str();
  config->disable_clean_session = clean_session ? 0 : 1;
  config->disable_auto_reconnect = !auto_reconnect;
  config_ = (void*)config;
}

MqttClient::~MqttClient() {
  if (config_) {
    free(config_);
    config_ = nullptr;
  }
  Stop();
}

void MqttClient::SkipCertCommonNameCheck(bool skip) {
  if (config_) {
    ((esp_mqtt_client_config_t*)config_)->skip_cert_common_name_check = skip;
  }
}

void MqttClient::SetCert(const char* cert, size_t cert_len) {
  if (config_) {
    ((esp_mqtt_client_config_t*)config_)->cert_pem = cert;
    ((esp_mqtt_client_config_t*)config_)->cert_len = cert_len;
  }
}

void MqttClient::SetClientCertPem(const char* client_cert_pem,
                                  size_t client_cert_pem_len) {
  if (config_) {
    ((esp_mqtt_client_config_t*)config_)->client_cert_pem = client_cert_pem;
    ((esp_mqtt_client_config_t*)config_)->client_cert_len = client_cert_pem_len;
  }
}

void MqttClient::SetOnReadyCallback(OnReadyCallback callback) {
  ready_callback_ = std::move(callback);
}

void MqttClient::SetOnReceiveMsgCallback(OnReceiveMsgCallback callback) {
  receive_msg_callback_ = std::move(callback);
}

void MqttClient::SetOnDisconnectCallback(OnDisconnectCallback callback) {
  disconnect_callback_ = std::move(callback);
}

void MqttClient::OnReady() {
  is_ready_ = true;
  if (ready_callback_) {
    ready_callback_();
  }
}

void MqttClient::OnDisconnect() {
  is_ready_ = false;
  if (disconnect_callback_) {
    disconnect_callback_();
  }
}

void MqttClient ::OnReceiveMsg(const char* topic, int32_t topic_len,
                               const char* data, int32_t len) {
  if (receive_msg_callback_) {
    receive_msg_callback_(std::string(topic, topic_len), data, len);
  }
}

bool MqttClient::Start() {
  if (is_start_) {
    ESP_LOGI(TAG, "already started");
    return false;
  }
  handle_ = esp_mqtt_client_init((const esp_mqtt_client_config_t*)config_);
  if (!handle_) {
    ESP_LOGE(TAG, "init mqtt client failed");
    return false;
  }
  auto ret = esp_mqtt_client_register_event(
      (esp_mqtt_client_handle_t)handle_, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID,
      mqtt_event_handler, this);
  if (ret != ESP_OK) {
    esp_mqtt_client_destroy((esp_mqtt_client_handle_t)handle_);
    handle_ = nullptr;
    ESP_LOGE(TAG, "mqtt_client_register_event failed:%s", esp_err_to_name(ret));
    return false;
  }
  ret = esp_mqtt_client_start((esp_mqtt_client_handle_t)handle_);
  if (ret != ESP_OK) {
    esp_mqtt_client_destroy((esp_mqtt_client_handle_t)handle_);
    handle_ = nullptr;
    ESP_LOGE(TAG, "start failed:%s", esp_err_to_name(ret));
    return false;
  }
  is_start_ = true;
  return true;
}

bool MqttClient::Stop() {
  if (!is_start_) {
    return true;
  }
  // TODO: fixme, how to unregister event?
  esp_mqtt_client_stop((esp_mqtt_client_handle_t)handle_);
  esp_mqtt_client_destroy((esp_mqtt_client_handle_t)handle_);
  handle_ = nullptr;
  is_start_ = false;
  return true;
}

bool MqttClient::Publish(const char* topic, const char* data, int32_t len,
                         int32_t qos, int32_t retain) {
  if (!is_ready_) {
    ESP_LOGW(TAG, "not ready, so cannot publish");
    return false;
  }
  auto ret = esp_mqtt_client_publish((esp_mqtt_client_handle_t)handle_, topic,
                                     data, len, qos, retain);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "publish failed:%s", esp_err_to_name(ret));
    return false;
  }
  return true;
}

bool MqttClient::AsyncPublish(const char* topic, const char* data, int32_t len,
                              int32_t qos, int32_t retain) {
  if (!is_ready_) {
    ESP_LOGW(TAG, "not ready, so cannot publish");
    return false;
  }
  auto ret = esp_mqtt_client_enqueue((esp_mqtt_client_handle_t)handle_, topic,
                                     data, len, qos, retain, true);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "publish failed:%s", esp_err_to_name(ret));
    return false;
  }
  return true;
}

bool MqttClient::Subscribe(const char* topic, int32_t qos) {
  if (!is_ready_) {
    ESP_LOGW(TAG, "not ready, so cannot publish");
    return false;
  }
  auto ret =
      esp_mqtt_client_subscribe((esp_mqtt_client_handle_t)handle_, topic, qos);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "subscribe failed:%s", esp_err_to_name(ret));
    return false;
  }
  return true;
}

bool MqttClient::Unsubscribe(const char* topic) {
  if (!is_ready_) {
    ESP_LOGW(TAG, "not ready, so cannot publish");
    return false;
  }
  auto ret =
      esp_mqtt_client_unsubscribe((esp_mqtt_client_handle_t)handle_, topic);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "unsubscribe failed:%s", esp_err_to_name(ret));
    return false;
  }
  return true;
}

}  // namespace esp
