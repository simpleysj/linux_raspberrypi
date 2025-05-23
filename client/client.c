#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>

#define BUF_SIZE 512

int sockfd;

void handle_sigint(int sig) {
    if (sockfd > 0) close(sockfd);
    printf("\n[클라이언트 종료]\n");
    exit(0);
}

void print_menu() {
    printf("\n[사용 가능한 명령어]\n");
    printf("LED_ON       - LED 켜기\n");
    printf("LED_OFF      - LED 끄기\n");
    printf("LED_HIGH     - LED 밝게\n");
    printf("LED_MID      - LED 중간 밝기\n");
    printf("LED_LOW      - LED 어둡게\n");
    printf("BUZZER_ON    - 부저 켜기\n");
    printf("BUZZER_OFF   - 부저 끄기\n");
    printf("LIGHT        - 조도 센서 값 읽기\n");
    printf("[0~9]        - 7세그먼트 카운트다운 실행\n");
    printf("SCHEDULE <명령어> AFTER <초> - 일정시간 후 명령 예약 실행\n");
    printf("exit         - 클라이언트 종료\n\n");
}


int main(int argc, char *argv[]) {
    struct sockaddr_in serv;
    char buf[BUF_SIZE];

    if (argc != 3) {
        fprintf(stderr, "사용법: %s <서버 IP> <포트>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    signal(SIGINT, handle_sigint);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port   = htons(port);
    if (inet_pton(AF_INET, server_ip, &serv.sin_addr) <= 0) {
        perror("inet_pton");
        return 1;
    }

    if (connect(sockfd, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("connect");
        return 1;
    }

    int n = read(sockfd, buf, BUF_SIZE - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    } else {
        printf("[서버 연결 실패]\n");
        return 1;
    }

    print_menu();

    while (1) {
        printf("cmd> ");
        if (!fgets(buf, BUF_SIZE, stdin)) break;

        if (strncmp(buf, "exit", 4) == 0)
            break;

        if (write(sockfd, buf, strlen(buf)) < 0) {
            perror("write");
            break;
        }

        n = read(sockfd, buf, BUF_SIZE - 1);
        if (n <= 0) {
            printf("[서버 연결 끊김]\n");
            break;
        }

        buf[n] = '\0';
        printf("resp: %s", buf);
    }

    close(sockfd);
    return 0;
}

