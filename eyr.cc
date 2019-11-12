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

DeclInst *Function::allocLocalVar(BasicBlock *block, bool temp, int width, bool constant) {
    std::string var_name;
    if (temp)
        var_name = std::string("t") + std::to_string(module->t_id++);
    else
        var_name = std::string("T") + std::to_string(module->T_id++);
    auto var = new DeclInst(block, std::move(var_name), temp, width, constant);
    local_vars.push_back(var);
    // block->addInst(var);
    return var;
}

Function::Function(Module *m, std::string n, int argc) :
        module(m), name(std::move(n)), params(argc),
        fake(new BasicBlock(this)) {
    entry = allocBlock();
    for (int i = 0; i < argc; ++i)
        params[i] = new DeclInst(fake, std::string("p") + std::to_string(i), false);
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

    struct BBComp {
        bool operator()(BasicBlock *const &a, BasicBlock *const &b) {
            int a1 = a->reachable ? 1 : 0;
            int b1 = b->reachable ? 1 : 0;
            int a2 = a->fall_in ? a->fall_in->reachable ? 2 : 1 : 0;
            int b2 = b->fall_in ? b->fall_in->reachable ? 2 : 1 : 0;
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

    blocks = tmp_blocks;
    for (size_t i = 0; i < blocks.size(); ++i)
        blocks[i]->f_idx = i;
}

void Module::addFunction(Function *f) {
    global_items.push_back(f);
    global_funcs.push_back(f);
}

DeclInst *Module::allocGlobalVar(int width, bool constant) {
    auto var = new
            DeclInst(nullptr, std::string("T") + std::to_string(T_id++), false, width, constant);
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
    if (!jump_in.empty())
        os << "l" << label << ":" << std::endl;
    for (auto inst : insts)
        inst->print(os);
    return os;
}

std::ostream &DeclInst::print(std::ostream &os) const {
    if (block)
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
    os << "\t" << dst->name << '[' << idx << ']' << " = " << src << std::endl;
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
