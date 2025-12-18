#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <driver/gpio.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_timer.h>
#include <esp_littlefs.h>
#include <string.h>
#include <lwip/sockets.h>
#include <lwip/err.h>
#include <lwip/sys.h>

#define HALL_SENSOR_GPIO 23

#define START_DELAY 1000
#define ISR_DEBOUNCE_MS 10
#define F2A_TARGET_LAPS 9
#define F2A_CIRCUMFERENCE 111.149f // 2 * pi * 17.69
#define VFS_TARGET_LAPS 10
#define VFS_CIRCUMFERENCE 100.028f // 2 * pi * 15.92
#define MIN_LAP_TIME_MS 1000
#define ACK_FREQ 3000
#define UDP_PORT 3333
#define WAIT_LAPS 2

int lap_count = -1 - WAIT_LAPS;
static int udp_socket = -1;
bool system_active = false;
int target_laps = F2A_TARGET_LAPS;
float target_circumference = F2A_CIRCUMFERENCE;
char laptop_ip[16] = {0};

wifi_ap_record_t ap_info;
struct sockaddr_in dest_addr;
QueueHandle_t hall_event_queue;
