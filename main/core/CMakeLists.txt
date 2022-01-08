
add_library(core STATIC
  "led/led_indicator_wrapper.cc"
  "util/delay.cc"
  "util/board_info.cc"
  )

target_link_libraries(core PRIVATE
  idf::indicator
  idf::spi_flash
  )
