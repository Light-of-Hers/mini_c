//
// Created by herlight on 2019/10/5.
//
#include "eyr.hh"
#include <algorithm>
#include <cassert>
#include <list>
#include "ast.hh"


namespace mc {
namespace eyr {

BasicBlock *Function::allocBlock() {
    auto blk = new BasicBlock(this);
    blk->f_idx = blocks.size();
    blk->label = module->l_id++;
    blocks.push_back(blk);
    return blk;
}

Variable *Function::allocLocalVar(bool temp, int width, bool constant) {
    std::string var_name;
    if (temp)
        var_name = std::string("t") + std::to_string(module->t_id++);
    else
        var_name = std::string("T") + std::to_string(module->T_id++);
    auto var = new Variable(this, std::move(var_name), temp, width, constant);
    local_vars.push_back(var);
    return var;
}

Function::Function(Module *m, std::string n, int argc) :
        module(m), name(std::move(n)), params(argc) {
    entry = allocBlock();
    for (int i = 0; i < argc; ++i)
        params[i] = new Variable(this, std::string("p") + std::to_string(i), false);
}

std::ostream &Function::print(std::ostream &os) const {
    os << "f_" << name << " [" << params.size() << ']' << std::endl;
    for (auto var: local_vars)
        var->print(os);
    for (auto blk: blocks)
        blk->print(os);
    os << "end f_" << name << std::endl;
    return os;
}
void Function::arrangeBlock() {
    for (auto blk: blocks)
        blk->reachable = false;
    std::list<BasicBlock *> que;
    que.push_back(entry);
    while (!que.empty()) {
        auto blk = que.front();
        que.pop_front();
        if (blk->reachable)
            continue;
        blk->reachable = true;
        if (blk->fall_out)
            que.push_back(blk->fall_out);
        if (blk->jump_out)
            que.push_back(blk->jump_out);
    }
    for (auto b: blocks) {
        if (!b->reachable)
            b->safeRemove();
    }

    struct BBComp {
        bool operator()(BasicBlock *const &a, BasicBlock *const &b) {
            int a1 = a->reachable ? 1 : 0;
            int b1 = b->reachable ? 1 : 0;
            int a2 = a->fall_in ? 1 : 0;
            int b2 = b->fall_in ? 1 : 0;
            return a1 != b1 ? a1 < b1 : a2 != b2 ? a2 < b2 : a < b;
        }
    };
    std::set<BasicBlock *, BBComp> left(blocks.begin(), blocks.end());
    while (!(*left.begin())->reachable)
        left.erase(left.begin());
    std::vector<BasicBlock *> tmp_blocks;
    auto blk = entry;
    while (true) {
        for (auto b = blk; b; b = b->fall_out)
            tmp_blocks.push_back(b), left.erase(b);
        if (left.empty())
            break;
        blk = *left.begin();
    }
    for (auto b: blocks) {
        if (!b->reachable)
            delete (b);
    }
    blocks = tmp_blocks;

    int f_bid = 0;
    int f_iid = 0;
    for (auto b : blocks) {
        b->f_idx = f_bid++;
        int b_iid = 0;
        for (auto i: b->insts)
            i->f_idx = f_iid++, i->b_idx = b_iid++;
    }
}

void Module::addFunction(Function *f) {
    global_items.push_back(f);
    global_funcs.push_back(f);
}

Variable *Module::allocGlobalVar(int width, bool constant) {
    auto var = new
            Variable(nullptr, std::string("T") + std::to_string(T_id++), false, width, constant);
    global_vars.push_back(var);
    global_items.push_back(var);
    return var;
}
std::ostream &operator<<(std::ostream &os, const Module &mod) {
    for (auto item : mod.global_items)
        item->print(os);
    return os;
}

std::ostream &BasicBlock::print(std::ostream &os) const {
//    if (!jump_in.empty())
    os << "l" << label << ":" << std::endl;
    for (auto inst : insts)
        inst->print(os);
    return os;
}
void BasicBlock::safeRemove() {
    assert(!reachable);
    if (fall_out) {
        fall_out->fall_in = nullptr;
        fall_out = nullptr;
    }
    if (fall_in) {
        fall_in->fall_out = nullptr;
        fall_in = nullptr;
    }
    if (jump_out) {
        jump_out->jump_in.erase(this);
        jump_out = nullptr;
    }
    for (auto b: jump_in)
        b->jump_out = nullptr;
    jump_in.clear();
}
std::vector<BasicBlock *> BasicBlock::outBlocks() const {
    std::vector<BasicBlock *> res;
    if (fall_out)
        res.push_back(fall_out);
    if (jump_out)
        res.push_back(jump_out);
    return res;
}
std::vector<BasicBlock *> BasicBlock::inBlocks() const {
    std::vector<BasicBlock *> res;
    if (fall_in)
        res.push_back(fall_in);
    for (auto b: jump_in)
        res.push_back(b);
    return res;
}

std::ostream &Variable::print(std::ostream &os) const {
    if (func)
        os << '\t';
    os << "var ";
    if (width > 0)
        os << width << ' ';
    os << name << std::endl;
    return os;
}

std::ostream &BinaryInst::print(std::ostream &os) const {
    os << '\t' << dst->name << " = ";
    os << lhs << ' ' << BinaryExpr::OpStr[static_cast<int>(opt)] << ' ' << rhs << std::endl;
    return os;
}

std::ostream &UnaryInst::print(std::ostream &os) const {
    os << '\t' << dst->name << " = ";
    os << UnaryExpr::OpStr[static_cast<int>(opt)] << ' ' << opr << std::endl;
    return os;
}

std::ostream &CallInst::print(std::ostream &os) const {
    for (auto arg: args)
        os << "\tparam " << arg << std::endl;
    os << '\t' << dst->name << " = call f_" << name << std::endl;
    return os;
}

std::ostream &MoveInst::print(std::ostream &os) const {
    os << '\t' << dst->name << " = " << src << std::endl;
    return os;
}

std::ostream &StoreInst::print(std::ostream &os) const {
    os << "\t" << base->name << '[' << idx << ']' << " = " << src << std::endl;
    return os;
}

std::ostream &LoadInst::print(std::ostream &os) const {
    os << '\t' << dst->name << " = " << src->name << '[' << idx << ']' << std::endl;
    return os;
}

std::ostream &JumpInst::print(std::ostream &os) const {
    os << '\t' << "goto l" << dst->label << std::endl;
    return os;
}

std::ostream &BranchInst::print(std::ostream &os) const {
    os << "\t" << "if ";
    os << lhs << ' ' << BinaryExpr::OpStr[static_cast<int>(opt)] << ' ' << rhs;
    os << " goto l" << dst->label << std::endl;
    return os;
}

std::ostream &ReturnInst::print(std::ostream &os) const {
    os << "\t" << "return " << opr << std::endl;
    return os;
}

};
};
