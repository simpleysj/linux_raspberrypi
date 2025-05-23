
# TCP를 이용한 원격 장치 제어 시스템

### 개발소스코드(구현내용)
#### ① 멀티 프로세스 또는 스레드 이용한 장치 제어
1. 클라이언트 연결마다 새로운 스레드 생성 → 병렬 클라이언트 처리

```c
while (1) {
    int *client_sock = malloc(sizeof(int));
    *client_sock = accept(server_fd, NULL, NULL);
    if (*client_sock < 0) {
        write_log("accept failed");
        free(client_sock);
        continue;
    }

    pthread_t tid;
    pthread_create(&tid, NULL, client_thread, client_sock);
    pthread_detach(tid);
}
```

- 각 클라이언트의 연결 요청마다 새로운 스레드를 생성하고 `client_thread()`에서 처리
- 여러 클라이언트가 동시에 접속해 명령을 보내더라도 병렬로 처리됨

2. 예약 명령(SCHEDULE)도 별도 스레드로 분리 실행

```c
struct schedule_data {
    char cmd[BUF_SIZE];
    int delay;
};

void *schedule_thread(void *arg) {
    struct schedule_data *data = (struct schedule_data *)arg;
    sleep(data->delay);
    ...
    free(data);
    return NULL;
}
```

```c
else if (strncmp(buf, "SCHEDULE ", 9) == 0) {
    ...
    pthread_t sched_tid;
    pthread_create(&sched_tid, NULL, schedule_thread, data);
    pthread_detach(sched_tid);
}
```

- 클라이언트가 `SCHEDULE BUZZER_ON AFTER 5`와 같은 명령을 보낼 경우,
  해당 명령을 별도 스레드에서 일정 시간 후 실행
- 서버 메인 흐름을 차단하지 않고 병렬로 예약 작업을 수행 가능

---
#### ② 공유 라이브러리(.so) 구조
1. 동적 로딩 기반 장치 제어 (공유 라이브러리 사용)

서버는 각 장치별 제어 코드를 `.so` 형태의 **공유 라이브러리**로 분리하여, 런타임에 동적으로 불러오는 방식으로 구성

2. 동적 로딩 순서 및 구현

서버 실행 시 다음과 같이 각 장치에 대한 `.so` 파일을 **`dlopen()`** 으로 불러옴

```c
void *h_led = dlopen("/home/sejin/project/led/led.so", RTLD_NOW);
if (!h_led) {
    write_log("dlopen led failed: %s", dlerror());
    exit(1);
}
```

다음으로, 라이브러리에서 함수 포인터를 `dlsym()`으로 가져옵니다:

```c
led_init = dlsym(h_led, "led_init");
led_on   = dlsym(h_led, "led_on");
led_off  = dlsym(h_led, "led_off");
```

이 방식은 다른 장치(buzzer, light, 7segment)에도 동일하게 적용됩니다:

```c
void *h_light = dlopen("/home/sejin/project/light/light.so", RTLD_NOW);
read_light_sensor = dlsym(h_light, "read_light_sensor");

void *h_buzzer = dlopen("/home/sejin/project/buzzer/buzzer.so", RTLD_NOW);
buzzer_on = dlsym(h_buzzer, "buzzer_on");

void *h_seg = dlopen("/home/sejin/project/7segment/7segment.so", RTLD_NOW);
countdownWithSound = dlsym(h_seg, "countdownWithSound");
```

서버 종료 시 동적 라이브러리 해제

```c
dlclose(h_led);
dlclose(h_light);
dlclose(h_buzzer);
dlclose(h_seg);
```

---

#### ③ 강제 종료 방지를 위한 시그널 처리

서버는 **데몬 프로세스로 실행되기 때문에** 종료 시 리소스를 정리하고, 장치를 초기 상태로 되돌리는 것이 중요합니다. 이를 위해 **`SIGTERM` 시그널 처리 함수**를 등록하였습니다.

구현 코드

```c
#include <signal.h>

void handle_sigterm(int sig) {
    write_log("Received SIGTERM, shutting down...");
    if (server_fd > 0) close(server_fd);
    exit(0);
}

int main() {
    signal(SIGTERM, handle_sigterm);  // 종료 시그널 등록
    ...
}
```

* `SIGTERM` 발생 시 `handle_sigterm()` 함수가 호출되어 서버 소켓을 닫고, 로그를 남긴 뒤 정상 종료합니다.
* **systemctl stop server.service** 또는 시스템 종료 시 안전하게 종료되도록 설계되어 있습니다.

---


#### ④ 데몬 프로세스로 실행되는 서버 구성 (`server3` 기준)

* 이 프로젝트의 `server3`는 데몬 형태로 동작하도록 구현되어 있어, 시스템 시작과 함께 자동 실행되며 TCP 클라이언트의 명령을 지속적으로 대기합니다.

데몬화 구현 코드

서버는 내부에서 다음과 같이 **`daemonize()` 함수**를 호출하여 데몬 프로세스로 전환됩니다:

```c
void daemonize() {
    if (fork() != 0) exit(EXIT_SUCCESS);  // 부모 종료
    setsid();                             // 세션 리더 설정
    if (fork() != 0) exit(EXIT_SUCCESS);  // 세션 리더 종료
    umask(0);
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "a+", stdout);
    freopen("/dev/null", "a+", stderr);
}
```

---
systemd 유닛 파일 (`/etc/systemd/system/server3.service`)

```ini
[Unit]
Description=Server3 TCP Device Daemon
After=network.target

[Service]
Type=forking
ExecStart=/usr/local/bin/server3
Restart=on-failure
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```
---

#### ⑤ 서버용 Makefile (`project/Makefile3`)

**기능**: LED, 조도센서, 부저, 7세그먼트 제어용 `.so` 라이브러리와 데몬 서버(`server3`)를 빌드합니다.

```make
CC      := gcc
CFLAGS  := -Wall -fPIC
LDLIBS  := -ldl -lpthread -lwiringPi

.PHONY: all led light buzzer 7segment server3 clean

all: led light buzzer 7segment server3

led:
    $(CC) $(CFLAGS) -shared -o led/led.so led/led.c -lwiringPi

light:
    $(CC) $(CFLAGS) -shared -o light/light.so light/light.c -lwiringPi

buzzer:
    $(CC) $(CFLAGS) -shared -o buzzer/buzzer.so buzzer/buzzer.c -lwiringPi

7segment:
    $(CC) $(CFLAGS) -shared -o 7segment/7segment.so 7segment/7segment.c -lwiringPi

server3:
    $(CC) $(CFLAGS) -o server/server3 server/server3.c $(LDLIBS)

clean:
    rm -f led/led.so \
          light/light.so \
          buzzer/buzzer.so \
          7segment/7segment.so \
          server/server3
```

---

#### ⑥ 클라이언트용 Makefile (`client/Makefile`)

**기능**: 클라이언트 프로그램(`client`)을 빌드합니다.

```make
CC      := gcc
CFLAGS  := -Wall
LDLIBS  := -ldl -lpthread

.PHONY: all clean

all: client

client: client.c
    $(CC) $(CFLAGS) -o client client.c $(LDLIBS)

clean:
    rm -f client
```
좋습니다! 주신 코드들과 이미지 설명 내용을 기반으로 각 장치의 핵심 구현 내용만 정리한 `README.md` 형식의 섹션을 아래와 같이 구성해드릴게요:

---

#### ⑦ 장치별 기능 코드

1) LED 제어 (`led.c`)

* 클라이언트 명령으로 LED `ON/OFF` 및 밝기 3단계 조절 (최대/중간/최저)
* 소프트웨어 PWM(`softPwm`)을 이용해 밝기 제어 구현

```c
void led_on()  { softPwmWrite(LED_PIN, 100); }
void led_off() { softPwmWrite(LED_PIN, 0);   }
void led_high(){ softPwmWrite(LED_PIN, 100); }
void led_mid() { softPwmWrite(LED_PIN, 75);  }
void led_low() { softPwmWrite(LED_PIN, 50);  }
```

---

2) 부저 제어 (`buzzer.c`)

* 클라이언트에서 BUZZER\_ON 시 음악 멜로디 실행 (비동기 스레드)
* `softTone`과 `pthread` 조합으로 실행 중 중단 가능

```c
void buzzer_on() {
    buzzer_playing = 1;
    pthread_create(&buzzer_thread, NULL, buzzer_worker, NULL);
}
void buzzer_off() {
    buzzer_playing = 0;
    softToneWrite(BUZZER_PIN, 0);
}
```

---

3) 조도 센서 (`light.c`)

* 조도 센서 값을 읽고, 빛이 감지되지 않으면 자동으로 LED ON
* wiringPi 디지털 입력으로 구현

```c
int read_light_sensor() {
    return digitalRead(SENSOR_PIN);  // 0: 어두움 → LED ON
}
```

---

4) 7세그먼트 디스플레이 (`7segment.c`)

* 클라이언트에서 숫자(0\~9) 전송 시 디스플레이에 표시
* 1초마다 1씩 감소하는 **카운트다운 기능** 구현
* 0이 되면 종료 알림 부저음 출력

```c
void countdownWithSound(int num) {
    for (int i = num; i >= 0; i--) {
        displayNumber(i);
        sleep(1);
    }
    SoundWithClear();  // 0에서 부저 울림
}
```

---
## 서버 (Raspberry Pi) 빌드 및 실행 방법

### 1️⃣ 프로젝트 빌드
![image](https://github.com/user-attachments/assets/8da3cd3f-1974-4ceb-9071-0584da1bc8e0)
```bash
cd ~/project                      # 프로젝트 디렉토리로 이동
make -f Makefile3 clean           # 전체 모듈 및 서버 정리
make -f Makefile3                 # 전체 모듈 및 서버 컴파일
```

> 빌드 완료 후 `server/server3` 실행 파일과 각 모듈의 `.so` 파일이 생성됩니다.


### 2️⃣ 서버 실행 파일 및 서비스 등록
![image](https://github.com/user-attachments/assets/a516e398-8850-4f6b-a83c-17ffbe2867d0)

```bash
# 실행 파일 복사
sudo cp server/server3 /usr/local/bin/

# systemd 서비스 파일 복사
sudo cp server/server3.service /etc/systemd/system/

# systemd 서비스 재로드
sudo systemctl daemon-reexec
```


### 3️ 서버 데몬 등록 및 실행

```bash
# 부팅 시 자동 실행 설정
sudo systemctl enable server3.service

# 즉시 서버 실행
sudo systemctl start server3.service
```


### 4️⃣ 서버 상태 및 로그 확인
![image](https://github.com/user-attachments/assets/d5382b08-4245-4622-8aba-8016e4a910c8)

#### 1. 서버 상태 확인

```bash
sudo systemctl status server3.service
```

> 정상 동작 중일 경우 `Active: active (running)`으로 표시됩니다.

#### 2. 서버에서 로그 확인

```bash
tail -n 30 /tmp/server_action.log
```

> 클라이언트 연결, 명령 처리, 센서 감지 등 모든 활동 기록이 저장됩니다.


### 5️⃣ 서버 제어 명령 (필요 시)

```bash
# 서버 재시작
sudo systemctl restart server3.service

# 서버 중지
sudo systemctl stop server3.service
```

### 서버 주요 기능

* 클라이언트로부터 명령 수신 (`LED_ON`, `BUZZER_OFF`, 등)
* 조도 센서 상태에 따른 자동 LED 제어
* 예약 명령어 실행 (`SCHEDULE LED_ON AFTER 3`)
* 모든 동작 `/tmp/server_action.log`에 기록
* systemd 데몬 형태로 백그라운드 실행

---

## 클라이언트 (Ubuntu) 빌드 및 실행 방법

### 1️⃣ 클라이언트 디렉토리로 이동
![image](https://github.com/user-attachments/assets/741b1c7d-c84d-4099-ac93-441b3fcfa34e)

```bash
cd ~/client
```

### 2️⃣ 빌드

```bash
make
```

> `Makefile`을 이용해 `client.c`가 컴파일되고 실행 파일 `client`가 생성됩니다.

### 3️⃣ 클라이언트 실행

```bash
./client <서버_IP주소> <포트번호>
```

예시:

```bash
./client 192.168.0.10 12345
```

> Raspberry Pi에서 실행 중인 서버에 TCP로 연결됩니다.

---

### 사용 가능한 명령어

| 명령어                              | 설명                                      |
| -------------------------------- | --------------------------------------- |
| `LED_ON`, `LED_OFF`              | LED 켜기/끄기                               |
| `LED_HIGH`, `LED_MID`, `LED_LOW` | LED 밝기 단계 조절                            |
| `BUZZER_ON`, `BUZZER_OFF`        | 부저 켜기/끄기                                |
| `LIGHT`                          | 조도 센서 상태 확인 및 LED 자동 제어                 |
| `0`\~`9`                         | 입력한 숫자부터 카운트다운                          |
| `SCHEDULE <CMD> AFTER <초>`       | 명령 예약 실행 (`예: SCHEDULE LED_ON AFTER 3`) |
| `exit`                           | 클라이언트 종료                                |

### 안전 종료

```bash
exit OR Ctrl + C
```

> 인터럽트 시 자동으로 서버와의 소켓을 닫고 종료 메시지를 출력합니다.

---

위 내용을 `README.md`에 그대로 붙여 넣으면 일관성 있는 클라이언트 섹션이 완성됩니다.
다음으로 필요하신 내용은 `명령어 프로토콜 정리`, `문서 제출용 압축 구성`, 또는 `.md` 완성본 내보내기 등 도와드릴 수 있어요.


## 클라이언트(Ubuntu) 빌드 및 실행 방법
![image](https://github.com/user-attachments/assets/b5665dc4-1b20-4c14-974b-f5c5c00ab33f)
```bash
cd client
make
./client <라즈베리파이_IP주소> <포트>
```

예시:

```bash
./client 192.168.0.10 12345:
```

