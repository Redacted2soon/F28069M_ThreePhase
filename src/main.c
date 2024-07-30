/* HTerm 0.8.9
 * Baud rate = 9600
 * Data = 8
 * Stop bit = 1
 * Parity = none
 * Newline at CR+LF
 * Make sure terminating character is sent with data (marks end)
 */

#include "main.h"

/*
 * Main function for controlling PWM parameters via SCI interface.
 * Initializes system, sets up PWM peripherals and interrupts,
 * and handles user interaction through SCI for parameter configuration.
 * All main logic for PWM occurs at interrupt
 */
void main(void)
{
    // Calculate the ePWM timer period, .5 is used because timer is in up/down count mode
    liveEpwmParams.epwmTimerTBPRD =
            (Uint32)(0.5 * (PWMCLK / liveEpwmParams.pwmWavFreq));

    // Copy default PWM parameters to new PWM parameters
    memcpy(&bufferEpwmParams, &liveEpwmParams, sizeof(EPwmParams));

    /// System initialization
    InitSysCtrl();
    InitSciaGpio();
    scia_fifo_init();
    scia_echoback_init();
    InitEPwm1Gpio();
    InitEPwm2Gpio();
    InitEPwm3Gpio();
    init_epwm_interrupts();

    print_welcome_screen(); // Print the welcome message

    Uint16 ReceivedChar;

    // Process user communications from serial port and write to global structure to reconfigure PWM to generate different sin wave outputs
    while (FOREVER)
    {
        // Wait for a character to be received
        while (SciaRegs.SCIFFRX.bit.RXFFST < 1)
        {
        }
        ReceivedChar = SciaRegs.SCIRXBUF.all;  // Read received character
        handle_received_char(ReceivedChar);
    }

}
