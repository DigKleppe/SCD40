/*
 * settings.c
 *
 *  Created on: Nov 30, 2017
 *      Author: dig
 */
#include "wifiConnect.h"
#include "settings.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include <string.h>
#include <cerrno>

#define STORAGE_NAMESPACE "storage"

extern settingsDescr_t settingsDescr[];
extern int myRssi;
bool settingsChanged;

char checkstr[MAX_STRLEN+1];


const userSettings_t userSettingsDefaults = {
	"0.0", 	//spiffsVersion[16]
	2,  	//temperatureOffset
	0,  	//CO2offset
	0.0,	//RHoffset
	{CONFIG_MDNS_HOSTNAME },
	USERSETTINGS_CHECKSTR };

userSettings_t userSettings;

//esp_err_t saveUserSettings(void)
//{
//	FILE *fd = fopen("/spiffs/settings", "wb");
//	if (fd == NULL) {
//		printf("  Error opening file (%d) %s\n", errno, strerror(errno));
//		printf("\n");
//		return-1;
//	}
//	int len = sizeof (userSettings_t);
//	int res = fwrite( &userSettings, 1, len, fd);
//	if (res != len) {
//		printf("  Error writing to file(%d <> %d\n", res, len);
//		res = fclose(fd);
//		return -1;
//	}
//	res = fclose(fd);
//	return 0;
//}
//
//esp_err_t loadUserSettings(){
//	esp_err_t res = 0;
//	FILE *fd = fopen("/spiffs/settings", "rb");
//	if (fd == NULL) {
//		printf("  Error opening settings file (%d) %s\n", errno, strerror(errno));
//
//	}
//	else {
//		int len = sizeof (userSettings_t);
//		res = fread( &userSettings, 1, len, fd);
//		if (res <= 0) {
//			printf("  Error reading from file\n");
//		}
//		res = fclose(fd);
//	}
//	if (strcmp(userSettings.checkstr, CHECKSTR) != 0 ){
//		userSettings = userSettingsDefaults;
//		printf( " ** defaults loaded");
//		saveUserSettings();
//	}
//	return res;
//}

/*
 * settings.c
 *
 *  Created on: Nov 30, 2017
 *      Author: dig
 */
#include "esp_log.h"
#include "settings.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "wifiConnect.h"
#include <string.h>
#include <cerrno>

#define STORAGE_NAMESPACE "storage"

static const char *TAG = "Settings";

//bool settingsChanged;
extern "C" {

esp_err_t saveSettings(void) {
	nvs_handle_t my_handle;
	esp_err_t err = 0;

	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
	} else {
		err = nvs_set_blob(my_handle, "WifiSettings",(const void *) &wifiSettings, sizeof(wifiSettings_t));
		err |= nvs_set_blob(my_handle, "userSettings",(const void *) &userSettings, sizeof(userSettings_t));

		switch (err) {
		case ESP_OK:
			ESP_LOGI(TAG, "settings written");
			break;
		default:
			ESP_LOGE(TAG, "Error (%s) writing!", esp_err_to_name(err));
		}
		err = nvs_commit(my_handle);

		switch (err) {
		case ESP_OK:
			ESP_LOGI(TAG, "Committed");
			break;
		default:
			ESP_LOGE(TAG, "Error (%s) commit", esp_err_to_name(err));
		}
		nvs_close(my_handle);
	}
	return err;
}

esp_err_t loadSettings() {
	nvs_handle_t my_handle;
	esp_err_t err = 0;
	bool doSave = false;


	err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
	size_t len;
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
	} else {
		ESP_LOGI(TAG, "reading settings");
		len =  sizeof(wifiSettings_t);
		err = nvs_get_blob(my_handle, "WifiSettings", (void *) &wifiSettings, &len);
		len = sizeof(userSettings_t);
		err |= nvs_get_blob(my_handle, "userSettings",(void *) &userSettings, &len);
		switch (err) {
		case ESP_OK:
			ESP_LOGI(TAG, "OTABootSSID: %s", wifiSettings.SSID);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			ESP_LOGE(TAG, "The value is not initialized yet!");
			break;
		default:
			ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
		}
		nvs_close(my_handle);
	}

	if (strcmp(wifiSettings.upgradeFileName, CONFIG_FIRMWARE_UPGRADE_FILENAME) != 0) {
		strcpy(wifiSettings.upgradeFileName, CONFIG_FIRMWARE_UPGRADE_FILENAME);
		doSave = true;  // set filename for OTA via factory firmware
	}

	if(strncmp(userSettings.checkstr,USERSETTINGS_CHECKSTR, strlen (USERSETTINGS_CHECKSTR) )	!= 0)
	{
		userSettings = userSettingsDefaults;
		wifiSettings = wifiSettingsDefaults;
		ESP_LOGE(TAG, "default settings loaded");
		doSave = true;  // set filename for OTA via factory firmware
	}

	if ( doSave)
		return saveSettings();
	else {
		ESP_LOGI(TAG, "usersettings loaded");
	}
	return err;

}
}

