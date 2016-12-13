//
//  AmeBluetooth.swift
//  ameshble
//
//  Created by Takahiro Okada on 2016/12/13.
//  Copyright © 2016年 Takahiro Okada. All rights reserved.
//

import Foundation
import CoreBluetooth

class AmeBluetooth: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    
    private let AME_SERVICE = CBUUID(string: "C334A1D5-2D05-4865-B121-6DBC8BD37F31")
    private let AME_WEATHER_C12S = CBUUID(string: "2DCA3470-42F2-4FB0-AC9F-4203EB80B1EE")
    private let WEATHER_SUNNY = "sunny"
    private let WEATHER_RAINY = "rainy"
    
    var centralManager: CBCentralManager!
    var peripheral: CBPeripheral!
    
    enum Weather: UInt16 {
        case Sunny = 0x41
        case Rainy = 0x43
    }
    private var weather = 0x41
    
    
    func startScan() {
        self.centralManager = CBCentralManager(delegate: self, queue: nil)
    }
    
    func setWeather(weather: String) {
        if (weather == WEATHER_SUNNY) {
            print("set sunny value to bluetooth")
            self.weather = Int(Weather.Sunny.rawValue)
        } else if (weather == WEATHER_RAINY) {
            print("set rainy value to bluetooth")
            self.weather = Int(Weather.Rainy.rawValue)
        }
    }
    
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        switch (central.state) {
            
        case .unknown:
            print("BLE: unknown")
        case .resetting:
            print("BLE: uresetting")
        case .unsupported:
            print("BLE: unsupported")
        case .unauthorized:
            print("BLE: unauthorized")
        case .poweredOff:
            print("BLE: power off")
        case .poweredOn:
            print("BLE: power on")
            self.centralManager.scanForPeripherals(withServices: nil, options: nil)
        }
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        print("peripheral: \(peripheral)")

        print("peripheral: \(peripheral)")
        if (peripheral.name == "AME") {
            self.peripheral = peripheral
            self.centralManager.stopScan()
            self.centralManager.connect(peripheral, options: nil)
        }
    }
    
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        print("connected to peripheral")
        peripheral.delegate = self
        
        peripheral.discoverServices([AME_SERVICE])
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        print("failed to connect")
        // TODO retry
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let services = peripheral.services else{
            print("error to find service")
            return
        }
        for service in services {
            print("found service:\(service.uuid)")
            if (service.uuid.isEqual(AME_SERVICE)) {
                peripheral.discoverCharacteristics([AME_WEATHER_C12S], for: service)
            }
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard let c12ss = service.characteristics else {
            print("error to finc c12s")
            return
        }
        for c12s in c12ss {
            print("found c12s:\(c12s.uuid)")
            if (c12s.uuid.isEqual(AME_WEATHER_C12S)) {
                print("write:\(self.weather)")
                var value: CUnsignedInt = CUnsignedInt(self.weather)
                let data = NSData(bytes: &value, length: 1)
                peripheral.writeValue(data as Data, for: c12s, type: CBCharacteristicWriteType.withResponse)
            }
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        print("done. disconnect")
        self.centralManager.cancelPeripheralConnection(peripheral)
        
        if #available(iOS 10.0, *) {
            Timer.scheduledTimer(withTimeInterval: 60.0, repeats: false, block: { (Timer) in
                self.centralManager.scanForPeripherals(withServices: nil, options: nil)
            })
        } else {
            self.centralManager.scanForPeripherals(withServices: nil, options: nil)
        }
    }
    
}
