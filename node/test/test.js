"use strict";

// test target
const Amesh = require('../modules/amesh.js');
const amesh = new Amesh();

// other modules
const moment = require('moment');

// TAP framework
const test = require('tape');
test('amesh getMapUrl', (t) => {
    const time = moment("2016-11-27 22:43");
    t.equal(amesh.getMapUrl(time.toDate()),
        "http://tokyo-ame.jwa.or.jp/mesh/000/201611272235.gif");
    t.end();
});
test('amesh getMapPosition', (t) => {
    t.equal(amesh.getMapPosition(35.1375,140.5655).x, 760);
    t.equal(amesh.getMapPosition(35.1375,140.5655).y, 10);
    t.equal(amesh.getMapPosition(36.2183,138.3642).x, 10);
    t.equal(amesh.getMapPosition(36.2183,138.3642).y, 470);
    t.throws(() => {
        amesh.getMapPosition(36,142);
    });
    t.end();
});
test('amesh getWeather', (t) => {
    t.plan(1);
    const p = {"x": 300, "y": 300};
    amesh.getWeather(p, (a, weather) => {
        console.log(weather);
        t.pass("done");
    });
});
