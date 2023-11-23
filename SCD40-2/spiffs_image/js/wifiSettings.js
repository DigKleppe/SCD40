var SSID = ' ';
var APSSID = ' ';
var channel= 0;
var IPAddress= '100.100.100.100 ';
var rssi = 99;

function getSettings() {
	var req = new XMLHttpRequest();
	var str;
	
	req.open("GET", "cgi-bin/Readvar?allSettings", false);
	req.send();
	if (req.readyState == 4) {
		if (req.status == 200) {
			str = req.responseText.toString();
			var arr = str.split(",");
			channel = arr[0];
			SSID = arr[1];
			IPAddress = arr[2];
			APSSID = arr[3];
			rssi = arr[4];
		}
	}
}

function displaySettings() {
	getSettings();
	document.connectionSettingss.SSID.value = SSID;
	document.connectionSettingss.IPAddress.value = IPAddress;
	document.connectionSettingss.APSSID.value = APSSID;
	document.connectionSettingss.rssi.value = rssi;
	document.setPoint.setPoint.value = setpoint;
}

