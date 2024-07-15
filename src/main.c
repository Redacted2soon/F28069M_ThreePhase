/* HTerm 0.8.9
 * Baud rate = 9600
 * Data = 8
 * Stop bit = 1
 * Parity = none
 * Newline at CR+LF
 * Make sure terminating character is sent with data (marks end)
 */

// Included Files
#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

///Define Valid Ranges for parameters
#define PWM_FREQUENCY_MIN 687
#define PWM_FREQUENCY_MAX 10000
#define SIN_FREQUENCY_MIN 0
#define SIN_FREQUENCY_MAX 300
#define MODULATION_DEPTH_MIN 0.0
#define MODULATION_DEPTH_MAX 1.0
#define CLKRATE 90.0*1000000.0
#define MIN_ANGLE -360
#define MAX_ANGLE 360
#define MAX_BUFFER_SIZE 100
#define MAX_MSG_SIZE 100

#define NEWLINE "\r\n" // Change depending on serial terminal new line character

// Struct to hold PWM parameters
typedef struct
{
    float pwm_frequency;
    float sin_frequency;
    float modulation_depth;
    float offset;
    float angle_1;
    float angle_2;
    float angle_3;
    Uint32 epwmTimerTBPRD;
} EPwmParams;

// Initialize default PWM parameters
EPwmParams originalePwmParams = {
        .pwm_frequency = 2500,      // frequency of pwm DO NOT GO BELOW 687Hz, counter wont work properly 65535 < 90*10^6 / (687*2)
        .sin_frequency = 60,        // sin frequency 0-150Hz
        .modulation_depth = 1.0,    // modulation depth between 0 and 1
        .offset = 0.0,              // make sure offset is between +-(1-MODULATION_DEPTH)/2
        .angle_1 = 0,               // Phase shift angle in degree
        .angle_2 = 120,             // Phase shift angle in degree
        .angle_3 = 240,             // Phase shift angle in degree
        .epwmTimerTBPRD = 0
};

// Struct for new PWM parameters
EPwmParams newEpwmParams;

// Function Prototypes
void scia_echoback_init(void);
void scia_fifo_init(void);
void scia_xmit(int a);
void scia_msg(const char *msg);
int populate_variable(const char *arr, float *var, float min, float max,
                      int *pindex);
int process_buffer(const char *buffer);
void report_invalid_input(char invalid_char);
int confirm_values(void);
void print_params(const EPwmParams *arr);
void float_to_string(float value);
void clear_scia_rx_buffer(void);
void print_welcome_screen(void);
void Init_Epwmm(void);

///prototypes for interrupt service routines (ISRs)
__interrupt void epwm1_isr(void);
__interrupt void epwm2_isr(void);
__interrupt void epwm3_isr(void);

// Main function
void main(void)
{
    // Calculate the ePWM timer period
    originalePwmParams.epwmTimerTBPRD = (Uint32)(0.5 * (CLKRATE / originalePwmParams.pwm_frequency));

    // Copy default PWM parameters to new PWM parameters
    memcpy(&newEpwmParams, &originalePwmParams, sizeof(EPwmParams));

    /// System initialization
    InitSysCtrl();
    InitSciaGpio();
    scia_fifo_init();
    scia_echoback_init();
    InitEPwm1Gpio();
    InitEPwm2Gpio();
    InitEPwm3Gpio();

    DINT; /// Disable CPU interrupts

    /// Initialize the PIE control registers to their default state
    InitPieCtrl();

    IER = 0x0000; /// Clear all CPU interrupt flags
    IFR = 0x0000; /// Clear all CPU interrupt flags

    /// Initialize the PIE vector table with pointers to the ISR
    InitPieVectTable();

    EALLOW; /// This is needed to write to EALLOW protected registers
    PieVectTable.EPWM1_INT = &epwm1_isr;
    PieVectTable.EPWM2_INT = &epwm2_isr;
    PieVectTable.EPWM3_INT = &epwm3_isr;
    EDIS; /// This is needed to disable write to EALLOW protected registers

    /// Initialize the ePWM modules, uncomment if you want epwm to initialize on startup
    /*
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0; /// Disable TBCLK within the ePWM
    EDIS;

    Init_Epwmm();

    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1; /// Enable TBCLK within the ePWM
    EDIS;
    */

    IER |= M_INT3; /// Enable CPU INT3 which is connected to EPWM1-3 INT

    /// Enable EPWM INTn in the PIE: Group 3 interrupt 1-3
    PieCtrlRegs.PIEIER3.bit.INTx1 = 1;
    PieCtrlRegs.PIEIER3.bit.INTx2 = 1;
    PieCtrlRegs.PIEIER3.bit.INTx3 = 1;

    EINT;    /// Enable Global interrupt INTM
    ERTM;    /// Enable Global real time interrupt DBGM

    print_welcome_screen(); // Print the welcome message

    // Buffer for incoming data and buffer index
    static char buffer[MAX_BUFFER_SIZE];
    Uint16 bufferIndex = 0;
    Uint16 ReceivedChar;

    // Main loop
    while (1)
    {
        // Wait for a character to be received
        while (SciaRegs.SCIFFRX.bit.RXFFST < 1)
        {
        }

        ReceivedChar = SciaRegs.SCIRXBUF.all;  // Read received character

        // Check for terminating character
        if (ReceivedChar == '\0')
        {
            //echo back user input
            scia_msg(NEWLINE NEWLINE"You sent: ");
            buffer[bufferIndex] = '\0';
            scia_msg(buffer);

            // Process the buffer
            int confirm = process_buffer(buffer); //returns 0 if there's an error with input

            if (confirm) // Checks if there was an error in processing
            {
                confirm = confirm_values(); // Allows user to confirm values, Y -> confirm = 1
            }

            // Confirm the values
            if (confirm)
            {
                scia_msg(NEWLINE NEWLINE"Values confirmed and set.");
                memcpy(&originalePwmParams, &newEpwmParams, sizeof(EPwmParams)); // Copy new values to original
                originalePwmParams.epwmTimerTBPRD = (Uint32)(0.5 * (CLKRATE / originalePwmParams.pwm_frequency));
                Init_Epwmm();
            }
            else
            {
                scia_msg(NEWLINE NEWLINE"Values reset to:");
                print_params(&originalePwmParams);  // Print the original values
            }

            // Reset the buffer for the next input
            bufferIndex = 0;
            memset(buffer, 0, MAX_BUFFER_SIZE);
            clear_scia_rx_buffer();
            print_welcome_screen();  // Print the welcome message again

        }
        else
        {
            // Add character to buffer array if there's space
            if (bufferIndex < MAX_BUFFER_SIZE - 1)
            {
                buffer[bufferIndex++] = ReceivedChar;
            }
            else
            {
                // Handle buffer overflow error
                scia_msg(
                        NEWLINE NEWLINE"Error: Input buffer overflow. Too many characters added. Buffer reset");
                bufferIndex = 0; // Reset buffer index to avoid overflow
                memset(buffer, 0, MAX_BUFFER_SIZE);
                clear_scia_rx_buffer();
                print_welcome_screen();  // Print the welcome message again
            }
        }
    }
}

// Interrupt service routine for ePWM1
__interrupt void epwm1_isr(void)
{
    ///initialize angle and convert from degrees to radians
    static float angle = 0;

    // Calculate the angle increment per PWM cycle
    float angleincrement = 2 * M_PI
            /(originalePwmParams.pwm_frequency / originalePwmParams.sin_frequency);

    // If the angle exceeds 2*PI, wrap it around
    if (angle > 2 * M_PI)
        angle = angle - 2 * M_PI;

    // Calculate the duty cycle for the PWM signal (had to put angle1*pi/180 inside sin because angle is static
    float duty_cycle = (sinf(angle + originalePwmParams.angle_1 * M_PI / 180.0) * originalePwmParams.modulation_depth + 1) * .5
            - originalePwmParams.offset;

    // Set the compare value for the PWM signal
    EPwm1Regs.CMPA.half.CMPA = (Uint32) ((duty_cycle)
            * ((float) originalePwmParams.epwmTimerTBPRD));

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

    float angleincrement = 2 * M_PI / (originalePwmParams.pwm_frequency / originalePwmParams.sin_frequency);

    if (angle > 2 * M_PI)
        angle = angle - 2 * M_PI;

    float duty_cycle = (sinf(angle + originalePwmParams.angle_2 * M_PI / 180.0) * originalePwmParams.modulation_depth + 1) * .5 - originalePwmParams.offset;

    EPwm2Regs.CMPA.half.CMPA = (Uint32) ((duty_cycle) * ((float) originalePwmParams.epwmTimerTBPRD));

    angle += angleincrement;

    EPwm2Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}
//same as epwm1
__interrupt void epwm3_isr(void)
{
    static float angle = 0;

    float angleincrement = 2 * M_PI / (originalePwmParams.pwm_frequency / originalePwmParams.sin_frequency);

    if (angle > 2 * M_PI)
        angle = angle - 2 * M_PI;

    float duty_cycle = (sinf(angle + originalePwmParams.angle_3 * M_PI / 180.0) * originalePwmParams.modulation_depth + 1) * .5 - originalePwmParams.offset;

    EPwm3Regs.CMPA.half.CMPA = (Uint32) ((duty_cycle) * ((float) originalePwmParams.epwmTimerTBPRD));

    angle += angleincrement;

    EPwm3Regs.ETCLR.bit.INT = 1;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}

// Process the input buffer and extract PWM parameters
int process_buffer(const char *buffer)
{

    int i = 0, error = 0;
    while (buffer[i] != '\0' && error == 0) // Loops until null character is found
    {
        // Skips white space and skips to parameter
        if (buffer[i] == ',' || buffer[i] == '.')
            i++;
        while (buffer[i] == ' ')
            i++;

        // Identify the parameter and populate its value
        switch (buffer[i])
        {
        case 'P':
        case 'p':
            error = populate_variable(&(buffer[i]),
                                      &newEpwmParams.pwm_frequency,
                                      PWM_FREQUENCY_MIN,
                                      PWM_FREQUENCY_MAX, &i);
            break;
        case 'S':
        case 's':
            error = populate_variable(&(buffer[i]),
                                      &newEpwmParams.sin_frequency,
                                      SIN_FREQUENCY_MIN,
                                      SIN_FREQUENCY_MAX, &i);
            break;
        case 'M':
        case 'm':
            error = populate_variable(&(buffer[i]),
                                      &newEpwmParams.modulation_depth,
                                      MODULATION_DEPTH_MIN,
                                      MODULATION_DEPTH_MAX, &i);
            break;
        case 'O':
        case 'o':
            error = populate_variable(&(buffer[i]), &newEpwmParams.offset, -1,
                                      1, &i);
            break;
        case 'A':
        case 'a':
            i++;
            if (buffer[i] == '1')
            {
                error = populate_variable(&buffer[i], &newEpwmParams.angle_1,
                MIN_ANGLE,
                                          MAX_ANGLE, &i);
            }
            else if (buffer[i] == '2')
            {
                error = populate_variable(&buffer[i], &newEpwmParams.angle_2,
                MIN_ANGLE,
                                          MAX_ANGLE, &i);
            }
            else if (buffer[i] == '3')
            {
                error = populate_variable(&buffer[i], &newEpwmParams.angle_3,
                MIN_ANGLE,
                                          MAX_ANGLE, &i);
            }
            else
            {
                report_invalid_input(buffer[i]);
                error = 1;
            }
            break;

        default:
            report_invalid_input(buffer[i]);
            error = 1;
        }
    }

    // Check offset range (since offset depends on modulation depth
    if (newEpwmParams.offset > (1 - newEpwmParams.modulation_depth) / 2
            || newEpwmParams.offset < -(1 - newEpwmParams.modulation_depth) / 2)
    {
        newEpwmParams.offset = originalePwmParams.offset;
        scia_msg(NEWLINE NEWLINE"Offset out of range");
        error = 1;
    }

    return error ? 0 : 1;    //Returns 0 if there was an error
}

// Function to populate a variable from the buffer
int populate_variable(const char *arr, float *var, const float min, const float max,
                      int *pindex)
{
    char temp[8] = { '\0' };  // Temporary array to hold input value

    int i = 1, j = 0, periodCount = 0;

    // Check for invalid input (multiple letters)
    if (arr[i] > '@' && arr[i] < 'z')
    {
        scia_msg("\r\nInvalid input : ");
        scia_msg(&arr[i - 1]);
        return 1;
    }

    // Skips space between letter and number
    while (arr[i] == ' ')
        i++;

    // Extract the number
    while (arr[i] != '\0' && (arr[i] == '.' || arr[i] == '-' || (arr[i] >= '0' && arr[i] <= '9')))
    {
        // Check if number is too big
        if (j > 8)
        {
            scia_msg(NEWLINE"Number input has too many digits");
            report_invalid_input(*temp);
            return 1;
        }

        if (arr[i] == '.')
        {
            periodCount++;  // Increment period count
        }

        temp[j++] = arr[i++];   // Copy the character to the temporary array
    }

    // Check if there are multiple periods
    if (periodCount > 1)
    {
        scia_msg(NEWLINE"Input has too many decimal points");
        report_invalid_input(*temp);
        return 1;
    }

    // Convert the temporary array to a float and check its range
    *var = atof(temp);
    if (*var < min || *var > max)
    {
        scia_msg(NEWLINE"Value out of bound: ");
        scia_msg(temp);
        return 1;
    }

    // Update the index and return success
    *pindex += i;
    return 0;
}

// Function to confirm the values with the user
int confirm_values(void)
{
    Uint16 confirm = 0;
    Uint16 RecivedChar = 0;

    // Ask the user to confirm the values
    scia_msg(NEWLINE NEWLINE"PLEASE CONFIRM THE VALUES (Y/N):  ");
    print_params(&newEpwmParams);

    // Wait for a valid response
    while (confirm == 0)
    {
        while (SciaRegs.SCIFFRX.bit.RXFFST < 1)
        {
            // Wait for a character
        }
        RecivedChar = SciaRegs.SCIRXBUF.all;    // Read received character

        // Check the response
        if (RecivedChar == 'Y' || RecivedChar == 'y')
        {
            confirm = 1;
        }
        else if (RecivedChar == 'N' || RecivedChar == 'n')
        {
            confirm = 2; //number that is not one
        }
        else
        {
            scia_msg(NEWLINE "Invalid input. Please enter Y or N.");
            SciaRegs.SCIRXBUF.all;
        }

    }
    return (confirm == 1) ? 1 : 0; //if Y in input, return 1
}

//prints to serial terminal
void print_params(const EPwmParams *arr)
{
    scia_msg(NEWLINE NEWLINE "PWM frequency = ");
    float_to_string(arr->pwm_frequency);

    scia_msg(NEWLINE "Sin wave frequency = ");
    float_to_string(arr->sin_frequency);

    scia_msg(NEWLINE "Modulation depth = ");
    float_to_string(arr->modulation_depth);

    scia_msg(NEWLINE "Offset = ");
    float_to_string(arr->offset);

    scia_msg(NEWLINE "Angle 1 = ");
    float_to_string(arr->angle_1);

    scia_msg(NEWLINE "Angle 2 = ");
    float_to_string(arr->angle_2);

    scia_msg(NEWLINE "Angle 3 = ");
    float_to_string(arr->angle_3);
}

// Function to convert a float to a string and send it via SCI
void float_to_string(const float value)
{
    // Assuming msg is large enough
    char msg[20];

    //split float value into whole and fractional
    int integerPart = (int) value;
    float fractionalPart = value - (float) integerPart;
    //absolute value of a negative decimal
    if (fractionalPart < 0)
        fractionalPart = -fractionalPart;

    //round to 3 decimal places
    int fractionalAsInt = (int) ((fractionalPart + .0001) * 1000); // 3 decimal places

    //print checks if whole or decimal was input
    if (fractionalAsInt == 0)
    {
        sprintf(msg, "%d", integerPart);
    }
    else
    {
        sprintf(msg, "%d.%03d", integerPart, fractionalAsInt); // Ensure 3 decimal places
    }

    //print value
    scia_msg(msg);
}

// Function to report invalid input
void report_invalid_input(const char invalid_char)
{
    char msg[50];
    sprintf(msg, NEWLINE NEWLINE "Invalid character: %c", invalid_char);
    scia_msg(msg);
}

// Function to clear the SCI A RX buffer
void clear_scia_rx_buffer()
{
    while (SciaRegs.SCIFFRX.bit.RXFFST != 0)
    {
        char temp = SciaRegs.SCIRXBUF.all;
    }
}

// scia_xmit - Transmit a character from the SCI
void scia_xmit(int a)
{
    while (SciaRegs.SCIFFTX.bit.TXFFST != 0)
    {
        // Wait for the transmit buffer to be empty
    }
    SciaRegs.SCITXBUF = a; // Placed below to ensure that the transmit buffer is empty before attempting to send data
}

// Transmit a message to the SCI
void scia_msg(const char *msg)
{

    while (*msg) // Keeps transmitting data until end of line (enter has been pressed, denoted by '\0')
    {
        scia_xmit(*msg++);
    }
}

// Function to print the welcome screen
void print_welcome_screen(void)
{
    scia_msg(
            NEWLINE "-------------------------------------------------------------------------------------------------"

            NEWLINE "Please Enter a string in the format PARAMATER1 VALUE1,PARAMATER2 VALUE2 (for example: P 2500, S 60,M .13)"

            NEWLINE NEWLINE "P = PWM frequency (in Hz,ACCEPTABLE INPUTS: 687 - 10000)"

            NEWLINE "S = Sin wave frequency (in Hz, ACCEPTABLE INPUTS: 1 - 300)"

            NEWLINE "M = Modulation depth (ACCEPTABLE INPUTS: 0.0 - 1.0, up to three decimal points)"

            NEWLINE "O = Offset (volts, ACCEPTABLE INPUTS: +-((1-Modulation depth) / 2), up to three decimal points )"

            NEWLINE "A1 = Angle 1 offset (in degrees, ACCEPTABLE INPUTS: -360 to 360)"

            NEWLINE "A2 = Angle 2 offset (in degrees, ACCEPTABLE INPUTS:  -360 to 360)"

            NEWLINE "A3 = Angle 3 offset (in degrees, ACCEPTABLE INPUTS:  -360 to 360)");

}

// Functions for SCI initialization and communication
void scia_echoback_init()
{
    SciaRegs.SCICCR.all = 0x0007; // 1 stop bit,  No loopback, No parity, 8 char bits, async mode, idle-line protocol

    // Enable TX, RX, internal SCICLK, Disable RX ERR, SLEEP, TXWAKE
    SciaRegs.SCICTL1.all = 0x0003;
    SciaRegs.SCICTL2.bit.TXINTENA = 0;
    SciaRegs.SCICTL2.bit.RXBKINTENA = 0;

    // 9600 baud @LSPCLK = 22.5MHz (90 MHz SYSCLK)
    SciaRegs.SCIHBAUD = 0x0001;
    SciaRegs.SCILBAUD = 0x0024;

    SciaRegs.SCICCR.bit.LOOPBKENA = 0;  // Enable loop back
    SciaRegs.SCICTL1.all = 0x0023;  // Relinquish SCI from Reset
}

// scia_fifo_init - Initalize the SCI FIFO
void scia_fifo_init()
{
    SciaRegs.SCIFFTX.all = 0xE040;
    SciaRegs.SCIFFRX.all = 0x2044;
    SciaRegs.SCIFFCT.all = 0x0;
}

void Init_Epwmm()
{
    ///setup sync
    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;  /// Pass through
    EPwm2Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;  /// Pass through
    EPwm3Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;  /// Pass through

    ///allow sync
    EPwm1Regs.TBCTL.bit.PHSEN = TB_ENABLE;
    EPwm2Regs.TBCTL.bit.PHSEN = TB_ENABLE;
    EPwm3Regs.TBCTL.bit.PHSEN = TB_ENABLE;

    ///sets the phase angle for each wave
    EPwm1Regs.TBPHS.half.TBPHS =
            (Uint16) (originalePwmParams.angle_1 / (360) * (originalePwmParams.epwmTimerTBPRD));
    EPwm2Regs.TBPHS.half.TBPHS =
            (Uint16) (originalePwmParams.angle_1 / (360) * (originalePwmParams.epwmTimerTBPRD));
    EPwm3Regs.TBPHS.half.TBPHS =
            (Uint16) (originalePwmParams.angle_1 / (360) * (originalePwmParams.epwmTimerTBPRD));

    EPwm1Regs.TBPRD = originalePwmParams.epwmTimerTBPRD;         /// Set timer period
    EPwm2Regs.TBPRD = originalePwmParams.epwmTimerTBPRD;         /// Set timer period
    EPwm3Regs.TBPRD = originalePwmParams.epwmTimerTBPRD;         /// Set timer period

    EPwm1Regs.TBCTR = 0x0000;                   /// Clear counter
    EPwm2Regs.TBCTR = 0x0000;                   /// Clear counter
    EPwm3Regs.TBCTR = 0x0000;                   /// Clear counter

    EPwm1Regs.CMPA.half.CMPA = 0;               /// starts compare A value
    EPwm2Regs.CMPA.half.CMPA = 0;               /// starts compare A value
    EPwm3Regs.CMPA.half.CMPA = 0;               /// starts compare A value

    EPwm1Regs.TBCTL.bit.CTRMODE = 2;            /// Count up/down
    EPwm2Regs.TBCTL.bit.CTRMODE = 2;            /// Count up/down
    EPwm3Regs.TBCTL.bit.CTRMODE = 2;            /// Count up/down

    EPwm1Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;  /// Interrupt on when counter = 0
    EPwm2Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;  /// Interrupt on when counter = 0
    EPwm3Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;  /// Interrupt on when counter = 0

    EPwm1Regs.ETSEL.bit.INTEN = 1;              /// Enable INT
    EPwm2Regs.ETSEL.bit.INTEN = 1;              /// Enable INT
    EPwm3Regs.ETSEL.bit.INTEN = 1;              /// Enable INT

    EPwm1Regs.ETPS.bit.INTPRD = 1;              /// Generate INT on 1st event
    EPwm2Regs.ETPS.bit.INTPRD = 1;              /// Generate INT on 1st event
    EPwm3Regs.ETPS.bit.INTPRD = 1;              /// Generate INT on 1st event

    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;    /// Clock ratio to SYSCLKOUT
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;    /// Clock ratio to SYSCLKOUT
    EPwm3Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;     /// Clock ratio to SYSCLKOUT

    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;           /// Clock ratio to SYSCLKOUT
    EPwm2Regs.TBCTL.bit.CLKDIV = TB_DIV1;           /// Clock ratio to SYSCLKOUT
    EPwm3Regs.TBCTL.bit.CLKDIV = TB_DIV1;           /// Clock ratio to SYSCLKOUT

    ///shaddow
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;// Enable shadow mode for Compare A registers of ePWM1
    EPwm2Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;// Enable shadow mode for Compare A registers of ePWM2
    EPwm3Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;// Enable shadow mode for Compare A registers of ePWM3

    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;  /// Load on Zero
    EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;  /// Load on Zero
    EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;  /// Load on Zero

    EPwm1Regs.AQCTLA.bit.CAU = 2;    /// Set PWM1A on event A, up count
    EPwm2Regs.AQCTLA.bit.CAU = 2;    /// Set PWM1A on event A, up count
    EPwm3Regs.AQCTLA.bit.CAU = 2;    /// Set PWM1A on event A, up count

    EPwm1Regs.AQCTLA.bit.CAD = 1;   /// Clear PWM1A on event A, down count
    EPwm2Regs.AQCTLA.bit.CAD = 1;   /// Clear PWM1A on event A, down count
    EPwm3Regs.AQCTLA.bit.CAD = 1;   /// Clear PWM1A on event A, down count

}
