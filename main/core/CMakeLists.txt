
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(core STATIC
  "init.cc"
  "util/delay.cc"
  "util/board_info.cc"
  "util/http_client.cc"
  "util/http_request.cc"
  "util/http_response.cc"
  "util/http_download.cc"
  "led/led_indicator_wrapper.cc"
  "manager/wifi_manager.cc"
  "manager/sntp_manager.cc"
  "mqtt/mqtt_client_wrapper.cc"
  )

target_link_libraries(core PRIVATE
  idf::nvs_flash
  idf::spi_flash
  idf::esp_wifi
  idf::esp_http_client
  idf::esp-tls
  idf::mqtt

  idf::indicator
  )
