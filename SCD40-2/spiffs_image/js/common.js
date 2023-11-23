var SIMULATE = false;

var data2 = 0;
var simValue1 = 0;
var simValue2 = 0;

var ACTCOLLUMN = 1;
var OFFSETCOLLUN = 2;
var TEMPINFOROW = 1;
var RHINFOROW = 2;
var CO2INFOROW = 3;


function sendItem(item) {
	console.log("sendItem: " + item);
	if (SIMULATE)
		return "OK";

	var str;
	var req = new XMLHttpRequest();
	req.open("POST", "/upload/cgi-bin/", false);
	req.send(item);
	if (req.readyState == 4) {
		if (req.status == 200) {
			str = req.responseText.toString();
			return str;
		}
	}
}

function getItem(item) {
	console.log("getItem " + item);
	if (SIMULATE)
		return "OK";

	var str;
	var req = new XMLHttpRequest();
	req.open("GET", "cgi-bin/" + item, false);
	req.send();
	if (req.readyState == 4) {
		if (req.status == 200) {
			str = req.responseText.toString();
			return str;
		}
	}
}
// returns array with accumulated momentary values
function getLogMeasValues() {
	console.log("getLogMeasValues");
	if (SIMULATE)
		return "OK";

	var str;
	var req = new XMLHttpRequest();
	req.open("GET", "cgi-bin/getLogMeasValues", false);
	req.send();
	if (req.readyState == 4) {
		if (req.status == 200) {
			str = req.responseText.toString();
			return str;
			//	var arr = str.split(",");
			//	return arr;
		}
	}
}

// returns array minute  averaged values
function getAvgMeasValues() {
	console.log("getAvgMeasValues");
	if (SIMULATE)
		return "OK";

	var str;
	var req = new XMLHttpRequest();
	req.open("GET", "cgi-bin/getAvgMeasValues", false);
	req.send();
	if (req.readyState == 4) {
		if (req.status == 200) {
			str = req.responseText.toString();
			return str;
			//	var arr = str.split(",");
			//	return arr;
		}
	}
}

// returns array with momentarty  values
function getRTMeasValues() {
	console.log("getRTMeasValues");
	if (SIMULATE)
		return "OK";

	var str;
	var req = new XMLHttpRequest();
	req.open("GET", "cgi-bin/getRTMeasValues", false);
	req.send();
	if (req.readyState == 4) {
		if (req.status == 200) {
			str = req.responseText.toString();
			return str;
			//	var arr = str.split(",");
			//	return arr;
		}
	}
}
