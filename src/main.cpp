#include "config.h"
#include "espnow.h"
#include "sensors.h"

extern "C" void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(START_DELAY));

    init_espnow();
    init_hall();

    ESP_LOGI("PYLON", "sistema iniciado");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(START_DELAY));
    }
}
