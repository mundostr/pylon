// Alternative code with ESPNOW gateway usage

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <string.h>

#define HALL_SENSOR_GPIO 23

#define START_DELAY 1000
#define ISR_DEBOUNCE_MS 10
#define F2A_TARGET_LAPS 9
#define F2A_CIRCUMFERENCE 111.149f // 2 * pi * 17.69
#define VFS_TARGET_LAPS 10
#define VFS_CIRCUMFERENCE 100.028f // 2 * pi * 15.92
#define MIN_LAP_TIME_MS 1000
#define ESPNOW_CHANNEL 1
#define ESPNOW_STUPDATE_FREQ 3000

int lap_count = -1;
bool system_active = false;
int target_laps = F2A_TARGET_LAPS;
float target_circumference = F2A_CIRCUMFERENCE;
uint8_t gateway_mac[] = {0xc8, 0xf0, 0x9e, 0xf5, 0x0e, 0x58};

QueueHandle_t hall_event_queue;

typedef enum {
    ESPNOW_CMD_ACK,
    ESPNOW_CMD_ACTIVATE,
    ESPNOW_CMD_DEACTIVATE,
    ESPNOW_CMD_CHANGE_F2A,
    ESPNOW_CMD_CHANGE_VFS,
} espnow_command_t;

typedef struct {
    espnow_command_t command;
} espnow_control_msg_t;

typedef struct {
    int lap_number;
    float elapsed_time;
    float speed;
    bool is_final;
} espnow_lap_msg_t;
