//
// Created by herlight on 2019/12/19.
//

#ifndef __MC_EYR_OPTIMIZER_HH__
#define __MC_EYR_OPTIMIZER_HH__

#include "eyr.hh"
#include <map>
#include <set>

namespace mc {
namespace eyr {

class EyrOptimizer {
public:
    void optimize(Module *mod);

private:
    class ConstFolder {
    public:
        bool optimize(Module *mod);
    private:

        struct MaybeConst {
            int val{0};
            bool is_const{false};

            MaybeConst() = default;
            MaybeConst(int val) : val(val), is_const(true) {}

            friend bool operator==(const MaybeConst &a, const MaybeConst &b) {
                if (a.is_const && b.is_const)
                    return a.val == b.val;
                return a.is_const == b.is_const;
            }
            friend bool operator!=(const MaybeConst &a, const MaybeConst &b) {
                return !(a == b);
            }
        };

        void runOnFunction(Function *fun);
        void runOnBlock(BasicBlock *blk);
        void runOnInstruction(Instruction *ins);

        void handle(BinaryInst *ins);
        void handle(UnaryInst *ins);
        void handle(CallInst *ins);
        void handle(MoveInst *ins);
        void handle(StoreInst *ins);
        void handle(LoadInst *ins);
        void handle(BranchInst *ins);
        void handle(JumpInst *ins);
        void handle(ReturnInst *ins);

        bool isConst(Operand opr);
        int getConst(Operand opr);

        using ConstTable = std::map<Variable *, MaybeConst>;
        std::map<BasicBlock *, ConstTable> in_consts, out_consts;
        std::set<BasicBlock *> work_list;
        ConstTable cur_consts;
        bool rewrite{false};
        Instruction *cur_ins{nullptr};
        bool flow_changed{false}; // 是否改变了控制流

    } const_folder;

    class Simplifier {
    public:
        bool optimize(Module *mod);
    private:

        void runOnFunction(Function *fun);
        void runOnBlock(BasicBlock *blk);
        void runOnInstruction(Instruction *ins);

        void handle(AssignInst *ins);
        void handle(CallInst *ins);
        void handle(StoreInst *ins);
        void handle(BranchInst *ins);
        void handle(JumpInst *ins);
        void handle(ReturnInst *ins);
        void updateLives(Instruction *ins);

        using VarSet = std::set<Variable *>;
        std::map<BasicBlock *, VarSet> in_lives, out_lives, live_gen, live_kill;
        VarSet cur_lives;
        std::set<BasicBlock *> work_list;
        bool rewrite{false};
        bool changed{false};
        Instruction *cur_ins{nullptr};

    } simplifier;
};

}
}

#endif //__MC_EYR_OPTIMIZER_HH__
