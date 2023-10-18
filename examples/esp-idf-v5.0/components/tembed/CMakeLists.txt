set(srcs "src/tembed.c")

if(CONFIG_TEMBED_INIT_LCD)
  list(APPEND srcs "src/lcd_st7789.c")
endif()

idf_component_register(SRCS "${srcs}"
  REQUIRED_IDF_TARGETS "esp32s3"
  INCLUDE_DIRS "include"
  REQUIRES driver esp_lcd apa102 button knob)
