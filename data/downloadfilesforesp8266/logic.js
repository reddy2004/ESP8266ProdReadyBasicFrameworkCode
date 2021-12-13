document.getElementById('manual').onchange = function () {
	document.getElementById('_ip').disabled = !this.checked;
	document.getElementById('_subnet').disabled = !this.checked;
	document.getElementById('_gateway').disabled = !this.checked;
}

document.getElementById('uniqueWifi').style.display = "none";

function DeleteWifi(ssid) {
	fetch("/delete?SSID=" + ssid).then(function (response) {
		alert("Command sent. Rebooting. Please check /config after reboot");
	}).then(function (data) {
		alert("Command sent. Rebooting. Please check /config after reboot");
	}).catch(function () {

	});
}
document.getElementById('addNewWifi').onclick = function () {
	document.getElementById('tableOfWifi').style.display = "none";
	document.getElementById('uniqueWifi').style.display = '';
	document.getElementById('uniqueWifi').display = 'block';
}

function EditWifiSetting(ssid) {
	//Update the form with data

	var found = false;
	for (var i = 0; i < obj1.wifiList.length; i++) {
		if (obj1.wifiList[i].ssid == ssid) {
			document.getElementById('curr_ssid').value = obj1.wifiList[i].ssid;
			document.getElementById('curr_pw').value = obj1.wifiList[i].pw;
			document.getElementById('_ip').value = obj1.wifiList[i].ip;
			document.getElementById('_subnet').value = obj1.wifiList[i].subnet;
			document.getElementById('_gateway').value = obj1.wifiList[i].gateway;
			found = true;
		}
	}
	if (found == false) {
		alert("Error!");
		return;
	}

	document.getElementById('tableOfWifi').style.display = "none";
	document.getElementById('uniqueWifi').style.display = '';
	document.getElementById('uniqueWifi').display = 'block';
}

function GoBack() {
	document.getElementById('uniqueWifi').style.display = "none";
	document.getElementById('tableOfWifi').style.display = '';
	document.getElementById('tableOfWifi').display = 'block';
}


//hide the text area
var toggleConfigJson = false;
document.getElementById('jsondump').style.display = "none";

function toggleConfigJsonF() {
	if (toggleConfigJson) {
		toggleConfigJson = false;
		document.getElementById('jsondump').style.display = "none";
	} else {
		document.getElementById('jsondump').style.display = '';
		document.getElementById('jsondump').display = 'block';
		document.getElementById('obj1').value = JSON.stringify(obj1, null, 2);
		toggleConfigJson = true;
	}
}

//Now generate the table dynamically
for (var i = 0; i < obj1.wifiList.length; i++) {
	var table = document.getElementById("wifiTable");
	let ssid1 = obj1.wifiList[i].ssid;

	var row = table.insertRow(1);
	// Insert new cells (<td> elements) at the 1st and 2nd position of the "new" <tr> element:
	var cell1 = row.insertCell(0);
	var cell2 = row.insertCell(1);
	var cell3 = row.insertCell(2);
	var cell4 = row.insertCell(3);

	// Add some text to the new cells:
	cell1.innerHTML = obj1.wifiList[i].ssid;
	cell2.innerHTML = obj1.wifiList[i].pw;

	cell3.innerHTML = "<a href='#'> Edit </a>";
	cell3.onclick = function () { EditWifiSetting(ssid1); };
	cell4.innerHTML = "<a href='#'> Delete </a>";
	cell4.onclick = function () { DeleteWifi(ssid1); };
}

//show the AP Mode settings
document.getElementById("curr_ap_ssid").value = obj1.APMode.apssid;
document.getElementById("curr_ap_pw").value = obj1.APMode.appw;

if (typeof (obj1.updates) != "undefined" && obj1.updates.isUpdateAvailable) {
	document.getElementById('currentBuild').innerHTML = "current:" + obj1.updates.currentBuild;
	document.getElementById('newBuild').innerHTML = "new:" + obj1.updates.newBuild;
	document.getElementById('newBuildLink').innerHTML = "link:" + obj1.updates.newBuildLink;
	document.getElementById('newBuildDesc').innerHTML += "<a href='" + obj1.updates.newBuildDesc + "' target='_blank'> read description </a>";
	document.getElementById('update').style.display = '';
	document.getElementById('update').display = 'block';
} else {
	document.getElementById('update').style.display = "none";
}

var hasMQTT = false;
console.log(obj1.Features);
for (var i=0;i<obj1.Features.length;i++) {

	if (obj1.Features[i] == "mqttClient") {
		hasMQTT = true;
		break;
	}
}
if (hasMQTT) {
	var mqtt = obj1.MQTT;
	document.getElementById('mqttServer').value = mqtt.server;
	document.getElementById('mqttPort').value = mqtt.port;
	document.getElementById('mqttUn').value = mqtt.username;
	document.getElementById('mqttPw').value = mqtt.password;
	document.getElementById('mqttSub').value = mqtt.subscribe;
	document.getElementById('mqttPub').value = mqtt.publish;
	document.getElementById('mqttClient').value = mqtt.clientid;
	document.getElementById('mqttconfig').value = 'block';
} else {
	document.getElementById('mqttconfig').style.display = "none";
}