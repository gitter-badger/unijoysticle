/****************************************************************************
http://retro.moe/unijoysticle

Copyright 2017 Ricardo Quesada

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
****************************************************************************/

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"

#include "nvs_flash.h"

#include "driver/gpio.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

//
// contants
// 
static const int WIFI_CONNECTED_BIT = 1 << 0;
static const int POT_PORT1_BIT = 1 << 0;
static const int POT_PORT2_BIT = 1 << 1;
static const int BUFSIZE = 512;


//
// global variables
//
static EventGroupHandle_t g_pot_event_group, g_wifi_event_group;
static unsigned char g_potx = 0;
static unsigned char g_poty = 0;

//
// structs
//
struct uni_proto_v3
{
    char version;
    char ports_enabled;
    char joy1;
    char joy2;
    char joy1_potx;
    char joy1_poty;
    char joy2_potx;
    char joy2_poty;
};

void IRAM_ATTR gpio_isr_handler_up(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    (void)gpio_num;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(g_pot_event_group, POT_PORT1_BIT, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR();
}

void main_loop(void* arg)
{
    (void)arg;

    // wait 50ms
    const TickType_t xTicksToWait = 50 / portTICK_PERIOD_MS;
    while(1) {

        EventBits_t uxBits = xEventGroupWaitBits(g_pot_event_group, (POT_PORT1_BIT | POT_PORT2_BIT), pdTRUE, pdFALSE, xTicksToWait);
//        xEventGroupClearBits(g_pot_event_group, (POT_PORT1_BIT | POT_PORT2_BIT));

        // if not timeout, change the state
        if (uxBits != 0) {

            gpio_set_level(GPIO_NUM_5, 1);
            const int j = g_potx * 30;
            for (int i=0; i<j; ++i)
                __asm__("nop");
            gpio_set_level(GPIO_NUM_5, 0);
        }
    }
}

void wifi_loop(void* arg)
{
    /* Wait for internet to be up */
    xEventGroupWaitBits(g_wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

    int sockfd;                         /* socket */
    socklen_t clientlen;                /* byte size of client's address */
    struct sockaddr_in serveraddr;      /* server's addr */
    struct sockaddr_in clientaddr;      /* client addr */
    char buf[BUFSIZE];                  /* message buf */
    int n;                              /* message byte size */

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        printf("ERROR opening socket");

    /*
     * build the server's Internet address
     */
    memset((char *) &serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(6464);

    if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
        printf("ERROR on binding\n");

    clientlen = sizeof(clientaddr);

    struct uni_proto_v3* proto;
    while (1) {

        n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
        if (n == 8) {
            proto = (struct uni_proto_v3*) &buf;
            g_potx = proto->joy1_potx;
            g_poty = proto->joy1_poty;
        }
    }
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            printf("SYSTEM_EVENT_STA_START\n");
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            printf("SYSTEM_EVENT_STA_GOT_IP\n");
            xEventGroupSetBits(g_wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            printf("SYSTEM_EVENT_STA_DISCONNECTED\n");
            /* This is a workaround as ESP32 WiFi libs don't currently auto-reassociate. */
            esp_wifi_connect();
            xEventGroupClearBits(g_wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void setup_gpios()
{
    // internal LED
    ESP_ERROR_CHECK( gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT) );
    ESP_ERROR_CHECK( gpio_set_level(GPIO_NUM_5, 1) );

    // read POT X
//    gpio_config_t io_conf;
//    io_conf.intr_type = GPIO_INTR_POSEDGE;
//    io_conf.mode = GPIO_MODE_INPUT;
    // PotX, PotY from port#1 and port#2
//    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_36);
//    io_conf.pull_down_en = 0;
//    io_conf.pull_up_en = 1;
//    ESP_ERROR_CHECK( gpio_config(&io_conf) );

    ESP_ERROR_CHECK( gpio_set_direction(GPIO_NUM_21, GPIO_MODE_INPUT) );
    ESP_ERROR_CHECK( gpio_set_intr_type(GPIO_NUM_21, GPIO_INTR_POSEDGE) );
    ESP_ERROR_CHECK( gpio_set_pull_mode(GPIO_NUM_21, GPIO_PULLUP_ONLY) );
    ESP_ERROR_CHECK( gpio_intr_enable(GPIO_NUM_21) );

    //create a queue to handle gpio event from isr
//    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
//    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    ESP_ERROR_CHECK( gpio_install_isr_service(0) );

    //hook isr handler for specific gpio pin
//    ESP_ERROR_CHECK( gpio_isr_handler_add(GPIO_NUM_36, gpio_isr_handler_up, (void*) GPIO_NUM_36) );
    ESP_ERROR_CHECK( gpio_isr_handler_add(GPIO_NUM_21, gpio_isr_handler_up, (void*) GPIO_NUM_21) );
}

static void setup_wifi()
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    wifi_config_t sta_config = {
        .sta = {
            .ssid = "queque2",
            .password = "locopajaro",
            .bssid_set = false
        }
    };
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );
}

void app_main(void)
{
    nvs_flash_init();

    g_pot_event_group = xEventGroupCreate();
    g_wifi_event_group = xEventGroupCreate();

    setup_wifi();
    setup_gpios();

    printf("v3 size: %d\n", sizeof(struct uni_proto_v3));

    xTaskCreate(main_loop, "main_loop", 2048, NULL, 10, NULL);
    xTaskCreate(wifi_loop, "wifi_loop", 2048, NULL, 10, NULL);
}

