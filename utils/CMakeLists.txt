include_directories(
    include
)

set(CMAKE_REQUIRED_LIBRARIES "-L${atomic_lib_dir} -latomic")

add_library(
    liblogger
    STATIC
    src/logger.cpp
)

# Include headers
target_include_directories(
    liblogger
    PUBLIC
    include
)