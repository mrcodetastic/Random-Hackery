/**
 * @file      HttpsBuiltlnGet.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-11-29
 * @note
 * * Example is suitable for A7670X/A7608X/SIM7672 series
 * * Connect https://httpbin.org test get request
 * * Example uses a forked TinyGSM <https://github.com/lewisxhe/TinyGSM>, which will not compile successfully using the mainline TinyGSM.
 */
#define TINY_GSM_RX_BUFFER          1024 // Set RX buffer to 1Kb
#include <Arduino.h>

/*
WE DON'T GET ANY HEADER OR BODY!!>>>

Wait...
Initializing modem...
Modem Info: Manufacturer: SIMCOM INCORPORATED Model: SIM7670G-MNGV Revision: V1.9.01 IMEI: 864643060054915
Start modem...

SIM card online
Connecting to uk.lebara.mobi success
GPRS connected
Set network apn : uk.lebara.mobi
Wait for the modem to register with the network.
Online registration successful

Registration Status:1
Inquiring UE system information:LTE,Online,234-15,0x1833,4568092,258,EUTRAN-BAND7,2850,5,5,-6,-85,-79,33
Network IP:
Request URL : http://httpbin.org/get
Got some data??
Length: 226
HTTP Header :
HTTP body : 
-------------------------------------
Request URL : https://httpbin.org/get
Got some data??
Length: 226
HTTP Header :
HTTP body : 
-------------------------------------


OK

*/

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

//#include "utilities.h"
#define MODEM_BAUDRATE  (115200)
#define MODEM_RX_PIN    17
#define MODEM_TX_PIN    18

#ifndef TINY_GSM_MODEM_A76XXSSL
#define TINY_GSM_MODEM_A76XXSSL
#endif


#define SerialMon       Serial
#define SerialAT        Serial1

#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS  // if enabled it requires the streamDebugger lib
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

// It depends on the operator whether to set up an APN. If some operators do not set up an APN,
// they will be rejected when registering for the network. You need to ask the local operator for the specific APN.
// APNs from other operators are welcome to submit PRs for filling.
// #define NETWORK_APN     "CHN-CT"             //CHN-CT: China Telecom

const char *request_url[] = {
    "http://httpbin.org/get",  
    "https://httpbin.org/get"
};


// Your GPRS credentials, if any
const char apn[]      = "uk.lebara.mobi";
const char gprsUser[] = "wap";
const char gprsPass[] = "wap";

#define NETWORK_APN "uk.lebara.mobi"

void setup()
{
    // Set console baud rate
    SerialMon.begin(115200);
    SerialMon.println("Wait...");

    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
    SerialAT.println("Test message to modem...");  // Test if the modem is responding

    // Set GSM module baud rate
    TinyGsmAutoBaud(SerialAT, 115200, 115200);
    modem.setBaud(115200);

    delay(2000);

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    SerialMon.println("Initializing modem...");
    modem.restart();
    // modem.init();

    String modemInfo = modem.getModemInfo();
    SerialMon.print("Modem Info: ");
    SerialMon.println(modemInfo);
 

    // Check if the modem is online
    Serial.println("Start modem...");

    int retry = 0;
    while (!modem.testAT(1000)) {
        Serial.println(".");
        delay(1000);
    }
    Serial.println();

    // Check if SIM card is online
    SimStatus sim = SIM_ERROR;
    while (sim != SIM_READY) {
        sim = modem.getSimStatus();
        switch (sim) {
        case SIM_READY:
            Serial.println("SIM card online");
            break;
        case SIM_LOCKED:
            Serial.println("The SIM card is locked. Please unlock the SIM card first.");
            // const char *SIMCARD_PIN_CODE = "123456";
            // modem.simUnlock(SIMCARD_PIN_CODE);
            break;
        default:
            break;
        }
        delay(1000);
    }

  // GPRS connection parameters are usually set after network registration
  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isGprsConnected()) { SerialMon.println("GPRS connected"); }

#ifdef NETWORK_APN
    Serial.printf("Set network apn : %s\n", NETWORK_APN);
    modem.sendAT(GF("+CGDCONT=1,\"IP\",\""), NETWORK_APN, "\"");
    if (modem.waitResponse() != 1) {
        Serial.println("Set network apn error !");
    }
#endif

    // Check network registration status and network signal status
    int16_t sq ;
    Serial.println("Wait for the modem to register with the network.");
    RegStatus status = REG_NO_RESULT;
    while (status == REG_NO_RESULT || status == REG_SEARCHING || status == REG_UNREGISTERED) {
        status = modem.getRegistrationStatus();
        switch (status) {
        case REG_UNREGISTERED:
        case REG_SEARCHING:
            sq = modem.getSignalQuality();
            Serial.printf("[%lu] Signal Quality:%d\n", millis() / 1000, sq);
            delay(1000);
            break;
        case REG_DENIED:
            Serial.println("Network registration was rejected, please check if the APN is correct");
            return ;
        case REG_OK_HOME:
            Serial.println("Online registration successful");
            break;
        case REG_OK_ROAMING:
            Serial.println("Network registration successful, currently in roaming mode");
            break;
        default:
            Serial.printf("Registration Status:%d\n", status);
            delay(1000);
            break;
        }
    }
    Serial.println();


    Serial.printf("Registration Status:%d\n", status);
    delay(1000);

    String ueInfo;
    if (modem.getSystemInformation(ueInfo)) {
        Serial.print("Inquiring UE system information:");
        Serial.println(ueInfo);
    }
/*
    if (!modem.setNetworkActive()) {
        Serial.println("Enable network failed!");
    }

    delay(5000);
*/

    String ipAddress = modem.getLocalIP();
    Serial.print("Network IP:"); Serial.println(ipAddress);

    // Initialize HTTPS
    modem.https_begin();
    modem.https_begin();
    modem.https_begin();

    // If the status code 715 is returned, please see here
    // https://github.com/Xinyuan-LilyGO/LilyGO-T-A76XX/issues/117

    for (int i = 0; i < sizeof(request_url) / sizeof(request_url[0]); ++i) {

        int retry = 3;

        while (retry--) {

            Serial.print("Request URL : ");
            Serial.println(request_url[i]);

            // Set GET URT
            if (!modem.https_set_url(request_url[i])) {
                Serial.print("Failed to request : "); Serial.println(request_url[i]);

                // Debug
                // modem.sendAT("+CSSLCFG=\"enableSNI\",0,1");
                // modem.waitResponse();
                delay(3000);
                continue;
            }

            // Send GET request
            int httpCode = 0;
            httpCode = modem.https_get();
            if (httpCode != 200) {
                Serial.print("HTTP get failed ! error code = ");
                Serial.println(httpCode);
                delay(3000);
                continue;
            }

            // Get HTTPS header information
            String header = modem.https_header();
            Serial.print("HTTP Header : ");
            Serial.println(header);

            delay(1000);

            // Get HTTPS response
            String body = modem.https_body();
            Serial.print("HTTP body : ");
            Serial.println(body);

            delay(3000);

            break;
        }

        Serial.println("-------------------------------------");
    }
}

void loop()
{
    // Debug AT
    if (SerialAT.available()) {
        Serial.write(SerialAT.read());
    }
    if (Serial.available()) {
        SerialAT.write(Serial.read());
    }
    delay(1);
}

#ifndef TINY_GSM_FORK_LIBRARY
#error "No correct definition detected, Please copy all the [lib directories](https://github.com/Xinyuan-LilyGO/LilyGO-T-A76XX/tree/main/lib) to the arduino libraries directory , See README"
#endif