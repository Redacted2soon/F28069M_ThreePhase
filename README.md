# Three Phase Function Generator

## About

This repository contains embedded software for a function generator implemented on the TMS320F28069M microcontroller. The function generator is capable of generating three-phase wave with configurable parameters such as: PWM frequency, sin frequency, modulation depth, offset, and phase (of each wave).  <br>
![image](https://github.com/user-attachments/assets/8706b4a3-3700-4cee-b6b9-43aea860b3e6)

### Built With

IDE: Code Composer Studio v12
Compiler: TI v22.6.1 LTS
Serial terminal: HTerm 0.8.9

## Target

Microcontroller: Texas Instruments TMS320F28069M

## Project Structure

The project is structured as a Code Composer Studio project.

### Prerequisites

1. Download Code Composer Studio.
2. Download a serial terminal

### Setting up Serial terminal

1. Set: Correct port (found in device manager), baud rate to 9600, Data 8, Stop bit = 1, Parity = 0
2. Newline at CR+LF
3. Send on enter: NULL
4. Set transmit and recieve on "Ascii"
5. Click the "connect" button (In HTerm, disconnect should be shown once connect is pressed)  <br>
![image](https://github.com/user-attachments/assets/689ff867-cf3d-43e5-a622-711a59307de0)

### Opening the Project

To run the project for development:

1. Clone the repo
2. Open Code Composer Studio at the directory the repository is located in such as `C:\MyProjects\FunctionGenerator\`
3. Import the project
4. Link "common" folder and link "headers" folder (links shown in picture below)
5. Exclude three files from build (files shown in picture below)
   ![image](https://github.com/user-attachments/assets/d341707f-03d1-488c-83df-72f6f3123ce0)

### Setting up the wireing
The first wave comes out of P0, Second P2, third P4
Connect the pins like this (each output having its own low pass filter): <br>
![image](https://github.com/user-attachments/assets/57ce7bc7-7716-41ac-a33b-a9d45dc43dde)

Here are the components used in each filter: <br>
![image](https://github.com/user-attachments/assets/fc0d6c6c-d055-4cc7-9661-5fd793b61c73)

(you can switch the 100k to a bigger resistor, the bigger the better) 

### Running the Project for Development

To run the project for development:

1. Open and setup serial terminal 
2. From Code Composer Studio, click Build (hammer icon).
3. Attach a XDS510USB debugger to the development board.
4. Click Debug (bug icon).
5. After the target has been flashed, click the start button (play icon).
6. This screen should show up in the serial terminal, follow directions to change wave.  <br>
![image](https://github.com/user-attachments/assets/20328408-a91d-4a27-ae49-3265f13b7e54)


### Running the Tests

Unit tests currently aren't integrated into this project.

### Workflow

The development workflow used to develop this project is [GitHub Flow](https://docs.github.com/en/get-started/quickstart/github-flow).

In summary:

1. Create a branch.
2. Make changes.
3. Create a Pull Request.
4. Address Review Comments.
5. Merge your Pull Request.
6. Delete your branch.

### Program logic explained 

The PWM (Pulse Width Modulation) creates the waveform by generating a sinusoidal signal modulated by a triangular carrier signal. Here’s a breakdown of how this logic works:
1. Basic PWM Setup
  A higher PWM frequency means the ISR is triggered more frequently (smaller amout of clocks), allowing for more updates to the PWM duty cycle, which can improve accuracy and resolution of the output signal.
  Adjusting the sin frequency changes the rate at which the angle increments in the ISR. A higher sin frequency results in faster angle increments, which increases the frequency of the generated sinusoidal waveform.

 The smallest value for the PWM frequency is 687 because the pwm counter is only 16 bits, 2^(16) > 90 * 10^6 / (pwmWavFreq * 2).
   
3. Sinusoidal Signal Generation
  Angle Calculation: An angle is incremented in each ISR call to simulate the sinusoidal waveform.
  Angle Increment: This value determines how fast the angle progresses based on the sinusoidal frequency.
    float angleincrement = 2 * M_PI * (liveEpwmParams.sinWavFreq / liveEpwmParams.pwmWavFreq);
  Wrap-around Logic: If the angle exceeds 2 * M_PI, it wraps around to keep it within the range of 0 to 2 * M_PI.

4. Duty Cycle Calculation
   Sine Calculation: The sine of the current angle, adjusted by a phase shift (phaseLead1, phaseLead2, phaseLead3), is used to create a sinusoidal modulation.
    float duty_cycle = (sinf(angle + liveEpwmParams.phaseLead1 * M_PI / 180.0) * liveEpwmParams.modulation_depth + 1) * 0.5 - liveEpwmParams.offset;
   
6. Compare
   The one minus the duty cycle (calculated as a percentage) is then multiplied by the period and stored in the compare register (one minus just makes the math a little better to follow)

   ![image](https://github.com/user-attachments/assets/d8db373e-0a5c-4746-aa1f-c6b0851fce76)

## Acknowledgements

This project has been developed by Ethan Robotham.
