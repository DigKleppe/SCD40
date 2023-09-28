/*
 * updateSpiffsTask.cpp
 *
 *  Created on: May 24, 2023
 *      Author: dig
 */

#include <string.h>
#include "errno.h"
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_image_format.h"
#include "updateSpiffsTask.h"

#ifdef CONFIG_EXAMPLE_USE_CERT_BUNDLE
#include "esp_crt_bundle.h"
#endif

#include "nvs.h"
#include "nvs_flash.h"

#include "wifiConnect.h"
#include "settings.h"

static const char *TAG = "updateSPIFFSTask";

#define BUFFSIZE 1024
#define HASH_LEN 32 /* SHA-256 digest length */

#define OTA_URL_SIZE 256

//#define SPIFFS_UPGRADE_FILENAME "storage.bin"
//#define SPIFFS_INFO_FILENAME "storageVersion.txt"

static char ota_write_data[BUFFSIZE + 1] = { 0 };
volatile bool spiffsUpdateFinised;

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

static void http_cleanup(esp_http_client_handle_t client) {
	esp_http_client_close(client);
}

static void exitSpiffsTask(void) {
	ESP_LOGE(TAG, "Exiting Spiffs updatetask ");

	spiffsUpdateFinised = true;
	(void) vTaskDelete(NULL);
}


void updateSpiffsTask(void *pvParameter) {
	esp_err_t err;
	size_t binary_file_length = 0;
	char updateURL[96];
	char newStorageVersion[MAX_STORAGEVERSIONSIZE];

	strcpy(updateURL, wifiSettings.upgradeURL);
	strcat(updateURL, CONFIG_SPIFFS_INFO_FILENAME);

	int block = 0;
	spiffsUpdateFinised = false;

	ESP_LOGI(TAG, "Starting updateSpiffsTask");

	const esp_partition_t *spiffsPartition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);

	ESP_LOGI(TAG, "SPIFFS partition type %d subtype %d (offset 0x%08"PRIx32")", spiffsPartition->type, spiffsPartition->subtype, spiffsPartition->address);

	esp_http_client_config_t config = { .url = updateURL, .cert_pem = (char*) server_cert_pem_start, .timeout_ms = CONFIG_OTA_RECV_TIMEOUT, .keep_alive_enable = true, };

#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

	esp_http_client_handle_t client = esp_http_client_init(&config);
	if (client == NULL) {
		ESP_LOGE(TAG, "Failed to initialise HTTP connection");
		exitSpiffsTask();
	}
	err = esp_http_client_open(client, 0);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to open read new storage version: %s", esp_err_to_name(err));
		esp_http_client_cleanup(client);
		exitSpiffsTask();
	}
	esp_http_client_fetch_headers(client);
	int data_read = esp_http_client_read(client, newStorageVersion, MAX_STORAGEVERSIONSIZE - 1);
	newStorageVersion[data_read] = 0;
	ESP_LOGI(TAG, "New storage version: %s", newStorageVersion);

	if (strcmp(newStorageVersion, userSettings.spiffsVersion) != 0) {
		ESP_LOGI(TAG, "updating storage");

		http_cleanup(client);

		strcpy(updateURL, wifiSettings.upgradeURL);
		strcat(updateURL, CONFIG_SPIFFS_UPGRADE_FILENAME);
		client = esp_http_client_init(&config);

		err = esp_http_client_open(client, 0);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Failed to open image: %s", esp_err_to_name(err));
			esp_http_client_cleanup(client);
			exitSpiffsTask();
		}
		esp_http_client_fetch_headers(client);

		int binary_file_length = 0;
		/*deal with all receive packet*/
		bool started = false;
		while (1) {
			data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
			ESP_LOGI(TAG, "read :%d %d", data_read, block++);
			if (data_read < 0) {
				ESP_LOGE(TAG, "Error: SSL data read error");
				http_cleanup(client);
				exitSpiffsTask();
			} else {
				if (!started) {
					started = true;
					if (data_read > 0) {
						err = esp_partition_erase_range(spiffsPartition , 0 ,spiffsPartition->size);
						if (err != ESP_OK) {
							ESP_LOGE(TAG, "spiffs partition erase failed: (%s)", esp_err_to_name(err));
							http_cleanup(client);
							exitSpiffsTask();
						}
						ESP_LOGI(TAG, "writing spiffs partition started");
					} else {
						ESP_LOGE(TAG, "received package is not fit len");
						http_cleanup(client);
						exitSpiffsTask();
					}
				}
				err = esp_partition_write (spiffsPartition, binary_file_length ,(const void*) ota_write_data, data_read);
				if (err != ESP_OK) {
					ESP_LOGE(TAG, "Error ota write (%s)", esp_err_to_name(err));
					http_cleanup(client);
					exitSpiffsTask();
				}
				binary_file_length += data_read;
				ESP_LOGD(TAG, "Written image length %d", binary_file_length);
			}
			if (data_read == 0) {
				/*
				 * As esp_http_client_read never returns negative error code, we rely on
				 * `errno` to check for underlying transport connectivity closure if any
				 */
				if (errno == ECONNRESET || errno == ENOTCONN) {
					ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
					break;
				}
				if (esp_http_client_is_complete_data_received(client) == true) {
					ESP_LOGI(TAG, "Connection closed");
					break;
				}
			}
		}
		ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);
		if (esp_http_client_is_complete_data_received(client) != true) {
			ESP_LOGE(TAG, "Error in receiving complete file");
			http_cleanup(client);
			exitSpiffsTask();
		}
		strcpy((char*) pvParameter,newStorageVersion);
	}
	http_cleanup(client);
	exitSpiffsTask();
}


