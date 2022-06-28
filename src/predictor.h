//
// Created by StillMe on 2022/6/22.
//

#ifndef RISC_V_SIMULATOR_PREDICTOR_H
#define RISC_V_SIMULATOR_PREDICTOR_H


#include "utility.h"
#include <cstring>

class Predictor {
    friend class cpu;
private:
    u8 bht[4096];  // Branch History Table, 取ins的后12位
    u32 btb[256];  // Branch Target Buffer, 取ins的后8位
    u32 predPc;
    u32 totPred, goodPred;
public:
    /*
     * 11 : strongly taken     10 : weakly taken
     * 01 : weakly not taken   00 : strongly not taken
     */
    Predictor() : predPc(0), totPred(0), goodPred(0) {
        std::memset(bht, 0b01u, sizeof(bht));
        std::memset(btb, 0, sizeof(btb));
    }
    ~Predictor() { }

    u32 predictPC(u32 pc, u32 ins) {
        return predPc = pc + 4;
        totPred++;
        if (INSTRUCTION::Instruction::isBranchIns(ins) && bht[ins & 0xfffu] & 0b10u) {
            return predPc = btb[ins & 0xffu];
        } else {
            return predPc = pc + 4;
        }
    }

    void update(u32 ins, u32 jd, bool isBranchTaken, bool isPredSuccess) {
        goodPred += isPredSuccess;
        if (isBranchTaken) {
            bht[ins & 0xfffu]++;
            btb[ins & 0xffu] = jd;
        } else {
            bht[ins & 0xfffu]--;
        }
    }
};


#endif //RISC_V_SIMULATOR_PREDICTOR_H
