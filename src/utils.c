#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NUM_NEIGHBOURS 3
#define MATRIX_SIZE 10

#define TEMP_TIMER 5
#define HUMID_TIMER 7
#define AIR_QUAL_TIMER 10

typedef enum {
    SENSOR_UNKNOWN = -1,
    SENSOR_TEMP = 0,
    SENSOR_HUMIDITY = 1,
    SENSOR_AIR_QUALITY = 2
} SensorType;

SensorType StringToSensorType(const char *str) {
    if ((str == "") || (str == NULL)) return SENSOR_UNKNOWN;

    if (strcmp(str, "temperature") == 0) return SENSOR_TEMP;
    if (strcmp(str, "humidity") == 0)    return SENSOR_HUMIDITY;
    if (strcmp(str, "air_quality") == 0) return SENSOR_AIR_QUALITY;

    return SENSOR_UNKNOWN;
}

const char* SensorTypeToString(SensorType type) {
    switch (type) {
        case SENSOR_TEMP:        return "temperature";
        case SENSOR_HUMIDITY:    return "humidity";
        case SENSOR_AIR_QUALITY: return "air_quality";
        default:                 return "unknown";
    }
}