//
// Created by herlight on 2019/11/12.
//

#ifndef __MC_TGR_HH__
#define __MC_TGR_HH__

#include <vector>
#include <map>
#include <set>
#include <type_traits>
#include <array>
#include <string>
#include <list>
#include "eyr.hh"

namespace mc {
namespace tgr {

enum class Reg {
    X0 = 0,
    S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11,
    T0, T1, T2, T3, T4, T5, T6,
    A0, A1, A2, A3, A4, A5, A6, A7,
};
static inline int R2I(Reg r) { return static_cast<int>(r); }
template<typename Int, typename = typename std::enable_if<std::is_integral<Int>::value>::type>
static inline Reg I2R(Int i) { return static_cast<Reg>(i); }
static inline bool isCallerSaved(Reg r) {
    return R2I(r) >= R2I(Reg::T0) && R2I(r) <= R2I(Reg::A7);
}
static inline bool isCalleeSaved(Reg r) {
    return R2I(r) >= R2I(Reg::S0) && R2I(r) <= R2I(Reg::S11);
}
const char *RegStr(Reg reg);
const std::vector<Reg> &CallerSavedRegs();
const std::vector<Reg> &CalleeSavedRegs();
const std::vector<Reg> &AllRegs();
constexpr int RegNum = static_cast<int>(Reg::A7) + 1;

struct Module;
struct Variable;
struct Function;
struct BasicBlock;
struct Operation;
struct Operand;

struct Module {
    std::vector<Variable *> vars;
    std::vector<Function *> funcs;
    int next_vir_reg_id{0};
    int next_label{0};
    int next_glb_var_id{0};

    void addVar(Variable *var);
    void addFunc(Function *func);
};

struct Variable {
    Module *module{nullptr};
    int id;
    int width{0}; // 0 if not an array

    explicit Variable(int i, int w = 0)
            : id(i), width(w) {}
};

struct Function {
    Module *module{nullptr};
    std::string name;
    int argc;
    int frame_size{0};
    std::vector<BasicBlock *> blocks;

    void addBlock(BasicBlock *blk);
    int extendFrame(int cnt);

    explicit Function(std::string n, int ac) : name(std::move(n)), argc(ac) {}
};

struct Operand {
    enum Tag {
        NONE = 0, PHY_REG, VIR_REG, INTEGER, FRM_SLT, GLB_VAR,
        BSC_BLK, FUN_NME,
    } tag{NONE};
    union {
        Reg phy_reg;
        int vir_reg;
        int integer;
        int frm_slt;
        Variable *glb_var;
        BasicBlock *bsc_blk;
        std::string *fun_nme;
    } val{.integer = 0};

    Operand() = default;
    explicit Operand(Reg r) : tag(PHY_REG) { val.phy_reg = r; }
    explicit Operand(Variable *var) : tag(GLB_VAR) { val.glb_var = var; }
    explicit Operand(BasicBlock *blk) : tag(BSC_BLK) { val.bsc_blk = blk; }
    explicit Operand(std::string fn)
            : tag(FUN_NME) { val.fun_nme = new std::string(std::move(fn)); }
    Operand(Tag t, int i);
    static Operand PhyReg(Reg r);
    static Operand GlbVar(Variable *v);
    static Operand BscBlk(BasicBlock *b);
    static Operand VirReg(int r);
    static Operand FrmSlt(int f);
    static Operand Integer(int i);
    static Operand FuncName(const std::string &name);
};

struct Operation {
    BasicBlock *block{nullptr};
    typename std::list<Operation *>::iterator link;
    enum Opt {
        UN_NEG = 0, UN_NOT,
        BIN_EQ, BIN_NE, BIN_LT, BIN_GT, BIN_OR, BIN_AND,
        BIN_ADD, BIN_SUB, BIN_MUL, BIN_DIV, BIN_REM,
        MOV, IDX_LD, IDX_ST,
        BR_EQ, BR_NE, BR_LT, BR_GT, BR_OR, BR_AND, JUMP,
        CALL, STORE, LOAD, LOAD_ADDR, RET,
        __SET_PARAM, __BEGIN_PARAM, __GET_PARAM, __SET_RET, __GET_RET,
    } opt;
    std::array<Operand, 3> oprs;

    Operation(Opt opt, std::array<Operand, 3> opr)
            : opt(opt), oprs(opr), def_bits({false}) {}
    inline bool isBinOp() const { return opt >= BIN_EQ && opt <= BIN_REM; }
    inline bool isUnOp() const { return opt >= UN_NEG && opt <= UN_NOT; }
    inline bool isBranch() const { return opt >= BR_EQ && opt <= BR_AND; }
    Operation *prev();
    Operation *next();
    void addBefore(Operation *op);
    void addAfter(Operation *op);
    const std::set<int> &getUsedVirRegs();
    const std::set<int> &getDefinedVirRegs();
    void rewrite(int vr, Reg pr);

private:
    bool modified[2]{true, true};
    std::set<int> uses, defs;
    std::array<bool, 3> def_bits;
};


struct BasicBlock {
    Function *function{nullptr};
    int label;
    BasicBlock *fall_out{nullptr};
    BasicBlock *jump_out{nullptr};
    BasicBlock *fall_in{nullptr};
    std::set<BasicBlock *> jump_in;
    std::list<Operation *> ops;
    std::set<int> live_gen, live_kill, live_in, live_out;

    explicit BasicBlock(int l) : label(l) {}

    void jump(BasicBlock *blk);
    void fall(BasicBlock *blk);
    std::vector<BasicBlock *> outBlocks();
    std::vector<BasicBlock *> inBlocks();

    void addOp(Operation *op);
    void addOp(Operation::Opt opt, std::array<Operand, 3> opr);

    Operation *prevOpOf(Operation *op);
    Operation *nextOpOf(Operation *op);
    void addOpBefore(Operation *pos, Operation *op);
    void addOpAfter(Operation *pos, Operation *op);
    void removeOp(Operation *op);
};

std::ostream &operator<<(std::ostream &os, const Module &mod);

std::ostream &operator<<(std::ostream &os, const Variable &var);

std::ostream &operator<<(std::ostream &os, const Function &func);

std::ostream &operator<<(std::ostream &os, const Operand &opr);

std::ostream &operator<<(std::ostream &os, const Operation &op);

std::ostream &operator<<(std::ostream &os, const BasicBlock &blk);

};
};

#endif //__MC_TGR_HH__
