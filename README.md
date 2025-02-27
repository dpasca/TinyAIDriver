# TinyAIDriver

A minimal obstacle avoidance self-driving AI written in C++.
The AI is built on a **Neural Network** trained with Genetic Algorithms.

The goal of this project is to have a minimal neural net training infrastructure for simple simulations where controllers are involved.

![TinyAIDriver Screenshot](Docs/tinyaidriver_screenshot.png)

## Features

- Neural network-driven self-driving AI (obstacle avoidance)
- Neural Network training and inference in C++ with no external dependencies
- Real-time visualization of the simulation and training progress
- Minimal immediate-mode rendering API based on OpenGL

## Building and Running

### Prerequisites

- CMake 3.7 or higher
- C++20 compatible compiler
- Git (Git Bash on Windows)

### Getting the Dependencies

Required external dependencies are fetched by calling:

```bash
./get_externals.sh
```

### Building the Project

```bash
./build.sh
```

The executable will be placed in the `_bin` directory.

### Running the Simulation

```bash
./_bin/TinyFreeway
```

Wait a few seconds while the first batch of networks is trained, then the simulation will start to play, progressively improving over time.

#### Command-Line Options

- `--help`: Display help information
- `--use_swrenderer`: Use software rendering instead of hardware acceleration
- `--autoexit_delay <frames>`: Automatically exit after a specified number of frames
- `--autoexit_savesshot <fname>`: Save a screenshot before automatic exit

## Controls

The user interaction is limited to tweaking the GUI controls. It's safe to play and see.

## Project Structure

- `TinyFreeway/`: Self-driving AI implementation and simulation
- `Common/`: Core utilities for SDL2, OpenGL, and ImGui integration
- `_externals/`: External dependencies

## External Dependencies

Graphic display and user interface are implemented using the following libraries:
- `SDL2`
- `ImGui`
- `GLM`
- `fmt`

## License

See the `license.txt` file for details.

## Acknowledgements

This project is a spin-off of `Demo9` from the [dpasca-sdl2-template](https://github.com/dpasca/dpasca-sdl2-template) repository.
