cmake_minimum_required(VERSION 3.16)
project(Trabalho1_JSON_MIPS CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Pega todos os .cpp de src/
file(GLOB SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
add_executable(trabalho1 ${SRC_FILES})

target_include_directories(trabalho1 PRIVATE src)

# nlohmann/json via apt (nlohmann-json3-dev) ou fallback para external/
find_package(nlohmann_json QUIET)
if (nlohmann_json_FOUND)
  target_link_libraries(trabalho1 PRIVATE nlohmann_json::nlohmann_json)
else()
  message(STATUS "nlohmann_json não encontrado — usando include manual (external/)")
  target_include_directories(trabalho1 PRIVATE external)
endif()

# Copiar tasks/ para a pasta do binário após o build
add_custom_command(TARGET trabalho1 POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/tasks
          $<TARGET_FILE_DIR:trabalho1>/tasks
)
