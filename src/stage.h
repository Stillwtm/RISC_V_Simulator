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
    };

    struct ID_EX_Buffer {
        u32 ins, rv1, rv2, insCode, imm;
        u8 rd;
    };

    struct EX_MEM_Buffer {
        u32 insCode, res1, rv2;
        u32 jd;  // jump direction
        u8 rd;
    };

    struct MEM_WB_Buffer {
        u32 insCode, res;
        u8 rd;
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
    IF_ID_Buffer preBuffer;
    ID_EX_Buffer nxtBuffer;
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
private:
    EX_MEM_Buffer preBuffer;
    MEM_WB_Buffer nxtBuffer;
public:
    StageMEM(cpu* _ctx = nullptr) : Stage(_ctx), preBuffer{}, nxtBuffer{} { }
    virtual void work();
};

class StageWB : public Stage {
    friend class cpu;
private:
    MEM_WB_Buffer preBuffer;
public:
    StageWB(cpu* _ctx = nullptr) : Stage(_ctx), preBuffer{} { }
    virtual void work();
};


#endif //RISC_V_SIMULATOR_STAGE_H
