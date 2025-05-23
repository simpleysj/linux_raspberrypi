#include <wiringPi.h>
#include "light.h"

#define SENSOR_PIN 0  // wiringPi 0 → BCM17

void sensor_init() {
    wiringPiSetup();
    pinMode(SENSOR_PIN, INPUT);
}

void sensor_cleanup() {
    // 필요시 리소스 해제
}

int read_light_sensor() {
    return digitalRead(SENSOR_PIN);
}

