/*
. /home/dig/esp/master/esp-idf/export.sh
  idf.py monitor -p /dev/ttyUSB2
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"

#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_image_format.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_wifi.h"

#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "mdns.h"

#include "wifiConnect.h"
#include "settings.h"
#include "updateSpiffsTask.h"

#include "sensirionTask.h"

#define LOWPOWERDELAY  				5 // minutes before going to sleepmode after powerup (no TCP anymore)
#define WAKEUP_INTERVAL				60 // seconds

//#define SDA_PIN  					21 // DMM board
//#define SCL_PIN 					22

#define SDA_PIN  					19  // sensor miniboard
#define SCL_PIN 					23

#define LED_PIN 					GPIO_NUM_2

#define I2C_CLK						100000
#define I2C_MASTER_SCL_IO           SCL_PIN      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           SDA_PIN      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0    /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          I2C_CLK      /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0            /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0            /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

static const char *TAG = "main";

esp_err_t init_spiffs(void);

extern bool settingsChanged; // from settingsScreen
uint32_t stackWm[5];
uint32_t upTime;

__attribute__((weak)) int getLogScript(char *pBuffer, int count) {
	return 0;
}

__attribute__((weak)) int getRTMeasValuesScript(char *pBuffer, int count) {
	return 0;
}

__attribute__((weak)) int getInfoValuesScript(char *pBuffer, int count) {
	return 0;
}

// ensure after reset back to factory app for OTA
static void setBootPartitionToFactory(void) {
	esp_image_metadata_t metaData;
	esp_err_t err;

	const esp_partition_t *factPart = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "factory");
	if (factPart != NULL) {
		esp_partition_pos_t factPartPos;
		factPartPos.offset = factPart->address;
		factPartPos.size = factPart->size;

		esp_image_verify(ESP_IMAGE_VERIFY, &factPartPos, &metaData);

		if (metaData.image.magic == ESP_IMAGE_HEADER_MAGIC) {
			ESP_LOGI(TAG, "Setting bootpartition to OTA factory");

			err = esp_ota_set_boot_partition(factPart);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
			}
		}
	}
}

/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void) {
	i2c_port_t i2c_master_port = I2C_MASTER_NUM;
// @formatter:off
	i2c_config_t conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = I2C_MASTER_SDA_IO,
		.scl_io_num = I2C_MASTER_SCL_IO,
		.sda_pullup_en = GPIO_PULLUP_DISABLE, // no pullups, externally
		.scl_pullup_en = GPIO_PULLUP_DISABLE,
		.master = I2C_MASTER_FREQ_HZ,
		.clk_flags = 0
	};
// @formatter:on
	esp_err_t err = i2c_param_config(i2c_master_port, &conf);
	if (err != ESP_OK) {
		return err;
	}
	return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

static void deep_sleep_register_rtc_timer_wakeup(void) {
	const int wakeup_time_sec = WAKEUP_INTERVAL;
	printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);
	ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
}

int testSCD40(i2c_port_t i2c_master_port);

extern "C" {
void app_main() {
	esp_err_t err;
	bool inLowPowerMode = false;
	bool toggle = false;
	int lowPowerTimer;

	esp_rom_gpio_pad_select_gpio(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(LED_PIN, 0);

	char newStorageVersion[MAX_STORAGEVERSIONSIZE] = { };
	TaskHandle_t otaTaskh;

	ESP_ERROR_CHECK(esp_event_loop_create_default());

	ESP_LOGI(TAG, "\n **************** start *****************\n");

	esp_reset_reason_t reason = esp_reset_reason();
	if (reason == ESP_RST_DEEPSLEEP) {
		// printf("Was powered up\n");
		printf("Deep sleep awoke\n");
		inLowPowerMode = true;
	} else {
		lowPowerTimer = LOWPOWERDELAY * 60;
		printf("awoke reason: %d\n", reason);
	}

	deep_sleep_register_rtc_timer_wakeup();
	err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
		ESP_LOGI(TAG, "nvs flash erased");
	}
	ESP_ERROR_CHECK(err);

	if (!inLowPowerMode) {
		ESP_ERROR_CHECK(init_spiffs());
		setBootPartitionToFactory();
	}
	err = loadSettings();

	int I2CmasterPort = I2C_MASTER_NUM;
	i2c_master_init();
	xTaskCreate(sensirionTask, "sensirionTask", 4 * 1024, &I2CmasterPort, 0, &SensirionTaskh);

	wifiConnect();

	while (1) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		upTime++;

		if (!connected) {
			toggle = !toggle;
			gpio_set_level(LED_PIN, toggle);
		} else
			gpio_set_level(LED_PIN, false);

		if (inLowPowerMode) {
			if (sensorDataIsSend) {
				vTaskDelete(connectTaskh);
				esp_wifi_stop();
				esp_deep_sleep_start();
			}
		}
		if (connected && !inLowPowerMode) {
			if (wifiSettings.updated) {
				wifiSettings.updated = false;
				saveSettings();
			}
			newStorageVersion[0] = 0;
			spiffsUpdateFinised = true;
			xTaskCreate(&updateSpiffsTask, "updateSpiffsTask", 8192, (void*) newStorageVersion, 5, &otaTaskh);
			while (!spiffsUpdateFinised)
				vTaskDelay(1000);
			if (newStorageVersion[0]) {
				strcpy(userSettings.spiffsVersion, newStorageVersion);
				saveSettings();
			}
			do {
				if (settingsChanged) {
					settingsChanged = false;
					vTaskDelay(1000 / portTICK_PERIOD_MS);
					saveSettings();
				}
				vTaskDelay(1000 / portTICK_PERIOD_MS);
			} while (lowPowerTimer-- > 0);
			//		}	while (1);

			vTaskDelete(connectTaskh);
			esp_wifi_stop();
			esp_deep_sleep_start();
		}

//		stackWm[0] = uxTaskGetStackHighWaterMark( connectTaskh );
//		stackWm[1] = uxTaskGetStackHighWaterMark( guiCommonTaskh );
//		stackWm[2] = uxTaskGetStackHighWaterMark( guiTaskh );
//		stackWm[3] = uxTaskGetStackHighWaterMark( SensirionTaskh );
////		printf ( "freeHeapSize %d\n",  xPortGetFreeHeapSize());
//		printf ( "freeHeapSize MALLOC_CAP_DMA:\n");
//		heap_caps_print_heap_info(MALLOC_CAP_DMA);

//
//		cntr++;
//		sprintf( str,"status %d",cntr);
//		displayMssg.displayItem = DISPLAY_ITEM_STATUSLINE;
//
//		if ( xQueueSend( displayMssgBox, &displayMssg, 0 )== pdPASS)
//			xQueueReceive(displayReadyMssgBox, &dummy, 500);// if accepted wait until data is displayed
//
//		displayMssg.displayItem = DISPLAY_ITEM_MEASLINE;
//
//		for (int n = 0; n < 4; n++){
//			displayMssg.line = n;
//			sprintf (str , "8888888%d", cntr);
//			if ( xQueueSend( displayMssgBox, &displayMssg, 0 )== pdPASS)
//				xQueueReceive(displayReadyMssgBox, &dummy, 500);// if accepted wait until data is displayed
//		}
	}
}
}

