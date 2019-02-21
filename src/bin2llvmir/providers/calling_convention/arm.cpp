/**
 * @file src/bin2llvmir/providers/calling_convention/arm.cpp
 * @brief Calling conventions of ARM architecture.
 * @copyright (c) 2019 Avast Software, licensed under the MIT license
 */

#include "retdec/bin2llvmir/providers/calling_convention/arm.h"
#include "retdec/capstone2llvmir/arm/arm.h"

namespace retdec {
namespace bin2llvmir {

ArmCallingConvention::ArmCallingConvention(const Abi* a) :
	CallingConvention(a)
{
	_paramRegs = {
		ARM_REG_R0,
		ARM_REG_R1,
		ARM_REG_R2,
		ARM_REG_R3};

	_paramFPRegs = {
		ARM_REG_D0,
		ARM_REG_D1,
		ARM_REG_D2,
		ARM_REG_D3};

	_returnRegs = {
		ARM_REG_R0,
		ARM_REG_R1};

	_returnFPRegs = {
		ARM_REG_D0,
		ARM_REG_D1};

	_regNumPerParam = 2;
}

ArmCallingConvention::~ArmCallingConvention()
{
}

CallingConvention::Ptr ArmCallingConvention::create(const Abi* a)
{
	return std::make_unique<ArmCallingConvention>(a);
}

}
}
