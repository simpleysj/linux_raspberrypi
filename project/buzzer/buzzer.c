#include <wiringPi.h>
#include <softTone.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include "buzzer.h"

#define BUZZER_PIN 2

#define G  392
#define A  440
#define B  494
#define C  523
#define D  587
#define E  659
#define F  698
#define XX 0

#define BPM 100
#define NOTE_UNIT_MS (60000 / BPM / 4)

static int melody[] = {
    G, G, A, A, G, G, E,
    G, G, E, E, D,
    A, A, G, G, E, E, G,
    G, D, E, G, A, G
};

static uint8_t duration[] = {
    2, 2, 2, 2, 2, 2, 4,
    2, 2, 2, 2, 4,
    2, 2, 2, 2, 2, 2, 4,
    2, 2, 2, 2, 4
};

static pthread_t buzzer_thread;
static int buzzer_playing = 0;

void buzzer_init() {
    wiringPiSetup();
    softToneCreate(BUZZER_PIN);
    buzzer_playing = 0;
}

void *buzzer_worker(void *arg) {
    int len = sizeof(melody) / sizeof(int);
    for (int i = 0; i < len && buzzer_playing; i++) {
        int freq = melody[i];
        int dur = NOTE_UNIT_MS * duration[i];

        softToneWrite(BUZZER_PIN, freq);
        usleep(dur * 1000);
        softToneWrite(BUZZER_PIN, 0);
        usleep(dur * 1000 / 5);
    }

    softToneWrite(BUZZER_PIN, 0);
    buzzer_playing = 0;
    return NULL;
}

void buzzer_on() {
    if (buzzer_playing) return;

    buzzer_playing = 1;
    pthread_create(&buzzer_thread, NULL, buzzer_worker, NULL);
    pthread_detach(buzzer_thread);  // 자동 자원 회수
}

void buzzer_off() {
    buzzer_playing = 0;
    softToneWrite(BUZZER_PIN, 0);
}

void buzzer_cleanup() {
    buzzer_off();
}

