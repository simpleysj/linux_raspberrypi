# 조도 센서 기반 원격 제어 시스템 (Light Sensor Remote Control System)

## 📌 프로젝트 개요

본 프로젝트는 Raspberry Pi에서 조도 센서를 이용해 주변 밝기를 감지하고, 특정 조건에 따라 부저를 작동시키거나 LED 및 7-Segment를 제어하는 데몬 서버를 구축한 시스템입니다. Ubuntu PC에서는 TCP 클라이언트를 통해 부저 등의 장치를 원격 제어할 수 있습니다.

---

## 🧰 사용 환경 및 기술 스택

- Raspberry Pi OS (서버)
- Ubuntu Linux (클라이언트)
- C 언어 (모듈화 및 시스템 제어)
- WiringPi (GPIO 제어)
- systemd (서버 데몬 등록)
- TCP 소켓 통신 (IPv4)
- 조도센서 (GL5528), 피에조 부저, LED, 7-Segment

---

## 🗂 시스템 구조 및 파일 구성

### 📍 Raspberry Pi (서버)

project/
├── Makefile
├── 7segment/
│ ├── 7segment.c
│ ├── 7segment.h
│ └── 7segment.so
├── buzzer/
│ ├── buzzer.c
│ ├── buzzer.h
│ └── buzzer.so
├── led/
│ ├── led.c
│ ├── led.h
│ └── led.so
├── light/
│ ├── light.c
│ ├── light.h
│ └── light.so
├── server/
│ ├── server3.c
│ └── server3 (컴파일된 데몬 실행파일)

shell
복사
편집

### 📍 Ubuntu (클라이언트)

client/
├── Makefile
└── client2.c

yaml
복사
편집

---

## 🛠️ Raspberry Pi: 서버 빌드 및 실행 방법

### 1. 의존성 설치
```bash
sudo apt update
sudo apt install wiringpi
2. 전체 빌드
bash
복사
편집
cd project
make
3. 데몬 등록 및 실행
bash
복사
편집
sudo cp server/server3 /usr/local/bin/
sudo cp server/server3.service /etc/systemd/system/
sudo systemctl daemon-reexec
sudo systemctl enable server3.service
sudo systemctl start server3.service
4. 로그 확인
bash
복사
편집
tail -n 30 /tmp/server_action.log
💻 Ubuntu: 클라이언트 빌드 및 실행 방법
bash
복사
편집
cd client
make
./client2 <라즈베리파이_IP주소> <포트번호>
예:

bash
복사
편집
./client2 192.168.0.10 12345
🧪 명령 예시 및 동작 방식
자동 작동
조도센서 값이 임계값 이하 → 부저 울림, LED 켜짐, 세그먼트 표시

클라이언트 수동 제어 명령어
명령어	기능
BUZZER_ON	부저 켜기
BUZZER_OFF	부저 끄기
LED_HIGH	밝은 LED 표시
LED_MID	중간 밝기 LED 표시
LED_LOW	어두운 LED 표시
SEGMENT_ON	세그먼트 카운트다운
SEGMENT_OFF	세그먼트 끄기

⚠️ 문제점 및 개선 방향
현재 임계 조도값은 코드에 하드코딩되어 있으며, 설정 파일화가 필요함

WiringPi는 공식 지원 중단됨 → pigpio나 GPIO sysfs로 교체 고려

클라이언트에 GUI를 추가해 사용자 친화성 향상 가능
