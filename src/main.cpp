#include "config.h"
#include "sensors.h"
#include "wifi.h"

extern "C" void app_main(void) {
    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
    if (wakeup_cause == ESP_SLEEP_WAKEUP_EXT0) ESP_LOGI("PYLON", "Reiniciado luego de inactividad");

    vTaskDelay(pdMS_TO_TICKS(START_DELAY));

    init_wifi_sta();
    init_udp_socket();
    init_hall();
    init_inactivity_timer();
    
    xTaskCreate(udp_receive_task, "udp_receive_task", 4096, NULL, 5, NULL);
    xTaskCreate(udp_send_ack_task, "udp_send_ack_task", 2048, NULL, 5, NULL);

    ESP_LOGI("PYLON", "sistema iniciado (modo UDP)");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(START_DELAY));
    }
}
