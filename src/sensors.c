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

MsgAction CalcNewPossibleNgb(Sensor s_main,
                        Sensor s_candidate,
                        Sensor neighbours[NUM_NEIGHBOURS]) {
    float dist;
    float neighbours_dist[NUM_NEIGHBOURS];
    
    if(s_candidate.coords[0] == s_main.coords[0] &&
       s_candidate.coords[1] == s_main.coords[1]) {
        
        // DEBUG
        //printf("\n[NGB_CALC] Mensagem do próprio sensor. Nada acontece...\n");
        return SAME_LOC;
       }

    // if(SENSOR REMOVIDO){
    //     printf("[NGB_CALC] Sensor REMOVIDO do Grid!")
    //     return REMOVED;
    // }

    for(unsigned i=0;i<NUM_NEIGHBOURS;i++){
        if(s_candidate.coords[0] == neighbours[i].coords[0] &&
           s_candidate.coords[1] == neighbours[i].coords[1]){
            // DEBUG
            //printf("[NGB_CALC] Já é vizinho!! Posicao: %i!!\n", i);
            //printf("[NGB_CALC] COORDS CANDIDATO: [%d][%d]\n", s_candidate.coords[0], s_candidate.coords[1]);
            //printf("[NGB_CALC] COORDS VIZINHO[%u]: [%d][%d]\n", i, s_candidate.coords[0], s_candidate.coords[1]);

            //printf("[NGB_CALC] IS_NGB\n");
            return IS_NGB; 
        }
        neighbours_dist[i] = CalcEuclidianDistance(s_main,neighbours[i]);
        // DEBUG
        //printf("[NGB_CALC] Dist from ngb %u: %f\n", i, neighbours_dist[i]);
    }
       
    dist = CalcEuclidianDistance(s_main,s_candidate);
    // DEBUG
    //printf("[NGB_CALC] Distancia Euclidiana Calculada: %f\n", dist);

    for(unsigned i=0;i<NUM_NEIGHBOURS;i++){
        // DEBUG
        //printf("[NGB_CALC] ngb_dist[%u]=%.3f\n", i, neighbours_dist[i]);
        if(dist < neighbours_dist[i]){
            // DEBUG
            //printf("[NGB_CALC] dist < ngb_dist posicao %u\n", i);
            for(unsigned k=NUM_NEIGHBOURS-1;k<i;k--){
                neighbours_dist[k] = neighbours_dist[k-1];
                neighbours[k] = neighbours[k-1];
            }
            neighbours_dist[i] = dist;
            neighbours[i] = s_candidate;
            
            // DEBUG
            //printf("[NGB_CALC] IS_NGB\n");
            return IS_NGB;
        }
    }

    // DEBUG
    //printf("[NGB_CALC] NOT_NGB\n");
    return NOT_NGB;
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

// request_type:
//      0 -> client
//      1 -> server
void PrintClientLogMsg(Sensor s_main,
                       Sensor s_received,
                       MsgAction action_rcv,
                       float measure_correction,
                       int request_type){

    char action_print[64];

    switch (action_rcv){
        case NOT_NGB:
            strcpy(action_print, "not neighbour");
            break;    
        case IS_NGB:
            snprintf(action_print, sizeof(action_print), "correction of %.4f", measure_correction);
            break; 
        case SAME_LOC:
            strcpy(action_print, "same location");    
            break;
        case REMOVED:
            strcpy(action_print, "removed");  
            break;  
        default:
            strcpy(action_print, "unknown action");
            break;
    }

    if(request_type == 0){
        printf("\n\nlog:\n%s sensor in (%d,%d)\nmeasurement: %.4f\naction: %s\n\n",
            s_received.type, s_received.coords[0], s_received.coords[1], s_received.measurement, action_print);
        printf("Sensor (%d,%d)-> real-time measurement: %.4f\n\n", s_main.coords[0], s_main.coords[1], s_main.measurement);
    }
    else if(request_type == 1){
        printf("\n\nlog:\n%s sensor in (%d,%d)\nmeasurement: %.4f\n\n",
            s_received.type, s_received.coords[0], s_received.coords[1], s_received.measurement);

    }
}