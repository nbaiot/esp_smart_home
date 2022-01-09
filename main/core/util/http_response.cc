#include "http_response.h"

#include <cstring>
#include <string>

namespace esp {

#define DEFAULT_DATA_SIZE 2048

void HttpResponse::SetStatusCode(int32_t code) { code_ = code; }

int32_t HttpResponse::GetStatusCode() const { return code_; }

void HttpResponse::SetResponseDataCapacity(size_t size) {
  response_data_.reserve(size);
}

void HttpResponse::AddHeader(const std::string& header,
                             const std::string& value) {
  headers_[header] = value;
}

void HttpResponse::SetResponseData(ResponseData data) {
  has_response_ = true;
  response_data_.swap(data);
  current_size_ = 0;
}

void HttpResponse::AppendResponseData(const char* data, size_t size) {
  has_response_ = true;
  if (current_size_ + size >= response_data_.size()) {
    size_t new_add_size = DEFAULT_DATA_SIZE;
    if (size >= new_add_size) {
      new_add_size = size + 1;
    }
    response_data_.resize(response_data_.size() + new_add_size, 0);
  }
  std::memcpy(response_data_.data() + current_size_, data, size);
  current_size_ += size;
}

std::string HttpResponse::GetHeader(const std::string& header) const {
  auto it = headers_.find(header);
  if (it != headers_.end()) {
    return it->second;
  }
  return "";
}

void HttpResponse::ReleaseReponseData(ResponseData& data) {
  has_response_ = false;
  data.swap(response_data_);
  data.resize(current_size_);
  current_size_ = 0;
}

char* HttpResponse::RawResponseData() { return response_data_.data(); }

void HttpResponse::SetError(std::string error) { error_ = std::move(error); }

std::string HttpResponse::GetError() const { return error_; }

bool HttpResponse::HasResponseBody() const { return has_response_; }
void HttpResponse::SetProcessDisconnected() {
  already_process_disconnected_ = true;
}

bool HttpResponse::IsAlreadyPrcessDisconnected() {
  return already_process_disconnected_;
}
}  // namespace esp
