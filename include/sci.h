/*
 * sci.h
 *
 *  Created on: Jun 26, 2024
 *      Author: admin
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pwm.h"

#ifndef SCI_H
#define SCI_H

#define NEWLINE "\r\n"

extern EPwmParams liveEpwmParams;
extern EPwmParams bufferEpwmParams;

// Function Prototypes
// Peripheral initialization
void scia_echoback_init(void);     // Rx and Tx register initialization
void scia_fifo_init(void);         // Initialize registers the SCI FIFO

// Utility functions
void handle_received_char(Uint16 ReceivedChar); // Handles a received character from SCI, processes input buffer for PWM parameters.
int populate_variable(const char *arr, float *var, float min, float max,
                      int *pindex);             // Populates a float variable with a value from a given string, ensuring the value is within the specified range.
int process_buffer(const char *buffer);         // Processes the input buffer to extract and update the PWM parameters.
int confirm_values(void);                       // Prompts the user to confirm the new PWM values and returns the confirmation status.
void print_params(const EPwmParams *arr);       // Prints the given PWM parameters to the serial terminal.
void float_to_string(float value);              // Function to convert a float to a string and send it via SCI
void report_invalid_input(char invalid_char);   // Reports an invalid input character via the serial terminal.
void clear_scia_rx_buffer(void);                // Clears the SCI A RX buffer to remove any remaining data.
void scia_msg(const char *msg);                 // Transmits a message (string) via the SCI.
void scia_xmit(int asciiValue);                 // Transmits a single ASCII character via the SCI.
void print_welcome_screen(void);                // Prints the welcome screen message to the serial terminal.

#endif
