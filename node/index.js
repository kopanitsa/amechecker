'use strict';

let Amesh = require('./modules/amesh.js');
let amesh = new Amesh();

/**
 * api : /weather
 * type : POST
 * request : {"latityde": hoge, "landitude" : fuga, "accuracy" : 1}
 * response : {"weather" : weather, "accuracy" : 1}
 */
exports.handler = (event, context, callback) => {
    console.log('Received event:' + JSON.stringify(event, null, 2));
    amesh.processWeather(event, (weather) => {
        console.log('weather:' + weather);
        callback(null, weather);
    });
};
