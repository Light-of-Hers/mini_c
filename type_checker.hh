//
// Created by herlight on 2019/10/1.
//

#ifndef __MC_TYPE_CHECKER_HH__
#define __MC_TYPE_CHECKER_HH__

#include "ast.hh"
#include <unordered_map>
#include <map>
#include <string>
#include <list>

namespace mc {

struct TypeChecker : public ASTVisitor {
#define V(x) void visit(x *ast) override;
    VISITS()
#undef V
public:
    bool check(const std::vector<AST *> &tops);

private:
    Type *lookup(const std::string &id);
    void def(const std::string &id, Type *type);
    inline void enter_scope() { environ.emplace_front(); }
    inline void leave_scope() { environ.pop_front(); }

    Type *cur_ret_type;
    std::list<std::map<std::string, Type *>> environ;
    bool okay;
};

};

#endif //__MC_TYPE_CHECKER_HH__
