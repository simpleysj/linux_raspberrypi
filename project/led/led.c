#include <wiringPi.h>
#include <softPwm.h>
#include "led.h"

#define LED_PIN 1

void led_init() {
    wiringPiSetup();
    softPwmCreate(LED_PIN, 0, 100);
}

void led_cleanup() {
    // 필요시 정리
}

void led_on() {
    softPwmWrite(LED_PIN, 100);  // HIGH
}

void led_off() {
    softPwmWrite(LED_PIN, 0);    // OFF
}

void led_high() {
    softPwmWrite(LED_PIN, 100);
}

void led_mid() {
    softPwmWrite(LED_PIN, 75);
}

void led_low() {
    softPwmWrite(LED_PIN, 50);
}

