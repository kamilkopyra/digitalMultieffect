#include <Encoder.h>

#define ENC_SW1 4
#define ENC_SW2 7
#define ENC_SW3 10

Encoder enc1(2, 3);
Encoder enc2(5, 6);
Encoder enc3(8, 9);

long lastPos[3] = {0, 0, 0};

void checkButton(int idx, int pin) {
    static int lastState[3] = {HIGH, HIGH, HIGH};
    static unsigned long lastTime[3] = {0, 0, 0};
    int state = digitalRead(pin);
    if (state == LOW && lastState[idx] == HIGH && millis() - lastTime[idx] > 50) {
        Serial.print("B ");
        Serial.println(idx);
        lastTime[idx] = millis();
    }
    lastState[idx] = state;
}

void checkEncoder(int idx, long pos) {
    long delta = pos - lastPos[idx];
    if (abs(delta) >= 2) {
        int dir = delta > 0 ? 1 : -1;
        Serial.print("E ");
        Serial.print(idx);
        Serial.print(" ");
        Serial.println(dir);
        lastPos[idx] = pos;
    }
}
void setup() {
    Serial.begin(9600);
    pinMode(ENC_SW1, INPUT_PULLUP);
    pinMode(ENC_SW2, INPUT_PULLUP);
    pinMode(ENC_SW3, INPUT_PULLUP);
}

void loop() {
    checkEncoder(0, enc1.read());
    checkEncoder(1, enc2.read());
    checkEncoder(2, enc3.read());
    checkButton(0, ENC_SW1);
    checkButton(1, ENC_SW2);
    checkButton(2, ENC_SW3);
}
