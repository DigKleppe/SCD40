/**
* 
*/
var CO2Data;
var tempAndRHdata;

var chartRdy = false;
var tick = 0;
var dontDraw = false;
var halt = false;
var chartHeigth = 500;
var simValue1 = 0;
var simValue2 = 0;
var table;
var presc = 1;
var simMssgCnts = 0;
var lastTimeStamp = 0;
var REQINTERVAL = 30; // sec

var MINUTESPERTICK = 5;// log interval 
var LOGDAYS = 7;
var MAXPOINTS = LOGDAYS * 24 * 60 / MINUTESPERTICK;

var displayNames = ["", "temperatuur", "vochtigheid", "CO2"];
var NRItems = displayNames.length;

var dayNames = ['zo', 'ma', 'di', 'wo', 'do', 'vr', 'za'];


var CO2Options = {
	title: '',
	curveType: 'function',
	legend: { position: 'bottom' },
	heigth: 200,
	crosshair: { trigger: 'both' },	// Display crosshairs on focus and selection.
	explorer: {
		actions: ['dragToZoom', 'rightClickToReset'],
		//actions: ['dragToPan', 'rightClickToReset'],
		axis: 'horizontal',
		keepInBounds: true,
		maxZoomIn: 100.0
	},
	chartArea: { 'width': '90%', 'height': '60%' },
};

var tempAndRHoptions = {
	title: '',
	curveType: 'function',
	legend: { position: 'bottom' },

	heigth: 200,
	crosshair: { trigger: 'both' },	// Display crosshairs on focus and selection.
	explorer: {
		actions: ['dragToZoom', 'rightClickToReset'],
		//actions: ['dragToPan', 'rightClickToReset'],
		axis: 'horizontal',
		keepInBounds: true,
		maxZoomIn: 100.0
	},
	chartArea: { 'width': '90%', 'height': '60%' },

	vAxes: {
		0: { logScale: false },
		1: { logScale: false },
		2: { logScale: false },
	},
	series: {
		0: { targetAxisIndex: 0 },// temperature
		1: { targetAxisIndex: 1 },// RH
	},
};



function clear() {
	tempAndRHdata.removeRows(0, tempAndRHdata.getNumberOfRows());
	chart.draw(tempAndRHdata, options);
	tick = 0;
}



//var formatter_time= new google.visualization.DateFormat({formatType: 'long'});
// channel 1 .. 5

function plotCO2(channel, value) {
	if (chartRdy) {
		if (channel == 1) {
			CO2Data.addRow();
			if (CO2Data.getNumberOfRows() > MAXPOINTS == true)
				CO2Data.removeRows(0, CO2Data.getNumberOfRows() - MAXPOINTS);
		}
		value = parseFloat(value); // from string to float
		CO2Data.setValue(CO2Data.getNumberOfRows() - 1, channel, value);
	}
}

function plotTempAndRH(channel, value) {
	if (chartRdy) {
		if (channel == 1) {
			tempAndRHdata.addRow();
			if (tempAndRHdata.getNumberOfRows() > MAXPOINTS == true)
				tempAndRHdata.removeRows(0, tempAndRHdata.getNumberOfRows() - MAXPOINTS);
		}
		value = parseFloat(value); // from string to float
		tempAndRHdata.setValue(tempAndRHdata.getNumberOfRows() - 1, channel, value);
	}
}

function initChart() {

	CO2chart = new google.visualization.LineChart(document.getElementById('CO2chart'));
	CO2Data = new google.visualization.DataTable();
	CO2Data.addColumn('string', 'Time');
	CO2Data.addColumn('number', 'CO2');

	tRHchart = new google.visualization.LineChart(document.getElementById('tRHchart'));
	tempAndRHdata = new google.visualization.DataTable();
	tempAndRHdata.addColumn('string', 'Time');
	tempAndRHdata.addColumn('number', 't');
	tempAndRHdata.addColumn('number', 'RH');

	chartRdy = true;
	dontDraw = false;
	if (SIMULATE) {
		simplot();
	}
	else {
		startTimer();
	}
	
}

function startTimer() {
	if (!SIMULATE)
		setInterval(function() { timer() }, 1000);  
}

var firstRequest = true;
var plotTimer = 6; // every 60 seconds plot averaged value
var rows = 0;

function updateLastDayTimeLabel(data) {

	var ms = Date.now();

	var date = new Date(ms);
	var labelText = date.getHours() + ':' + date.getMinutes();
	data.setValue(data.getNumberOfRows() - 1, 0, labelText);

}


function updateAllDayTimeLabels(data) {
	var rows = data.getNumberOfRows();
	var minutesAgo = rows * MINUTESPERTICK;
	var ms = Date.now();
	ms -= (minutesAgo * 60 * 1000);
	for (var n = 0; n < rows; n++) {
		var date = new Date(ms);
		var labelText = dayNames[date.getDay()] + ';' + date.getHours() + ':' + date.getMinutes();
		data.setValue(n, 0, labelText);
		ms += 60 * 1000 * MINUTESPERTICK;

	}
}

function simplot() {
	var w = 0;
	var str = ",2,3,4,5,\n";
	var str2 = "";
	for (var n = 0; n < 3 * 24 * 4; n++) {
		simValue1 += 0.01;
		simValue2 = Math.sin(simValue1);
		if ((n & 16) > 12)
			w += 20;

		//                                    temperatuur       hum                        		co2                                                                                                            
		str2 = str2 + simMssgCnts++ + "," + simValue2 + "," + (100 * (simValue2 + 3)) + "," + (simValue2 + 20) + ","  + "\n";
	}
	plotArray(str2);
	for (var m = 1; m < NRItems ; m++) { // time not used for now 
		var value = simValue2; // from string to float
		document.getElementById(displayNames[m]).innerHTML = value.toFixed(2);
	}
	
}

function plotArray(str) {
	var arr;
	var arr2 = str.split("\n");
	var nrPoints = arr2.length - 1;

	var mm = 0;

	let now = new Date();
	var today = now.getDay();
	var hours = now.getHours();
	var minutes = now.getMinutes();
	var quartersToday = (hours * 4 + minutes / 15);
	var daysInLog = ((nrPoints - quartersToday) / (24 * 4)); // complete days
	if (daysInLog < 0)
		daysInLog = 0;
	daysInLog -= daysInLog % 1;
	var dayIdx = today - daysInLog - 1; // where to start 
	if (dayIdx < 0)
		dayIdx += LOGDAYS;
	var quartersFirstDay = nrPoints - quartersToday - (daysInLog * 24 * 4);// first day probably incomplete
	if (quartersFirstDay < 0)
		quartersFirstDay = nrPoints;

	var quartersToday = 24 * 4;

	for (var p = 0; p < nrPoints; p++) {
		arr = arr2[p].split(",");
		if (arr.length >= NRItems) {
			if (quartersFirstDay > 0) {
				quartersFirstDay--;
				if (quartersFirstDay <= 0) {
					dayIdx++;
				}
			}
			else {
				if (quartersToday > 0)
					quartersToday--;
				if ((quartersToday <= 0) || (p == (nrPoints - 1))) {
					quartersToday = 24 * 4;

				}
			}
			if (dayIdx >= LOGDAYS)
				dayIdx = 0;
			plotTempAndRH(1, arr[1]); // temperature
			plotTempAndRH(2, arr[2]); // RH
			plotCO2(1, arr[3]); // CO2
		}
	}
	if (nrPoints == 1) { // then single point added 
		updateLastDayTimeLabel(CO2Data);
		updateLastDayTimeLabel(tempAndRHdata);
	}
	else {
		updateAllDayTimeLabels(CO2Data);
		updateAllDayTimeLabels(tempAndRHdata);
	}
	tRHchart.draw(tempAndRHdata, tempAndRHoptions);
	CO2chart.draw(CO2Data, CO2Options);
}

function timer() {
	var arr;
	var str;
	presc--
	
	if (SIMULATE) {
		simplot();
	}
	else {
		if (presc == 0) {
			presc = REQINTERVAL; 

			str = getRTMeasValues();
			arr = str.split(",");
			// print RT values 
			if (arr.length >= NRItems) {
				if (arr[0] > 0) {
					if (arr[0] != lastTimeStamp) {
						lastTimeStamp = arr[0];
						for (var m = 1; m <NRItems; m++) { // time not used for now 
							var value = parseFloat(arr[m]); // from string to float
							if (value < -100)
								arr[m] = "--";
							document.getElementById(displayNames[m]).innerHTML = arr[m];
						}

						plotTempAndRH(1, arr[1]); // temperature
						plotTempAndRH(2, arr[2]); // RH
						plotCO2(1, arr[3]); // CO2

						updateLastDayTimeLabel(tempAndRHdata);
						updateLastDayTimeLabel(CO2Data);
						tRHchart.draw(tempAndRHdata, tempAndRHoptions);
						CO2chart.draw(CO2Data, CO2Options);
					}
				}
			}

			if (firstRequest) {
				arr = getLogMeasValues();
				plotArray(arr);
				firstRequest = false;
			}
		}
	}
}




