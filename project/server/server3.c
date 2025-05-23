#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <wiringPi.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

#define PORT     12345
#define BACKLOG  5
#define BUF_SIZE 256

static int server_fd;

static void (*led_init)(), (*led_cleanup)(), (*led_on)(), (*led_off)();
static void (*led_high)(), (*led_mid)(), (*led_low)();

static void (*buzzer_init)(), (*buzzer_on)(), (*buzzer_off)(), (*buzzer_cleanup)();

static void (*sensor_init)(), (*sensor_cleanup)();
static int  (*read_light_sensor)();

static void (*countdownWithSound)(int);
static void (*clearSegment)();

void write_log(const char *format, ...) {
    FILE *log = fopen("/tmp/server_action.log", "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    va_list args;
    va_start(args, format);
    vfprintf(log, format, args);
    va_end(args);

    fprintf(log, "\n");
    fflush(log);
    fclose(log);
}

void handle_sigterm(int sig) {
    write_log("Received SIGTERM, shutting down...");
    if (server_fd > 0) close(server_fd);
    exit(0);
}

void daemonize() {
    if (fork() != 0) exit(EXIT_SUCCESS);
    setsid();
    if (fork() != 0) exit(EXIT_SUCCESS);
    umask(0);
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "a+", stdout);
    freopen("/dev/null", "a+", stderr);
}

struct schedule_data {
    char cmd[BUF_SIZE];
    int delay;
};

void *schedule_thread(void *arg) {
    struct schedule_data *data = (struct schedule_data *)arg;
    sleep(data->delay);
    write_log("Scheduled command executed: %s", data->cmd);

    if (strcmp(data->cmd, "LED_ON") == 0) led_on();
    else if (strcmp(data->cmd, "LED_OFF") == 0) led_off();
    else if (strcmp(data->cmd, "BUZZER_ON") == 0) buzzer_on();
    else if (strcmp(data->cmd, "BUZZER_OFF") == 0) buzzer_off();
    else if (strcmp(data->cmd, "LED_HIGH") == 0) led_high();
    else if (strcmp(data->cmd, "LED_MID") == 0) led_mid();
    else if (strcmp(data->cmd, "LED_LOW") == 0) led_low();
    else if (strlen(data->cmd) == 1 && isdigit(data->cmd[0]))
        countdownWithSound(data->cmd[0] - '0');
    else
        write_log("Scheduled unknown command: %s", data->cmd);

    free(data);
    return NULL;
}

void *client_thread(void *arg) {
    int sock = *(int *)arg;
    free(arg);

    char buf[BUF_SIZE];
    write(sock, "CONNECTED\n", 10);
    sleep(1);

    while (1) {
        int n = read(sock, buf, BUF_SIZE - 1);
        if (n <= 0) break;
        buf[n] = '\0';
        buf[strcspn(buf, "\r\n")] = '\0';

        for (int i = 0; buf[i]; i++) buf[i] = toupper(buf[i]);

        if (strcmp(buf, "LED_ON") == 0) { led_on(); write(sock, "OK\n", 3); write_log("LED_ON"); }
        else if (strcmp(buf, "LED_OFF") == 0) { led_off(); write(sock, "OK\n", 3); write_log("LED_OFF"); }
        else if (strcmp(buf, "LED_HIGH") == 0) { led_high(); write(sock, "OK\n", 3); write_log("LED_HIGH"); }
        else if (strcmp(buf, "LED_MID") == 0) { led_mid(); write(sock, "OK\n", 3); write_log("LED_MID"); }
        else if (strcmp(buf, "LED_LOW") == 0) { led_low(); write(sock, "OK\n", 3); write_log("LED_LOW"); }
        else if (strcmp(buf, "BUZZER_ON") == 0) { buzzer_on(); write(sock, "PLAYED\n", 7); write_log("BUZZER_ON"); }
        else if (strcmp(buf, "BUZZER_OFF") == 0) { buzzer_off(); write(sock, "OK\n", 3); write_log("BUZZER_OFF"); }
        else if (strcmp(buf, "LIGHT") == 0) {
            int value = read_light_sensor();
            if (value == 0) {
                led_on();
                write(sock, "LIGHT: DARK - LED ON\n", 22);
                write_log("LIGHT: DARK");
            } else {
                led_off();
                write(sock, "LIGHT: BRIGHT - LED OFF\n", 25);
                write_log("LIGHT: BRIGHT");
            }
        } else if (strncmp(buf, "SCHEDULE ", 9) == 0) {
            char cmd[BUF_SIZE];
            int delay;
            if (sscanf(buf + 9, "%s AFTER %d", cmd, &delay) == 2) {
                struct schedule_data *data = malloc(sizeof(struct schedule_data));
                strcpy(data->cmd, cmd); data->delay = delay;

                pthread_t sched_tid;
                pthread_create(&sched_tid, NULL, schedule_thread, data);
                pthread_detach(sched_tid);

                write(sock, "SCHEDULED\n", 10);
                write_log("Scheduled: %s after %d", cmd, delay);
            } else {
                write(sock, "INVALID_SCHEDULE_FORMAT\n", 25);
            }
        } else if (strlen(buf) == 1 && isdigit(buf[0])) {
            int num = buf[0] - '0';
            countdownWithSound(num);
            write(sock, "COUNTDOWN_DONE\n", 16);
            write_log("COUNTDOWN: %d", num);
        } else {
            write(sock, "UNKNOWN_CMD\n", 13);
            write_log("Unknown command: %s", buf);
        }
    }

    led_off();
    clearSegment();
    close(sock);
    write_log("Client disconnected");
    return NULL;
}

int main() {
    signal(SIGTERM, handle_sigterm);
    write_log("server3 starting...");
    write_log("Loading shared libraries...");

    daemonize();

    void *h_led = dlopen("/home/sejin/project/led/led.so", RTLD_NOW);
    if (!h_led) {
        write_log("dlopen led failed: %s", dlerror());
        exit(1);
    }

    void *h_light = dlopen("/home/sejin/project/light/light.so", RTLD_NOW);
    if (!h_light) {
        write_log("dlopen light failed: %s", dlerror());
        exit(1);
    }

    void *h_buzzer = dlopen("/home/sejin/project/buzzer/buzzer.so", RTLD_NOW);
    if (!h_buzzer) {
        write_log("dlopen buzzer failed: %s", dlerror());
        exit(1);
    }

    void *h_seg = dlopen("/home/sejin/project/7segment/7segment.so", RTLD_NOW);
    if (!h_seg) {
        write_log("dlopen 7segment failed: %s", dlerror());
        exit(1);
    }

    led_init = dlsym(h_led, "led_init");
    led_cleanup = dlsym(h_led, "led_cleanup");
    led_on = dlsym(h_led, "led_on");
    led_off = dlsym(h_led, "led_off");
    led_high = dlsym(h_led, "led_high");
    led_mid = dlsym(h_led, "led_mid");
    led_low = dlsym(h_led, "led_low");

    buzzer_init = dlsym(h_buzzer, "buzzer_init");
    buzzer_on = dlsym(h_buzzer, "buzzer_on");
    buzzer_off = dlsym(h_buzzer, "buzzer_off");
    buzzer_cleanup = dlsym(h_buzzer, "buzzer_cleanup");

    sensor_init = dlsym(h_light, "sensor_init");
    sensor_cleanup = dlsym(h_light, "sensor_cleanup");
    read_light_sensor = dlsym(h_light, "read_light_sensor");

    countdownWithSound = dlsym(h_seg, "countdownWithSound");
    clearSegment = dlsym(h_seg, "clearSegment");

    led_init();
    sensor_init();
    buzzer_init();

    write_log("Socket setup...");
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        write_log("socket() failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT)
    };

    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        write_log("bind failed");
        exit(1);
    }

    if (listen(server_fd, BACKLOG) < 0) {
        write_log("listen failed");
        exit(1);
    }

    write_log("Server ready for clients...");

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

    led_cleanup();
    sensor_cleanup();
    buzzer_cleanup();
    dlclose(h_led);
    dlclose(h_light);
    dlclose(h_buzzer);
    dlclose(h_seg);
    close(server_fd);

    return 0;
}

