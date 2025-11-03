# ESP32/ESP-IDF configuration for libiohub

include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)

# Get ESP32 sources
get_iohub_sources("${CMAKE_CURRENT_LIST_DIR}/.." "esp32" IOHUB_SOURCES)

# Register ESP-IDF component
idf_component_register(
    SRCS ${IOHUB_SOURCES} 
    INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../include" 
    REQUIRES driver esp_common esp_timer
)

# Configure target
configure_iohub_target(${COMPONENT_LIB} "esp32")
target_compile_definitions(${COMPONENT_LIB} INTERFACE ESP_PLATFORM=1)

