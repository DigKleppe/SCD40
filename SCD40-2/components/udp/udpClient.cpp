/* BSD Socket API Example

 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

static const char *TAG = "udpClient";

int UDPsendMssg(int port, void *mssg, int len) {
	int sockfd;
	int opt = 1;

	struct sockaddr_in servaddr;
	// Creating socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		return -1;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = IPADDR_BROADCAST; // INADDR_ANY;
//	inet_pton(AF_INET, "192.168.2.255", &servaddr.sin_addr.s_addr);
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

	// int err = sendto(sockfd, payload, strlen(payload), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	int err = sendto(sockfd, (const char*) mssg, len, 0, (const struct sockaddr*) &servaddr, sizeof(servaddr));

	if (err < 0) {
		ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
	}
	// ESP_LOGI(TAG, "Message sent to port: %d", port);

	close(sockfd);

	return 0;

}

//
//static void udp_client_task(void *pvParameters)
//{
//    char rx_buffer[128];
//    char host_ip[] = HOST_IP_ADDR;
//    int addr_family = 0;
//    int ip_protocol = 0;
//
//    while (1) {
//        struct sockaddr_in dest_addr;
//        dest_addr.sin_addr.s_addr = IP;
//        dest_addr.sin_family = AF_INET;
//        dest_addr.sin_port = htons(PORT);
//        addr_family = AF_INET;
//        ip_protocol = IPPROTO_IP;
//
//        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
//        if (sock < 0) {
//            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
//            break;
//        }
//        ESP_LOGI(TAG, "Socket created, sending to %s:%d", HOST_IP_ADDR, PORT);
//
//        while (1) {
//
//            int err = sendto(sock, payload, strlen(payload), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
//            if (err < 0) {
//                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
//                break;
//            }
//            ESP_LOGI(TAG, "Message sent");
//
//            struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
//            socklen_t socklen = sizeof(source_addr);
//            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
//
//            // Error occurred during receiving
//            if (len < 0) {
//                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
//                break;
//            }
//            // Data received
//            else {
//                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
//                ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
//                ESP_LOGI(TAG, "%s", rx_buffer);
//                if (strncmp(rx_buffer, "OK: ", 4) == 0) {
//                    ESP_LOGI(TAG, "Received expected message, reconnecting");
//                    break;
//                }
//            }
//
//            vTaskDelay(2000 / portTICK_PERIOD_MS);
//        }
//
//        if (sock != -1) {
//            ESP_LOGE(TAG, "Shutting down socket and restarting...");
//            shutdown(sock, 0);
//            close(sock);
//        }
//    }
//    vTaskDelete(NULL);
//}
//
//void app_main(void)
//{
//    ESP_ERROR_CHECK(nvs_flash_init());
//    ESP_ERROR_CHECK(esp_netif_init());
//    ESP_ERROR_CHECK(esp_event_loop_create_default());
//
//    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
//     * Read "Establishing Wi-Fi or Ethernet Connection" section in
//     * examples/protocols/README.md for more information about this function.
//     */
//    ESP_ERROR_CHECK(example_connect());
//
//    xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL);
//}
