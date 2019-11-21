//
// Created by herlight on 2019/11/18.
//

#ifndef __MC_LIVE_ANALYZER_HH__
#define __MC_LIVE_ANALYZER_HH__

#include "tgr.hh"

namespace mc {
namespace tgr {

class LiveAnalyzer {
public:
    static void analyze(Module *mod);

private:
    static void localAnalyze(BasicBlock *blk);
    static void globalAnalyze(Function *func);
};

}
}

#endif //__MC_LIVE_ANALYZER_HH__
