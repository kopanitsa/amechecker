"use strict";

const http = require('http');
let assert = require('assert');
let Amesh = require('./modules/amesh.js');
let amesh = new Amesh();

const server = http.createServer(serverCallback);
server.listen(8000);

function serverCallback(req, res) {
    let _res = res;
    req.on('data', (data) =>{
        const event = JSON.parse(data.toString());
        assert.notEqual(event.latitude, undefined);
        assert.notEqual(event.longtitude, undefined);
        assert.notEqual(event.accuracy, undefined);
        amesh.processWeather(event, (data) => {
            _res.writeHead(200, { 'Content-Type': 'application/json' });
            _res.end(data);
        });
    });
}
