//
// Created by StillMe on 2022/6/21.
//

#ifndef RISC_V_SIMULATOR_CPU_HPP
#define RISC_V_SIMULATOR_CPU_HPP

#include "utility.h"
#include "stage.h"
#include "predictor.h"

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
    Predictor predictor;
    u32 pc;
    u32 MEM_StallCnt;
    u32 IF_ID_EX_Buffer_StallCnt;
    bool stopAll;

private:
    void updateBuffer() {

        // process stalling
        if (MEM_StallCnt) return;
//        if (IF_ID_EX_StallCnt) return;

        // update pc
        if (EX->nxtBuffer.jd != -1u) pc = EX->nxtBuffer.jd;
        // ID->IF->EX->MEM->WB
        if (!IF_ID_EX_Buffer_StallCnt) {
            ID->preBuffer = IF->nxtBuffer;
            EX->preBuffer = ID->nxtBuffer;
            MEM->preBuffer = EX->nxtBuffer;
        }
        WB->preBuffer = MEM->nxtBuffer;
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
        if (IF_ID_EX_Buffer_StallCnt) {
            IF_ID_EX_Buffer_StallCnt--;
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
            pc(0),
            MEM_StallCnt(0),
            IF_ID_EX_Buffer_StallCnt(0),
            stopAll(false) {

    }

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
        while (!stopAll) {
            IF->work();
            ID->preBuffer = IF->nxtBuffer;
            ID->work();
            EX->preBuffer = ID->nxtBuffer;
            EX->work();
            MEM->preBuffer = EX->nxtBuffer;
            MEM->work();
            WB->preBuffer = MEM->nxtBuffer;
            WB->work();
        }
//        debugPrint("stop All!");
//            debugPrint(stage[1]->nxtBuffer.insCode, "  imm:", stage[1]->nxtBuffer.imm);

//        while (!stopAll) {
//            updateBuffer();
//            updateUnit();
//        }
    }
};


#endif //RISC_V_SIMULATOR_CPU_HPP
