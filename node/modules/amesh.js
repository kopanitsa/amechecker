"use strict";

const request = require('request');
const im = require('imagemagick-stream');
const async = require('async');
const assert = require('assert');
let _this;

class Amesh {
    constructor(){
        _this = this;
    }

    processWeather(data, cb) {
        const event = data;
        // 天気をasyncで取得する。
        async.waterfall([
            // まず現在地が画像上のどこに当たるかを計算し
            (callback) => {
                let pos = _this.getMapPosition(event.latitude, event.longtitude);
                callback(null, pos);
            },
            // 次に該当箇所の画像を切り出して雨が降ってるかを計算し
            (pos, callback) => {
                _this.getWeather(pos, callback);
            },
            // データを返す。
            (w, callback) => {
                const weather = w;
                cb({'weather':weather, 'accuracy':event.accuracy });
                // cb(JSON.stringify({'weather':weather, 'accuracy':event.accuracy }));
            }
        ]);
    }

    // amesh viewの表示範囲は
    //   N35° 8'15" E140°33'56" ~
    //   N36°13' 6" E138°21'51"
    // 表示範囲を10進数で表すと
    //   (35.1375, 140.5655) ~
    //   (36,2183, 138.3642)
    // 画像サイズは w770 x h480
    getMapPosition(latitude, longtitude) {
        assert.equal(latitude >= 35.1375 && latitude <= 36.2183, true, "latitude is wrong:" + latitude);
        assert.equal(longtitude >= 138.3642 && longtitude <= 140.5655, true, "longtitude is wrong:" + longtitude);

        const ax = 770 / (140.5655 - 138.3642);
        const ay = 480 / (36.2183 - 35.1375);
        let x = ax * (longtitude - 138.3642);
        let y = ay * (latitude - 35.1375);
        x = x < 10 ? 10 : x;
        y = y < 10 ? 10 : y;
        x = x > 760 ? 760 : x;
        y = y > 470 ? 470 : y;
        return {"x": Math.floor(x), "y": Math.floor(y)};
    }

    getMapUrl(now) {
        const Y = now.getFullYear();
        const M = now.getMonth() + 1;
        const D = now.getDate();
        const h = now.getHours();
        const _h = h < 9 ? "0" + h : h;
        const m = now.getMinutes();
        let _m = Math.floor(m / 5) * 5;
        _m = (_m % 5 === 0) ? _m - 5 : _m;
        _m = (_m < 9) ? "0"+_m : _m;
        return 'http://tokyo-ame.jwa.or.jp/mesh/000/' + Y + M + D + _h + _m + '.gif';
    }

    getWeather(pos, callback) {
        // 東京アメッシュからデータを取得する
        const path = _this.getMapUrl(new Date());
        console.log(path);
        const read = request.get(path);
        const x = pos.x - 10;
        const y = pos.y - 10;
        const croppos = "20x20+" + x + "+" + y;
        // const convert = im().set('crop', croppos).outputFormat('bmp3');
        const convert = im().set('crop', croppos).outputFormat('bmp');
        // debug
        // const fs = require('fs');
        // const write = fs.createWriteStream('debug.bmp');
        // let conv = read.pipe(convert).pipe(write);
        let conv = read.pipe(convert);

        // 色が0でなければ雨が降っている。
        conv.on('data', (data) => {
            const buf = Buffer(data);
            let sum = 0;
            for (var i = buf[10]; i < buf.length; i++) {
                sum += buf[i];
            }
            let weather = sum === 0 ? "sunny" : "rainy";
            callback(null, weather);
        });
    }

}

module.exports = Amesh;
