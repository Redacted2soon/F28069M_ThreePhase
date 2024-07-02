# Function Generator

## About

This repository contains embedded software for a function generator implemented on the TMS320F28069M microcontroller. The function generator is capable of generating three-phase wave with configurable parameters such as frequency of PWM and wave, amplitude, offset and phase.

### Built With

IDE: Code Composer Studio v12
Compiler: TI v22.6.1 LTS

## Target

Microcontroller: Texas Instruments TMS320F28069M

## Project Structure

The project is structured as a Code Composer Studio project.

### Prerequisites

1. Download Code Composer Studio.
2. Obtain the necessary hardware and software libraries (c2000ware).
3. Ensure the `FunctionGeneratorConfig.h` file is correctly placed in the project directory.

### Opening the Project

To run the project for development:

1. Clone the repo
2. Open Code Composer Studio at the directory the repository is located in such as `C:\MyProjects\FunctionGenerator\`
3. Import the project

### Running the Project for Development

To run the project for development:

1. From Code Composer Studio, click Build (hammer icon).
2. Attach a XDS510USB debugger to the development board.
3. Power up the electronics.
4. Click Debug (bug icon).
5. After the target has been flashed, click the start button (play icon).

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

## Acknowledgements

This project has been developed by Ethan Robotham.
