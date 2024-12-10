#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <bitset>
#include <stdexcept>
#include <algorithm>
#include <bits/stdc++.h>

using namespace std;


#define BASE_DATA_MEMORY 0
#define INSTRUCTION_MEMORY_BASE 0
vector<int> dataMemory(32*1024 , 0);
vector<string> insMemory(1024*4);
int currentIns = INSTRUCTION_MEMORY_BASE;

// Maps for registers and opcodes/functions
std::unordered_map<std::string, std::string> registerMap = {
    {"$zero", "00000"}, {"$at", "00001"}, // Zero and assembler temporary
    {"$v0", "00010"},
    {"$v1", "00011"}, // Function return values
    {"$a0", "00100"},
    {"$a1", "00101"},
    {"$a2", "00110"},
    {"$a3", "00111"}, // Function arguments
    {"$t0", "01000"},
    {"$t1", "01001"},
    {"$t2", "01010"},
    {"$t3", "01011"}, // Temporary registers
    {"$t4", "01100"},
    {"$t5", "01101"},
    {"$t6", "01110"},
    {"$t7", "01111"}, // Temporary registers
    {"$s0", "10000"},
    {"$s1", "10001"},
    {"$s2", "10010"},
    {"$s3", "10011"}, // Saved registers
    {"$s4", "10100"},
    {"$s5", "10101"},
    {"$s6", "10110"},
    {"$s7", "10111"}, // Saved registers
    {"$t8", "11000"},
    {"$t9", "11001"}, // More temporary registers
    {"$k0", "11010"},
    {"$k1", "11011"}, // Kernel registers
    {"$gp", "11100"}, // Global pointer
    {"$sp", "11101"}, // Stack pointer
    {"$fp", "11110"}, // Frame pointer
    {"$ra", "11111"}  // Return address
};

// Map Containing all the Opcodes for various instructions
std::unordered_map<std::string, std::string> opcodeMap = {
    {"add", "000000"}, {"sub", "000000"}, {"and", "000000"}, {"or", "000000"}, {"slt", "000000"}, {"lw", "100011"}, {"sw", "101011"}, {"beq", "000100"}, {"addi", "001000"}, {"li", "001000"}, {"j", "000010"}, {"lui", "001111"}, {"ori", "001101"}};

// Map containing funct values for various R-Type Instructions
std::unordered_map<std::string, std::string> functMap = {
    {"add", "100000"}, {"sub", "100010"}, {"and", "100100"}, {"or", "100101"}, {"slt", "101010"}};

// Labelled Map and dataMap for labelled values in .data section to point to address in Memory
std::unordered_map<std::string, int> labelledMap;
int labelCount = 0;
std::unordered_map<std::string, int> dataMap;
int dataAddress = 5000;  // Starting address for .data section

// Map for various Labels for the .text section
unordered_map<string, int> labelling;
unordered_map<int, int> words;
unordered_map<int, string> asciiz;

static inline std::string ltrim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    return s;
}

// Trim from the end (right side) of the string
static inline std::string rtrim(std::string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
    return s;
}

// Trim from both ends of the string
static inline std::string trim(std::string s) {
    return ltrim(rtrim(std::move(s)));
}

// Helper function to convert integer to binary string of specified length
std::string intToBinary(int num, int bits)
{
    if (num < 0)
    {
        num = (1 << bits) + num;
    }
    return std::bitset<32>(num).to_string().substr(32 - bits, bits);
}

// Function to compile R-type instructions
// Format -> 6 bit-opcode + 5-bit rs + 5-bit rt + 5-bit rd + 5-bit shamt + 6-bit funct
std::string compileRType(std::vector<std::string> &tokens)
{
    std::string op = opcodeMap[tokens[0]];
    std::string rs = registerMap[tokens[2]];
    std::string rt = registerMap[tokens[3]];
    std::string rd = registerMap[tokens[1]];
    std::string shamt = "00000";
    std::string funct = functMap[tokens[0]];

    return op + rs + rt + rd + shamt + funct;
}
// Function to compile I-type instructions
// Format -> 6 bit-opcode + 5-bit rs + 5-bit rt + 16-bit Address/Constant
std::string compileIType(std::vector<std::string> &tokens)
{
    std::string op;
    std::string rs, rt;
    int immediate;

    if (tokens[0] == "li")
    {
        // li $reg, imm is treated as addi $reg, $zero, imm
        op = opcodeMap["addi"];
        rs = registerMap["$zero"];        // $zero register
        rt = registerMap[tokens[1]];      // Destination register
        immediate = std::stoi(tokens[2]); // Immediate value
    }
    else if (tokens[0] == "addi")
    {
        op = opcodeMap["addi"];
        rs = registerMap[tokens[2]];
        rt = registerMap[tokens[1]];
        immediate = std::stoi(tokens[3]);
    }
    else if (tokens[0] == "lw" || tokens[0] == "sw")
    {
        op = opcodeMap[tokens[0]];
        rt = registerMap[tokens[1]];
        std::string offsetReg = tokens[2];

        size_t openParen = offsetReg.find('(');
        size_t closeParen = offsetReg.find(')');

        if (openParen != std::string::npos && closeParen != std::string::npos)
        {
            immediate = std::stoi(offsetReg.substr(0, openParen));
            rs = registerMap[offsetReg.substr(openParen + 1, closeParen - openParen - 1)];
        }
        else if(dataMap[offsetReg]!=0){
            immediate = dataMap[offsetReg];
            rs = intToBinary(0, 5);
        }
        else if (registerMap[offsetReg].length() == 5)
        {
            immediate = 0;
            rs = registerMap[offsetReg];
        }
        else
        {
            throw std::invalid_argument("Invalid lw/sw syntax");
        }
    }
    else
    {
        rs = registerMap[tokens[2]];
        rt = registerMap[tokens[1]];
        immediate = std::stoi(tokens[3]);
    }

    return op + rs + rt + intToBinary(immediate, 16);
}

// Function to compile 'la' insruction
std::vector<std::string> compileLAInstruction(std::vector<std::string> &tokens)
{
    std::vector<std::string> instructions;

    std::string rt = registerMap[tokens[1]]; // Destination register
    int address = dataMap[tokens[2]];    // Get address from label map

    // Split the address into upper 16 bits and lower 16 bits
    int upper = (address >> 16) & 0xFFFF;
    int lower = address & 0xFFFF;

    // First instruction: lui rt, upper
    instructions.push_back(opcodeMap["lui"] + "00000" + rt + intToBinary(upper, 16));
    insMemory[currentIns] = (opcodeMap["lui"] + "00000" + rt + intToBinary(upper, 16));
    currentIns++;

    // Second instruction: ori rt, rt, lower
    instructions.push_back(opcodeMap["ori"] + rt + rt + intToBinary(lower, 16));
    insMemory[currentIns] = (opcodeMap["ori"] + rt + rt + intToBinary(lower, 16));
    currentIns++;

    return instructions;
}

// Function to remove commas from the input line
std::string removeCommas(const std::string &line)
{
    std::string result = line;
    result.erase(std::remove(result.begin(), result.end(), ','), result.end());
    return result;
}

// To store the labels introduced in the data section
void processDataSection(std::ifstream &inFile)
{
    std::string line;
    
    while (std::getline(inFile, line)) {
        // Remove commas and trim whitespace
        line = removeCommas(line);
        line = trim(line);

        if (line.empty()) continue;

        // Check for label (variable) in the .data section
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            string label = "";
            int i = 0;
            for(;i <line.length(); i++)
            {
                if(line[i]!=' ')
                    break;
            }
            for(int i = 0; i<line.length(); i++)
            {
                if(line[i] == ':')
                    break;
                label+=line[i];
            }

            dataMap[label] = dataAddress;  // Assign current dataAddress to the label

            labelledMap[label] = dataAddress;
            
            // Check if it's a word (or other type) to allocate memory
            size_t directivePos = line.find(".word");
            if (directivePos != std::string::npos) {
                // Allocate 4 bytes (1 word)
                int i = directivePos;
                for(; i<line.length(); i++)
                {
                    if(line[i]==' ')
                        break;
                }
                for(; i<line.length(); i++)
                {
                    if(line[i] != ' ')
                        break;
                }
                int num = 0;
                for(; i<line.length(); i++)
                {
                    num = num*10 + line[i] - '0';
                }
                dataMemory[dataAddress] = num;
                dataAddress += 4;
            }
        }
    }
}


// Handle syscall
std::string compileSyscall()
{
    // Syscall is represented by the fixed binary string "00000000000000000000000000001100"
    return "00000000000000000000000000001100"; // Syscall binary format
}

// Handle move instructions
std::string compileMove(std::vector<std::string> &tokens)
{
    std::string op = opcodeMap["add"];       // 'move' is equivalent to 'add' with $zero
    std::string rs = registerMap[tokens[2]]; // Source register
    std::string rt = registerMap["$zero"];   // Move uses $zero as the second source
    std::string rd = registerMap[tokens[1]]; // Destination register
    std::string shamt = "00000";             // Shift amount is 0 for move
    std::string funct = functMap["add"];     // Use 'add' function

    return op + rs + rt + rd + shamt + funct;
}

// Going through the entire code and scanning the labels given in the .text section
void scanForLabels(string inputFile)
{
    std::ifstream inFile(inputFile);
    std::string line;
    int lineCount = 0;

    // Reset the stream to the beginning for label scanning
    inFile.clear();
    inFile.seekg(0);
    int inDataSection = false;

    while (std::getline(inFile, line))
    {
        line = removeCommas(line);
        line = trim(line);

        if (line == ".data") {
            inDataSection = true;
            continue;
        }

        if (line == ".text") {
            inDataSection = false;
            continue;
        }

        if(inDataSection)
            continue;

        // Skip empty lines
        if (line.empty())
            continue;

        // If the line contains a label (ending with ':')
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            std::string label = line.substr(0, colonPos);
            labelling[label] = lineCount; // Store the label with its line number
        }
        else
        {
            lineCount++;
        }
    }
}

// Compiling Branch Instructions like beq and bne
std::string compileBranchInstruction(std::vector<std::string> &tokens, int currentLine)
{
    std::string op = opcodeMap[tokens[0]];  // Get opcode (e.g., beq)
    std::string rs = registerMap[tokens[1]]; // Source register 1
    std::string rt = registerMap[tokens[2]]; // Source register 2

    // Calculate the branch offset based on the label location
    int labelAddress = labelling[tokens[3]];
    int offset = labelAddress;  // PC-relative offset

    return op + rs + rt + intToBinary(offset, 16);  // Return the binary representation
}

// Function to compile J-type instructions
// Format -> 6-bit Opcode + 26-bit Address
std::string compileJType(std::vector<std::string> &tokens)
{
    std::string op = opcodeMap[tokens[0]];
    int address = labelling[tokens[1]];

    return op + intToBinary(address, 26);
}

void processMIPSCode(const std::string &inputFile)
{
    std::ifstream inFile(inputFile);
    std::ofstream outFile("mips_binary.txt");

    if (!inFile || !outFile) {
        std::cerr << "Error opening file!" << std::endl;
        return;
    }

    std::string line;
    bool inDataSection = false;
    
    // First pass: Process the .data section to allocate memory addresses
    while (std::getline(inFile, line)) {
        line = trim(line);

        if (line == ".data") {
            inDataSection = true;
        }

        if (line == ".text") {
            inDataSection = false;
            break;
        }

        if (inDataSection) {
            processDataSection(inFile);
        }
    }

    // Reset the file stream to start processing instructions
    inFile.clear();
    inFile.seekg(0);

    int currentLine = 0;

    scanForLabels(inputFile);

    while (std::getline(inFile, line)) {
        line = trim(line);
        line = removeCommas(line);


        // Skip label definitions
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            line = line.substr(colonPos + 1);
        }

        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;

        while (iss >> token) {
            tokens.push_back(token);
        }

        if (tokens.empty()) continue;

        try {
            cout<<line<<endl;
            if (tokens[0] == "syscall") {
                outFile << compileSyscall() << std::endl;
                insMemory[currentIns] = compileSyscall();
                cout<<"Instruction code: "<<compileSyscall()<<endl;
                currentIns++;
                
            } else if (tokens[0] == "move") {
                outFile << compileMove(tokens) << std::endl;
                cout<<"Instruction code: "<<compileMove(tokens)<<endl;
            } else if (tokens[0] == "la") {
                std::vector<std::string> laInstructions = compileLAInstruction(tokens);
                for (const std::string &instruction : laInstructions) {
                    outFile << instruction << std::endl;
                }
            } else if (tokens[0] == "beq" || tokens[0] == "bne") {
                outFile << compileBranchInstruction(tokens, currentLine) << std::endl;
                insMemory[currentIns] = compileBranchInstruction(tokens, currentLine);
                cout<<"Instruction code: "<<compileBranchInstruction(tokens, currentLine)<<endl;
                currentIns++;
            } else if (opcodeMap[tokens[0]] == "000000") {
                outFile << compileRType(tokens) << std::endl;
                insMemory[currentIns] = compileRType(tokens);
                cout<<"Instruction code: "<<compileRType(tokens)<<endl;
                currentIns++;
            } else if (tokens[0] == "lw" || tokens[0] == "sw" || tokens[0] == "addi" || tokens[0] == "li") {
                outFile << compileIType(tokens) << std::endl;
                insMemory[currentIns] = compileIType(tokens);
                cout<<"Instruction code: "<<compileIType(tokens)<<endl;
                currentIns++;
            } else if (tokens[0] == "j") {
                outFile << compileJType(tokens) << std::endl;
                insMemory[currentIns] = compileJType(tokens);
                cout<<"Instruction code: "<<compileJType(tokens)<<endl;
                currentIns++;
            }
            cout<<endl;
            currentLine++;

        } catch (const std::invalid_argument &e) {
            std::cerr << "Error processing instruction: " << line << " - " << e.what() << std::endl;
        }
    }

    inFile.close();
    outFile.close();

    std::cout << "MIPS binary code saved to mips_binary.txt" << std::endl;
}


// Task 1 completed

// Task 2 Started

int registers[32] = {0}; // 32 MIPS registers
int PC = 0; // Program Counter


void initRegs() {
    for(int i = 0; i<32; i++)
    {
        registers[i] = i*32*4;
    }
}

// Define the control signals structure
struct ControlSignals {
    int RegDst;
    int RegWrite;
    int ALUSrc;
    int MemToReg;
    int MemRead;
    int MemWrite;
    int Branch;
    int Jump;
    bool ALUOp[2];
    int ALUControl = 0;
};

void printControl(ControlSignals control) {
    const int width = 15;

    cout << "Control Signal Values:" << endl;
    cout << "---------------------------" << endl;
    cout << left << setw(width) << "Signal" << "Value" << endl;
    cout << "---------------------------" << endl;
    
    cout << left << setw(width) << "RegDst" << control.RegDst << endl;
    cout << left << setw(width) << "ALUSrc" << control.ALUSrc << endl;
    cout << left << setw(width) << "MemToReg" << control.MemToReg << endl;
    cout << left << setw(width) << "RegWrite" << control.RegWrite << endl;
    cout << left << setw(width) << "MemRead" << control.MemRead << endl;
    cout << left << setw(width) << "MemWrite" << control.MemWrite << endl;
    cout << left << setw(width) << "Branch" << control.Branch << endl;
    cout << left << setw(width) << "Jump" << control.Jump << endl;
    
    cout << left << setw(width) << "ALUOp[2]" << control.ALUOp[0] << " " << control.ALUOp[1] << endl;
    
    cout << left << setw(width) << "ALUControl" << control.ALUControl << endl;
    
    cout << "---------------------------" << endl;
}

ControlSignals generateRControlSignals(string opcode, string funct) {
    ControlSignals control = {};

    // Define opcodes
    const string R_TYPE = "000000";
    const string LW     = "100011";
    const string SW     = "101011";
    const string BEQ    = "000100";
    const string ADDI   = "001000";
    const string LI     = "001000";  // li shares the same opcode as addi
    const string J      = "000010";
    const string LUI    = "001111";
    const string ORI    = "001101";

    // Define function codes for R-type instructions
    const string ADD    = "100000";
    const string SUB    = "100010";
    const string AND    = "100100";
    const string OR     = "100101";
    const string SLT    = "101010";

    // Handle R-type instructions based on function code
    if (opcode == R_TYPE) {
        control.RegDst = 1;
        control.RegWrite = 1;
        control.ALUOp[1] = true; // R-type uses ALUOp = 10

        if (funct == ADD || funct == SUB || funct == AND || funct == OR || funct == SLT) {
            // R-type instructions
            if (funct == ADD) {
                // ALU performs addition (ALU control logic handled separately)
                control.ALUControl = 0;
            } else if (funct == SUB) {
                // ALU performs subtraction
                control.ALUControl = 1;
            } else if (funct == AND) {
                // ALU performs AND
                control.ALUControl = 2;
            } else if (funct == OR) {
                // ALU performs OR
                control.ALUControl = 3;
            } else if (funct == SLT) {
                // ALU performs Set Less Than
                control.ALUControl = 4;
            }
        }
        return control;
    }

    // Handle I-type and J-type instructions based on opcode
    control.ALUSrc = (opcode == LW) | (opcode == SW) | (opcode == ADDI) | (opcode == LI) | (opcode == ORI) | (opcode == LUI);  // ALUSrc for lw, sw, addi, li, ori
    control.MemToReg = (opcode == LW);             // MemToReg is 1 for lw
    control.MemRead = (opcode == LW);              // MemRead is 1 for lw
    control.MemWrite = (opcode == SW);             // MemWrite is 1 for sw
    control.Branch = (opcode == BEQ);              // Branch is 1 for beq
    control.RegWrite = (opcode == LW) | (opcode == ADDI) | (opcode == LI) | (opcode == ORI) | (opcode == LUI); // RegWrite for lw, addi, li, ori, lui
    control.Jump = (opcode == J);                  // Jump is 1 for j

    // ALUOp encoding for specific instructions
    control.ALUOp[1] = (opcode == R_TYPE);           // ALUOp[1] is 1 for R-type
    control.ALUOp[0] = (opcode == BEQ);              // ALUOp[0] is 1 for beq (subtraction)

    return control;
}

//001101

ControlSignals generateControlSignals(string opcode, string funct = "") {
    ControlSignals control = {};

    // Define opcodes
    const string R_TYPE = "000000";
    const string LW     = "100011";
    const string SW     = "101011";
    const string BEQ    = "000100";
    const string ADDI   = "001000";
    const string LI     = "001000";  // li shares the same opcode as addi
    const string J      = "000010";
    const string LUI    = "001111";
    const string ORI    = "001101";

    // Define function codes for R-type instructions
    const string ADD    = "100000";
    const string SUB    = "100010";
    const string AND    = "100100";
    const string OR     = "100101";
    const string SLT    = "101010";

    // Handle R-type instructions based on function code
    if (opcode == R_TYPE) {
        control.RegDst = 1;
        control.RegWrite = 1;
        control.ALUOp[1] = true; // R-type uses ALUOp = 10

        if (funct == ADD || funct == SUB || funct == AND || funct == OR || funct == SLT) {
            // R-type instructions
            if (funct == ADD) {
                // ALU performs addition (ALU control logic handled separately)
                control.ALUControl = 0;
            } else if (funct == SUB) {
                // ALU performs subtraction
                control.ALUControl = 1;
            } else if (funct == AND) {
                // ALU performs AND
                control.ALUControl = 2;
            } else if (funct == OR) {
                // ALU performs OR
                control.ALUControl = 3;
            } else if (funct == SLT) {
                // ALU performs Set Less Than
                control.ALUControl = 4;
            }
        }
        return control;
    }

    // Handle I-type and J-type instructions based on opcode
    control.ALUSrc = (opcode == LW) | (opcode == SW) | (opcode == ADDI) | (opcode == LI) | (opcode == ORI) | (opcode == LUI);  // ALUSrc for lw, sw, addi, li, ori
    control.MemToReg = (opcode == LW);             // MemToReg is 1 for lw
    control.MemRead = (opcode == LW);              // MemRead is 1 for lw
    control.MemWrite = (opcode == SW);             // MemWrite is 1 for sw
    control.Branch = (opcode == BEQ);              // Branch is 1 for beq
    control.RegWrite = (opcode == LW) | (opcode == ADDI) | (opcode == LI) | (opcode == ORI) | (opcode == LUI); // RegWrite for lw, addi, li, ori, lui
    control.Jump = (opcode == J);                  // Jump is 1 for j

    // ALUOp encoding for specific instructions
    control.ALUOp[1] = (opcode == R_TYPE);           // ALUOp[1] is 1 for R-type
    control.ALUOp[0] = (opcode == BEQ);              // ALUOp[0] is 1 for beq (subtraction)

    // ALUOp for immediate instructions (addi, ori)
    if (opcode == ADDI || opcode == LI) {
        // ALU performs addition for addi and li
    } else if (opcode == ORI) {
        // ALU performs bitwise OR for ori
    }

    return control;
}

// Function to simulate ALU operations
int ALU(int operand1, int operand2, bool* ALUOp, int ALUcontrol) {
    if (ALUOp[1]) { // Add for R-type or lw/sw
        if(ALUcontrol == 0)
        {
            int op1 = dataMemory[operand1];
            int op2 = dataMemory[operand2];
            return op1 + op2;
        }
        else if(ALUcontrol == 1)
        {
            int op1 = dataMemory[operand1];
            int op2 = dataMemory[operand2];
            return op1 - op2;
        }
        else if (ALUcontrol == 2)
        {
            int op1 = dataMemory[operand1];
            int op2 = dataMemory[operand2];
            return (op1 & op2);
        }
        else if (ALUcontrol == 3)
        {
            int op1 = dataMemory[operand1];
            int op2 = dataMemory[operand2];
            return (op1 | op2);
        }
        else if (ALUcontrol == 4)
        {
            int op1 = dataMemory[operand1];
            int op2 = dataMemory[operand2];
            if(op1 < op2)
            return op1;
            else
            return op2;
        }
        else
        {
            int op1 = dataMemory[operand1];
            int op2 = dataMemory[operand2];
            return op1 + op2;
        }
    } else if (ALUOp[0]) { // Subtract for beq
        int op1 = dataMemory[operand1];
        int op2 = dataMemory[operand2];
        return op1 - op2;
    }
    int op1 = dataMemory[operand1];
    int op2 = dataMemory[operand2];
    return op1 + op2;
}

// Simulate Instruction Fetch (IF) stage
string instructionFetch(vector<string>& instructionMemory) {
    return instructionMemory[PC++];
}

// Simulate Instruction Decode (ID) stage
void instructionDecode(const string& instruction, string& opcode, string& rs, string& rt, string& rd, int& immediate) {
    opcode = instruction.substr(0, 6); // First 6 bits are opcode
    rs = instruction.substr(6, 5); // 5 bits for rs
    rt = instruction.substr(11, 5); // 5 bits for rt
    rd = instruction.substr(16, 5); // 5 bits for rd (R-type) or immediate (I-type)
    immediate = stoi(instruction.substr(16), nullptr, 2); // Immediate for I-type
}

int execute(const string& opcode, int rsVal, int rtVal, int immediate, bool* ALUOp, ControlSignals control) {
    int ALUResult;

    // Check if the instruction is an I-type (e.g., addi) by using control signals
    if (control.ALUSrc) {
        // ALUSrc = 1 indicates that we are dealing with an I-type instruction like addi
        // Perform ALU operation with the immediate value
        if(opcode == "001000")
            ALUResult = dataMemory[rsVal] + immediate;
        else if(opcode == "101011") {
            ALUResult = rsVal + immediate;
        }
        else
            ALUResult = ALU(rsVal, immediate, ALUOp, control.ALUControl); // Immediate value for addi or lw/sw
        cout << "ALU operation with immediate: rsVal + immediate = " << ALUResult << endl;
    } else {
        // ALUSrc = 0 indicates R-type instruction, so we use rtVal
        ALUResult = ALU(rsVal, rtVal, ALUOp, control.ALUControl); // R-type instruction
        cout << "ALU operation with register values: rsVal + rtVal = " << ALUResult << endl;
    }

    return ALUResult;
}

// Simulate Memory Access (MEM) stage
int memoryAccess(ControlSignals control, int ALUResult, int rtVal) {
    if (control.MemRead) {
        return dataMemory[ALUResult]; // Load word from memory
    } else if (control.MemWrite) {
        dataMemory[ALUResult] = dataMemory[rtVal]; // Store word in memory
        cout<<endl<<"Store Word Instruction execute:"<<endl;
        cout<<"Stored " << dataMemory[ALUResult] << " at  address " << ALUResult << " in the Memory." << endl;
        cout<<endl;
    }
    return 0;
}

// Simulate Write Back (WB) stage
void writeBack(ControlSignals control, int ALUResult, int memData, int& writeReg) {
    if (control.RegWrite) {
        dataMemory[registers[writeReg]] = ALUResult; // Write data from memory
    } else if (control.MemToReg) {
        dataMemory[registers[writeReg]] = memData; // Write result from ALU
    }
}

void printReg() {
    // Print header
    cout << left << setw(10) << "Register" << "|" << setw(15) << "Value" << endl; // Header with vertical line
    cout << string(28, '-') << endl; // Separator between header and values

    for (int i = 0; i < 32; i++) {
        // Print register name and value with vertical line
        cout << left << setw(10) << ("Reg " + to_string(i)) << "| " << setw(15) << dataMemory[registers[i]] << endl;
    }
}
bool isRType(const string& instruction) {
    string opcode = instruction.substr(0, 6); // R-Type opcode is 000000
    return opcode == "000000";
}

bool isIType(const string& instruction) {
    string opcode = instruction.substr(0, 6);
    // List of I-Type opcodes: addi, lw, sw, etc.
    return opcode == "001000" || opcode == "100011" || opcode == "101011" || opcode == "001111" || opcode == "001101" || opcode == "000100"; // addi, lw, sw for example
}

bool isJType(const string& instruction) {
    string opcode = instruction.substr(0, 6);
    // J-Type instructions are j and jal
    return opcode == "000010" || opcode == "000011"; // j, jal
}

void instructionDecodeR(const string& instruction, string& opcode, string& rs, string& rt, string& rd, string& shamt, string& funct) {
    opcode = instruction.substr(0, 6);    // opcode (6 bits)
    rs = instruction.substr(6, 5);        // source register 1 (5 bits)
    rt = instruction.substr(11, 5);       // source register 2 (5 bits)
    rd = instruction.substr(16, 5);       // destination register (5 bits)
    shamt = instruction.substr(21, 5);
    funct = instruction.substr(26, 6);
}

void instructionDecodeI(const string& instruction, string& opcode, string& rs, string& rt, int& immediate) {
    opcode = instruction.substr(0, 6);     // opcode (6 bits)
    rs = instruction.substr(6, 5);         // source register 1 (5 bits)
    rt = instruction.substr(11, 5);        // destination/source register (5 bits)
    immediate = stoi(instruction.substr(16, 16), nullptr, 2);  // immediate value (16 bits)
}

void instructionDecodeJ(const string& instruction, string& opcode, int& address) {
    opcode = instruction.substr(0, 6);    // opcode (6 bits)
    address = stoi(instruction.substr(6, 26), nullptr, 2); // address (26 bits)
}

int binaryStringToInteger(const string& binaryString) {
    
    int result = 0;

    // Iterate over each character in the binary string
    for (int i = 0; i < 16; ++i) {
        // Check if the character is '0' or '1'
        if (binaryString[i] != '0' && binaryString[i] != '1') {
            throw invalid_argument("Input string must only contain '0' and '1'.");
        }
        
        // Calculate the value
        result = result * 2 + (binaryString[i] - '0'); // Convert char to int and update result
    }

    return result;
}

void execution() {
    // Reading instructions from the input file
    ifstream inputFile("mips_binary.txt");
    vector<string> instructionMemory;

    // if (!inputFile) {
    //     cerr << "Error: Could not open mips_binary.txt" << endl;
    //     return;
    // }

    // string instruction;
    // while (getline(inputFile, instruction)) {
    //     instructionMemory.push_back(instruction);
    // }

    // inputFile.close();

    for(int i = 0; insMemory[i]!="" && i<insMemory.size(); i++)
    {
        instructionMemory.push_back(insMemory[i]);
    }

    // Simulate the execution
    while (PC < instructionMemory.size()) {
        // Fetch the instruction
        string instruction = instructionFetch(instructionMemory);
        cout << left << setw(10) << "PC:" << setw(10) << (PC * 4)<<endl << "Instruction:" << instruction << endl;
        cout<<endl;

        // Variables for decoded fields
        string opcode, rs, rt, rd, shamt, funct;
        int immediate, address;

        // Decode based on the type
        if (isRType(instruction)) {
            instructionDecodeR(instruction, opcode, rs, rt, rd, shamt, funct);
        } else if (isIType(instruction)) {
            instructionDecodeI(instruction, opcode, rs, rt, immediate);
        } else if (isJType(instruction)) {
            instructionDecodeJ(instruction, opcode, address);
        }

        // Generate control signals
        
        ControlSignals control = generateControlSignals(opcode);
        control = generateRControlSignals(opcode, funct);
        
        if(isJType(instruction))
        {
            printControl(control);
            printReg();
            PC = address;
            continue;
        }

        // Get register values
        int rsVal = registers[stoi(rs, nullptr, 2)];
        int rtVal = registers[stoi(rt, nullptr, 2)];
        int writeReg = (isRType(instruction)) ? stoi(rd, nullptr, 2) : stoi(rt, nullptr, 2);

        // Execute the instruction
        int ALUResult = execute(opcode, rsVal, rtVal, immediate, control.ALUOp, control);

        if(control.Branch && (ALUResult == 0))
        {
            PC = binaryStringToInteger(instruction.substr(16,16));
        }

        // Memory Access
        int memData = memoryAccess(control, ALUResult, rtVal);

        // Write Back
        if(opcode == "001101")
        {
            if(words[ALUResult]!=0)
                ALUResult = words[ALUResult];
        }
        writeBack(control, ALUResult, memData, writeReg);

        // Debugging output
        printControl(control);
        printReg();
        cout<<endl;
    }
}

// Task 2 completed
void printInstruction(){
    for(int i = INSTRUCTION_MEMORY_BASE; i<currentIns; i++)
    {
        cout<<insMemory[i]<<endl;
    }
}

int main()
{
    std::string inputFile = "test_code_5_mips_sim.asm";
    initRegs();
    processMIPSCode(inputFile);
    cout << "The compiled instructions are: "<<endl;
    // printInstruction();
    cout<<endl;
    execution();
    return 0;
}


