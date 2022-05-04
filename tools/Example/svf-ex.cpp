
#include "Graphs/SVFG.h"
#include "SVF-FE/LLVMUtil.h"
#include "SVF-FE/SVFIRBuilder.h"
#include "Util/Options.h"
#include "WPA/Andersen.h"
#include "WPA/TypeAnalysis.h"
#include <iostream>

using namespace llvm;
using namespace std;
using namespace SVF;

static llvm::cl::opt<std::string> InputFilename(cl::Positional,
    llvm::cl::desc("<input bitcode>"), llvm::cl::init("-"));
/*!
 * An example to query alias results of two LLVM values
 */
AliasResult aliasQuery(PointerAnalysis* pta, Value* v1, Value* v2)
{
    return pta->alias(v1, v2);
}

std::string printPts(PointerAnalysis* pta, Value* val)
{

    std::string str;
    raw_string_ostream rawstr(str);

    NodeID pNodeId = pta->getPAG()->getValueNode(val);
    const PointsTo& pts = pta->getPts(pNodeId);
    for (PointsTo::iterator ii = pts.begin(), ie = pts.end();
         ii != ie; ii++) {
        rawstr << " " << *ii << " ";
        PAGNode* targetObj = pta->getPAG()->getGNode(*ii);
        if (targetObj->hasValue()) {
            rawstr << "(" << *targetObj->getValue() << ")\t ";
        }
    }

    return rawstr.str();
}

void traverseOnICFG(ICFG* icfg, const Instruction* inst)
{
    ICFGNode* iNode = icfg->getICFGNode(inst);
    FIFOWorkList<const ICFGNode*> worklist;
    Set<const ICFGNode*> visited;
    worklist.push(iNode);

    while (!worklist.empty()) {
        const ICFGNode* vNode = worklist.pop();
        for (ICFGNode::const_iterator it = iNode->OutEdgeBegin(), eit = iNode->OutEdgeEnd(); it != eit; ++it) {
            ICFGEdge* edge = *it;
            ICFGNode* succNode = edge->getDstNode();
            if (visited.find(succNode) == visited.end()) {
                visited.insert(succNode);
                worklist.push(succNode);
            }
        }
    }
}

void traverseOnVFG(const SVFG* vfg, Value* val)
{
    SVFIR* pag = SVFIR::getPAG();

    PAGNode* pNode = pag->getGNode(pag->getValueNode(val));
    const VFGNode* vNode = vfg->getDefSVFGNode(pNode);
    FIFOWorkList<const VFGNode*> worklist;
    Set<const VFGNode*> visited;
    worklist.push(vNode);

    while (!worklist.empty()) {
        const VFGNode* vNode = worklist.pop();
        for (VFGNode::const_iterator it = vNode->OutEdgeBegin(), eit = vNode->OutEdgeEnd(); it != eit; ++it) {
            VFGEdge* edge = *it;
            VFGNode* succNode = edge->getDstNode();
            if (visited.find(succNode) == visited.end()) {
                visited.insert(succNode);
                worklist.push(succNode);
            }
        }
    }
}

int main(int argc, char** argv)
{

    int arg_num = 0;
    char** arg_value = new char*[argc];
    std::vector<std::string> moduleNameVec;
    SVFUtil::processArguments(argc, argv, arg_num, arg_value, moduleNameVec);
    cl::ParseCommandLineOptions(arg_num, arg_value,
        "Whole Program Points-to Analysis\n");

    if (Options::WriteAnder == "ir_annotator") {
        LLVMModuleSet::getLLVMModuleSet()->preProcessBCs(moduleNameVec);
    }

    SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);
    svfModule->buildSymbolTableInfo();

    /// Build Program Assignment Graph (SVFIR)
    SVFIRBuilder builder;
    SVFIR* pag = builder.build(svfModule);

    /// Create Andersen's pointer analysis
    Andersen* ander = AndersenWaveDiff::createAndersenWaveDiff(pag);

    /// Query aliases
    /// aliasQuery(ander,value1,value2);

    /// Print points-to information
    /// printPts(ander, value1);

    /// Call Graph
    // PTACallGraph* callgraph = ander->getPTACallGraph();

    /// ICFG
    // ICFG* icfg = pag->getICFG();

    /// Value-Flow Graph (VFG)
    // VFG* vfg = new VFG(callgraph);

    /// Sparse value-flow graph (SVFG)
    // SVFGBuilder svfBuilder(true);
    // SVFG* svfg = svfBuilder.buildFullSVFG(ander);

    // cout << "\n\n\n\n\n*************HERE*************\n\n\n\n\n\n\n";
    // FlowSensitive* fspta = new FlowSensitive(pag);
    // fspta->analyze();
    // cout << "\n\n\n\n\nIndirect Calls:\n\n\n\n";
    // auto maps = fspta->getIndCallMap();
    // for (auto itr = maps.begin(); itr != maps.end(); itr++) {
    //     itr->first->getCallSite()->print(errs());
    //     cout << " is an indirect call to:" << endl
    //          << "{ ";
    //     int n_funcs = itr->second.size();
    //     for (auto funcs : itr->second) {
    //         cout << funcs->getLLVMFun()->getFunction().getName().str();
    //         n_funcs--;
    //         if (n_funcs != 0)
    //             cout << " OR ";
    //     }
    //
    //     cout << " }" << endl
    //          << endl
    //          << endl;
    // }
    //
    int n_indirect = 0, n_monomorphic = 0, n_polymorphic = 0, n_resolved = 0;
    TypeAnalysis* cha = new TypeAnalysis(pag);
    // Andersen *cha =  ander;
    cha->analyze();
    cout << "\n\n\n\n\nIndirect Calls using CHA:\n\n\n\n";
    auto maps_cha = cha->getIndCallMap();
    std::unordered_map<NodeID , int> cha_calls;
    int comparative_n_monomorphic_cha = 0;
    int comparative_n_polymorphic_cha = 0;
    int comparative_n_resolved_cha = 0;
    for (auto itr = maps_cha.begin(); itr != maps_cha.end(); itr++) {
        n_indirect++;
        itr->first->getCallSite()->print(errs());
        // if(cha_calls.find(itr->first->getId()) != cha_calls.end()){
        //     cout<<"\n\n\n!!!!!!!!!!!!!!!!!!!!REPETITION!!!!!!!!\n\n\n\n";
        // }
        cout << " is an indirect call to:" << endl
             << "{ ";
        int n_funcs = itr->second.size();
        cha_calls[itr->first->getId()] = n_funcs;
        n_resolved += n_funcs;
        if (n_funcs == 1)
            n_monomorphic++;
        else
            n_polymorphic++;
        for (auto funcs : itr->second) {
            cout << funcs->getLLVMFun()->getFunction().getName().str();
            n_funcs--;
            if (n_funcs != 0)
                cout << " OR ";
        }

        cout << " }" << endl
             << endl
             << endl;
    }
    

    string stats = "Stats: \n\n";
    stats += "\n\nCHA stats:\n";
    stats += "Total indirect calls = " + to_string(n_indirect);
    stats += "\nTotal monomorphic calls = " + to_string(n_monomorphic);
    stats += "\nTotal Polymorphic calls = " + to_string(n_polymorphic);
    stats += "\nIndirect calls resolved to total " + to_string(n_resolved) + " functions.";

    n_indirect = 0;
    n_monomorphic = 0;
    n_polymorphic = 0;
    n_resolved = 0;
    int comparative_n_monomorphic_vsfs = 0;
    int comparative_n_polymorphic_vsfs = 0;
    int comparative_n_resolved_vsfs = 0;
    VersionedFlowSensitive* vfspta = new VersionedFlowSensitive(pag);
    vfspta->analyze();
    cout << "\n\n\n\n\nIndirect Calls using Versioned Flow Sensitive Analysis:\n\n\n\n";
    auto maps_vfspta = vfspta->getIndCallMap();
    for (auto itr = maps_vfspta.begin(); itr != maps_vfspta.end(); itr++) {
        n_indirect++;
        itr->first->getCallSite()->print(errs());
        cout << " is an indirect call to:" << endl
             << "{ ";
        int n_funcs = itr->second.size();
        if (cha_calls.find(itr->first->getId()) != cha_calls.end()) {
            if(cha_calls[itr->first->getId()] == 1)
                comparative_n_monomorphic_cha++;
            else
                comparative_n_polymorphic_cha++;
            if (n_funcs == 1)
                comparative_n_monomorphic_vsfs++;
            else
                comparative_n_polymorphic_vsfs++;
            comparative_n_resolved_vsfs += n_funcs;
            comparative_n_resolved_cha += cha_calls[itr->first->getId()];
        }
        n_resolved += n_funcs;
        if (n_funcs == 1)
            n_monomorphic++;
        else
            n_polymorphic++;
        for (auto funcs : itr->second) {
            cout << funcs->getLLVMFun()->getFunction().getName().str();
            n_funcs--;
            if (n_funcs != 0)
                cout << " OR ";
        }

        cout << " }" << endl
             << endl
             << endl;
    }
    stats += "\n\nVSFS stats:\n";
    stats += "Total indirect calls = " + to_string(n_indirect);
    stats += "\nTotal monomorphic calls = " + to_string(n_monomorphic);
    stats += "\nTotal Polymorphic calls = " + to_string(n_polymorphic);
    stats += "\nIndirect calls resolved to total " + to_string(n_resolved) + " functions.";

    cout << "\n\n\n\n*******************************************************\n";
    cout << stats;
    cout << "\n\nComparative Study(For same indirect calls):\n";
    cout << "Monomorphic calls :\t VSFS : " << comparative_n_monomorphic_vsfs << "\t CHA : " << comparative_n_monomorphic_cha << endl;
    cout << "Polymorphic calls :\t VSFS : " << comparative_n_polymorphic_vsfs << "\tCHA : " << comparative_n_polymorphic_cha << endl;
    cout << "Total Resolved :   \t VSFS : " << comparative_n_resolved_vsfs << "\tCHA : " << comparative_n_resolved_cha << endl;
    cout << "\n*******************************************************\n";

    /// Collect uses of an LLVM Value
    /// traverseOnVFG(svfg, value);

    /// Collect all successor nodes on ICFG
    /// traverseOnICFG(icfg, value);

    // clean up memory
    // delete vfg;
    // delete svfg;
    AndersenWaveDiff::releaseAndersenWaveDiff();
    SVFIR::releaseSVFIR();

    LLVMModuleSet::getLLVMModuleSet()->dumpModulesToFile(".svf.bc");
    SVF::LLVMModuleSet::releaseLLVMModuleSet();

    llvm::llvm_shutdown();
    return 0;
}
