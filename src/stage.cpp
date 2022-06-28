//
// Created by StillMe on 2022/6/21.
//

#include "stage.h"
#include "cpu.h"

//#define DEBUG_IF
//#define DEBUG_ID
//#define DEBUG_EX
//#define DEBUG_MEM
//#define DEBUG_WB

void StageIF::work() {
    nxtBuffer.pc = ctx->pc;
    nxtBuffer.ins = ctx->memory->get4Byte(ctx->pc);
    nxtBuffer.predPc = ctx->predictor.predictPC(ctx->pc, nxtBuffer.ins);

#ifdef DEBUG_IF
    std::cout << std::hex << "IF:===" << ctx->pc << "   " << nxtBuffer.ins;
    debugPrint("===  ", " predPc: ", nxtBuffer.predPc, "}");
#endif
}

void StageID::work() {
    INSTRUCTION::Instruction insObj(preBuffer.ins);
    insObj.getCategory();
    insObj.getImm();
    nxtBuffer.ins = preBuffer.ins;
    nxtBuffer.pc = preBuffer.pc;
    nxtBuffer.predPc = preBuffer.predPc;
    nxtBuffer.imm = insObj.imm;
    nxtBuffer.insCode = insObj.insCode;
    // set rd
    switch (insObj.type) {
        case INSTRUCTION::R:
        case INSTRUCTION::I:
        case INSTRUCTION::U:
        case INSTRUCTION::UJ:
            nxtBuffer.rd = insObj.rd;
            break;
        default:
            nxtBuffer.rd = -1u;
            break;
    }
    // set rv1
    switch (insObj.type) {
        case INSTRUCTION::R:
        case INSTRUCTION::I:
        case INSTRUCTION::S:
        case INSTRUCTION::B:
            nxtBuffer.rs1 = insObj.rs1;
            nxtBuffer.rv1 = ctx->reg->at(insObj.rs1);
            break;
        default:
            nxtBuffer.rs1 = -1u;
            nxtBuffer.rv1 = -1u;
            break;
    }
    // forwarding: get data from MEM/WB Buffer
    if (ctx->WB->preBuffer.rd == nxtBuffer.rs1) {
        nxtBuffer.rv1 = ctx->WB->preBuffer.res;
    }
    // set rv2
    switch (insObj.type) {
        case INSTRUCTION::R:
        case INSTRUCTION::S:
        case INSTRUCTION::B:
            nxtBuffer.rs2 = insObj.rs2;
            nxtBuffer.rv2 = ctx->reg->at(insObj.rs2);
            break;
        default:
            nxtBuffer.rs2 = -1u;
            nxtBuffer.rv2 = -1u;
            break;
    }
    // forwarding: get data from MEM/WB Buffer
    if (ctx->WB->preBuffer.rd == nxtBuffer.rs2) {
        nxtBuffer.rv2 = ctx->WB->preBuffer.res;
    }

#ifdef DEBUG_ID
    debugPrint("ID:===", preBuffer.pc, "  ", preBuffer.ins, "==={ op:", (u32)insObj.opcode, ", funct3:", (u32)insObj.funct3, ", funct7:",
               (u32)insObj.funct7, " rd:", (u32)nxtBuffer.rd, ", rs1:", (u32)insObj.rs1, " rs2:", (u32)insObj.rs2, " imm:", nxtBuffer.imm,
               "rv1:", (u32)nxtBuffer.rv1, "rv2:", (u32)nxtBuffer.rv2, "}");
#endif
}

void StageEX::work() {
    using namespace INSTRUCTION;

    if (preBuffer.insCode == BUBBLE) {
        ctx->jumpOrBranchWrong = false;
        return;  // TODO 是否合适？
    }

    // 退出 TODO 是否要更换位置
    if (preBuffer.ins == 0x0ff00513u) {
        std::cout << std::dec << (ctx->reg->at(10) & 0xffu) << std::endl;
        ctx->stopAll = true;
    }

    nxtBuffer.insCode = preBuffer.insCode;
    nxtBuffer.rd = preBuffer.rd;
    nxtBuffer.jd = preBuffer.pc + 4;  // 默认没有跳转

    u32 imm = preBuffer.imm;

    u32 rv1 = preBuffer.rv1;
    // forwarding1: get data from MEM/WB Buffer
    if (ctx->WB->preBuffer.rd == preBuffer.rs1) {
        rv1 = ctx->WB->preBuffer.res;
    }
    // forwarding2: get data from EX/MEM Buffer
    if (ctx->MEM->preBuffer.rd == preBuffer.rs1) {
        if (Instruction::isLoadInsCode(ctx->MEM->preBuffer.insCode)) {
            // TODO stall IF,ID,EX till after MEM finish
            // 设定ctx.IFIDEX_flag = true;
            // 事实上在这个clock IDIF可以正常做，只要下个上升沿的时候不更新前三个buffer即可
#ifdef DEBUG_EX
            debugPrint("  EX1:SET IF_ID_EX_Buffer_Stall_Flag = 1");
#endif
            if (ctx->IF_ID_EX_Buffer_StallCnt == 1) {
                ctx->IF_ID_EX_Buffer_StallCnt = 0;
            } else {
                ctx->IF_ID_EX_Buffer_StallCnt = 1;
                return;
            }
        } else {
            rv1 = ctx->MEM->preBuffer.res1;
        }
    }

    u32 rv2 = preBuffer.rv2;
    // forwarding1: get data from MEM/WB Buffer
    if (ctx->WB->preBuffer.rd == preBuffer.rs2) {
        rv2 = ctx->WB->preBuffer.res;
    }
    // forwarding2: get data from EX/MEM Buffer
    if (ctx->MEM->preBuffer.rd == preBuffer.rs2) {
        if (Instruction::isLoadInsCode(ctx->MEM->preBuffer.insCode)) {
//            ctx->IF_ID_EX_Buffer_StallCnt = 1;
#ifdef DEBUG_EX
            debugPrint("  EX2:SET IF_ID_EX_Buffer_Stall_Flag = 1");
#endif
            if (ctx->IF_ID_EX_Buffer_StallCnt == 1) {
                ctx->IF_ID_EX_Buffer_StallCnt = 0;
            } else {
                ctx->IF_ID_EX_Buffer_StallCnt = 1;
                return;
            }
        } else {
            rv2 = ctx->MEM->preBuffer.res1;
        }
    }

    nxtBuffer.rv2 = rv2;  // TODO

    bool isBranchTaken = false;
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
        case JALR:
            nxtBuffer.res1 = preBuffer.pc + 4;
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
            if (rv1 == rv2) isBranchTaken = true, nxtBuffer.jd = preBuffer.pc + imm;
            break;
        case BNE:
            if (rv1 != rv2) isBranchTaken = true, nxtBuffer.jd = preBuffer.pc + imm;
            break;
        case BLT:
            if ((i32)rv1 < (i32)rv2) isBranchTaken = true, nxtBuffer.jd = preBuffer.pc + imm;
            break;
        case BGE:
            if ((i32)rv1 >= (i32)rv2) isBranchTaken = true, nxtBuffer.jd = preBuffer.pc + imm;
            break;
        case BLTU:
            if ((u32)rv1 < (u32)rv2) isBranchTaken = true, nxtBuffer.jd = preBuffer.pc + imm;
            break;
        case BGEU:
            if ((u32)rv1 >= (u32)rv2) isBranchTaken = true, nxtBuffer.jd = preBuffer.pc + imm;
            break;
        case AUIPC:
            nxtBuffer.res1 = preBuffer.pc + (imm << 12);  // TODO 此处是否应吧ctx.pc换成preBuffer.pc
            break;
        case LUI:
            nxtBuffer.res1 = imm << 12;
            break;
        case JAL:
            nxtBuffer.res1 = preBuffer.pc + 4;
            nxtBuffer.jd = preBuffer.pc + imm;
            break;
        default:
//            std::cerr << "error in EXE: no such insType!" << std::endl;
//            exit(1);
            break;
    }

    // update predictor & pc
    ctx->jumpOrBranchWrong = (preBuffer.predPc != nxtBuffer.jd);
    ctx->predictor.update(preBuffer.ins, nxtBuffer.jd, isBranchTaken, !ctx->jumpOrBranchWrong);

#ifdef DEBUG_EX
    debugPrint("EX:===", preBuffer.pc, "  ", preBuffer.ins, "==={", " jd: ", nxtBuffer.jd, " predPc:", preBuffer.predPc, " branchTaken:", isBranchTaken, "rv1:", rv1, "rv2:", rv2, "res1:", nxtBuffer.res1, " }");
#endif
}

void StageMEM::work() {
    using namespace INSTRUCTION;

    nxtBuffer.insCode = preBuffer.insCode;
    nxtBuffer.rd = preBuffer.rd;

    if (preBuffer.insCode == BUBBLE) return;

#ifdef DEBUG_MEM
    debugPrint("MEM: { memPos:", preBuffer.res1, "rv2:", preBuffer.rv2, "}");
#endif

    ctx->MEM_StallCnt = 2;
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
            ctx->MEM_StallCnt = 0;
            nxtBuffer.res = preBuffer.res1;
            break;
    }
}

void StageWB::work() {
    using namespace INSTRUCTION;

    if (preBuffer.insCode == BUBBLE) return;

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
#ifdef DEBUG_WB
                debugPrint("WB:{", "rd:", preBuffer.rd, " res:", preBuffer.res);
#endif
//                debugPrint("WB:reg modified:", "rd:", (u32)preBuffer.rd, " res:", (u32)preBuffer.res);
            }
            break;
        default:
#ifdef DEBUG_WB
            debugPrint("WB: dont need to write reg.");
#endif
            break;
    }
}