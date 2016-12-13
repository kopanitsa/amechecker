//
//  ViewController.swift
//  ameshble
//
//  Created by Takahiro Okada on 2016/11/29.
//  Copyright © 2016年 Takahiro Okada. All rights reserved.
//

import UIKit
import Alamofire
import SwiftyJSON
import RAMPaperSwitch
import CoreLocation

class AMEViewController: UIViewController, CLLocationManagerDelegate {

    private var bluetooth : AmeBluetooth!
    
    private var locationManager: CLLocationManager!
    private var latitude: CLLocationDegrees!
    private var longitude: CLLocationDegrees!
    
    @IBOutlet weak var connectLabel: UILabel!
    @IBOutlet weak var connectSwitch: RAMPaperSwitch!
    
    private let url: String = "https://xxxxxxxxxx.execute-api.ap-northeast-1.amazonaws.com/xxxxxxxxx"
    
    override func viewDidLoad() {
        super.viewDidLoad()

        // UI callbacks
        self.connectSwitch.animationDidStartClosure = {(onAnimation: Bool) in
            UIView.transition(with: self.connectLabel,
                              duration: self.connectSwitch.duration,
                              options: UIViewAnimationOptions.transitionCrossDissolve,
                              animations: {
                                self.connectLabel.textColor = onAnimation ? UIColor.white : UIColor.black
                                self.connectLabel.text = onAnimation ? "Connected" : "Disconnected"
            },
                              completion:nil)
            if (onAnimation) {
                self.getWeather();
            }
        }
        
        // initialize BLE
        bluetooth = AmeBluetooth()
        bluetooth.startScan()
        
        // initialize CoreLocation
        locationManager = CLLocationManager()
        locationManager.delegate = self
        locationManager.desiredAccuracy = kCLLocationAccuracyKilometer
        locationManager.distanceFilter = 1000
        longitude = CLLocationDegrees()
        latitude = CLLocationDegrees()
        locationManager.requestAlwaysAuthorization()
        locationManager.startUpdatingLocation()
        
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

    // MARK: - API
    
    // [GET] /weather
    // get JSON represent weather of current position
    func getWeather() {
        let params: Parameters = [
            "latitude": latitude,
            "longtitude": longitude,
            "accuracy": 1
        ]
        Alamofire.request(self.url, method: .post, parameters: params, encoding: JSONEncoding.default)
            .responseJSON { response in
                if let json = response.result.value {
                    print("JSON: \(json)")
                    self.bluetooth.setWeather(weather: "sunny")
                }
        }
    }
    
    // MARK: - CLLocationManagerDelegate (GPS)
    
    // delegate to aquire current location
    // call when current location is updated
    func locationManager(_ manager: CLLocationManager, didUpdateLocations locations: [CLLocation]) {
        let newLocation = locations.last
        latitude = newLocation!.coordinate.latitude
        longitude = newLocation!.coordinate.longitude
        NSLog("latiitude: \(latitude) , longitude: \(longitude)")

        // get weather when location is identified
        getWeather()
    }
}

