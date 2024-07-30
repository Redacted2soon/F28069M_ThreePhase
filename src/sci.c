#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File
#include "sci.h"

// Data from serial communications writes to live structure once processed and confirmed

/**
 * Handles a received character from SCI, builds a command buffer, and processes it.
 * Echoes back user input and manages parameter confirmation and update.
 *
 *
 */
void handle_received_char(Uint16 ReceivedChar)
{
    // Buffer for incoming data and buffer index
    static char buffer[MAX_BUFFER_SIZE];
    static Uint16 bufferIndex = 0;

    // Check to see if final charcater was processed (always end with terminating character)
    if (ReceivedChar == '\0')
    {
        //echo back user input
        buffer[bufferIndex] = '\0';
        scia_msg(NEWLINE NEWLINE "You sent: ");
        scia_msg(buffer);

        // Check for errors and user confirmation of values
        if (process_buffer(buffer) && confirm_values())
        {
            scia_msg(NEWLINE NEWLINE"Values confirmed and set.");
            memcpy(&liveEpwmParams, &bufferEpwmParams, sizeof(EPwmParams)); // Copy new values to original
            liveEpwmParams.epwmTimerTBPRD = (Uint32)(0.5 * (PWMCLK / liveEpwmParams.pwmWavFreq));
            Init_Epwmm();
        }
        else
        {
            scia_msg(NEWLINE NEWLINE"Values reset to:");
            print_params(&liveEpwmParams);  // Print the original values
            memcpy(&bufferEpwmParams, &liveEpwmParams, sizeof(EPwmParams)); // Copy old values
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
                 NEWLINE NEWLINE"Error: Input buffer overflow. Buffer reset.");
            bufferIndex = 0; // Reset buffer index to avoid overflow
            memset(buffer, 0, MAX_BUFFER_SIZE);
            clear_scia_rx_buffer();
            print_welcome_screen();  // Print the welcome message again
        }
    }
}

/*
 * Processes the input buffer to extract and update PWM parameters.
 * Returns 1 if successful, 0 if errors occurred during processing.
 */
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
                                      &bufferEpwmParams.pwmWavFreq,
                                      pwmWavFreq_MIN,
                                      pwmWavFreq_MAX, &i);
            break;
        case 'S':
        case 's':
            error = populate_variable(&(buffer[i]),
                                      &bufferEpwmParams.sinWavFreq,
                                      sinWavFreq_MIN,
                                      sinWavFreq_MAX, &i);
            break;
        case 'M':
        case 'm':
            error = populate_variable(&(buffer[i]),
                                      &bufferEpwmParams.modulation_depth,
                                      MODULATION_DEPTH_MIN,
                                      MODULATION_DEPTH_MAX, &i);
            break;
        case 'O':
        case 'o':
            error = populate_variable(&(buffer[i]), &bufferEpwmParams.offset, -1,
                                      1, &i);
            break;
        case 'A':
        case 'a':
            i++;
            if (buffer[i] == '1')
            {
                error = populate_variable(&buffer[i], &bufferEpwmParams.phaseLead1,
                MIN_ANGLE,
                                          MAX_ANGLE, &i);
            }
            else if (buffer[i] == '2')
            {
                error = populate_variable(&buffer[i], &bufferEpwmParams.phaseLead2,
                MIN_ANGLE,
                                          MAX_ANGLE, &i);
            }
            else if (buffer[i] == '3')
            {
                error = populate_variable(&buffer[i], &bufferEpwmParams.phaseLead3,
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
            break;
        }
    }

    // Check offset range (since offset depends on modulation depth
    if (bufferEpwmParams.offset > (1 - bufferEpwmParams.modulation_depth) / 2
            || bufferEpwmParams.offset < -(1 - bufferEpwmParams.modulation_depth) / 2)
    {
        bufferEpwmParams.offset = liveEpwmParams.offset;
        scia_msg(NEWLINE NEWLINE"Offset out of range");
        error = 1;
    }

    return error ? 0 : 1;    //Returns 0 if there was an error
}

/**
 * Parses a portion of the input buffer to populate a float variable.
 * Checks if the value is within specified range.
 * Returns 0 if successful, 1 if error occurred (out of range or invalid input).
 */
int populate_variable(const char *arr, float *var, const float min, const float max,
                      int *pindex)
{
    char temp[5] = { '\0' };  // Temporary array to hold input value

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

    // Copies number from arr (buffer) to temp array
    while (arr[i] != '\0' && (arr[i] == '.' || arr[i] == '-' || (arr[i] >= '0' && arr[i] <= '9')))
    {
        // Check if number is too many digits, 5 because max digits is 5 (10000)
        if (j > 5)
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

/**
 * Prompts the user to confirm the new PWM values and returns the confirmation status.
 * Returns 1 if confirmed, 0 if not confirmed.
 */
int confirm_values(void)
{

    Uint16 confirm = 2;
    Uint16 RecivedChar = 0;

    // Ask the user to confirm the values
    scia_msg(NEWLINE NEWLINE"PLEASE CONFIRM THE VALUES (Y/N):  ");
    print_params(&bufferEpwmParams);

    // Wait for a valid response
    while (confirm == 2)
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
            confirm = 0;
        }
        else
        {
            scia_msg(NEWLINE "Invalid input. Please enter Y or N.");
            SciaRegs.SCIRXBUF.all;
        }

    }
    return confirm; //if Y in input, return 1 to handle receive char, N returns 0 as false
}

// Prints the given PWM parameters to the serial terminal.
void print_params(const EPwmParams *arr)
{
    scia_msg(NEWLINE NEWLINE "PWM frequency = ");
    float_to_string(arr->pwmWavFreq);

    scia_msg(NEWLINE "Sin wave frequency = ");
    float_to_string(arr->sinWavFreq);

    scia_msg(NEWLINE "Modulation depth = ");
    float_to_string(arr->modulation_depth);

    scia_msg(NEWLINE "Offset = ");
    float_to_string(arr->offset);

    scia_msg(NEWLINE "Angle 1 = ");
    float_to_string(arr->phaseLead1);

    scia_msg(NEWLINE "Angle 2 = ");
    float_to_string(arr->phaseLead2);

    scia_msg(NEWLINE "Angle 3 = ");
    float_to_string(arr->phaseLead3);
}

// Converts a float value to a string and sends it via SCI.
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

// Reports an invalid input character via the serial terminal.
void report_invalid_input(const char invalid_char)
{
    char msg[50];
    sprintf(msg, NEWLINE NEWLINE "Invalid character: %c", invalid_char);
    scia_msg(msg);
}

// Clears the SCI A RX buffer to remove any remaining data.
void clear_scia_rx_buffer()
{
    while (SciaRegs.SCIFFRX.bit.RXFFST != 0)
    {
        char temp = SciaRegs.SCIRXBUF.all;
    }
}

// Transmits a single ASCII character via the SCI.
void scia_xmit(int asciiValue)
{
    while (SciaRegs.SCIFFTX.bit.TXFFST != 0)
    {
        // Wait for the transmit buffer to be empty
    }
    SciaRegs.SCITXBUF = asciiValue; // Placed below to ensure that the transmit buffer is empty before attempting to send data
}

// Transmits a message (string) via the SCI.
void scia_msg(const char *msg)
{

    while (*msg)            // Keeps transmitting data until null character (enter has been pressed, denoted by '\0' which is 0 in ascii)
    {
        scia_xmit(*msg++);  // Passes through ASCII value than increments to next element
    }
}

// Prints the welcome screen message to the serial terminal.
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

// SCI register initialization (Communication)
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

// Initialize registers the SCI FIFO
void scia_fifo_init()
{
    SciaRegs.SCIFFTX.all = 0xE040;
    SciaRegs.SCIFFRX.all = 0x2044;
    SciaRegs.SCIFFCT.all = 0x0;
}
