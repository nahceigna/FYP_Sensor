struct debugMode
{
    const uint8_t pin;       // switch used for switching between debug mode and operation mode
    const uint8_t WiFiLED;   // the green led
    const uint8_t SwitchLED; // the red led, to indicate switch is triggered
    bool isPressed;

    void init()
    {
        pinMode(pin, INPUT_PULLUP); // connect internal resistor for pullup
        digitalWrite(WiFiLED, LOW);
        pinMode(SwitchLED, OUTPUT);
        digitalWrite(SwitchLED, LOW);
        pinMode(WiFiLED, OUTPUT);
    }
};