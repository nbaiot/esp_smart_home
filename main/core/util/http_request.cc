#include "http_request.h"

namespace esp {

HttpRequest::HttpRequest(std::string url, Method method)
    : url_(std::move(url)), method_(method) {}

void HttpRequest::SetHeader(const std::string& header,
                            const std::string& value) {
  headers_[header] = value;
}

void HttpRequest::SetRawHeader(const char* header, const char* value) {
  raw_headers_[header] = value;
}

void HttpRequest::SetUrl(std::string url) {
  url_ = std::move(url);
}

void HttpRequest::SetMethod(Method method) {
  method_ = method;
}

void HttpRequest::SetRequestBody(const uint8_t* data, size_t len) {
  request_data_.assign(data, data + len);
}

void HttpRequest::SetRequestBody(HttpRequest::RequestBody data) {
  request_data_ = std::move(data);
}

std::string HttpRequest::GetHeader(const std::string& name) const {
  auto it = headers_.find(name);
  if (it != headers_.end()) {
    return it->second;
  }
  return "";
}

HttpRequest::Headers& HttpRequest::GetHeaders() {
  return headers_;
}

HttpRequest::RawHeaders& HttpRequest::GetRawHeaders() {
  return raw_headers_;
}

std::string& HttpRequest::GetUrl() {
  return url_;
}

HttpRequest::Method HttpRequest::GetMethod() const {
  return method_;
}

void HttpRequest::ReleaseGetRequestBody(RequestBody& data) {
  data.swap(request_data_);
}

void HttpRequest::SetRawRequestBody(char* data) {
  raw_request_data_ = data;
}

char* HttpRequest::RawRequestBody() {
  return raw_request_data_;
}

}  // namespace esp
