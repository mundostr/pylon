#pragma once

#include "config.h"

void espnow_send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        ESP_LOGE("ESPNOW", "error al enviar mensaje");
    }
}

void espnow_recv_cb(const esp_now_recv_info_t *rx_info, const uint8_t *data, int len) {
    espnow_control_msg_t *msg = (espnow_control_msg_t *)data;

    if (msg->command == ESPNOW_CMD_ACTIVATE) {
        if (!system_active) {
            lap_count = -3;
            system_active = true;
            
            ESP_LOGI("CONTROL", "sistema activado");
        }
    } else if (msg->command == ESPNOW_CMD_DEACTIVATE) {
        lap_count = -1;
        system_active = false;
        
        ESP_LOGI("CONTROL", "sistema desactivado");
    } else if (msg->command == ESPNOW_CMD_CHANGE_F2A) {
        if (!system_active) {
            target_laps = F2A_TARGET_LAPS;
            target_circumference = F2A_CIRCUMFERENCE;
            
            ESP_LOGI("CONTROL", "cambiado a F2A");
        }
    } else if (msg->command == ESPNOW_CMD_CHANGE_VFS) {
        if (!system_active) {
            target_laps = VFS_TARGET_LAPS;
            target_circumference = VFS_CIRCUMFERENCE;
            
            ESP_LOGI("CONTROL", "cambiado a VFS");
        }
    }
}

void espnow_status_update_task(void* arg) {
    for(;;) {
        espnow_control_msg_t ack_msg = {
            .command = ESPNOW_CMD_ACK
        };
        esp_err_t result = esp_now_send(gateway_mac, (uint8_t *)&ack_msg, sizeof(ack_msg));
        if (result != ESP_OK) ESP_LOGE("ESPNOW", "error al enviar ACK al gateway");

        vTaskDelay(pdMS_TO_TICKS(ESPNOW_STUPDATE_FREQ));
    }
}

void init_espnow() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

    esp_now_peer_info_t peer_info = {};
    memcpy(peer_info.peer_addr, gateway_mac, 6);
    peer_info.channel = 1;
    peer_info.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer_info));

    xTaskCreate(espnow_status_update_task, "espnow_status_update_task", 1024, NULL, 10, NULL);
}
