
# TCP를 이용한 원격 장치 제어 시스템

Raspberry Pi 서버와 Ubuntu 클라이언트 간 TCP 통신을 통해 LED, 부저, 조도센서, 7세그먼트를 제어하는 원격 제어 시스템입니다.
장치 제어 코드는 모듈별 공유 라이브러리(`.so`)로 설계되었습니다.
데몬 형태로 실행되는 서버와 명령어를 전달하는 클라이언트 구조로 구성되어 있습니다.

## 🛠️ 프로젝트 개요

* **목표**: 조도 센서를 통해 주변 밝기를 감지하고, 클라이언트 명령 또는 센서 조건에 따라 다양한 장치를 제어합니다.
* **서버**: Raspberry Pi (TCP 서버 + 데몬 프로세스)
* **클라이언트**: Ubuntu (TCP 클라이언트)
* **제어 장치**: LED, 부저, 조도 센서, 7세그먼트
* **구성 방식**: 각 장치를 `.so` 형태의 동적 라이브러리로 분리하고, 서버에서 `dlopen` 방식으로 제어


## 💻 사용 기술 및 환경

* **OS**: Raspberry Pi OS (Server), Ubuntu (Client)
* **언어**: C
* **라이브러리**:

  * WiringPi
  * POSIX Thread
  * systemd (데몬)
  * `dlopen` (동적 로딩)
* **통신 방식**: TCP IPv4
* **제어 방식**: 각 장치별 공유 라이브러리 (`.so`) 로드 및 실행
* **실행 구조**:

  * 데몬 (Server)
  * CLI (Client)

## 🔧 하드웨어 구성 예시
아래 사진은 Raspberry Pi와 센서 및 제어 장치가 실제로 연결된 구성입니다.
![circuit](https://github.com/user-attachments/assets/bb7ad799-9d7d-4da6-a183-1493dd44fec3)


## 📁 디렉토리 및 파일 구조

```
project/
├── Makefile
├── 7segment/
│   ├── 7segment.c
│   ├── 7segment.h
│   └── 7segment.so
├── buzzer/
│   ├── buzzer.c
│   ├── buzzer.h
│   └── buzzer.so
├── led/
│   ├── led.c
│   ├── led.h
│   └── led.so
├── light/
│   ├── light.c
│   ├── light.h
│   └── light.so
├── server/
│   ├── server3.c
│   └── server3
```


---

### 🟠 Ubuntu (`client/`)

> Ubuntu 환경에서 서버에 접속하고 명령어를 전송하는 TCP 클라이언트 프로그램 디렉토리

```plaintext
client/
├── Makefile
└── client.c
```

---
⚙️ 서버 (Raspberry Pi) 빌드 및 실행 방법
1️⃣ WiringPi 설치 (GPIO 제어 라이브러리)
bash
복사
편집
sudo apt update
sudo apt install wiringpi
참고: Raspberry Pi OS 최신 버전에서는 기본 제공될 수 있습니다.
설치 후 gpio -v 명령으로 정상 설치 여부 확인 가능

2️⃣ 프로젝트 빌드
bash
복사
편집
cd ~/project   # 프로젝트 디렉토리로 이동
make           # 전체 모듈 및 서버 컴파일
빌드 후 server/server3 실행 파일과 .so 라이브러리들이 생성됩니다.

3️⃣ 서버 실행 파일 설치 및 서비스 등록
bash
복사
편집
# 실행 파일을 시스템 경로에 복사
sudo cp server/server3 /usr/local/bin/

# 서비스 파일을 systemd 디렉토리에 복사
sudo cp server/server3.service /etc/systemd/system/

# systemd에 새 서비스 반영
sudo systemctl daemon-reexec
4️⃣ 서버 데몬 자동 실행 설정 및 시작
bash
복사
편집
# 부팅 시 자동 시작 설정
sudo systemctl enable server3.service

# 서버 즉시 시작
sudo systemctl start server3.service
5️⃣ 서버 동작 확인
✅ 상태 확인
bash
복사
편집
sudo systemctl status server3.service
정상일 경우 Active: active (running)으로 표시됩니다.

✅ 실시간 로그 확인
bash
복사
편집
tail -n 30 /tmp/server_action.log
클라이언트 접속 및 장치 제어 기록이 이 로그 파일에 남습니다.

6️⃣ 서버 재시작 및 중지 명령 (필요 시)
bash
복사
편집
# 서버 재시작
sudo systemctl restart server3.service

# 서버 중지
sudo systemctl stop server3.service
💡 서버가 하는 일 요약
클라이언트 명령어 수신 (LED_ON, BUZZER_OFF, 등)

조도센서 자동 감지 및 장치 제어

예약 명령(SCHEDULE BUZZER_ON AFTER 3) 처리

모든 활동 로그 /tmp/server_action.log에 기록

데몬 형태로 백그라운드 실행됨 (systemd 기반)




## ⚙️ 서버(Raspberry Pi) 빌드 및 실행 방법

### 1. 의존성 설치
```bash
sudo apt update
sudo apt install wiringpi
````

### 2. 전체 빌드

```bash
cd project
make
```

### 3. 데몬 등록 및 실행

```bash
sudo cp server/server3 /usr/local/bin/
sudo cp server/server3.service /etc/systemd/system/
sudo systemctl daemon-reexec
sudo systemctl enable server3.service
sudo systemctl start server3.service
```

### 4. 서버 로그 확인

```bash
tail -n 30 /tmp/server_action.log
```

---

## 💻 클라이언트(Ubuntu) 빌드 및 실행 방법

```bash
cd client
make
./client <라즈베리파이_IP주소> <포트>
```

예시:

```bash
./client 192.168.0.10 12345:
```

---

## 🔌 클라이언트 → 서버 명령어 프로토콜

| 명령어           | 기능 설명                   |
| ------------- | ----------------------- |
| `BUZZER_ON`   | 부저 켜기                   |
| `BUZZER_OFF`  | 부저 끄기                   |
| `LED_HIGH`    | 밝은 조도 LED 점등            |
| `LED_MID`     | 중간 조도 LED 점등            |
| `LED_LOW`     | 어두운 조도 LED 점등           |
| `SEGMENT_ON`  | 7세그먼트 카운트다운 시작 (10 → 0) |
| `SEGMENT_OFF` | 7세그먼트 종료                |

---

## 🧪 실행 예시

* 클라이언트에서 `BUZZER_ON` 전송 → 라즈베리파이에서 부저 작동
* 조도센서 값이 임계값 이하일 경우 자동으로 부저 작동
* 클라이언트가 연결되지 않아도 서버는 자동 감지 기능 수행

---

## ⚠️ 문제점 및 향후 개선 방향

* 조도센서 임계값이 고정되어 있음 → 설정값으로 분리 필요
* WiringPi는 더 이상 공식 지원되지 않음 → `pigpio` 또는 `gpiochip` 기반으로 개선 예정
* 클라이언트에 GUI 또는 웹 기반 인터페이스 추가 가능

---

## 🗃 제출 문서 구성

* `개발문서`: 개요, 일정, 구현 내용, 문제점, 보완 사항 포함
* `README.md`: 전체 코드 설명 및 빌드 방법 포함
* `실행과정.txt`: 실행 명령어 및 결과 기록

---
위 내용을 `README.md`로 저장하시면 완벽한 제출용 문서가 됩니다.  
`.md` 파일로 따로 받아보고 싶으시면 말씀해주세요!  
추가하거나 수정하고 싶은 내용이 있다면 언제든지 알려주세요.
```
