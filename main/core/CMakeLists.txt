
add_library(core STATIC
  "led/led_indicator_wrapper.cc"
  "util/delay.cc"
  )

target_link_libraries(core PRIVATE idf::indicator)
