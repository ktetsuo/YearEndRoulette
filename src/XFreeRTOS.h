#ifndef __XFREE_RTOS_H__
#define __XFREE_RTOS_H__

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#elif defined(ARDUINO_ARCH_RP2040)
#include <FreeRTOS.h>
#include <event_groups.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>
#else
#error "Unsupported architecture for FreeRTOS headers"
#endif

#endif // __XFREE_RTOS_H__
