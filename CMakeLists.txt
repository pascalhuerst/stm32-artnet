cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(stm32_artnet)

SET(app_sources
  src/artnet/dispatcher.c
  src/artnet/fixup.c
  src/artnet/network.c
  src/artnet/receive.c
  src/artnet/artnet.c
  src/artnet/transmit.c
  src/artnet/tod.c
  src/artnet/misc.c
  src/artnet/dmx.c
  src/main.c
)


#FILE(GLOB app_sources src/*.c*)
target_sources(app PRIVATE ${app_sources})
