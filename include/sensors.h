#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>

#define MATRIX_SIZE 10
#define NUM_NEIGHBOURS 3
#define MAX_SUBSCRIBERS MATRIX_SIZE*MATRIX_SIZE

#define TEMP_TIMER 5
#define HUMID_TIMER 7
#define AIR_QUAL_TIMER 10

// Definição da struct do sensor
struct SensorMessage {
    char type[12];
    int coords[2];
    float measurement;
};
typedef struct SensorMessage Sensor;

typedef enum {
    SENSOR_UNKNOWN = -1,
    SENSOR_TEMP = 0,
    SENSOR_HUMIDITY = 1,
    SENSOR_AIR_QUALITY = 2
} SensorType;


typedef enum {
    NOT_NGB = 0,
    IS_NGB = 1,
    SAME_LOC = 2,
    REMOVED = 3,
    SERVER = 4
} MsgAction;


SensorType StringToSensorType(const char *str);
const char* SensorTypeToString(SensorType type);
double clampf(float val, float min, float max);

Sensor InitEmptySensor(void);
void InitNgbsVec(Sensor neighbours[]);
void RemoveSensorFromGrid(int coords[2], Sensor sensor_grid[MATRIX_SIZE][MATRIX_SIZE]);
int InsertSensorInGrid(Sensor s, Sensor sensor_grid[MATRIX_SIZE][MATRIX_SIZE]);
void InitSensorGrid(Sensor sensor_grid[MATRIX_SIZE][MATRIX_SIZE]);
void InitSensMeasure(Sensor *s);
void ConfigSensorVals(Sensor *s, SensorType s_type, int coordX, int coordY);
float CalcEuclidianDistance(Sensor s1, Sensor s2);
float CalcNewMeasure(Sensor s, Sensor neighbours[]);
MsgAction CalcNewPossibleNgb(Sensor s_main, Sensor s_candidate, Sensor neighbours[NUM_NEIGHBOURS]);
float CheckMeasureLimits(SensorType type, float measure);
void UpdateMeasure(Sensor *s, Sensor neighbours[]);
void PrintClientLogMsg(Sensor s_main, Sensor s_received, MsgAction action_rcv, float measure_correction);
void PrintServerLogMsg(Sensor s_received);


#endif // SENSORS_H