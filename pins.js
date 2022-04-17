const fs = require('fs')

let global_led_state = undefined;
let pinB_ = undefined;
let pinA_ = undefined;

const pinMode = (pin,mode) => {
	try {
		fs.writeFileSync('/sys/class/gpio/export', pin.toString());
		fs.writeFileSync('/sys/class/gpio/gpio'+ pin.toString() + '/direction', mode);
	} catch (err) {
		console.error(err);
	}
}

const pinRead = (pin) => {
	try {
		return Number(fs.readFileSync('/sys/class/gpio/gpio'+ pin.toString() + '/value'));
	} catch (err) {
		console.error(err);
	}
}

const pinWrite = (pin,val) => {
	try {
		fs.writeFileSync('/sys/class/gpio/gpio'+ pin.toString() + '/value', val.toString());
	} catch (err) {
		console.error(err);
	}
}

const InitLight = (pinA,pinB) => {
	pinA_ = pinA;
	pinB_ = pinB;
	pinMode(pinB_,'in');
	pinMode(pinA_,'out');
}

const SetLight = (val) => {
	if(val == pinRead(pinB_))
		return;
	pinWrite(pinA_,1);
	setTimeout(() => {
		pinWrite(pinA_,0);
	}, 100);
}

const SetLightPoll = (callback) => {
	setInterval(() => {
		let current_state = pinRead(pinB_);
		if (current_state != global_led_state)
			callback(current_state == true);
		global_led_state = current_state;
	} ,500);
}

exports.InitLight = InitLight;
exports.SetLightPoll = SetLightPoll;
exports.SetLight = SetLight;
