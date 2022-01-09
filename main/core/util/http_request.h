#pragma once

#include <map>
#include <string>
#include <vector>

namespace esp {

class HttpRequest {
 public:
  using Headers = std::map<std::string, std::string>;
  using RawHeaders = std::map<const char*, const char*>;
  using RequestBody = std::vector<uint8_t>;

  enum Method {
    GET = 0,
    POST,
    PUT,
    PATCH,
    DELETE,
    HEAD,
  };

  HttpRequest(std::string url, Method method);

  void SetHeader(const std::string& header, const std::string& value);

  void SetRawHeader(const char* header, const char* value);

  void SetUrl(std::string url);

  void SetMethod(Method method);

  void SetRequestBody(const uint8_t* data, size_t len);

  void SetRequestBody(RequestBody data);

  void SetRawRequestBody(char* data);

  std::string GetHeader(const std::string& name) const;

  Headers& GetHeaders();

  RawHeaders& GetRawHeaders();

  std::string& GetUrl();

  Method GetMethod() const;

  void ReleaseGetRequestBody(RequestBody& data);

  char* RawRequestBody();

 private:
  std::string url_;
  Method method_;
  Headers headers_;
  RawHeaders raw_headers_;
  RequestBody request_data_;
  char* raw_request_data_{nullptr};
};

}  // namespace esp
