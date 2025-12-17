#include "config.h"
#include "sensors.h"
#include "wifi.h"

extern "C" void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(START_DELAY));

    init_wifi_sta();
    init_udp_socket();
    init_hall();
    
    xTaskCreate(udp_receive_task, "udp_receive_task", 4096, NULL, 5, NULL);
    xTaskCreate(udp_send_ack_task, "udp_send_ack_task", 2048, NULL, 5, NULL);

    ESP_LOGI("PYLON", "sistema iniciado (modo UDP)");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(START_DELAY));
    }
}
