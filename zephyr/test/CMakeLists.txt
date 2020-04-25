
cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(uhap)

FILE(GLOB app_src
  *.cpp
  ../common/*.cpp
  ../../Hap/*.cpp
  ../../Crypto/*.cpp
)

FILE(GLOB crypto_test_src
  ../../Crypto/Test/*.cpp
)

target_sources(app PRIVATE
  ${app_src}
  ../../Util/LbPwm.cpp
)

target_sources_ifdef(CONFIG_HAP_CRYPTO_TEST app PRIVATE ${crypto_test_src})

target_include_directories(app PRIVATE 
  .
  ..
  ../..
  ../../zephyr/common
  ../../Hap
  ../../Utils
  $ENV{ZEPHYR_BASE}
  $ENV{GNUARMEMB_TOOLCHAIN_PATH}/arm-none-eabi/include/c++/9.2.1
  $ENV{GNUARMEMB_TOOLCHAIN_PATH}/arm-none-eabi/include/c++/9.2.1/arm-none-eabi
)