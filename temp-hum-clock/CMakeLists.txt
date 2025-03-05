include_directories(
    include
)

add_executable(
    temp-hum-clock
    src/main.cpp
)

target_link_libraries(
    temp-hum-clock
    libsensors
    ${PIGPIO_LIBRARY}
)
