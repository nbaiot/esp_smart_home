#pragma once

#include <cstdint>

#include <map>
#include <vector>

namespace esp {

class HttpResponse {
 public:
  using Headers = std::map<std::string, std::string>;
  using ResponseData = std::vector<char>;

  void SetStatusCode(int32_t code);

  int32_t GetStatusCode() const;

  void SetResponseDataCapacity(size_t size);

  void AddHeader(const std::string& header, const std::string& value);

  void SetResponseData(ResponseData data);

  void AppendResponseData(const char* data, size_t size);

  std::string GetHeader(const std::string& header) const;

  void ReleaseReponseData(ResponseData& data);

  char* RawResponseData();

  void SetError(std::string error);

  std::string GetError() const;

  bool HasResponseBody() const;

  void SetProcessDisconnected();

  bool IsAlreadyPrcessDisconnected();

 private:
  int32_t code_{0};
  ResponseData response_data_;
  Headers headers_;
  bool has_response_{false};
  std::string error_;
  size_t current_size_{0};
  bool already_process_disconnected_{false};
};

}  // namespace esp
