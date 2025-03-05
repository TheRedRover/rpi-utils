find_library(PIGPIO_LIBRARY NAMES pigpio)

if(NOT PIGPIO_LIBRARY)
  message(FATAL_ERROR "Could not find the pigpio library")
endif()

include_directories(
    include
)

add_library(
    libsensors
    STATIC
    src/DHT11.cpp
    src/Display4Digits.cpp
)

# Include headers
target_include_directories(
    libsensors
    PUBLIC
    include
)

target_link_libraries(
    libsensors
    liblogger
)