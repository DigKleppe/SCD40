
var firstTime = true;
var firstTimeCal = true;

var body;
var infoTbl;
var calTbl;
var nameTbl;
var tblBody;
var INFOTABLENAME = "infoTable";
var CALTABLENAME = "calTable";
var NAMETABLENAME = "nameTable";

function makeNameTable(descriptorData) {
	var colls;
	nameTbl = document.getElementById(NAMETABLENAME);// ocument.createElement("table");
	var x = nameTbl.rows.length;
	for (var r = 0; r < x; r++) {
		nameTbl.deleteRow(-1);
	}
	tblBody = document.createElement("tbody");

	var rows = descriptorData.split("\n");

	for (var i = 0; i < rows.length - 1; i++) {
		var row = document.createElement("tr");
		if (i == 0) {
			colls = rows[i].split(",");
			for (var j = 0; j < colls.length; j++) {
				var cell = document.createElement("th");
				var cellText = document.createTextNode(colls[j]);
				cell.appendChild(cellText);
				row.appendChild(cell);
			}
		}
		else {
			var cell = document.createElement("td");
			var cellText = document.createTextNode(rows[i]);
			cell.appendChild(cellText);
			row.appendChild(cell);

			cell = document.createElement("td");
			var input = document.createElement("input");
			input.setAttribute("type", "text");
			cell.appendChild(input);
			row.appendChild(cell);

			cell = document.createElement("td");
			cell.setAttribute("nameItem", i);

			var button = document.createElement("button");
			button.innerHTML = "Stel in";
			button.className = "button-3";
			cell.appendChild(button);
			row.appendChild(cell);

			cell = document.createElement("td");
			cell.setAttribute("nameItem", i);

			var button = document.createElement("button");
			button.innerHTML = "Herstel";
			button.className = "button-3";
			button.setAttribute("id", "set" + i);

			cell.appendChild(button);
			row.appendChild(cell);
		}
		tblBody.appendChild(row);
	}
	nameTbl.appendChild(tblBody);

	const cells = document.querySelectorAll("td[nameItem]");
	cells.forEach(cell => {
		cell.addEventListener('click', function() { setNameFunction(cell.closest('tr').rowIndex, cell.cellIndex) });
	});
}

function setNameFunction(row, coll) {
	console.log("Row index:" + row + " Collumn: " + coll);
	var item = nameTbl.rows[row].cells[0].innerText;

	if (coll == 3)
		sendItem("revertName");
	else {
		var value = nameTbl.rows[row].cells[1].firstChild.value;
		console.log(item + value);
		if (value != "") {
			sendItem("setName:moduleName=" + value);
		}
		makeNameTable(value);
	}
}

function makeInfoTable(descriptorData) {

	infoTbl = document.getElementById(INFOTABLENAME);// ocument.createElement("table");
	var x = infoTbl.rows.length
	for (var r = 0; r < x; r++) {
		infoTbl.deleteRow(-1);
	}
	tblBody = document.createElement("tbody");

	var rows = descriptorData.split("\n");

	for (var i = 0; i < rows.length - 1; i++) {
		var row = document.createElement("tr");
		var colls = rows[i].split(",");

		for (var j = 0; j < colls.length; j++) {
			if (i == 0)
				var cell = document.createElement("th");
			else
				var cell = document.createElement("td");

			var cellText = document.createTextNode(colls[j]);
			cell.appendChild(cellText);
			row.appendChild(cell);
		}
		tblBody.appendChild(row);
	}
	infoTbl.appendChild(tblBody);
}

function makeCalTable(descriptorData) {

	var colls;
	calTbl = document.getElementById(CALTABLENAME);// ocument.createElement("table");
	var x = calTbl.rows.length;
	for (var r = 0; r < x; r++) {
		calTbl.deleteRow(-1);
	}
	tblBody = document.createElement("tbody");

	var rows = descriptorData.split("\n");

	for (var i = 0; i < rows.length - 1; i++) {
		var row = document.createElement("tr");
		if (i == 0) {
			colls = rows[i].split(",");
			for (var j = 0; j < colls.length - 1; j++) {
				var cell = document.createElement("th");
				var cellText = document.createTextNode(colls[j]);
				cell.appendChild(cellText);
				row.appendChild(cell);
			}
		}
		else {
			var cell = document.createElement("td");
			var cellText = document.createTextNode(rows[i]);
			cell.appendChild(cellText);
			row.appendChild(cell);

			cell = document.createElement("td");
			var input = document.createElement("input");
			input.setAttribute("type", "number");
			cell.appendChild(input);
			row.appendChild(cell);

			cell = document.createElement("td");
			cell.setAttribute("calItem", i);

			var button = document.createElement("button");
			button.innerHTML = "Stel in";
			//	button.className = "button buttonGreen";
			button.className = "button-3";
			cell.appendChild(button);
			row.appendChild(cell);

			cell = document.createElement("td");
			cell.setAttribute("calItem", i);

			var button = document.createElement("button");
			button.innerHTML = "Herstel";
			//	button.className = "button buttonGreen";
			button.className = "button-3";
			button.setAttribute("id", "set" + i);

			cell.appendChild(button);
			row.appendChild(cell);
		}
		tblBody.appendChild(row);
	}
	calTbl.appendChild(tblBody);

	const cells = document.querySelectorAll("td[calItem]");
	cells.forEach(cell => {
		cell.addEventListener('click', function() { setCalFunction(cell.closest('tr').rowIndex, cell.cellIndex) });
	});
}

function readInfo(str) {
	makeInfoTable(str);
}

function readCalInfo(str) {
	if (SIMULATE) {
		if (firstTimeCal) {
			makeCalTable(str);
			firstTimeCal = false;
		}
		return;
	}
	else {
		str = getItem("getCalValues");
		makeCalTable(str);
	}
}

function save() {
	getItem("saveSettings");
}

function cancel() {
	getItem("cancelSettings");
}


function setCalFunction(row, coll) {
	console.log("Row index:" + row + " Collumn: " + coll);
	var item = calTbl.rows[row].cells[0].innerText;

	if (coll == 3)
		sendItem("revertCal:" + item);
	else {
		//	var x = calTbl.rows[2].cells[3].firstChild.value;
		var value = calTbl.rows[row].cells[1].firstChild.value;
		console.log(item + value);
		if (value != "") {
			sendItem("setCal:" + item + '=' + value);
			//getItem("setcal:" + item + '=' + value);
		}
	}
}


function testInfo() {
	var str = "Meting,xActueel,xOffset,xx,\naap,2,3,4,\nnoot,5,6,7,\n,";
	readInfo(str);
	str = "Actueel,Nieuw,Stel in,Herstel,\nSensor 1\n";
	makeNameTable(str);
}

function testCal() {
	var str = "Meting,Referentie,Stel in,Herstel,\nTemperatuur\n RH\n CO2\n";
	readCalInfo(str);
}

function initSettings() {
	if (SIMULATE) {
		testInfo();
		testCal();
	}
	else {
		//document.visibilityState

	}
	readCalInfo();
	str = getItem("getSensorName");
	makeNameTable(str);
	setInterval(function() { settingsTimer() }, 1000);
}

function tempCal() {
	var str;
	testCal();
}
var xcntr = 1;

function getInfo() {
	if (SIMULATE) {
		infoTbl.rows[1].cells[1].innerHTML = xcntr++;
		return;
	}
	var arr;
	var str;
	str = getItem("getInfoValues");
	if (firstTime) {
		makeInfoTable(str);
		firstTime = false;
	}
	else {
		var rows = str.split("\n");
		for (var i = 1; i < rows.length - 1; i++) {
			var colls = rows[i].split(",");
			for (var j = 1; j < colls.length; j++) {
				infoTbl.rows[i].cells[j].innerHTML = colls[j];
			}
		}
	}
}


function settingsTimer() {

	if (document.visibilityState == "hidden")
		return;
	getInfo();

}
