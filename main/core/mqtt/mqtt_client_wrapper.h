#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace esp {

class MqttClient {
 public:
  using OnReadyCallback = std::function<void()>;
  using OnDisconnectCallback = std::function<void()>;
  using OnReceiveMsgCallback =
      std::function<void(const char* topic, const char* data, int32_t len)>;

  MqttClient(const char* broker_url, const char* client_id, bool clean_session, bool auto_reconnect);

  ~MqttClient();

  void SkipCertCommonNameCheck(bool skip);

  void SetCert(const char* cert, size_t cert_len);

  void SetClientCertPem(const char* client_cert_pem, size_t client_cert_pem_len);

  void SetOnReadyCallback(OnReadyCallback callback);
  
  void SetOnReceiveMsgCallback(OnReceiveMsgCallback callback);

  void SetOnDisconnectCallback(OnDisconnectCallback callback);

  bool Start();

  bool Stop();

  bool Publish(const char* topic, const char* data, int32_t len, int32_t qos,
               int32_t retain);

  bool AsyncPublish(const char* topic, const char* data, int32_t len,
                    int32_t qos, int32_t retain);

  bool Subscribe(const char* topic, int32_t qos);

  bool Unsubscribe(const char* topic);

  void OnReady();

  void OnDisconnect();

  void OnReceiveMsg(const char* topic, const char* data, int32_t len);

 private:
  std::string url_;
  std::string client_id_;
  void* config_{nullptr};
  void* handle_{nullptr};
  bool is_start_{false};
  bool is_ready_{false};
  OnReadyCallback ready_callback_;
  OnReceiveMsgCallback receive_msg_callback_;
  OnDisconnectCallback disconnect_callback_;
};

}  // namespace esp
