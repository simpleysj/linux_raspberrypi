#ifndef LIGHT_H
#define LIGHT_H

// 센서 초기화/정리
void sensor_init();
void sensor_cleanup();

// 조도 값 읽기 (0=어두움, 1=밝음)
int read_light_sensor();

#endif // LIGHT_H

