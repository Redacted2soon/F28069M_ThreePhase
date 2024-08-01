/*
 * main.h
 *
 *  Created on: Jun 26, 2024
 *      Author: admin
 */
#include <math.h>
#ifndef PWM_H
#define PWM_H

//TODO: make define into config file with constant global variables for easier debugging (for define now, unit is unknown)

//Define Valid Ranges for parameters (all frequencies are in measured in Hz)
#define PWMWAVFREQ_MIN 687
#define PWMWAVFREQ_MAX 100000
#define SINWAVFREQ_MIN 0
#define SINWAVFREQ_MAX 300
#define MODULATION_DEPTH_MIN 0.0
#define MODULATION_DEPTH_MAX 1.0
#define PWMCLKFREQ 90.0*1000000.0
#define MIN_ANGLE -360
#define MAX_ANGLE 360
#define MAX_BUFFER_SIZE 100
#define MAX_MSG_SIZE 100

// Declaration of struct to hold PWM parameters
typedef struct
{
    float pwmWavFreq;
    float sinWavFreq;
    float modulation_depth;
    float offset;
    float phaseLead1;
    float phaseLead2;
    float phaseLead3;
    Uint32 epwmTimerTBPRD;
} EPwmParams;

// Function prototypes

void Init_Epwmm(void);              // Initialize registers for ePWM 1, 2, and 3
void init_epwm_interrupts(void);    //Initialize interrupts for ePWM 1,2, and 3

// Interrupt service routines (ISRs)
__interrupt void epwm1_isr(void);               // ISR for ePWM1: Generates a sinusoidal PWM signal with the specified parameters.
__interrupt void epwm2_isr(void);               // ISR for ePWM2: Generates a sinusoidal PWM signal with the specified parameters.
__interrupt void epwm3_isr(void);               // ISR for ePWM3: Generates a sinusoidal PWM signal with the specified parameters.

#endif
