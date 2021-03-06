//
// Created by StillMe on 2022/6/21.
//

#ifndef RISC_V_SIMULATOR_CPU_HPP
#define RISC_V_SIMULATOR_CPU_HPP

#include "utility.h"
#include "stage.h"
#include "predictor.hpp"

//#define SHOW_RESULT

class cpu {
    friend class Stage;
    friend class StageIF;
    friend class StageID;
    friend class StageEX;
    friend class StageMEM;
    friend class StageWB;
private:
    STORAGE::Memory* memory;
    STORAGE::Register* reg;
    StageIF* IF;
    StageID* ID;
    StageEX* EX;
    StageMEM* MEM;
    StageWB* WB;
    Stage::AFTER_WB_Buffer afterWB_Buffer;
    Predictor predictor;
    u32 pc;
    u32 MEM_StallCnt;
    u32 IF_ID_EX_Buffer_StallCnt;
    bool jumpOrBranchWrong;
    bool stopAll;
    u32 clk;

private:
    void discard() {
        // 相当于把后面的所有指令变空
        IF->nxtBuffer.clear();
        ID->nxtBuffer.clear();
    }

    void updateBuffer() {
        // process stalling
        if (MEM_StallCnt) return;
        // update pc
        if (!IF_ID_EX_Buffer_StallCnt) {
            if (jumpOrBranchWrong) {
                pc = EX->nxtBuffer.jd;
                discard();
            } else {
                pc = predictor.predPc;
            }
        }
        // ID->IF->EX->MEM->WB
        if (!IF_ID_EX_Buffer_StallCnt) {
            ID->preBuffer = IF->nxtBuffer;
            EX->preBuffer = ID->nxtBuffer;
            MEM->preBuffer = EX->nxtBuffer;
        } else {
            MEM->preBuffer.clear();
        }
        WB->preBuffer = MEM->nxtBuffer;
        afterWB_Buffer = WB->nxtBuffer;
    }

    void updateUnit() {
        if (MEM_StallCnt) return;
        IF->work();
        ID->work();
        EX->work();
        MEM->work();
        WB->work();
    }

    void updateStallCnt() {
        if (MEM_StallCnt) {
            MEM_StallCnt--;
            return;
        }
    }

public:
    cpu() : memory(new STORAGE::Memory),
            reg(new STORAGE::Register),
            IF(new StageIF(this)),
            ID(new StageID(this)),
            EX(new StageEX(this)),
            MEM(new StageMEM(this)),
            WB(new StageWB(this)),
            afterWB_Buffer{},
            predictor(),
            pc(0),
            MEM_StallCnt(0),
            IF_ID_EX_Buffer_StallCnt(0),
            jumpOrBranchWrong(false),
            stopAll(false),
            clk(0) { }

    ~cpu() {
        delete memory;
        delete reg;
        delete IF;
        delete ID;
        delete EX;
        delete MEM;
        delete WB;
    }

    void init(std::istream& inStream) {
        memory->readData(inStream);
    }

    void run() {
//        debugPrint("cpu start running!");
        clk = 0;
        while (!stopAll) {
            clk++;
            updateBuffer();
            updateUnit();
            updateStallCnt();
        }
//        debugPrint("stop All!");
#ifdef SHOW_RESULT
        debugPrint("clock:", clk);
        debugPrint("totPred:", predictor.totPred, "success:", predictor.goodPred);
        debugPrint("success rate:", (double)predictor.goodPred / predictor.totPred);
        std::cout << clk << std::endl;
        std::cout << "local:" << predictor.localCnt << "  global:" << predictor.globalCnt << std::endl;
        std::cout << (double)predictor.goodPred / predictor.totPred << '\t' << predictor.goodPred << '\t' << predictor.totPred << std::endl;
#endif
    }
};


#endif //RISC_V_SIMULATOR_CPU_HPP
