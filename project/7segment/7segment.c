#include <wiringPi.h>
#include <softTone.h>
#include <stdio.h>
#include <unistd.h>

// BCD 핀 배열 (wiringPi 번호 기준)
int bcdPins[4] = {10, 13, 12, 14}; // GPIO 8, 9, 10, 11
#define BUZZER 2  // GPIO 27 → wiringPi 2

// 1. BCD 핀 초기화
void segmentInit(void) {
    for (int i = 0; i < 4; i++) {
        pinMode(bcdPins[i], OUTPUT);
        digitalWrite(bcdPins[i], LOW);  // common cathode: LOW = OFF
    }
}

// 2. 숫자 출력 (0~9)
void displayNumber(int num) {
    if (num < 0 || num > 9) return;

    for (int i = 0; i < 4; i++) {
        int bit = (num >> i) & 1;
        digitalWrite(bcdPins[i], bit);  // ✅ 반전 제거
    }
}

// 3. 모든 segment 끄기
void clearSegment(void) {
    for (int i = 0; i < 4; i++) {
        pinMode(bcdPins[i], OUTPUT);
        digitalWrite(bcdPins[i], LOW);  // LOW = OFF (common cathode)
    }
}

// 4. 종료 알림 사운드 + segment 끄기
void SoundWithClear(void) {
    for (int i = 0; i < 2; i++) {
        softToneWrite(BUZZER, 690);
        clearSegment();
        usleep(500000);

        softToneWrite(BUZZER, 610);
        clearSegment();
        usleep(500000);
    }
    softToneWrite(BUZZER, 0);
}

// 5. 외부 호출용 카운트다운 함수
void countdownWithSound(int num) {
    if (num < 0 || num > 9) return;

    segmentInit();
    softToneCreate(BUZZER);

    for (int i = num; i >= 0; i--) {
        displayNumber(i);
        sleep(1);
    }

    clearSegment();
    SoundWithClear();
}

