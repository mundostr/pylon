#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
// to get rid of annoying msgs about missing inits in some structs

#include "config.h"

static EventGroupHandle_t wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            ESP_LOGI("WIFI", "Inicio STA, conectando...");
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGW("WIFI", "STA desconectado, reintentando...");
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            // ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            // ESP_LOGI("WIFI", "IP: " IPSTR, IP2STR(&event->ip_info.ip));
            // ESP_LOGI("WIFI", "GW: " IPSTR, IP2STR(&event->ip_info.gw));
            
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}

void init_wifi_sta() {
    wifi_event_group = xEventGroupCreate();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold = {
                .authmode = WIFI_AUTH_WPA2_PSK,
            },
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    ESP_LOGI("WIFI", "inicializacion STA finalizada");

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI("WIFI", "conectado a %s", WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE("WIFI", "error al conectar a %s", WIFI_SSID);
    } else {
        ESP_LOGE("WIFI", "evento no esperado");
    }

    while (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) vTaskDelay(500 / portTICK_PERIOD_MS);
}

void udp_receive_task(void *pvParameters) {
    char rx_buffer[64];
    struct sockaddr_in source_addr;
    socklen_t socklen = sizeof(source_addr);

    for(;;) {
        int len = recvfrom(udp_socket, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
        if (len > 0) {
            rx_buffer[len] = '\0'; // Null-terminate
            // ESP_LOGI("UDP", "comando %s", rx_buffer);

            if (strcmp(rx_buffer, "act") == 0) {
                system_active = true;
                ESP_LOGI("UDP", "crono activado");
            } else if (strcmp(rx_buffer, "des") == 0) {
                lap_count = -2;
                system_active = false;
                ESP_LOGI("UDP", "crono desactivado");
            } else if (strstr(rx_buffer, "f2a") == rx_buffer) {
                target_laps = F2A_TARGET_LAPS;
                target_circumference = F2A_CIRCUMFERENCE;
                ESP_LOGI("UDP", "modo F2A activado");
            } else if (strstr(rx_buffer, "vfs") == rx_buffer) {
                target_laps = VFS_TARGET_LAPS;
                target_circumference = VFS_CIRCUMFERENCE;
                ESP_LOGI("UDP", "modo VFS activado");
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void init_udp_socket() {
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (udp_socket < 0) {
        ESP_LOGE("INIT_UDP", "error al crear socket");
        return;
    }

    struct sockaddr_in bind_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(UDP_PORT),
        .sin_addr = {.s_addr = INADDR_ANY,}
    };

    if (bind(udp_socket, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
        ESP_LOGE("UDP", "error bind: %d", errno);
        close(udp_socket);
        return;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);
    dest_addr.sin_addr.s_addr = inet_addr(LAPTOP_IP);
    
    ESP_LOGI("INIT_UDP", "socket creado y bindeado a %d", UDP_PORT);
}
