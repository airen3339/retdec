/**
 * @file include/retdec/bin2llvmir/providers/abi/x86.h
 * @brief ABI information for x86.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#ifndef RETDEC_BIN2LLVMIR_PROVIDERS_ABI_X86_WATCOM_H
#define RETDEC_BIN2LLVMIR_PROVIDERS_ABI_X86_WATCOM_H

#include "retdec/bin2llvmir/providers/abi/abi.h"

namespace retdec {
namespace bin2llvmir {

class AbiX86Watcom : public Abi
{
	// Ctors, dtors.
	//
	public:
		AbiX86Watcom(llvm::Module* m, Config* c);
		virtual ~AbiX86Watcom();

	// Registers.
	//
	public:
		virtual bool isGeneralPurposeRegister(const llvm::Value* val) override;

	// Instructions.
	//
	public:
		virtual bool isNopInstruction(cs_insn* insn) override;
};

} // namespace bin2llvmir
} // namespace retdec

#endif
