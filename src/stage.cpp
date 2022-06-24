//
// Created by StillMe on 2022/6/21.
//

#include "stage.h"
#include "cpu.h"

//#define DEBUG_IF
//#define DEBUG_ID
//#define DEBUG_MEM

void StageIF::work() {
    if (ctx->pc > 0x10 && ctx->pc < 0x1000) exit(1);

    nxtBuffer.ins = ctx->memory->get4Byte(ctx->pc);

#ifdef DEBUG_IF
    std::cout << std::hex << ctx->pc << "   " << nxtBuffer.ins << std::endl;
//    if (ctx->pc == 0x1024) {
//        debugPrint("stop meeting 1024!");
//        exit(1);
//    }
#endif
}

void StageID::work() {
    INSTRUCTION::Instruction insObj(preBuffer.ins);
    insObj.init();
    insObj.getCategory();
    insObj.getImm();
    nxtBuffer.ins = preBuffer.ins;
    nxtBuffer.imm = insObj.imm;
    nxtBuffer.insCode = insObj.insCode;
    nxtBuffer.rd = insObj.rd;
    switch (insObj.type) {
        case INSTRUCTION::R:
        case INSTRUCTION::I:
        case INSTRUCTION::S:
        case INSTRUCTION::B:
            nxtBuffer.rv1 = ctx->reg->at(insObj.rs1);
            break;
        default:
            nxtBuffer.rv1 = -1u;
            break;
    }
    switch (insObj.type) {
        case INSTRUCTION::R:
        case INSTRUCTION::S:
        case INSTRUCTION::B:
            nxtBuffer.rv2 = ctx->reg->at(insObj.rs2);
            break;
        default:
            nxtBuffer.rv2 = -1u;
            break;
    }

#ifdef DEBUG_ID
    debugPrint("Fetch ins:{op:", (u32)insObj.opcode, ", funct3:", (u32)insObj.funct3, ", funct7:",
               (u32)insObj.funct7, " rd:", (u32)nxtBuffer.rd, ", rs1:", (u32)insObj.rs1, " rs2:", (u32)insObj.rs2, " imm:", nxtBuffer.imm,
               "rv1:", (u32)nxtBuffer.rv1, "rv2:", (u32)nxtBuffer.rv2, "}");
#endif
}

void StageEX::work() {
    using namespace INSTRUCTION;

    // 退出
    if (preBuffer.ins == 0x0ff00513u) {
        std::cout << std::dec << (ctx->reg->at(10) & 0xffu) << std::endl;
        ctx->stopAll = true;
    }

    nxtBuffer.insCode = preBuffer.insCode;
    nxtBuffer.rd = preBuffer.rd;
    nxtBuffer.rv2 = preBuffer.rv2;
    nxtBuffer.jd = -1u;  // 表示没有跳转

    u32 rv1 = preBuffer.rv1;
    u32 imm = preBuffer.imm;
    u32 rv2 = preBuffer.rv2;

    switch (preBuffer.insCode) {
        case LB:
        case LH:
        case LW:
        case LBU:
        case LHU:
            nxtBuffer.res1 = rv1 + imm;
            break;
        case ADDI:
            nxtBuffer.res1 = rv1 + imm;
            break;
        case SLLI:
            nxtBuffer.res1 = rv1 << imm;
            break;
        case SLTI:
            nxtBuffer.res1 = ((i32)rv1 < (i32)imm);
            break;
        case SLTIU:
            nxtBuffer.res1 = ((u32)rv1 < (u32)imm);
            break;
        case XORI:
            nxtBuffer.res1 = rv1 ^ imm;
            break;
        case SRLI:
            nxtBuffer.res1 = rv1 >> imm;
            break;
        case SRAI:
            nxtBuffer.res1 = (i32)rv1 >> imm;
            break;
        case ORI:
            nxtBuffer.res1 = rv1 | imm;
            break;
        case ANDI:
            nxtBuffer.res1 = rv1 & imm;
            break;
        case JALR:  // TODO: a strange command
            nxtBuffer.res1 = ctx->pc + 4;
            nxtBuffer.jd = (rv1 + imm) & (~1u);
            break;
        case SB:
        case SH:
        case SW:
            nxtBuffer.res1 = imm + rv1;
            break;
        case ADD:
            nxtBuffer.res1 = rv1 + rv2;
            break;
        case SUB:
            nxtBuffer.res1 = rv1 - rv2;
            break;
        case SLL:
            nxtBuffer.res1 = rv1 << rv2;
            break;
        case SLT:
            nxtBuffer.res1 = ((i32)rv1 < (i32)rv2);
            break;
        case SLTU:
            nxtBuffer.res1 = ((u32)rv1 < (u32)rv2);
            break;
        case XOR:
            nxtBuffer.res1 = rv1 ^ rv2;
            break;
        case SRL:
            nxtBuffer.res1 = (u32)rv1 >> rv2;
            break;
        case SRA:
            nxtBuffer.res1 = (i32)rv1 >> rv2;
            break;
        case OR:
            nxtBuffer.res1 = rv1 | rv2;
            break;
        case AND:
            nxtBuffer.res1 = rv1 & rv2;
            break;
        case BEQ:
            if (rv1 == rv2) nxtBuffer.jd = ctx->pc + imm;
            break;
        case BNE:
            if (rv1 != rv2) nxtBuffer.jd = ctx->pc + imm;
            break;
        case BLT:
            if ((i32)rv1 < (i32)rv2) nxtBuffer.jd = ctx->pc + imm;
            break;
        case BGE:
            if ((i32)rv1 >= (i32)rv2) nxtBuffer.jd = ctx->pc + imm;
            break;
        case BLTU:
            if ((u32)rv1 < (u32)rv2) nxtBuffer.jd = ctx->pc + imm;
            break;
        case BGEU:
            if ((u32)rv1 >= (u32)rv2) nxtBuffer.jd = ctx->pc + imm;
            break;
        case AUIPC:
            nxtBuffer.res1 = ctx->pc + (imm << 12);
            break;
        case LUI:
            nxtBuffer.res1 = imm << 12;
            break;
        case JAL:
            nxtBuffer.res1 = ctx->pc + 4;
            nxtBuffer.jd = ctx->pc + imm;
            break;
        default:
//            std::cerr << "error in EXE: no such insType!" << std::endl;
//            exit(1);
            break;
    }
}

void StageMEM::work() {
    using namespace INSTRUCTION;

    nxtBuffer.insCode = preBuffer.insCode;
    nxtBuffer.rd = preBuffer.rd;

#ifdef DEBUG_MEM
    debugPrint("MEM: memPos:", preBuffer.res1);
#endif

    switch (preBuffer.insCode) {
        case LB:
            nxtBuffer.res = ctx->memory->at(preBuffer.res1);
            break;
        case LH:
            nxtBuffer.res = ctx->memory->get2Byte(preBuffer.res1);
            break;
        case LW:
            nxtBuffer.res = ctx->memory->get4Byte(preBuffer.res1);
            break;
        case LBU:
            nxtBuffer.res = ctx->memory->at(preBuffer.res1);
            break;
        case LHU:
            nxtBuffer.res = ctx->memory->get2Byteu(preBuffer.res1);
            break;
        case SB:
            ctx->memory->modify(preBuffer.res1, (preBuffer.rv2 & 0xffu));
            break;
        case SH:
            ctx->memory->set2Byte(preBuffer.res1, preBuffer.rv2);
            break;
        case SW:
            ctx->memory->set4Byte(preBuffer.res1, preBuffer.rv2);
            break;
        default:
            nxtBuffer.res = preBuffer.res1;
            break;
    }

    // update pc
    if (preBuffer.jd != -1u) {
        ctx->pc = preBuffer.jd;
    } else {
        ctx->pc += 4;
    }
}

void StageWB::work() {
    using namespace INSTRUCTION;

//    ctx->reg->modify(preBuffer.rd, preBuffer.res);

    switch (preBuffer.insCode) {
        case LB:
        case LH:
        case LW:
        case LBU:
        case LHU:
        case ADDI:
        case SLLI:
        case SLTI:
        case SLTIU:
        case XORI:
        case SRLI:
        case SRAI:
        case ORI:
        case ANDI:
        case JALR:
        case ADD:
        case SUB:
        case SLL:
        case SLT:
        case SLTU:
        case XOR:
        case SRL:
        case SRA:
        case OR:
        case AND:
        case AUIPC:
        case LUI:
        case JAL:
            if (preBuffer.rd) {
                ctx->reg->modify(preBuffer.rd, preBuffer.res);
//                debugPrint("WB:reg modified:", "rd:", (u32)preBuffer.rd, " res:", (u32)preBuffer.res);
            }
        default:
//            debugPrint("WB: dont need to write reg.");
            break;
    }
}