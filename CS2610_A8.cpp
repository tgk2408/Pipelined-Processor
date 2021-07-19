/***********************************************************************************************************
 *  $Id$
 *;  File:    CS2610_A8.cpp
 *
 *  Author:  Madhav Mittal    Kshitij Raj    Mandala Tejesh   
 *
 *  Roll No: CS19B029         CS19B061       CS19B062
 *
 *  Purpose: Scalar Pipelined Processor Design
 * 
 *  Input: 
            Files:
                * ICache.txt
                * DCache.txt
                * RF.txt
 *  Output:
            Files:
                * DCache.txt
                * Output.txt
 *  
*************************************************************************************/
#include <bits/stdc++.h>
#include <cstdint>
#include <fstream>
#include <iostream>

using namespace std;

/*
    temporary registers being used by the processor:

    1b -- Cond (boolean)
    1B -- A
    1B -- B 
    1B -- ALUOutput
    1B -- LMD
    1B -- PC
    1B -- L1
    1B -- X
    2B -- IR 
*/

typedef int8_t data;
typedef unsigned char instructionData;
/* register type structure */
class reg {
public:
    bool usable = 1;
    int8_t value; //8bit signed value
};

/* decoded instruction */
class instruction {
public:
    int opcode;
    int op1;
    int op2;
    int op3;
};

/* pipeline stage */
class pipeline {
public:
    instruction ins;
    bool usable = 0;
};

class ProgramCounter {
public:
    bool usable = 1;
    unsigned char value;
};

int main() {
    int totalInstructions = 0,
        arithmeticInstructions = 0,
        logicalInstructions = 0,
        dataInstructions = 0,
        controlInstructions = 0,
        haltInstructions = 0,
        totalStalls = 0,
        dataStalls = 0,
        controlStalls = 0,
        cycles;
    float cpi = 1;
    ifstream icache, dcache, rf;
    bool IF = 1;                //InstructionFetch
    pipeline ID, EX, MEM, WB;   //required pipelines
    pipeline id, ex, mem, wb;   //temporary pipelines
    vector<reg> RF1;            //To temporarily store registers
    vector<reg> RF;             //Storing the registers
    vector<instructionData> I$; //To scan the instruction cache
    vector<instructionData> D$; //To scan the data cache
    reg A, B;                   //ALU input registers
    ProgramCounter PC, pc;
    int8_t ALUOutput, LMD, L1, X;
    bool Cond;             //branch condition
    unsigned short int IR; //Instruction register
    icache.open("ICache.txt");
    dcache.open("DCache.txt");
    rf.open("RF.txt");

    for (int i = 0; i < 16; i++) {
        int inpt;
        reg temp;
        rf >> hex >> inpt;
        temp.value = inpt;
        RF1.push_back(temp);
        RF.push_back(temp);
    }
    rf.close();
    for (int i = 0; i < 256; i++) {
        int inpt;
        instructionData temp;
        icache >> hex >> inpt;
        temp = inpt;
        I$.push_back(temp);
    }
    icache.close();
    for (int i = 0; i < 256; i++) {
        int inpt;
        instructionData temp;
        dcache >> hex >> inpt;
        temp = inpt;
        D$.push_back(temp);
    }
    dcache.close();

    pc = PC;
    for (cycles = 0;; cycles++) {
        id = ID;
        ex = EX;
        mem = MEM;
        wb = WB;
        for (int i = 0; i < 16; i++) {
            RF[i] = RF1[i];
        }
        if (wb.usable) {
            WB.usable = 0;
            if (wb.ins.opcode == 8) { //load
                RF1[wb.ins.op1].value = LMD;
                RF1[wb.ins.op1].usable = 1;
            } else if (wb.ins.opcode < 8) {
                RF1[wb.ins.op1].value = ALUOutput;
                RF1[wb.ins.op1].usable = 1;
            } else if (wb.ins.opcode == 15) { //halt
                cycles += 2;
                break;
            }
        }
        if (mem.usable) {
            MEM.usable = 0;
            if (mem.ins.opcode == 8) { //load
                LMD = D$[ALUOutput];
                WB.usable = 1;
                WB.ins = mem.ins;
            } else if (mem.ins.opcode == 9) { //store
                D$[ALUOutput] = RF[mem.ins.op1].value;
            }
        }
        if (ex.usable) {
            /* execute, multiple functions */
            EX.usable = 0;
            if (ex.ins.opcode == 8 || ex.ins.opcode == 9) { /* effective address for load/store */
                ALUOutput = A.value + X;
            } else if (ex.ins.opcode == 10) { //unconditional jump
                ALUOutput = pc.value + (L1 << 1);
                pc.value = ALUOutput;
                pc.usable = 1;
            } else if (ex.ins.opcode == 11) { //branch instruction
                ALUOutput = pc.value + (L1 << 1);
                Cond = (A.value == 0);
                if (Cond) {
                    pc.value = ALUOutput;
                }
                pc.usable = 1;
            } else { /* register-register ALU instruction */
                switch (ex.ins.opcode) {
                case 0:
                    ALUOutput = A.value + B.value;
                    break;
                case 1:
                    ALUOutput = A.value - B.value;
                    break;
                case 2:
                    ALUOutput = A.value * B.value;
                    break;
                case 3:
                    ALUOutput = A.value + 1;
                    break;
                case 4:
                    ALUOutput = A.value & B.value;
                    break;
                case 5:
                    ALUOutput = A.value | B.value;
                    break;
                case 6:
                    ALUOutput = ~A.value;
                    break;
                case 7:
                    ALUOutput = A.value ^ B.value;
                    break;
                default:
                    break;
                }
            }
            /* pass the instruction onto next stage */
            if (ex.ins.opcode == 8 || ex.ins.opcode == 9) { //data instructions
                MEM.usable = 1;
                MEM.ins = ex.ins;
            } else if (ex.ins.opcode != 10 && ex.ins.opcode != 11) {
                WB.usable = 1;
                WB.ins = ex.ins;
            }
        }
        if (id.usable) { /* decode the instruction in IR */
            id.ins.opcode = IR >> 12;
            id.ins.op3 = IR % 16;
            id.ins.op2 = (IR % 256) / 16;
            id.ins.op1 = (IR % 4096) / 256;

            if (id.ins.opcode <= 3) {     //arithematic operation
                if (id.ins.opcode == 3) { //increment
                    if (RF[id.ins.op1].usable) {
                        arithmeticInstructions++;
                        RF1[id.ins.op1].usable = 0;
                        RF[id.ins.op1].usable = 0;
                        IF = 1;
                        ID.usable = 0;
                        EX.ins = id.ins;
                        EX.usable = 1;
                        A = RF[id.ins.op1];
                    } else {
                        totalStalls++;
                        dataStalls++;
                    }
                } else { //add, sub, mul
                    if (RF[id.ins.op2].usable && RF[id.ins.op3].usable) {
                        arithmeticInstructions++;
                        RF1[id.ins.op1].usable = 0;
                        RF[id.ins.op1].usable = 0;
                        IF = 1;
                        ID.usable = 0;
                        EX.ins = id.ins;
                        EX.usable = 1;
                        A = RF[id.ins.op2];
                        B = RF[id.ins.op3];
                    } else {
                        totalStalls++;
                        dataStalls++;
                    }
                }
            } else if (id.ins.opcode <= 7) { //logical operation
                if (id.ins.opcode == 6) {    //not
                    if (RF[id.ins.op2].usable) {
                        logicalInstructions++;
                        RF1[id.ins.op1].usable = 0;
                        IF = 1;
                        ID.usable = 0;
                        EX.ins = id.ins;
                        EX.usable = 1;
                        A = RF[id.ins.op2];
                    } else {
                        totalStalls++;
                        dataStalls++;
                    }
                } else {
                    if (RF[id.ins.op2].usable && RF[id.ins.op3].usable) { //and, or, xor
                        logicalInstructions++;
                        RF1[id.ins.op1].usable = 0;
                        IF = 1;
                        ID.usable = 0;
                        EX.ins = id.ins;
                        EX.usable = 1;
                        A = RF[id.ins.op2];
                        B = RF[id.ins.op3];
                    } else {
                        totalStalls++;
                        dataStalls++;
                    }
                }
            } else if (id.ins.opcode == 8) { //load
                if (RF[id.ins.op2].usable) {
                    dataInstructions++;
                    RF1[id.ins.op1].usable = 0;
                    IF = 1;
                    ID.usable = 0;
                    EX.ins = id.ins;
                    EX.usable = 1;
                    A = RF[id.ins.op2];
                    X = id.ins.op3 % 8;
                    if (id.ins.op3 > 7) { /* negative offset */
                        X++;
                        X *= -1;
                    }
                } else {
                    totalStalls++;
                    dataStalls++;
                }
            } else if (id.ins.opcode == 9) { //store
                if (RF[id.ins.op1].usable && RF[id.ins.op2].usable) {
                    dataInstructions++;
                    IF = 1;
                    ID.usable = 0;
                    EX.ins = id.ins;
                    EX.usable = 1;
                    A = RF[id.ins.op2];
                    X = id.ins.op3 % 8;
                    if (id.ins.op3 > 7) { /* negative offset */
                        X++;
                        X *= -1;
                    }
                } else {
                    totalStalls++;
                    dataStalls++;
                }
            } else if (id.ins.opcode == 10) { //unconditional jump

                L1 = IR / 16;
                L1 = L1 % 256;
                controlInstructions++;
                pc.usable = 0;
                PC.usable = 0;
                IF = 1;
                ID.usable = 0;
                EX.ins = id.ins;
                EX.usable = 1;
            } else if (id.ins.opcode == 11) { //branch
                if (RF[id.ins.op1].usable) {
                    L1 = IR % 256;
                    A = RF[id.ins.op1];
                    controlInstructions++;
                    pc.usable = 0;
                    PC.usable = 0;
                    IF = 1;
                    ID.usable = 0;
                    EX.ins = id.ins;
                    EX.usable = 1;
                } else {
                    totalStalls++;
                    dataStalls++;
                }
            } else if (id.ins.opcode == 15) { //halt
                haltInstructions++;
                IF = 0;
                ID.usable = 0;
                EX.ins = id.ins;
                EX.usable = 1;
            }
        }
        if (IF) {
            if (PC.usable) { //Checking from actual PC.
                /* fetch an instruction */
                totalInstructions++;
                IF = 0;
                IR = I$[PC.value];
                IR = IR << 8;
                IR = IR + I$[PC.value + 1];
                /* increment PC */
                pc.value += 2;
                ID.usable = 1;
            } else {
                totalStalls++;
                controlStalls++;
            }
        }
        PC = pc; //To give it the new value.
    }
    (totalInstructions == 0) ? cpi = 0 : cpi = (float)cycles / totalInstructions;

    ofstream outFile;
    outFile.open("Output.txt");
    outFile << "Total number of instructions executed: " << totalInstructions << endl;
    outFile << "Number of instructions in each class" << endl;
    outFile << "Arithmetic instructions              : " << arithmeticInstructions << endl;
    outFile << "Logical instructions                 : " << logicalInstructions << endl;
    outFile << "Data instructions                    : " << dataInstructions << endl;
    outFile << "Control instructions                 : " << controlInstructions << endl;
    outFile << "Halt instructions                    : " << haltInstructions << endl;
    outFile << "Cycles Per Instruction               : " << cpi << endl;
    outFile << "Total number of stalls               : " << totalStalls << endl;
    outFile << "Data stalls (RAW)                    : " << dataStalls << endl;
    outFile << "Control stalls                       : " << controlStalls << endl;
    outFile.close();

    ofstream dcacheout;
    dcacheout.open("DCache.txt");
    for (int i = 0; i < 256; i++) {
        dcacheout << hex << setfill('0') << setw(2) << (int)D$[i] << endl;
    }
    dcacheout.close();

    return 0;
}
/**************************************************************************
 *
 *                        End of CS2610_A8.cpp
 *
**************************************************************************/