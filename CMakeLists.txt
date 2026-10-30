cmake_minimum_required(VERSION 3.20)
project(Fissio C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Standalone Asio (No Boost binary dependencies required)
add_definitions(-DASIO_STANDALONE)
if (MSVC)
    add_compile_options(/utf-8)
endif()
if (WIN32)
    add_definitions(-D_WIN32_WINNT=0x0A00)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/boringssl/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/wintun/wintun/include
)

# BoringSSL build configuration
set(OPENSSL_NO_ASM ON CACHE BOOL "Disable assembly in BoringSSL" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
add_subdirectory(third_party/boringssl EXCLUDE_FROM_ALL)

# Common core sources
set(FISSIO_CORE_SOURCES
    src/socks5.cpp
    src/doh.cpp
    src/tls_fragment.cpp
    src/proxy_session.cpp
    src/system_proxy.cpp
    src/proxy_controller.cpp
    src/wireguard_config.cpp
    src/wireguard_tunnel.cpp
    src/wireguard_routing.cpp
    src/advanced_config.cpp
    src/i18n.cpp
)

# 1. CLI Executable: fissio.exe
set(FISSIO_CLI_SOURCES src/main.cpp ${FISSIO_CORE_SOURCES})
if (WIN32)
    list(APPEND FISSIO_CLI_SOURCES src/fissio.rc)
endif()
add_executable(fissio ${FISSIO_CLI_SOURCES})
target_link_libraries(fissio PRIVATE ssl crypto)
if (WIN32)
    target_link_libraries(fissio PRIVATE ws2_32 wsock32 wininet iphlpapi)
else()
    target_link_libraries(fissio PRIVATE pthread)
endif()
target_compile_definitions(fissio PRIVATE OPENSSL_IS_BORINGSSL)
if (MSVC)
    target_link_options(fissio PRIVATE "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'")
endif()

# 2. Windows Native GUI Executable: fissio-gui.exe
if (WIN32)
    add_executable(fissio-gui WIN32 src/gui_main.cpp ${FISSIO_CORE_SOURCES} src/fissio_gui.rc)
    target_link_libraries(fissio-gui PRIVATE
        ssl crypto
        ws2_32 wsock32 wininet iphlpapi
        comctl32 comdlg32 shell32 user32 gdi32 advapi32 dwmapi uxtheme
    )
    target_compile_definitions(fissio-gui PRIVATE OPENSSL_IS_BORINGSSL)
    if (MSVC)
        target_link_options(fissio-gui PRIVATE "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'")
    endif()
endif()
