# MIPS Processor Simulation Project

This project demonstrates the working of a MIPS processor through simulation and code execution. It includes a custom implementation in C++ (`MIPS.cpp`), test files for different assembly codes, and a Logisim circuit file for visualizing the processor's behavior.

---

## Project Structure

### Files:
1. **MIPS.cpp**  
   - Contains the implementation of the MIPS processor simulator.  
   - Simulates the MIPS instruction execution cycle, including fetch, decode, execute, memory access, and write-back stages.  

2. **Test Files:**  
   - `test_code_1_mips_sim.asm`  
   - `test_code_2_mips_sim.asm`  
   - `test_code_3_mips_sim.asm`  
   - `test_code_4_mips_sim.asm`  
   - `test_code_5_mips_sim.asm`  
   These files contain MIPS assembly code to test various instruction sets, including R-Type, I-Type, and J-Type instructions.  

3. **mips_circuit.circ**  
   - Logisim circuit file for simulating the MIPS processor's operation visually.  
   - Includes components like the ALU, control unit, registers, and memory modules.  

---

## Features

1. **Instruction Simulation:**  
   - Supports R-Type, I-Type, and J-Type instructions.  
   - Implements control signal generation, ALU operations, and memory access.  

2. **Test Coverage:**  
   - Comprehensive testing with five MIPS assembly files.  
   - Includes operations like addition, subtraction, memory load/store, branches, and jumps.  

3. **Visual Circuit Simulation:**  
   - Logisim file to visualize the working of the MIPS architecture.  
   - Enables debugging and a deeper understanding of the MIPS pipeline.

---

## How to Run

### 1. Running the MIPS Simulator
- **Requirements:**  
  - A C++ compiler (e.g., g++, clang++).  

- **Steps:**  
  1. Compile the simulator using:  
     ```bash
     g++ -o MIPS MIPS.cpp
     ```  
  2. Run the simulator with a test file:  
     ```bash
     ./MIPS test_code_1_mips_sim.asm
     ```  
  3. Observe the output in the console, showing the simulation results.

### 2. Using the Logisim Circuit
- **Requirements:**  
  - [Logisim Evolution](https://github.com/logisim-evolution/logisim-evolution).  

- **Steps:**  
  1. Open `mips_circuit.circ` in Logisim.  
  2. Load the assembly test code into the memory module.  
  3. Simulate the circuit step-by-step to observe the instruction flow.  

---

## Example Output

### Console Output:
When running `test_code_1_mips_sim.asm`:  


### Logisim Output:
- Register updates.  
- ALU computations.  
- Branching and jump behaviors.

---

## Future Improvements
- Add support for floating-point instructions.  
- Enhance memory management for larger programs.  
- Create a GUI for easier interaction with the simulator and visualizer.  

--
