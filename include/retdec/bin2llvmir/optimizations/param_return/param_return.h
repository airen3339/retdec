/**
* @file include/retdec/bin2llvmir/optimizations/param_return/param_return.h
* @brief Detect functions' parameters and returns.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#ifndef RETDEC_BIN2LLVMIR_OPTIMIZATIONS_PARAM_RETURN_PARAM_RETURN_H
#define RETDEC_BIN2LLVMIR_OPTIMIZATIONS_PARAM_RETURN_PARAM_RETURN_H

#include <map>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#include "retdec/bin2llvmir/analyses/reaching_definitions.h"
#include "retdec/bin2llvmir/providers/abi/abi.h"
#include "retdec/bin2llvmir/providers/config.h"
#include "retdec/bin2llvmir/providers/debugformat.h"
#include "retdec/bin2llvmir/providers/fileimage.h"
#include "retdec/bin2llvmir/providers/lti.h"
#include "retdec/utils/container.h"

namespace retdec {
namespace bin2llvmir {

class CallEntry
{
	public:
		CallEntry(llvm::CallInst* c);

	public:
		bool instructionStoresString(
				llvm::StoreInst* si,
				std::string& str,
				ReachingDefinitionsAnalysis& _RDA) const;

		void extractSpecificArgTypes(
				llvm::Module* m,
				ReachingDefinitionsAnalysis& _RDA,
				llvm::CallInst* wrappedCall = nullptr);

	private:
		std::string extractFormatString(ReachingDefinitionsAnalysis& _RDA) const;

	public:
		llvm::CallInst* call = nullptr;
		std::vector<llvm::Value*> possibleArgs;
		std::vector<llvm::StoreInst*> possibleArgStores;
		std::vector<llvm::LoadInst*> possibleRetLoads;
		std::vector<llvm::Type*> specTypes;
};

class ReturnEntry
{
	public:
		ReturnEntry(llvm::ReturnInst* r);

	public:
		llvm::ReturnInst* ret = nullptr;
		std::vector<llvm::StoreInst*> possibleRetStores;
};

class ParamFilter
{
	public:
		ParamFilter(
			llvm::CallInst* call,
			const std::vector<llvm::Value*>& paramValues,
			const std::vector<llvm::Type*> &types,
			const Abi* abi,
			Config* config);

		void orderStacks(std::vector<llvm::Value*>& stacks, bool asc = true) const;
		void orderRegistersBy(
				std::vector<uint32_t>& regs,
				const std::vector<uint32_t>& orderedVector) const;

		void leaveOnlyContinuousSequence();
		void leaveOnlyContinuousStackOffsets();
		void leaveOnlyPositiveStacks();

		void adjustValuesByKnownTypes(
				llvm::CallInst* call, 
				std::vector<llvm::Type*> &types);

		std::vector<llvm::Value*> getParamValues() const;

	private:
		void separateParamValues(const std::vector<llvm::Value*>& paramValues);
		void applyAlternatingRegistersFilter();
		void applySequentialRegistersFilter();
		llvm::Value* stackVariableForType(llvm::CallInst* call, llvm::Type* type) const;

		bool moveRegsByTypeSizeAtIdx(std::vector<uint32_t> &destinastion,
					const std::vector<uint32_t> &sourceTemplate,
					llvm::Type* type,
					uint32_t* idx);

	private:
		const Abi* _abi;
		Config* _config;

		llvm::CallInst* _call;

		std::vector<uint32_t> _regValues;
		std::vector<uint32_t> _fpRegValues;
		std::vector<llvm::Value*> _stackValues;

		std::vector<llvm::Type*> _paramTypes;
};

class DataFlowEntry
{
	public:
		DataFlowEntry(
				llvm::Module* m,
				ReachingDefinitionsAnalysis& rda,
				Config* c,
				Abi* abi,
				FileImage* img,
				DebugFormat* dbg,
				Lti* lti,
				llvm::Value* v);

		bool isFunctionEntry() const;
		bool isValueEntry() const;
		llvm::Value* getValue() const;
		llvm::Function* getFunction() const;
		void dump() const;

		void addCall(llvm::CallInst* call);

		void filter();

		void applyToIr();
		void connectWrappers();

	private:
		void addArgLoads();
		void addRetStores();
		void addCallArgs(llvm::CallInst* call, CallEntry& ce);
		void addCallReturns(llvm::CallInst* call, CallEntry& ce);

		std::set<llvm::Value*> collectArgsFromInstruction(
				llvm::Instruction* startInst,
				std::map<llvm::BasicBlock*, std::set<llvm::Value*>> &seenBlocks,
				std::vector<llvm::StoreInst*> *possibleArgStores = nullptr);

		void callsFilterCommonRegisters();
		void callsFilterSameNumberOfStacks();

		void setTypeFromExtraInfo();
		void setTypeFromUseContext();
		void setReturnType();
		void setArgumentTypes();

		llvm::CallInst* isSimpleWrapper(llvm::Function* fnc) const;

		void filterSortArgLoads();
		void filterNegativeStacks();

		void filterKnownParamPairs();

		void replaceCalls();
		std::map<llvm::CallInst*, std::vector<llvm::Value*>>
					fetchLoadsOfCalls() const;

		llvm::Value* joinParamPair(llvm::Value* low, llvm::Value* high,
				llvm::Type *type, llvm::Instruction *before) const;
		void splitIntoParamPair(
				llvm::AllocaInst* blob,
				std::pair<llvm::Value*, llvm::Value*> &paramPair) const;

	public:
		llvm::Module* _module = nullptr;
		ReachingDefinitionsAnalysis& _RDA;
		Config* _config = nullptr;
		Abi* _abi = nullptr;
		FileImage* _image = nullptr;
		Lti* _lti = nullptr;

		llvm::Value* called = nullptr;
		retdec::config::Function* configFnc = nullptr;
		retdec::config::Function* dbgFnc = nullptr;

		// In caller.
		//
		std::vector<CallEntry> calls;

		// In called function.
		//
		std::vector<llvm::Value*> regArgs;
		std::vector<llvm::Value*> args;
		std::vector<ReturnEntry> retStores;

		// Result.
		//
		bool typeSet = false;
		llvm::Type* retType = nullptr;
		std::vector<llvm::Type*> argTypes;
		std::map<std::size_t, llvm::Value*> specialArgStorage;
		bool isVarArg = false;
		llvm::CallInst* wrappedCall = nullptr;
		std::vector<std::string> argNames;
};

class ParamReturn : public llvm::ModulePass
{
	public:
		static char ID;
		ParamReturn();
		bool runOnModuleCustom(
				llvm::Module& m,
				Config* c,
				Abi* abi,
				FileImage* img = nullptr,
				DebugFormat* dbgf = nullptr,
				Lti* lti = nullptr);
		virtual bool runOnModule(llvm::Module& m) override;

	private:
		bool run();
		void dumpInfo();

		void collectAllCalls();

		void filterCalls();
		void filterSort(CallEntry& ce);

		void applyToIr();

	private:
		llvm::Module* _module = nullptr;
		Config* _config = nullptr;
		Abi* _abi = nullptr;
		FileImage* _image = nullptr;
		DebugFormat* _dbgf = nullptr;
		Lti* _lti = nullptr;

		std::map<llvm::Value*, DataFlowEntry> _fnc2calls;
		ReachingDefinitionsAnalysis _RDA;
};

} // namespace bin2llvmir
} // namespace retdec

#endif
