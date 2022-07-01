//
// Created by StillMe on 2022/6/22.
//

#ifndef RISC_V_SIMULATOR_PREDICTOR_H
#define RISC_V_SIMULATOR_PREDICTOR_H

//#define ONE_LEVEL_PREDICTOR
//#define TWO_LEVEL_PREDICTOR
//#define GLOBAL_PREDICTOR
#define COMPETITIVE_PREDICTOR

#include "utility.h"
#include <cstring>

#ifdef ONE_LEVEL_PREDICTOR

class Predictor {
    friend class cpu;
private:
    u8 bht[4096];  // Branch History Table, 取pc的后12位
    u32 btb[256];  // Branch Target Buffer, 取pc的后8位
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
//        return predPc = pc + 4;
        if (INSTRUCTION::Instruction::isBranchIns(ins) && (bht[pc & 0xfffu] & 0b10u)) {
            return predPc = btb[pc & 0xffu];
        } else {
            return predPc = pc + 4;
        }
    }

    void update(u32 pc, u32 jd, bool isBranchTaken, bool isPredSuccess) {
        totPred++;
        goodPred += isPredSuccess;
        if (isBranchTaken) {
            if (bht[pc & 0xfffu] < 0b11u) bht[pc & 0xfffu]++;
            btb[pc & 0xffu] = jd;
        } else {
            if (bht[pc & 0xfffu] > 0b00u) bht[pc & 0xfffu]--;
        }
    }
};

#endif

#ifdef TWO_LEVEL_PREDICTOR

class Predictor {
    friend class cpu;
private:
    u8 bht[256];      // Branch History Table, 取pc的后8位
    u8 pht[256][64];  // Pattern History Table
    u32 btb[256];     // Branch Target Buffer, 取pc的后8位
    u32 predPc;
    u32 totPred, goodPred;
#define INS (ins & 0xffu)
#define PC ((pc >> 2) & 0xffu)
public:
    /*
     * 11 : strongly taken     10 : weakly taken
     * 01 : weakly not taken   00 : strongly not taken
     */
    Predictor() : predPc(0), totPred(0), goodPred(0) {
        std::memset(bht, 0, sizeof(bht));
        std::memset(pht, 0b01u, sizeof(pht));
        std::memset(btb, 0, sizeof(btb));
    }
    ~Predictor() { }

    u32 predictPC(u32 pc, u32 ins) {
        if (INSTRUCTION::Instruction::isBranchIns(ins) && (pht[PC][bht[PC]] & 0b10u)) {
            return predPc = btb[PC];
        } else {
            return predPc = pc + 4;
        }
    }

    void update(u32 pc, u32 jd, bool isBranchTaken, bool isPredSuccess) {
        totPred++;
        goodPred += isPredSuccess;
        if (isBranchTaken) {
            if (pht[PC][bht[PC]] < 0b11u) pht[PC][bht[PC]]++;
            bht[PC] = (bht[PC] >> 1) | 0b100000u;
            btb[PC] = jd;
        } else {
            if (pht[PC][bht[PC]] > 0b00u) pht[PC][bht[PC]]--;
            bht[PC] >>= 1;
        }
    }
#undef INS
#undef PC
};

#endif

#ifdef GLOBAL_PREDICTOR

class Predictor {
    friend class cpu;
private:
    u32 ghr;           // Global History Table
    u8 pht[256][512];     // Pattern History Table
    u32 btb[256];      // Branch Target Buffer, 取pc的后8位
    u32 predPc;
    u32 totPred, goodPred;
#define INS (ins & 0xffu)
#define PC ((pc >> 2) & 0xffu)
#define PCg ((pc ^ ghr) & 0xffffu)
public:
    /*
     * 11 : strongly taken     10 : weakly taken
     * 01 : weakly not taken   00 : strongly not taken
     */
    Predictor() : ghr(0), predPc(0), totPred(0), goodPred(0) {
        std::memset(pht, 0b01u, sizeof(pht));
        std::memset(btb, 0, sizeof(btb));
    }
    ~Predictor() { }

    u32 predictPC(u32 pc, u32 ins) {
        if (INSTRUCTION::Instruction::isBranchIns(ins) && (pht[PC][ghr] & 0b10u)) {
            return predPc = btb[PC];
        } else {
            return predPc = pc + 4;
        }
    }

    void update(u32 pc, u32 jd, bool isBranchTaken, bool isPredSuccess) {
        totPred++;
        goodPred += isPredSuccess;
        if (isBranchTaken) {
            if (pht[PC][ghr] < 0b11u) pht[PC][ghr]++;
            btb[PC] = jd;
        } else {
            if (pht[PC][ghr] > 0b00u) pht[PC][ghr]--;
        }
        ghr = (((ghr << 1) | isBranchTaken) & 0xffu);
    }


#undef INS
#undef PC
#undef PCg
};

#endif

#ifdef COMPETITIVE_PREDICTOR

class Predictor {
    friend class cpu;
private:
    u32 ghr;            // Global History Register, 12位
    u8 bhr[256];        // Branch History Register, 6位
    u8 gpht[4096];  // Global Pattern History Table, [pc][ghr]
    u8 bpht[256][64];  // Branch Pattern History Table, [pc][bhr]
    u32 btb[256];       // Branch Target Buffer
    u8 cpht[4096];       // Choice PHT, 对于每个pc有一个二位饱和计数器, 判断采用哪种预测方法
    u32 predPc;
    u32 totPred, goodPred;
    u32 localCnt, globalCnt;
#define INS (ins & 0xffu)
#define PC ((pc >> 2) & 0xffu)
#define PCg ((pc ^ ghr) & 0xfffu)
public:
    /*
     * 11 : strongly taken     10 : weakly taken
     * 01 : weakly not taken   00 : strongly not taken
     */
    Predictor() : ghr(0), predPc(0), totPred(0), goodPred(0), localCnt(0), globalCnt(0) {
        std::memset(bhr, 0, sizeof(bhr));
        std::memset(gpht, 0b01u, sizeof(gpht));
        std::memset(bpht, 0b01u, sizeof(bpht));
        std::memset(btb, 0, sizeof(btb));
        std::memset(cpht, 0b10u, sizeof(cpht));  // weakly take local
    }
    ~Predictor() { }

    u32 predictPC(u32 pc, u32 ins) {
        if (INSTRUCTION::Instruction::isBranchIns(ins)) {
            if (cpht[ghr] & 0b10) {  // 采用局部预测
                localCnt++;
                if (bpht[PC][bhr[PC]] & 0b10u) {
                    return predPc = btb[PC];
                }
            } else {                 // 采用全局预测
                globalCnt++;
                if (gpht[PCg] & 0b10u) {
                    return predPc = btb[PC];
                }
            }
        }
        return predPc = pc + 4;
    }

    void update(u32 pc, u32 jd, bool isBranchTaken, bool isPredSuccess) {
        totPred++;
        goodPred += isPredSuccess;
        bool bhrTaken = (bpht[PC][bhr[PC]] >= 0b10u);
        bool ghrTaken = (gpht[PCg] >= 0b10u);
        if (bhrTaken ^ ghrTaken) {
            if (isBranchTaken && bhrTaken && cpht[ghr] < 0b11u) cpht[ghr]++;
            if (isBranchTaken && ghrTaken && cpht[ghr] > 0b00u) cpht[ghr]--;
        }
        if (isBranchTaken) {
            if (bpht[PC][bhr[PC]] < 0b11u) bpht[PC][bhr[PC]]++;
            bhr[PC] = (bhr[PC] >> 1) | 0x2fu;
            if (gpht[PCg] < 0b11u) gpht[PCg]++;
            ghr = (ghr >> 1) | 0x800u;
            btb[PC] = jd;
        } else {
            if (bpht[PC][bhr[PC]] > 0b00u) bpht[PC][bhr[PC]]--;
            bhr[PC] >>= 1;
            if (gpht[PCg] > 0b00u) gpht[PCg]--;
            ghr >>= 1;
        }
    }
#undef INS
#undef PC
#undef PCg
};

#endif


#endif //RISC_V_SIMULATOR_PREDICTOR_H
