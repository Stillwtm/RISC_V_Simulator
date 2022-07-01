//
// Created by StillMe on 2022/6/21.
//

#ifndef RISC_V_SIMULATOR_STAGE_H
#define RISC_V_SIMULATOR_STAGE_H

#include "utility.h"

class cpu;

class Stage {
    friend class cpu;
protected:
    struct IF_ID_Buffer {
        u32 ins;
        u32 pc, predPc;
        void clear() { ins = pc = predPc = 0; }
    };

    struct ID_EX_Buffer {
        u32 ins, rv1, rv2, insCode, imm;
        u32 rs1, rs2, rd;
        u32 pc, predPc;
        void clear() {
            ins = rv1 = rv2 = insCode = imm = pc = predPc = 0;
            rs1 = rs2 = rd = -1u;
        }
    };

    struct EX_MEM_Buffer {
        u32 ins, insCode;
        u32 res1;  // 指令计算出的结果
        u32 rv2;
        u32 jd;  // jump direction
        u32 rd;
        u32 pc, predPc;
        void clear() {
            insCode = res1 = rv2 = jd = pc = predPc = 0;
            rd = -1u;
        }
    };

    struct MEM_WB_Buffer {
        u32 ins, insCode;
        u32 res;  // 要写入rd的结果
        u32 rd;
        void clear() {
            insCode = res = 0;
            rd = -1u;
        }
    };

    struct AFTER_WB_Buffer {
        u32 rd, res;
    };

    cpu* ctx;

public:
    Stage(cpu* _ctx = nullptr) : ctx(_ctx) { }
    virtual ~Stage() { }
    virtual void work() = 0;
};

class StageIF : public Stage {
    friend class cpu;
private:
    IF_ID_Buffer nxtBuffer;
public:
    StageIF(cpu* _ctx = nullptr) : Stage(_ctx), nxtBuffer{} { }
    virtual void work();
};

class StageID : public Stage {
    friend class cpu;
private:
    IF_ID_Buffer preBuffer;  // preBuffer是传统意义上的Buffer
    ID_EX_Buffer nxtBuffer;  // nxtBuffer是为了模拟实现造的Buffer，或许是wire?
public:
    StageID(cpu* _ctx = nullptr) : Stage(_ctx), preBuffer{}, nxtBuffer{} { }
    virtual void work();
};

class StageEX : public Stage {
    friend class cpu;
private:
    ID_EX_Buffer preBuffer;
    EX_MEM_Buffer nxtBuffer;
public:
    StageEX(cpu* _ctx = nullptr) : Stage(_ctx), preBuffer{}, nxtBuffer{} { }
    virtual void work();
};

class StageMEM : public Stage {
    friend class cpu;
    friend class StageEX;
    friend class StageID;
private:
    EX_MEM_Buffer preBuffer;
    MEM_WB_Buffer nxtBuffer;
public:
    StageMEM(cpu* _ctx = nullptr) : Stage(_ctx), preBuffer{}, nxtBuffer{} { }
    virtual void work();
};

class StageWB : public Stage {
    friend class cpu;
    friend class StageEX;
    friend class StageID;
private:
    MEM_WB_Buffer preBuffer;
    AFTER_WB_Buffer nxtBuffer;
public:
    StageWB(cpu* _ctx = nullptr) : Stage(_ctx), preBuffer{}, nxtBuffer{} { }
    virtual void work();
};


#endif //RISC_V_SIMULATOR_STAGE_H
