/* WiFi station Example

 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "settings.h"
#include "mdns.h"
#include "lwip/ip4_addr.h"

#include "esp_smartconfig.h"
#include "wifiConnect.h"

void initialiseMdns( char * hostName);
esp_err_t start_file_server(const char *base_path);

bool DHCPoff;
bool IP6off;
bool DNSoff;
bool fileServerOff;

bool doStop;
esp_netif_t *s_sta_netif = NULL;

static void setStaticIp(esp_netif_t *netif);
esp_err_t saveSettings(void);

volatile  connectStatus_t connectStatus;

#define EXAMPLE_ESP_WIFI_SSID      "xxx"
#define EXAMPLE_ESP_WIFI_PASS      "yyy"

wifiSettings_t wifiSettings;
wifiSettings_t wifiSettingsDefaults = { CONFIG_EXAMPLE_WIFI_SSID, CONFIG_EXAMPLE_WIFI_PASSWORD,ipaddr_addr(DEFAULT_IPADDRESS),ipaddr_addr(DEFAULT_GW),CONFIG_DEFAULT_FIRMWARE_UPGRADE_URL,CONFIG_FIRMWARE_UPGRADE_FILENAME,false  };

/* The examples use WiFi configuration that you can set via project configuration menu

 If you'd rather not, just change the below entries to strings with
 the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
 */

#define EXAMPLE_ESP_MAXIMUM_RETRY  2
#define EXAMPLE_H2E_IDENTIFIER 		""
#define CONFIG_ESP_WPA3_SAE_PWE_BOTH 1
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define CONFIG_ESP_WIFI_PW_ID	""
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#define SMARTCONFIGTIMEOUT 20 // sec


#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define CONNECTED_BIT BIT0
static const int ESPTOUCH_DONE_BIT = BIT2;

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static void setStaticIp(esp_netif_t *netif)
{
	if (esp_netif_dhcpc_stop(netif) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop dhcp client");
        return;
    }
    esp_netif_ip_info_t ip;
    memset(&ip, 0 , sizeof(esp_netif_ip_info_t));

    if ( wifiSettings.ip4Address.addr == 0) {
    	ip.ip.addr = ipaddr_addr(DEFAULT_IPADDRESS);
        ip.gw.addr = ipaddr_addr(DEFAULT_GW);
    }
    else {
       	ip.ip =  wifiSettings.ip4Address;   //  ipaddr_addr(EXAMPLE_STATIC_IP_ADDR);
        ip.gw = wifiSettings.gw;
    }
    ip.netmask.addr = ipaddr_addr(STATIC_NETMASK_ADDR);

	ESP_LOGI(TAG, "Set fixed IPv4 address to: " IPSTR ",", IP2STR(&ip.ip));


    if (esp_netif_set_ip_info(netif, &ip) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set ip info");
        return;
    }

 //   ESP_LOGD(TAG, "Success to set static ip: %s, netmask: %s, gw: %s", EXAMPLE_STATIC_IP_ADDR, EXAMPLE_STATIC_NETMASK_ADDR, EXAMPLE_STATIC_GW_ADDR);
  //  ESP_ERROR_CHECK(example_set_dns_server(netif, ipaddr_addr(EXAMPLE_MAIN_DNS_SERVER), ESP_NETIF_DNS_MAIN));
  //  ESP_ERROR_CHECK(example_set_dns_server(netif, ipaddr_addr(EXAMPLE_BACKUP_DNS_SERVER), ESP_NETIF_DNS_BACKUP));
}



static void smartconfigTask(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
    while (1) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, (SMARTCONFIGTIMEOUT * 1000) / portTICK_PERIOD_MS);
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
		if (uxBits == 0) {  // timeout
			ESP_LOGI(TAG, "smartconfig timeout");
			esp_smartconfig_stop();
			esp_wifi_connect();
			vTaskDelete(NULL);
		}
    }
}


static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

	if ( doStop)
		return;

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
			esp_wifi_connect();
			connectStatus = CONNECTING;
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		} else {
			xTaskCreate(smartconfigTask, "smartconfig_example_task", 4096, NULL, 3, NULL);
			s_retry_num = 0;
			connectStatus = SMARTCONFIG_ACTIVE;
			ESP_LOGI(TAG, "Starting SmartConfig");
		}
		xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
		connectStatus = IP_RECEIVED;
	} else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
		ESP_LOGI(TAG, "Scan done");
	} else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
		ESP_LOGI(TAG, "Found channel");
	} else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
		ESP_LOGI(TAG, "Got SSID and password");
		connectStatus = CONNECTED;
		smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t*) event_data;
		wifi_config_t wifi_config;
		uint8_t ssid[33] = { 0 };
		uint8_t password[65] = { 0 };
		uint8_t rvd_data[33] = { 0 };

		bzero(&wifi_config, sizeof(wifi_config_t));
		memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
		memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
		wifi_config.sta.bssid_set = evt->bssid_set;
		if (wifi_config.sta.bssid_set == true) {
			memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
		}

		memcpy(ssid, evt->ssid, sizeof(evt->ssid));
		memcpy(password, evt->password, sizeof(evt->password));
		ESP_LOGI(TAG, "SSID:%s", ssid);
		ESP_LOGI(TAG, "PASSWORD:%s", password);

		memcpy ( (char *)wifiSettings.SSID, ssid, sizeof ( wifiSettings.SSID));
		memcpy ( (char *)wifiSettings.pwd, password,sizeof ( wifiSettings.pwd));
		saveSettings();

		if (evt->type == SC_TYPE_ESPTOUCH_V2) {
			ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
			ESP_LOGI(TAG, "RVD_DATA:");
			for (int i = 0; i < 33; i++) {
				printf("%02x ", rvd_data[i]);
			}
			printf("\n");
		}

		ESP_ERROR_CHECK(esp_wifi_disconnect());
		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
		esp_wifi_connect();
	} else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
		xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
	}
}

void wifi_init_sta(void) {
	bool connected = false;
	connectStatus = CONNECTING;
	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());

	if ( !DNSoff)
		initialiseMdns(userSettings.moduleName);

//	ESP_ERROR_CHECK(esp_event_loop_create_default());  in main
	s_sta_netif = esp_netif_create_default_wifi_sta();
	if( DHCPoff)
      setStaticIp((esp_netif_t*) s_sta_netif);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

	ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

	wifi_config_t wifi_config = {
	.sta = {
		.ssid = EXAMPLE_ESP_WIFI_SSID,
		.password = EXAMPLE_ESP_WIFI_PASS,
	/* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
	 * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
	 * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
	 * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
	 */
	//	.threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
	//	.sae_pwe_h2e = ESP_WIFI_SAE_MODE,
	//	.sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
	},
	};

	wifi_config.sta.threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD;
	wifi_config.sta.sae_pwe_h2e = ESP_WIFI_SAE_MODE;
	strcpy((char *)wifi_config.sta.sae_h2e_identifier,EXAMPLE_H2E_IDENTIFIER);

	strcpy((char*) wifi_config.sta.ssid, wifiSettings.SSID);
	strcpy((char*) wifi_config.sta.password, wifiSettings.pwd);

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_ERROR_CHECK(start_file_server("/spiffs"));
	ESP_LOGI(TAG, "wifi_init_sta finished.");
}

void wifi_stop(void)
{
    doStop = true;
	esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        return;
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(s_sta_netif));
    esp_netif_destroy(s_sta_netif);
    s_sta_netif = NULL;
}

void  wifiConnect(void) {
	wifi_init_sta();
}



