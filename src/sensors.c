#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <time.h>
#include <stdbool.h>

#include "../include/sensors.h"

//TODO: tem que ser criada na main
Sensor sensor_grid[MATRIX_SIZE][MATRIX_SIZE];


// ==================== GENERAL PURPOSE FUNCTIONS ===============================
double clampf(float val, float min, float max){
    return fmax(min, fmin(val, max));
}

SensorType StringToSensorType(const char *str){
    if ((str == "") || (str == NULL)) return SENSOR_UNKNOWN;

    if (strcmp(str, "temperature") == 0) return SENSOR_TEMP;
    if (strcmp(str, "humidity") == 0)    return SENSOR_HUMIDITY;
    if (strcmp(str, "air_quality") == 0) return SENSOR_AIR_QUALITY;

    return SENSOR_UNKNOWN;
}

const char* SensorTypeToString(SensorType type){
    switch (type) {
        case SENSOR_TEMP:        return "temperature";
        case SENSOR_HUMIDITY:    return "humidity";
        case SENSOR_AIR_QUALITY: return "air_quality";
        default:                 return "unknown";
    }
}
//==========================================================================


//=============================== FUNCTIONAL ===============================

Sensor InitEmptySensor(){
    Sensor s;
    strcpy(s.type, "");    
    s.coords[0] = 0;
    s.coords[1] = 0;
    s.measurement = 0.0f;

    return s;
}

void InitNgbsVec(Sensor neighbours[]){
    
    for(unsigned i=0;i<NUM_NEIGHBOURS;i++){
        neighbours[i] = InitEmptySensor();
    }
}

void RemoveSensorFromGrid(int coords[2],
                          Sensor sensor_grid[MATRIX_SIZE][MATRIX_SIZE]){
    Sensor s_init;
    s_init = InitEmptySensor();

    sensor_grid[coords[0]][coords[1]] = s_init;

}

int InsertSensorInGrid(Sensor s,Sensor sensor_grid[MATRIX_SIZE][MATRIX_SIZE]){
    unsigned row, col;

    if(s.coords[0] < 0 || s.coords[1] < 0 ||
       s.coords[0] > 9 || s.coords[1] > 9) {
        printf("ERROR: sensor coords must be in a range from 0 to 9");
        return -1;
    }
    row = s.coords[0];
    col = s.coords[1];

    sensor_grid[row][col] = s;
    return 0;
}

// TEM QUE SER A PRIMEIRA COISA QUE A MAIN FAZ
void InitSensorGrid(Sensor sensor_grid[MATRIX_SIZE][MATRIX_SIZE]){
    unsigned row, col;
    Sensor s_init;
    s_init = InitEmptySensor();
    
    for(row=0;row<MATRIX_SIZE;row++){
        for(col=0;col<MATRIX_SIZE;col++){
            sensor_grid[row][col] = s_init;
        }
    }
}


void InitSensMeasure(Sensor *s) {
    SensorType sensor_type = StringToSensorType(s->type);

    switch (sensor_type) {
        case SENSOR_TEMP:
            s->measurement = (rand()%21) + 20; //random 20 e 40
            break;
        case SENSOR_HUMIDITY:
            s->measurement = (rand()%81) + 10; //random 10 e 90
            break;
        case SENSOR_AIR_QUALITY:
            s->measurement = (rand()%31) + 15; //random 15 e 30
            break;
        default:
            s->measurement = 0;
            break;
        }
}


void ConfigSensorVals(Sensor *s, SensorType s_type, int coordX, int coordY){
    strcpy(s->type, SensorTypeToString(s_type));
    s->coords[0] = coordX;
    s->coords[1] = coordY;
    InitSensMeasure(s);
}


float CalcEuclidianDistance(Sensor s1, Sensor s2){
    float dist;

    // Garantindo que um sensor vazio nao vai quebrar a medicao
    if((StringToSensorType(s1.type) == SENSOR_UNKNOWN) || 
       (StringToSensorType(s2.type) == SENSOR_UNKNOWN)) return FLT_MAX;

    dist = sqrt(pow((s1.coords[0]-s2.coords[0]),2) +
                    pow((s1.coords[1]-s2.coords[1]),2));

    return dist;
}

float CalcNewMeasure(Sensor s, Sensor neighbours[]){
    float act_val = s.measurement;
    float ngb_val, dist;
    unsigned i;

    for (i=0;i<NUM_NEIGHBOURS;i++) {
        Sensor act_neighbour = neighbours[i]; 
        ngb_val = act_neighbour.measurement;

        dist = CalcEuclidianDistance(s, act_neighbour);

        act_val = act_val + (0.1f*(1.0f/(dist+1.0f))) * (ngb_val - act_val);
    }

    return act_val;
}


//TODO: ESTA FUNCAO VAI SER ADAPTADA PRA VIRAR DO SERVIDOR TALVEZ
// void ScanNeighbours(const Sensor *s_main, 
//                           Sensor sensor_grid[MATRIX_SIZE][MATRIX_SIZE],
//                           Sensor neighbours[NUM_NEIGHBOURS]) {
//     unsigned row, col, i, k;
//     float dist;
//     float neighbours_dist[NUM_NEIGHBOURS];
    
//     // inicializando o vetor com valor "infinito"
//     for(i=0;i<NUM_NEIGHBOURS;i++){
//         neighbours_dist[i] = FLT_MAX;
//     }

//     for(row=0;row<MATRIX_SIZE;row++){
//         for(col=0;col<MATRIX_SIZE;col++){
//             Sensor *s_possible = &sensor_grid[row][col];
            
//             if(s_possible->type[0] == '\0'){
//                 continue;
//             }

//             if(s_possible->coords[0] == s_main->coords[0] &&
//                s_possible->coords[1] == s_main->coords[1]){
//                 continue;
//             }

//             dist = CalcEuclidianDistance(*s_main,*s_possible);

//             for(i=0;i<NUM_NEIGHBOURS;i++){
//                 if(dist < neighbours_dist[i]){
//                     for(k=NUM_NEIGHBOURS-1;k<i;k--){
//                         neighbours_dist[k] = neighbours_dist[k-1];
//                         neighbours[k] = neighbours[k-1];
//                     }
//                     neighbours_dist[i] = dist;
//                     neighbours[i] = *s_possible;
//                 }
                
//             }
//         }
//     }
// }

bool CalcNewPossibleNgb(Sensor s_main,
                        Sensor s_candidate,
                        Sensor neighbours[NUM_NEIGHBOURS]) {
    float dist;
    float neighbours_dist[NUM_NEIGHBOURS];
    bool is_ngb = false;
    
    if(s_candidate.coords[0] == s_main.coords[0] &&
       s_candidate.coords[1] == s_main.coords[1]) return 0;

    for(unsigned i=0;i<NUM_NEIGHBOURS;i++){
        neighbours_dist[i] = CalcEuclidianDistance(s_main,neighbours[i]);
    }
       
    dist = CalcEuclidianDistance(s_main,s_candidate);

    for(unsigned i=0;i<NUM_NEIGHBOURS;i++){
        if(dist < neighbours_dist[i]){
            for(unsigned k=NUM_NEIGHBOURS-1;k<i;k--){
                neighbours_dist[k] = neighbours_dist[k-1];
                neighbours[k] = neighbours[k-1];
            }
            neighbours_dist[i] = dist;
            neighbours[i] = s_candidate;
            is_ngb = true;
        }
    }

    return is_ngb;
}


float CheckMeasureLimits(SensorType type, float measure) {
    switch (type) {
        case SENSOR_TEMP:
            return clampf(measure, 20.0f, 40.0f);
        case SENSOR_HUMIDITY:
            return clampf(measure, 10.0f, 90.0f);
        case SENSOR_AIR_QUALITY:
            return clampf(measure, 15.0f, 30.0f);
        default:
            return measure;
    }
}

void UpdateMeasure(Sensor *s, Sensor neighbours[]) {
    SensorType sensor_type = StringToSensorType(s->type);

    float new_measure = CalcNewMeasure(*s, neighbours);
    s->measurement = CheckMeasureLimits(sensor_type,new_measure);
}


