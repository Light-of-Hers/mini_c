//
// Created by herlight on 2019/10/5.
//
#include "eyr.hh"
#include <algorithm>
#include "ast.hh"


namespace mc {
namespace eyr {

BasicBlock *Function::allocBlock() {
    blocks.push_back(new BasicBlock(this));
    return blocks.back();
}

DeclInst *Function::allocLocalVar(BasicBlock *block, bool temp, int width) {
    std::string var_name;
    if (temp)
        var_name = std::string("t") + std::to_string(t_id++);
    else
        var_name = std::string("T") + std::to_string(T_id++);
    auto var = new DeclInst(block, std::move(var_name), temp, width);
    local_vars.push_back(var);
//    block->addInst(var);
    return var;
}

Function::Function(Module *m, std::string n, int argc) :
        module(m), name(std::move(n)), params(argc),
        fake(new BasicBlock(this)), entry(allocBlock()),
        T_id(m->global_vars.size()), t_id(0) {
    for (int i = 0; i < argc; ++i)
        params[i] = new DeclInst(fake, std::string("p") + std::to_string(i), false);
}

std::ostream &Function::print(std::ostream &os) const {
    os << "f_" << name << " [" << params.size() << ']' << std::endl;
    std::set<BasicBlock *> blks(blocks.begin(), blocks.end());
    auto blk = entry;
    for (auto var: local_vars)
        var->print(os);
    while (true) {
        for (auto b = blk; b; b = b->fall_out) {
            b->print(os);
            blks.erase(b);
        }
        if (blks.empty())
            break;
        auto it = std::find_if(blks.begin(), blks.end(), [](BasicBlock *b) {
            return b->fall_in == nullptr;
        });
        blk = *it;
    }
    os << "end f_" << name << std::endl;
    return os;
}

void Module::addFunction(Function *f) {
    global_items.push_back(f);
    global_funcs.push_back(f);
}

DeclInst *Module::allocGlobalVar(int width) {
    auto var = new
            DeclInst(nullptr, std::string("T") + std::to_string(global_vars.size()), false, width);
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
    os << '\t' << dst->name << " = f_" << name << std::endl;
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
    os << "\t" << "if ( ";
    os << lhs << ' ' << BinaryExpr::OpStr[static_cast<int>(opt)] << ' ' << rhs;
    os << " ) goto l" << dst->label << std::endl;
    return os;
}


std::ostream &ReturnInst::print(std::ostream &os) const {
    os << "\t" << "return " << opr << std::endl;
    return os;
}
};
};
