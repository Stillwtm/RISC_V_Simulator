//
// Created by StillMe on 2022/6/20.
//

#ifndef RISC_V_SIMULATOR_UTILITY_H
#define RISC_V_SIMULATOR_UTILITY_H


#include <string>
#include <iostream>
#include <fstream>

typedef unsigned char u8;
typedef unsigned int u32;
typedef int32_t i32;

namespace INSTRUCTION {

    enum InsCategory {
        I, U, S, R, B, UJ
    };

    enum InsCode {
        // [funct7-funct3-opcode]
        LB = (0b0000011) | (0b000 << 7),
        LH = (0b0000011) | (0b001 << 7),
        LW = (0b0000011) | (0b010 << 7),
        LBU = (0b0000011) | (0b100 << 7),
        LHU = (0b0000011) | (0b101 << 7),
        ADDI = (0b0010011) | (0b000 << 7),
        SLLI = (0b0010011) | (0b001 << 7) | (0b0000000 << 10),
        SLTI = (0b0010011) | (0b010 << 7),
        SLTIU = (0b0010011) | (0b011 << 7),
        XORI = (0b0010011) | (0b100 << 7),
        SRLI = (0b0010011) | (0b101 << 7) | (0b0000000 << 10),
        SRAI = (0b0010011) | (0b101 << 7) | (0b0100000 << 10),
        ORI = (0b0010011) | (0b110 << 7),
        ANDI = (0b0010011) | (0b111 << 7),
        AUIPC = (0b0010111),
        SB = (0b0100011) | (0b000 << 7),
        SH = (0b0100011) | (0b001 << 7),
        SW = (0b0100011) | (0b010 << 7),
        ADD = (0b0110011) | (0b000 << 7) | (0b0000000 << 10),
        SUB = (0b0110011) | (0b000 << 7) | (0b0100000 << 10),
        SLL = (0b0110011) | (0b001 << 7) | (0b0000000 << 10),
        SLT = (0b0110011) | (0b010 << 7) | (0b0000000 << 10),
        SLTU = (0b0110011) | (0b011 << 7) | (0b0000000 << 10),
        XOR = (0b0110011) | (0b100 << 7) | (0b0000000 << 10),
        SRL = (0b0110011) | (0b101 << 7) | (0b0000000 << 10),
        SRA = (0b0110011) | (0b101 << 7) | (0b0100000 << 10),
        OR = (0b0110011) | (0b110 << 7) | (0b0000000 << 10),
        AND = (0b0110011) | (0b111 << 7) | (0b0000000 << 10),
        LUI = (0b0110111),
        BEQ = (0b1100011) | (0b000 << 7),
        BNE = (0b1100011) | (0b001 << 7),
        BLT = (0b1100011) | (0b100 << 7),
        BGE = (0b1100011) | (0b101 << 7),
        BLTU = (0b1100011) | (0b110 << 7),
        BGEU = (0b1100011) | (0b111 << 7),
        JALR = (0b1100111) | (0b000 << 7),
        JAL = (0b1101111)
    };

    class Instruction {
    public:
        u32 ins;
        u8 opcode, rs1, rs2, rd, funct3, funct7;
        u32 insCode, imm;
        InsCategory type;
    public:
        static bool isLoadIns(u32 insCode) {
            return insCode == LB || insCode == LH || insCode == LW ||
                    insCode == LBU || insCode == LHU;
        }

    public:
        Instruction(u32 _ins = 0) : ins(_ins), imm(0) {
            opcode = (ins & 0x0000007fu);
            rd = (ins & 0x00000f80u) >> 7;
            funct3 = (ins & 0x00007000u) >> 12;
            rs1 = (ins & 0x000f8000u) >> 15;
            rs2 = (ins & 0x01f00000u) >> 20;
            funct7 = (ins & 0xfe000000u) >> 25;
        }

        void getCategory() {
            switch (opcode) {
                case 0b0000011u:
                case 0b0010011u:
                case 0b1100111u:
                    type = I;
                    insCode = opcode | (funct3 << 7);
                    if (funct3 == 0b101u) insCode |= (funct7 << 10);
                    break;
                case 0b0100011u:
                    type = S;
                    insCode = opcode | (funct3 << 7);
                    break;
                case 0b0110011u:
                case 0b0111011u:
                    type = R;
                    insCode = opcode | (funct3 << 7) | (funct7 << 10);
                    break;
                case 0b1100011u:
                    type = B;
                    insCode = opcode | (funct3 << 7);
                    break;
                case 0b0010111u:
                case 0b0110111u:
                    type = U;
                    insCode = opcode;
                    break;
                case 0b1101111u:
                    type = UJ;
                    insCode = opcode;
                    break;
                default:
//                    std::cout << "error in ins Category:" << std::hex << (u32)opcode << std::endl;
//                    std::cerr << "error in instruction category!" << std::endl;
//                    exit(1);
                    break;
            }
        }

        void getImm() {
            int32_t insi = static_cast<int32_t>(ins);
            switch (type) {
                case R:
                    imm = 0;
                    break;
                case I:
                    if (funct3 == 0b101u) imm = (i32)(insi & 0x01f00000) >> 20;  // srli & srai
                    else imm = (i32)(insi & 0xfff00000) >> 20;
                    break;
                case S:
                    imm = ((i32)(insi & 0x00000f80) >> 7) |
                            ((i32)(insi & 0xfe000000) >> 20);
                    break;
                case B:
                    imm = ((i32)(insi & 0x00000f00) >> 7) |
                            ((i32)(insi & 0x00000080) << 4) |
                            ((i32)(insi & 0x7e000000) >> 20) |
                            ((i32)(insi & 0x80000000) >> 19);
                    break;
                case U:
                    imm = (i32)(insi & 0xfffff000) >> 12;
                    break;
                case UJ:
                    imm = ((i32)(insi & 0x80000000) >> 11) |
                            ((i32)(insi & 0x7fe00000) >> 20) |
                            ((i32)(insi & 0x00100000) >> 9) |
                            (i32)(insi & 0x000ff000);
                    break;
            }
        }
    };

}

namespace STORAGE {

    template <typename DataType = u8, u32 Size = 65536>
    class Storage {
    protected:
        DataType data[Size];
    public:
        Storage() : data({}) { }
        ~Storage() = default;

        void readFile(std::string& fileName) {
            std::ifstream ifStream(fileName);
            readData(ifStream);
        }

        void readData(std::istream& inStream) {
            std::string inStr;
            u32 pos = 0;
            while (!(inStream >> inStr).eof()) {
//                std::cout << pos << " ";
//                std::cout << inStr << std::endl;
                if (inStr[0] == '@') {
                    pos = std::stoul(inStr.substr(1, 8), nullptr, 16);
                } else {
                    data[pos++] = std::stoul(inStr, nullptr, 16);
                }
            }
        }

        DataType at(u32 ptr) {
            return data[ptr];
        }

        void modify(u32 ptr, DataType val) {
            data[ptr] = val;
        }
    };

    class Memory : public Storage<u8, 524288> {
    public:
        Memory() = default;
        ~Memory() = default;

        u32 get2Byte(u32 ptr) {  // 符号扩展后返回
            i32 ret = data[ptr] + (data[ptr + 1] << 8);
            return (ret << 16) >> 16;
        }

        u32 get2Byteu(u32 ptr) {  // 无符号扩展后返回
            return data[ptr] + (data[ptr + 1] << 8);
        }

        u32 get4Byte(u32 ptr) {  // 小端
            return data[ptr] + (data[ptr + 1] << 8) +
                    (data[ptr + 2] << 16) + (data[ptr + 3] << 24);
        }

        void set2Byte(u32 ptr, u32 val) {
            data[ptr] = val & 0xffu;
            data[ptr + 1] = (val & 0xff00u) >> 8;
        }

        void set4Byte(u32 ptr, u32 val) {
            data[ptr] = val & 0xffu;
            data[ptr + 1] = (val & 0xff00u) >> 8;
            data[ptr + 2] = (val & 0xff0000u) >> 16;
            data[ptr + 3] = (val & 0xff000000u) >> 24;
        }
    };

    class Register : public Storage<u32, 32>  {
    public:
        Register() = default;
        ~Register() = default;
    };

}

// 调试输出
template<typename T>
void debugPrint(const T& t) {
    std::cout << t << std::endl;
}

template<typename T, typename...Args>
void debugPrint(const T& t, const Args&...rest) {
    std::cout << t << " ";
    debugPrint(rest...);
}


#endif //RISC_V_SIMULATOR_UTILITY_H
