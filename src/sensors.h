#pragma once

#include "config.h"

static void IRAM_ATTR hall_isr_handler(void* arg) {
    static uint32_t last_isr_time = 0;
    uint32_t now = esp_timer_get_time() / 1000;

    if (now - last_isr_time < ISR_DEBOUNCE_MS) return;
    last_isr_time = now;

    int level = gpio_get_level((gpio_num_t)HALL_SENSOR_GPIO);
    xQueueSendFromISR(hall_event_queue, &level, NULL);
}

void hall_task(void* arg) {
    int level;
    uint32_t total_time_ms = 0;
    uint32_t lap_start_time = 0;
    uint32_t lap_times[target_laps] = {0};

    for(;;) {
        if (xQueueReceive(hall_event_queue, &level, portMAX_DELAY) == pdTRUE) {
            if (level == 1 && system_active) {
                lap_count++;
                
                if (lap_count == 0) {
                    lap_start_time = esp_timer_get_time() / 1000;
                    
                    ESP_LOGI("CONTROL", "inicia crono");
                } else if (lap_count <= target_laps) {
                    uint32_t now = esp_timer_get_time() / 1000;
                    uint32_t elapsed = now - lap_start_time;

                    if (elapsed >= MIN_LAP_TIME_MS) {
                        lap_times[lap_count - 1] = elapsed;
                        lap_start_time = now;
                        float lap_speed_kph = (target_circumference / 1000.0f) / ((elapsed / 1000.0f) / 3600.0f);
                        
                        ESP_LOGI("CONTROL", "vta %d, tpo %.2f, vel %.2f", lap_count, elapsed / 1000.0f, lap_speed_kph);

                        espnow_lap_msg_t lap_msg = {
                            .lap_number = lap_count,
                            .elapsed_time = elapsed / 1000.0f,
                            .speed = lap_speed_kph,
                            .is_final = false
                        };
                        esp_err_t result = esp_now_send(gateway_mac, (uint8_t *)&lap_msg, sizeof(lap_msg));
                        if (result != ESP_OK) ESP_LOGE("ESPNOW", "error al enviar vuelta al gateway");
                    } else {
                        lap_count--;
                    }
                }

                if (lap_count == target_laps) {
                    for (int i = 0; i < target_laps; i++) total_time_ms += lap_times[i];

                    float total_time_sec = total_time_ms / 1000.0f;
                    float avg_speed_kph = (target_laps * target_circumference / 1000.0f) / (total_time_sec / 3600.0f);
                    
                    ESP_LOGI("CONTROL", "final, tpo %.2f, vel %.2f", total_time_sec, avg_speed_kph);
                    ESP_LOGI("CONTROL", "sistema desactivado");

                    espnow_lap_msg_t final_msg = {
                        .lap_number = lap_count,
                        .elapsed_time = total_time_sec,
                        .speed = avg_speed_kph,
                        .is_final = true
                    };
                    esp_err_t result = esp_now_send(gateway_mac, (uint8_t *)&final_msg, sizeof(final_msg));
                    if (result != ESP_OK) ESP_LOGE("ESPNOW", "error al enviar final al gateway");

                    lap_count = -1;
                    total_time_ms = 0;
                    system_active = false;
                }
            }
        }
    }
}

void init_hall() {
    gpio_config_t hall_conf = {
        .pin_bit_mask = (1ULL << HALL_SENSOR_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&hall_conf);
    
    hall_event_queue = xQueueCreate(10, sizeof(int));
    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)HALL_SENSOR_GPIO, hall_isr_handler, NULL);
    
    xTaskCreate(hall_task, "hall_task", 4096, NULL, 10, NULL);
}
