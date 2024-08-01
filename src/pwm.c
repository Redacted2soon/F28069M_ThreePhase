#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File
#include "pwm.h"

// Initialize default PWM parameters to be outputted
EPwmParams liveEpwmParams = {
        .pwmWavFreq = 2500,      // frequency of pwm DO NOT GO BELOW 687Hz, counter wont work properly 65535 < 90*10^6 / (687*2)
        .sinWavFreq = 60,        // sin frequency 0-150Hz
        .modulation_depth = 1.0,    // modulation depth between 0 and 1
        .offset = 0.0,              // make sure offset is between +-(1-MODULATION_DEPTH)/2
        .phaseLead1 = 0,               // Phase shift angle in degree
        .phaseLead2 = 120,             // Phase shift angle in degree
        .phaseLead3 = 240,             // Phase shift angle in degree
        .epwmTimerTBPRD = 0
};
// Structure to store values input from serial terminal
EPwmParams bufferEpwmParams;

/**
 * Interrupt service routine for ePWM1 module.
 * Generates a PWM signal with a sinusoidal modulation.
 * Calculates phase angle and sets duty cycle based on parameters.
 * Interrupt occurs when counter is set to 0
 */
__interrupt void epwm1_isr(void)
{
    //initialize angle and convert from degrees to radians
    static float angle = 0;

    // Calculate the angle increment per PWM cycle
    float angleincrement = 2 * M_PI
            /(liveEpwmParams.pwmWavFreq / liveEpwmParams.sinWavFreq);

    // If the angle exceeds 2*PI, wrap it around
    if (angle > 2 * M_PI)
        angle -= 2 * M_PI;

    // Calculate the duty cycle for the PWM signal
    float angle_rad = angle + liveEpwmParams.phaseLead1 * M_PI / 180.0;
    float duty_cycle = (sinf(angle_rad) * liveEpwmParams.modulation_depth + 1) * .5
            - liveEpwmParams.offset;

    // Set the compare value for the PWM signal
    EPwm1Regs.CMPA.half.CMPA = (Uint32) ((1-duty_cycle)
            * ((float) liveEpwmParams.epwmTimerTBPRD));

    // Increment the angle for the next cycle
    angle += angleincrement;

    // Clear the interrupt flag
    EPwm1Regs.ETCLR.bit.INT = 1;

    // Acknowledge the interrupt in the PIE control register
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}

//same as epwm1
__interrupt void epwm2_isr(void)
{
    static float angle = 0;

    float angleincrement = 2 * M_PI / (liveEpwmParams.pwmWavFreq / liveEpwmParams.sinWavFreq);

    if (angle > 2 * M_PI)
        angle -= 2 * M_PI;

    float angle_rad = angle + liveEpwmParams.phaseLead2 * M_PI / 180.0;
    float duty_cycle = (sinf(angle_rad) * liveEpwmParams.modulation_depth + 1) * .5 - liveEpwmParams.offset;

    EPwm2Regs.CMPA.half.CMPA = (Uint32) ((1-duty_cycle) * ((float) liveEpwmParams.epwmTimerTBPRD));

    angle += angleincrement;

    EPwm2Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}
//same as epwm1
__interrupt void epwm3_isr(void)
{
    static float angle = 0;

    float angleincrement = 2 * M_PI / (liveEpwmParams.pwmWavFreq / liveEpwmParams.sinWavFreq);

    if (angle > 2 * M_PI)
        angle -= 2 * M_PI;

    float angle_rad = angle + liveEpwmParams.phaseLead3 * M_PI / 180.0;
    float duty_cycle = (sinf(angle_rad) * liveEpwmParams.modulation_depth + 1) * .5 - liveEpwmParams.offset;

    EPwm3Regs.CMPA.half.CMPA = (Uint32) ((1-duty_cycle) * ((float) liveEpwmParams.epwmTimerTBPRD));

    angle += angleincrement;

    EPwm3Regs.ETCLR.bit.INT = 1;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}


void init_epwm_interrupts()
{
    DINT; //disable interrupts so no interrupts will interrupt the initialization of interrupts

    // Clear PIE (peripheral interrupt expansion) flags
    InitPieCtrl();

    IER = 0x0000; // Clear all CPU interrupt flags
    IFR = 0x0000; // Clear all CPU interrupt flags

    // Initialize the PIE vector table with pointers to the ISR
    InitPieVectTable();

    EALLOW; // Enables access to emulation and other protected registers
    PieVectTable.EPWM1_INT = &epwm1_isr;
    PieVectTable.EPWM2_INT = &epwm2_isr;
    PieVectTable.EPWM3_INT = &epwm3_isr;
    EDIS; // Disable access to emulation space and other protected registers

    // Initialize the ePWM modules, uncomment if you want epwm to initialize on startup
    /*
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0; // Disable TBCLK within the ePWM
    EDIS;
    Init_Epwmm();
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1; // Enable TBCLK within the ePWM
    EDIS;
    */

    IER |= M_INT3; // Enable CPU INT3 which is connected to EPWM1-3 INT

    // Enable EPWM INTn in the PIE: Group 3 interrupt 1-3
    PieCtrlRegs.PIEIER3.bit.INTx1 = 1;
    PieCtrlRegs.PIEIER3.bit.INTx2 = 1;
    PieCtrlRegs.PIEIER3.bit.INTx3 = 1;

    EINT;    // Enable Global interrupt INTM
    ERTM;    // Enable Global real time interrupt DBGM
}

// Initialize registers for ePWM 1, 2, and 3
void Init_Epwmm()
{
    DINT; // Disable interrupts so no interrupts will interrupt the initialization of interrupts

    //setup sync from EPWMxSYNC signal generated from PHSEN
    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;  // Pass through
    EPwm2Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;  // Pass through
    EPwm3Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;  // Pass through

    //allow sync
    EPwm1Regs.TBCTL.bit.PHSEN = TB_ENABLE;
    EPwm2Regs.TBCTL.bit.PHSEN = TB_ENABLE;
    EPwm3Regs.TBCTL.bit.PHSEN = TB_ENABLE;

    //The period register (TBPRD) is loaded from its shadow register
    EPwm1Regs.TBCTL.bit.PRDLD  = TB_SHADOW;
    EPwm2Regs.TBCTL.bit.PRDLD  = TB_SHADOW;
    EPwm3Regs.TBCTL.bit.PRDLD  = TB_SHADOW;

    // Count up/down mode
    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm3Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;

    // Clock ratio to SYSCLKOUT
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm3Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;

    // Clock ratio to SYSCLKOUT
    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;
    EPwm2Regs.TBCTL.bit.CLKDIV = TB_DIV1;
    EPwm3Regs.TBCTL.bit.CLKDIV = TB_DIV1;

    // Clear counter
    EPwm1Regs.TBCTR = 0x0000;
    EPwm2Regs.TBCTR = 0x0000;
    EPwm3Regs.TBCTR = 0x0000;

    //sets the phase angle for each wave
    EPwm1Regs.TBPHS.half.TBPHS =
            (Uint16) (liveEpwmParams.phaseLead1 / (360) * (liveEpwmParams.epwmTimerTBPRD));
    EPwm2Regs.TBPHS.half.TBPHS =
            (Uint16) (liveEpwmParams.phaseLead1 / (360) * (liveEpwmParams.epwmTimerTBPRD));
    EPwm3Regs.TBPHS.half.TBPHS =
            (Uint16) (liveEpwmParams.phaseLead1 / (360) * (liveEpwmParams.epwmTimerTBPRD));

    // These bits determine the period of the time-base counter (sets the PWM frequency)
    EPwm1Regs.TBPRD = liveEpwmParams.epwmTimerTBPRD;
    EPwm2Regs.TBPRD = liveEpwmParams.epwmTimerTBPRD;
    EPwm3Regs.TBPRD = liveEpwmParams.epwmTimerTBPRD;

    // Sets compare A value to 0
    EPwm1Regs.CMPA.half.CMPA = 0;
    EPwm2Regs.CMPA.half.CMPA = 0;
    EPwm3Regs.CMPA.half.CMPA = 0;

    // Interrupt when counter = 0
    EPwm1Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;
    EPwm2Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;
    EPwm3Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;

    // Enable INT generation
    EPwm1Regs.ETSEL.bit.INTEN = 1;
    EPwm2Regs.ETSEL.bit.INTEN = 1;
    EPwm3Regs.ETSEL.bit.INTEN = 1;

    // Generate INT on 1st event
    EPwm1Regs.ETPS.bit.INTPRD = ET_1ST;
    EPwm2Regs.ETPS.bit.INTPRD = ET_1ST;
    EPwm3Regs.ETPS.bit.INTPRD = ET_1ST;

    // Enable shadow mode for Compare A registers of ePWMx (Operates as a double buffer.)
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm2Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm3Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;

    // Load From Shadow Select Mode when counter = Zero
    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;

    // Set PWMxA on event A, up count
    EPwm1Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm2Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm3Regs.AQCTLA.bit.CAU = AQ_SET;

    // Clear PWMxA on event A, down count
    EPwm1Regs.AQCTLA.bit.CAD = AQ_CLEAR;
    EPwm2Regs.AQCTLA.bit.CAD = AQ_CLEAR;
    EPwm3Regs.AQCTLA.bit.CAD = AQ_CLEAR;

    EINT;  //   Enable interrupts

}
