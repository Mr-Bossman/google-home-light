const light = require("./pins")

light.InitLight('14','15');
let state = true;
setInterval(() => {
	state = state ? !state : state
	light.SetLight(state);
}, 4000);