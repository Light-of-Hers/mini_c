//
// Created by herlight on 2019/10/6.
//

#ifndef __MC_EYR_EMITTER_HH__
#define __MC_EYR_EMITTER_HH__

#include "ast.hh"
#include "eyr.hh"
#include <map>
#include <list>

namespace mc {
namespace eyr {

struct EyrEmitter : public ASTVisitor {
#define V(x) void visit(x *ast) override;
    VISITS()
#undef V
public:
    Module *emit(const Program &prog);
private:
    Variable *lookup(const std::string &id);
    void def(const std::string &id, Variable *var);
    inline void enter_scope() { environ.emplace_front(); }
    inline void leave_scope() { environ.pop_front(); }
    inline Variable *allocTemp() { return cur_func->allocLocalVar(); }

    std::list<std::map<std::string, Variable *>> environ;

    BasicBlock *cur_blk;
    Function *cur_func;
    Operand cur_opr;
    bool will_store;
    Module *module;
};

};
};

#endif //MINI_C_EYR_EMITTER_HH
