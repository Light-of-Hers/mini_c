//
// Created by herlight on 2019/11/18.
//

#include "live_analyzer.hh"

namespace mc {
namespace tgr {


void LiveAnalyzer::analyze(Module *mod) {
    for (auto func: mod->funcs)
        globalAnalyze(func);
}
void LiveAnalyzer::localAnalyze(BasicBlock *blk) {
    blk->live_gen.clear(), blk->live_kill.clear();
    for (auto op: blk->ops) {
        for (auto use: op->getUsedVirRegs()) {
            if (blk->live_kill.count(use) == 0)
                blk->live_gen.insert(use);
        }
        for (auto def: op->getDefinedVirRegs()) {
            blk->live_kill.insert(def);
        }
    }
}
void LiveAnalyzer::globalAnalyze(Function *func) {
    for (auto blk: func->blocks) {
        localAnalyze(blk);
        blk->live_in.clear(), blk->live_out.clear();
    }
    bool stop;
    do {
        stop = true;
        for (auto blk: func->blocks) {
            for (auto sux: blk->outBlocks()) {
                for (auto reg: sux->live_in)
                    blk->live_out.insert(reg);
            }
            auto old = blk->live_in;
            blk->live_in = blk->live_out;
            for (auto reg: blk->live_kill)
                blk->live_in.erase(reg);
            for (auto reg: blk->live_gen)
                blk->live_in.insert(reg);
            if (old != blk->live_in)
                stop = false;
        }
    } while (!stop);

//    for (auto blk: func->blocks) {
//        std::cerr << blk->label << ": ";
//        std::cerr << "live-in[";
//        for (auto reg: blk->live_in)
//            std::cerr << reg << ", ";
//        std::cerr << "], ";
//        std::cerr << "live-out[";
//        for (auto reg: blk->live_out)
//            std::cerr << reg << ", ";
//        std::cerr << "]" << std::endl;
//    }
}


}
}