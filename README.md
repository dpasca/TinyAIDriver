# TinyAIDriver

A minimal obstacle avoidance self-driving AI written in C++.
The AI is built on neural networks trained with an evolutionary algorithm (aka Genetic Algorithms).

The goal of this project is to have a minimal ANN infrastructure for simple simulations where controllers are involved.

This is far from being optimal in any way. It's meant as a starting point and an educational tool.
Practical applications are possible, but for better performance you're better off using [LibTorch](https://github.com/dpasca/dpasca-pytorch-examples/blob/master/cpp/example1.cpp).

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

## Code overview

The actual AI engine, reusable part of this demo, is defined in the following sources:
- `TA_EvolutionEngine.h`
- `TA_SimpleNN.h`
- `TA_Tensor.h`
- `TA_TrainingManager.h`
- `TA_QuickThreadPool.h`

**SimpleNN** and **Tensor** are the low-level building blocks of the neural network.

**EvolutionEngine** is responsible for the genetic algorithm that given a population of neural networks and their fitness, produces a new generation of networks.

**TrainingManager** orchestrates the training process, by calling the evaluation function and passing the results to the EvolutionEngine.

**QuickThreadPool** is a simple thread pool implementation that allows the training process to be parallelized.

The simulation logic for the synthetic environment is contained in the `Simulation` class.

In `main.cpp`, a `calcFitnessFn` function is defined for `TrainingManager`, which is responsible for running the simulation with a given neural network and returning its fitness (success score).

The rest of the code is for display and user interface.

## Contacts

- You can find me on **X** with the handle [109mae](https://x.com/109mae)
- My company: [NEWTYPE](https://newtypekk.com/)
- My game studio: [OYK Games](https://oykgames.com/)

## License

See the `license.txt` file for details.

## Acknowledgements

This project is a spin-off of `Demo9` from the [dpasca-sdl2-template](https://github.com/dpasca/dpasca-sdl2-template) repository.
