//
// Created by herlight on 2019/10/5.
//

#ifndef __MC_EYR_HH__
#define __MC_EYR_HH__

#include <vector>
#include <string>
#include <set>
#include <cassert>
#include <iostream>
#include <list>
#include "util.hh"

namespace mc {
namespace eyr {

struct Item;
struct Function;
struct BasicBlock;
struct Instruction;
struct Variable;
struct AssignInst;
struct BinaryInst;
struct UnaryInst;
struct MoveInst;
struct LoadInst;
struct StoreInst;
struct CallInst;
struct JumpInst;
struct BranchInst;
struct ReturnInst;

struct Module {
    std::vector<Item *> global_items;
    std::vector<Variable *> global_vars;
    std::vector<Function *> global_funcs;
    int T_id{0};
    int t_id{0};
    int l_id{0};

    void addFunction(Function *f);
    Variable *allocGlobalVar(int width = 0, bool constant = false);
    friend std::ostream &operator<<(std::ostream &os, const Module &mod);
};

struct Item {
    virtual ~Item() = default;
    virtual std::ostream &print(std::ostream &os) const = 0;
    friend std::ostream &operator<<(std::ostream &os, const Item &item) { return item.print(os); }
};

struct Function : public Item {
    Module *module;
    std::string name;
    std::vector<Variable *> params;
    std::vector<Variable *> local_vars;
    std::vector<BasicBlock *> blocks;
    BasicBlock *entry;

    Function(Module *m, std::string n, int argc);
    BasicBlock *allocBlock();

    Variable *
    allocLocalVar(bool temp = true, int width = 0, bool constant = false);

    void arrangeBlock();

    std::ostream &print(std::ostream &os) const override;
};

struct BasicBlock : public Item {
    Function *func;
    int label{0}; // global label
    int f_idx{0}; // in-function idx
    bool reachable{true};
    std::list<Instruction *> insts;

    BasicBlock *fall_out{nullptr};
    BasicBlock *fall_in{nullptr};
    BasicBlock *jump_out{nullptr};
    std::set<BasicBlock *> jump_in;

    explicit BasicBlock(Function *f) : func(f) {}
    inline void fall(BasicBlock *b) {
        fall_out = b, b->fall_in = this;
        debug(this->label, " fall to ", b->label);
    }
    inline void jump(BasicBlock *b) {
        jump_out = b, b->jump_in.insert(this);
        debug(this->label, " jump to ", b->label);
    }

    void safeRemove();
    std::vector<BasicBlock *> outBlocks() const;
    std::vector<BasicBlock *> inBlocks() const;

    inline void addInst(Instruction *i) { insts.push_back(i); }
    std::ostream &print(std::ostream &os) const override;
};

struct Variable : public Item {
    Function *func;
    std::string name;
    bool is_temp;
    int width;
    bool is_base_addr;

    std::vector<Instruction *> defers;
    std::vector<Instruction *> users;

    Variable(Function *f, std::string n, bool tmp = true, int w = 0, bool c = false) :
            func(f), name(std::move(n)), is_temp(tmp), width(w), is_base_addr(c) {}
    std::ostream &print(std::ostream &os) const override;

    inline bool isGlobal() const { return func == nullptr; }
    inline bool isLocal() const { return func != nullptr && !is_temp; }
    inline bool isTemp() const { return is_temp; }
};

struct Operand {
    bool imm;
    int val;
    Variable *var;
    explicit Operand(int v = 0) : imm(true) { var = nullptr, val = v; }
    explicit Operand(Variable *v) : imm(false) { val = 0, var = v; }
    friend std::ostream &operator<<(std::ostream &os, const Operand &opr) {
        if (opr.imm)
            os << opr.val;
        else
            os << opr.var->name;
        return os;
    }
};

struct Instruction : public Item {
    BasicBlock *block;
    int b_idx{0}; // in-block idx
    int f_idx{0}; // in-function idx

    std::vector<Variable *> uses;
    std::vector<Variable *> defs;

    int ra_id{0}; // for reg-alloc

    explicit Instruction(BasicBlock *b) : block(b) {}
    inline void use(Operand opr) {
        if (!opr.imm)
            use(opr.var);
    }
    inline void use(Variable *var) {
        var->users.push_back(this);
        this->uses.push_back(var);
    }
};

struct AssignInst : public Instruction {
    Variable *dst;

    AssignInst(BasicBlock *b, Variable *d) : Instruction(b), dst(d) {
        d->defers.push_back(this);
        defs.push_back(d);
    }
};

struct BinaryInst : public AssignInst {
    enum class BinOp {
        EQ = 2, NE, LT, GT, OR, AND, ADD, SUB, MUL, DIV, REM,
    };
    BinOp opt;
    Operand lhs, rhs;

    BinaryInst(BasicBlock *b, Variable *d, BinOp op, Operand l, Operand r) :
            AssignInst(b, d), opt(op), lhs(l), rhs(r) {
        use(l), use(r);
    }
    std::ostream &print(std::ostream &os) const override;
};

struct UnaryInst : public AssignInst {
    enum class UnOp {
        NEG = 0, NOT,
    };
    UnOp opt;
    Operand opr;

    UnaryInst(BasicBlock *b, Variable *d, UnOp op, Operand o) :
            AssignInst(b, d), opt(op), opr(o) {
        use(o);
    }
    std::ostream &print(std::ostream &os) const override;
};

struct CallInst : public AssignInst {
    std::string name;
    std::vector<Operand> args;

    CallInst(BasicBlock *b, Variable *d, std::string n, std::vector<Operand> a) :
            AssignInst(b, d), name(std::move(n)), args(std::move(a)) {
        for (auto arg: args)
            use(arg);
    }
    std::ostream &print(std::ostream &os) const override;
};

struct MoveInst : public AssignInst {
    Operand src;

    MoveInst(BasicBlock *b, Variable *d, Operand s) : AssignInst(b, d), src(s) {
        use(s);
    }
    std::ostream &print(std::ostream &os) const override;
};

struct StoreInst : public Instruction {
    Variable *base;
    Operand idx;
    Operand src;

    StoreInst(BasicBlock *b, Variable *bs, Operand i, Operand s) :
            Instruction(b), base(bs), idx(i), src(s) {
        use(i), use(s);
    }
    std::ostream &print(std::ostream &os) const override;
};

struct LoadInst : public AssignInst {
    Variable *src;
    Operand idx;

    LoadInst(BasicBlock *b, Variable *d, Variable *s, Operand i) :
            AssignInst(b, d), src(s), idx(i) {
        use(i);
    }
    std::ostream &print(std::ostream &os) const override;
};

struct JumpInst : public Instruction {
    BasicBlock *dst;

    JumpInst(BasicBlock *b, BasicBlock *d) : Instruction(b), dst(d) {}
    std::ostream &print(std::ostream &os) const override;
};

struct BranchInst : public JumpInst {
    enum class LgcOp {
        EQ = 2, NE, LT, GT, OR, AND,
    };
    LgcOp opt;
    Operand lhs, rhs;

    BranchInst(BasicBlock *b, BasicBlock *d, LgcOp op, Operand l, Operand r) :
            JumpInst(b, d), opt(op), lhs(l), rhs(r) {
        use(l), use(r);
    }
    std::ostream &print(std::ostream &os) const override;
};

struct ReturnInst : public Instruction {
    Operand opr;

    ReturnInst(BasicBlock *b, Operand o) : Instruction(b), opr(o) {
        use(o);
    }
    std::ostream &print(std::ostream &os) const override;
};

};
};

#endif
