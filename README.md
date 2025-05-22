
# TCP를 이용한 원격 장치 제어 시스템

Raspberry Pi 서버와 Ubuntu 클라이언트 간 TCP 통신을 통해 LED, 부저, 조도센서, 7세그먼트를 제어하는 원격 제어 시스템입니다.
장치 제어 코드는 모듈별 공유 라이브러리(`.so`)로 설계되었습니다.
데몬 형태로 실행되는 서버와 명령어를 전달하는 클라이언트 구조로 구성되어 있습니다.

## 프로젝트 개요

* **목표**: 조도 센서를 통해 주변 밝기를 감지하고, 클라이언트 명령 또는 센서 조건에 따라 다양한 장치를 제어합니다.
* **서버**: Raspberry Pi (TCP 서버 + 데몬 프로세스)
* **클라이언트**: Ubuntu (TCP 클라이언트)
* **제어 장치**: LED, 부저, 조도 센서, 7세그먼트
* **구성 방식**: 각 장치를 `.so` 형태의 동적 라이브러리로 분리하고, 서버에서 `dlopen` 방식으로 제어


## 사용 기술 및 환경

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

## 하드웨어 구성 예시
아래 사진은 Raspberry Pi와 센서 및 제어 장치가 실제로 연결된 구성입니다.
![circuit](https://github.com/user-attachments/assets/bb7ad799-9d7d-4da6-a183-1493dd44fec3)


## 디렉토리 및 파일 구조

### Raspberrypi (`server`)

> Raspberry Pi 환경에서 각 장치를 제어하며 클라이언트로부터 TCP 명령을 수신하고 실행하는 멀티스레드 기반의 서버 프로그램 디렉토리
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

### Ubuntu (`client`)

> Ubuntu 환경에서 서버에 접속하고 명령어를 전송하는 TCP 클라이언트 프로그램 디렉토리

```plaintext
client/
├── Makefile
└── client.c
```

---

## 서버 (Raspberry Pi) 빌드 및 실행 방법

### 1️⃣ 프로젝트 빌드

```bash
cd ~/project         # 프로젝트 디렉토리로 이동
make clean           # 전체 모듈 및 서버 정리
make                 # 전체 모듈 및 서버 컴파일
```

> 빌드 완료 후 `server/server3` 실행 파일과 각 모듈의 `.so` 파일이 생성됩니다.


### 2️⃣ 서버 실행 파일 및 서비스 등록

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

```bash
cd client
make
./client <라즈베리파이_IP주소> <포트>
```

예시:

```bash
./client 192.168.0.10 12345:
```

## 평가 항목 외 추가기능

<details>
<summary>클릭하여 펼치기</summary>

<br>

| 기능                                          | 설명                                                                  |
| ------------------------------------------- | ------------------------------------------------------------------- |
| **동적 명령 예약 실행**<br>`SCHEDULE ... AFTER ...` | 클라이언트가 명령어를 일정 시간 후에 실행하도록 예약 가능<br>예: `SCHEDULE LED_ON AFTER 5`    |
| **로그 기록 기능**<br>`/tmp/server_action.log`    | 모든 명령 처리 및 이벤트를 타임스탬프와 함께 로그 파일에 기록<br>→ 시스템 동작 추적 및 디버깅 용이         |
| **데몬 프로세스화**<br>(`systemd` 등록)              | 서버 프로그램을 `systemd` 서비스로 등록하여<br>백그라운드 실행 및 부팅 시 자동 시작 지원            |
| **명령어 유효성 검사 및 포맷 예외 처리**                   | 예약 명령 형식 오류 등 잘못된 입력에 대해<br>에러 메시지 응답 및 로그 기록 처리                    |
| **클라이언트 종료 시 자원 정리 처리**                     | 클라이언트 연결 종료 시 LED 및 세그먼트를 초기화<br>→ `led_off()`, `clearSegment()` 호출 |
| **명령어 입력 안내 메뉴 출력**                         | 클라이언트 프로그램 실행 시<br>사용 가능한 명령어를 보기 쉽게 안내 (`print_menu()`)            |

</details>

---

## ⚠ 문제점 및 향후 개선 방향

* 클라이언트 명령어 방식이 텍스트 기반이라 사용자 친화성이 낮음

* 예외 상황 처리 부족

* 설정 값(예: 조도센서 임계값 등)이 코드에 고정되어 있어 유지보수 불편

## ※ 향후 개선 방향
* 스레드 풀(Thread Pool) 또는 이벤트 기반 처리(epoll 등) 구조로 개선하여 리소스 효율 향상

* GUI 클라이언트 또는 웹 대시보드 구현으로 사용 편의성 강화

* 설정 파일 기반 시스템 구성 (.conf 파일 등으로 임계값, 포트 등 외부 설정화)


