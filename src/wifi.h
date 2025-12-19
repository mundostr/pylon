#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
// to get rid of annoying msgs about missing inits in some structs

#include "config.h"

static EventGroupHandle_t wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

void init_filesystem() {
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE("LITTLEFS", "error al montar");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE("LITTLEFS", "no se encuentra particion");
        } else {
            ESP_LOGE("LITTLEFS", "error al inicializar (%s)", esp_err_to_name(ret));
        }

        return;
    }

    ESP_LOGI("LITTLEFS", "inicializado");
}

void read_wifi_credentials(char* ssid, char* pass, char* pylon_ip, char* gateway_ip, char* netmask_ip, char* laptop_ip) {
    FILE* f = fopen("/littlefs/credentials.txt", "r");
    if (f == NULL) {
        ESP_LOGE("LITTLEFS", "error al leer credenciales");
        
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        char *delimiter = strchr(line, '=');
        
        if (delimiter != NULL) {
            char *key = line;
            char *value = delimiter + 1;
            *delimiter = 0;

            if (strcmp(key, "SSID") == 0) {
                strlcpy(ssid, value, 32); 
            } else if (strcmp(key, "PASS") == 0) {
                strlcpy(pass, value, 64);
            } else if (strcmp(key, "PYLON_IP") == 0) {
                strlcpy(pylon_ip, value, 16);
            } else if (strcmp(key, "GATEWAY_IP") == 0) {
                strlcpy(gateway_ip, value, 16);
            } else if (strcmp(key, "NETMASK_IP") == 0) {
                strlcpy(netmask_ip, value, 16);
            } else if (strcmp(key, "LAPTOP_IP") == 0) {
                strlcpy(laptop_ip, value, 16);
            }
        }
    }
    
    fclose(f);
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            ESP_LOGI("WIFI", "conectando modo STA...");
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGW("WIFI", "desconectado, reintentando...");
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

    init_filesystem();
    
    char ssid[32] = {0};
    char pass[64] = {0};
    char pylon_ip[16] = {0};
    char gateway_ip[16] = {0};
    char netmask_ip[16] = {0};
    // laptop_ip is kept global
    
    read_wifi_credentials(ssid, pass, pylon_ip, gateway_ip, netmask_ip, laptop_ip);
    
    ssid[strcspn(ssid, "\r\n")] = 0;
    pass[strcspn(pass, "\r\n")] = 0;
    pylon_ip[strcspn(pylon_ip, "\r\n")] = 0;
    gateway_ip[strcspn(gateway_ip, "\r\n")] = 0;
    netmask_ip[strcspn(netmask_ip, "\r\n")] = 0;
    laptop_ip[strcspn(laptop_ip, "\r\n")] = 0;
    
    // printf("SSID: [%s] (length: %d)\n", ssid, (int)strlen(ssid));
    // printf("PASS: [%s] (length: %d)\n", pass, (int)strlen(pass));

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold = {
                .authmode = WIFI_AUTH_WPA2_PSK,
            },
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    ESP_LOGI("WIFI", "inicializado modo STA");

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI("WIFI", "conectado");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE("WIFI", "error al conectar");
    } else {
        ESP_LOGE("WIFI", "evento no esperado");
    }

    while (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) vTaskDelay(500 / portTICK_PERIOD_MS);

    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(sta_netif));
    esp_netif_ip_info_t ip_info;
    inet_pton(AF_INET, pylon_ip, &ip_info.ip);
    inet_pton(AF_INET, gateway_ip, &ip_info.gw);
    inet_pton(AF_INET, netmask_ip, &ip_info.netmask);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(sta_netif, &ip_info));
}

void udp_send_ack_task(void *pvParameters) {
    for(;;) {
        sendto(udp_socket, "ACK", 3, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        vTaskDelay(ACK_FREQ / portTICK_PERIOD_MS);
    }
}

void udp_receive_task(void *pvParameters) {
    char rx_buffer[64];
    struct sockaddr_in source_addr;
    socklen_t socklen = sizeof(source_addr);

    for(;;) {
        int len = recvfrom(udp_socket, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
        if (len > 0) {
            rx_buffer[len] = '\0';
            // ESP_LOGI("UDP", "comando %s", rx_buffer);

            if (strcmp(rx_buffer, "act") == 0) {
                system_active = true;
                ESP_LOGI("UDP", "crono activado");
            } else if (strcmp(rx_buffer, "des") == 0) {
                lap_count = -1 - WAIT_LAPS;
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
    dest_addr.sin_addr.s_addr = inet_addr(laptop_ip);
    
    ESP_LOGI("INIT_UDP", "socket creado y bindeado a %d", UDP_PORT);
}
