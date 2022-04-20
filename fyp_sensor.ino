/// import libraries
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "esp_wpa2.h" // wpa2 library for connecting to HKUST WiFi
#include <HTTPClient.h>
#include "classes/debugMode.h"

/// gate switch struct
struct gateSwitch
{
    const uint8_t pin;
    String currentFloor;
    bool isPressed;
};

/// WiFi details
// const char *SSID = "homewifi";
// const char *password = "homepassword";

/// connect to HKUST WiFi
const char *SSID = "eduroam";
#define EAP_ANONYMOUS_IDENTITY "user@connect.ust.hk"
#define EAP_IDENTITY "user@connect.ust.hk"
#define EAP_PASSWORD "password"

const static char *cert PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";

/// the length of server
const String serverName = "https://061239230208.ctinets.com/api/library/entrances";
unsigned int inflow = 1; // equal to 1 because only one person is allowed to get in per gate opening

/// variables
debugMode debugMode = {22, 18, 21, false};
struct gateSwitch lg1GateSwitch = {4, "LG1", false};

void IRAM_ATTR switchTriggeredAction()
{
    if (digitalRead(lg1GateSwitch.pin) == LOW && lg1GateSwitch.isPressed == false)
    {
        if (debugMode.isPressed)
        {
            Serial.println("Gate switch is pressed");
            digitalWrite(debugMode.SwitchLED, HIGH);
        }
        lg1GateSwitch.isPressed = true;
    }
}

void setup()
{
    Serial.begin(115200);

    /// initialise
    pinMode(lg1GateSwitch.pin, INPUT); // it is pull down in the circuit
    attachInterrupt(lg1GateSwitch.pin, switchTriggeredAction, FALLING);

    debugMode.init();

    /// Switch between the debug mode and operation mode
    if (digitalRead(debugMode.pin))
    {
        debugMode.isPressed = true;
        Serial.println("In debug Mode");
    }

    /// get MAC Address
    WiFi.disconnect(true); // disconnect the WiFi
    WiFi.mode(WIFI_STA);   // initialise WiFi
    esp_wifi_sta_wpa2_ent_set_ca_cert((uint8_t *)cert, strlen(cert) + 1);
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_ANONYMOUS_IDENTITY, strlen(EAP_ANONYMOUS_IDENTITY));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
    esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT(); //set config settings to default
    esp_wifi_sta_wpa2_ent_enable(&config);                 //set config settings to enable function

    /// Connect to WiFI
    WiFi.begin(SSID);
    /// for home WiFi
    // WiFi.begin(SSID, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        if (debugMode.isPressed)
        {
            Serial.print(".");
            digitalWrite(debugMode.WiFiLED, HIGH);
            delay(500); // in milliseconds
            digitalWrite(debugMode.WiFiLED, LOW);
            delay(500);
        }
        else
        {
            delay(500); // in milliseconds
        }
    }
    if (debugMode.isPressed)
    {
        digitalWrite(debugMode.WiFiLED, HIGH);
        Serial.println("");
        Serial.print("Connected to WiFi network with IP Address: ");
        Serial.println(WiFi.localIP());
    }
}

void loop()
{
    /// Detect if the switch is on or not
    if (lg1GateSwitch.isPressed)
    {
        delay(20); // avoid potential spike
        if (debugMode.isPressed)
        {
            Serial.println("Switch triggered, upload data");
        }
        /// Turn on the LED light
        postOccupancyData();
        lg1GateSwitch.isPressed = false;
        if (debugMode.isPressed)
        {
            digitalWrite(debugMode.SwitchLED, LOW);
        }
    }

    if (debugMode.isPressed)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            digitalWrite(debugMode.WiFiLED, HIGH);
        }
        else
        {
            digitalWrite(debugMode.WiFiLED, LOW);
        }
    }
}

void postOccupancyData()
{
    // if (debugMode.isPressed)
    // {
    //     Serial.println("In postOccupancyData function");
    // }

    /// make sure it is connected to WiFi
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.begin(serverName); // domain name

        /// indicate that content type is JSON
        http.addHeader("Content-Type", "application/json");

        /// create the data
        StaticJsonDocument<50> data;
        data["people"] = inflow;
        data["floor"] = lg1GateSwitch.currentFloor;

        String requestBody;
        serializeJson(data, requestBody); // format the data using ArduinoJSON

        int httpResponseCode = http.POST(requestBody); // get the response message
        // if (debugMode.isPressed)
        // {
        //     Serial.println(requestBody);
        //     /// check response from server
        //     Serial.print("HTTP Response code: ");
        //     Serial.println(httpResponseCode);
        //     if (httpResponseCode == 400 || httpResponseCode == 500)
        //     {
        //         Serial.println("POST Request failed");
        //     }
        //     else if (httpResponseCode == 200)
        //     {
        //         Serial.println("POST Request succeed");
        //     }
        // }

        /// Free resources
        http.end();
    }
}