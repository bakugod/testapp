	const express = require('express');
	const app = express();
	app.use("/img", express.static(__dirname + "/img"));
	app.get('/', (req, res) => {
		res.sendFile(__dirname + "/http.html");
	});
		app.listen(8080);