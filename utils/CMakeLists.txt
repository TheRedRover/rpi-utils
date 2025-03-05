include_directories(
    include
)

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