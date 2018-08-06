/**
* @file tests/bin2llvmir/optimizations/param_return/tests/param_return_tests.cpp
* @brief Tests for the @c ParamReturn pass.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#include "retdec/bin2llvmir/optimizations/param_return/param_return.h"
#include "bin2llvmir/utils/llvmir_tests.h"

using namespace ::testing;
using namespace llvm;

namespace retdec {
namespace bin2llvmir {
namespace tests {

/**
 * @brief Tests for the @c ParamReturn pass.
 */
class ParamReturnTests: public LlvmIrTests
{
	protected:
		ParamReturn pass;
};

////
//// x86
////
//
// x86
//

TEST_F(ParamReturnTests, x86PtrCallBasicFunctionality)
{
	parseInput(R"(
		@r = global i32 0
		define void @fnc() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
			store i32 123, i32* %stack_-4
			store i32 456, i32* %stack_-8
			%a = bitcast i32* @r to void()*
			call void %a()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "little",
			"name" : "x86"
		},
		"functions" : [
			{
				"name" : "fnc",
				"locals" : [
					{
						"name" : "stack_-4",
						"storage" : { "type" : "stack", "value" : -4 }
					},
					{
						"name" : "stack_-8",
						"storage" : { "type" : "stack", "value" : -8 }
					}
				]
			}
		]
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r = global i32 0
		define void @fnc() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
			store i32 123, i32* %stack_-4
			store i32 456, i32* %stack_-8
			%a = bitcast i32* @r to void()*
			%1 = load i32, i32* %stack_-8
			%2 = load i32, i32* %stack_-4
			%3 = bitcast void ()* %a to void (i32, i32)*
			call void %3(i32 %1, i32 %2)
			ret void
		}
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, x86PtrCallPrevBbIsUsedOnlyIfItIsASinglePredecessor)
{
	parseInput(R"(
		@r = global i32 0
		define void @fnc() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
		br label %lab1
		lab1:
			store i32 123, i32* %stack_-4
		br label %lab2
		lab2:
			store i32 456, i32* %stack_-8
			%a = bitcast i32* @r to void()*
			call void %a()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "little",
			"name" : "x86"
		},
		"functions" : [
			{
				"name" : "fnc",
				"locals" : [
					{
						"name" : "stack_-4",
						"storage" : { "type" : "stack", "value" : -4 }
					},
					{
						"name" : "stack_-8",
						"storage" : { "type" : "stack", "value" : -8 }
					}
				]
			}
		]
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r = global i32 0
		define void @fnc() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
		br label %lab1
		lab1:
			store i32 123, i32* %stack_-4
		br label %lab2
		lab2:
			store i32 456, i32* %stack_-8
			%a = bitcast i32* @r to void()*
			%1 = load i32, i32* %stack_-8
			%2 = load i32, i32* %stack_-4
			%3 = bitcast void ()* %a to void (i32, i32)*
			call void %3(i32 %1, i32 %2)
			ret void
		}
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, x86PtrCallPrevBbIsNotUsedIfItIsNotASinglePredecessor)
{
	parseInput(R"(
		@r = global i32 0
		define void @fnc() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
		br label %lab1
		lab1:
			store i32 123, i32* %stack_-4
		br label %lab2
		lab2:
			store i32 456, i32* %stack_-8
			%a = bitcast i32* @r to void()*
			call void %a()
			br label %lab2
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "little",
			"name" : "x86"
		},
		"functions" : [
			{
				"name" : "fnc",
				"locals" : [
					{
						"name" : "stack_-4",
						"storage" : { "type" : "stack", "value" : -4 }
					},
					{
						"name" : "stack_-8",
						"storage" : { "type" : "stack", "value" : -8 }
					}
				]
			}
		]
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r = global i32 0
		define void @fnc() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
		br label %lab1
		lab1:
			store i32 123, i32* %stack_-4
		br label %lab2
		lab2:
			store i32 456, i32* %stack_-8
			%a = bitcast i32* @r to void()*
			%1 = load i32, i32* %stack_-8
			%2 = bitcast void ()* %a to void (i32)*
			call void %2(i32 %1)
			br label %lab2
			ret void
		}
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, x86PtrCallOnlyStackStoresAreUsed)
{
	parseInput(R"(
		@eax = global i32 0
		@r = global i32 0
		define void @fnc() {
			%stack_-4 = alloca i32
			%local = alloca i32
			store i32 123, i32* %stack_-4
			store i32 456, i32* %local
			store i32 789, i32* @eax
			%a = bitcast i32* @r to void()*
			call void %a()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "little",
			"name" : "x86"
		},
		"functions" : [
			{
				"name" : "fnc",
				"locals" : [
					{
						"name" : "stack_-4",
						"storage" : { "type" : "stack", "value" : -4 }
					}
				]
			}
		]
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(X86_REG_EAX, getGlobalByName("eax"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@eax = global i32 0
		@r = global i32 0
		define i32 @fnc() {
			%stack_-4 = alloca i32
			%local = alloca i32
			store i32 123, i32* %stack_-4
			store i32 456, i32* %local
			store i32 789, i32* @eax
			%a = bitcast i32* @r to void()*
			%1 = load i32, i32* %stack_-4
			%2 = bitcast void ()* %a to void (i32)*
			call void %2(i32 %1)
			%3 = load i32, i32* @eax
			ret i32 %3
		}
		declare void @0()
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, x86PtrCallStackAreUsedAsArgumentsInCorrectOrder)
{
	parseInput(R"(
		@r = global i32 0
		define void @fnc() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
			store i32 456, i32* %stack_-8
			store i32 123, i32* %stack_-4
			%a = bitcast i32* @r to void()*
			call void %a()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "little",
			"name" : "x86"
		},
		"functions" : [
			{
				"name" : "fnc",
				"locals" : [
					{
						"name" : "stack_-4",
						"storage" : { "type" : "stack", "value" : -4 }
					},
					{
						"name" : "stack_-8",
						"storage" : { "type" : "stack", "value" : -8 }
					}
				]
			}
		]
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r = global i32 0
		define void @fnc() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
			store i32 456, i32* %stack_-8
			store i32 123, i32* %stack_-4
			%a = bitcast i32* @r to void()*
			%1 = load i32, i32* %stack_-8
			%2 = load i32, i32* %stack_-4
			%3 = bitcast void ()* %a to void (i32, i32)*
			call void %3(i32 %1, i32 %2)
			ret void
		}
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, x86PtrCallOnlyContinuousStackOffsetsAreUsed)
{
	parseInput(R"(
		@r = global i32 0
		define void @fnc() {
			%stack_-4 = alloca i32
			%stack_-16 = alloca i32
			%stack_-20 = alloca i32
			%stack_-24 = alloca i32
			store i32 1, i32* %stack_-16
			store i32 2, i32* %stack_-20
			store i32 3, i32* %stack_-24
			store i32 4, i32* %stack_-4
			%a = bitcast i32* @r to void()*
			call void %a()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "little",
			"name" : "x86"
		},
		"functions" : [
			{
				"name" : "fnc",
				"locals" : [
					{
						"name" : "stack_-4",
						"storage" : { "type" : "stack", "value" : -4 }
					},
					{
						"name" : "stack_-16",
						"storage" : { "type" : "stack", "value" : -16 }
					},
					{
						"name" : "stack_-20",
						"storage" : { "type" : "stack", "value" : -20 }
					},
					{
						"name" : "stack_-24",
						"storage" : { "type" : "stack", "value" : -24 }
					}
				]
			}
		]
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r = global i32 0
		define void @fnc() {
			%stack_-4 = alloca i32
			%stack_-16 = alloca i32
			%stack_-20 = alloca i32
			%stack_-24 = alloca i32
			store i32 1, i32* %stack_-16
			store i32 2, i32* %stack_-20
			store i32 3, i32* %stack_-24
			store i32 4, i32* %stack_-4
			%a = bitcast i32* @r to void()*
			%1 = load i32, i32* %stack_-24
			%2 = load i32, i32* %stack_-20
			%3 = load i32, i32* %stack_-16
			%4 = bitcast void ()* %a to void (i32, i32, i32)*
			call void %4(i32 %1, i32 %2, i32 %3)
			ret void
		}
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, x86ExternalCallBasicFunctionality)
{
	parseInput(R"(
		declare void @print()
		define void @fnc() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
			store i32 123, i32* %stack_-4
			store i32 456, i32* %stack_-8
			call void @print()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "little",
			"name" : "x86"
		},
		"functions" : [
			{
				"name" : "fnc",
				"locals" : [
					{
						"name" : "stack_-4",
						"storage" : { "type" : "stack", "value" : -4 }
					},
					{
						"name" : "stack_-8",
						"storage" : { "type" : "stack", "value" : -8 }
					}
				]
			}
		]
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		declare void @print(i32, i32)
		declare void @0()
		define void @fnc() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
			store i32 123, i32* %stack_-4
			store i32 456, i32* %stack_-8
			%1 = load i32, i32* %stack_-8
			%2 = load i32, i32* %stack_-4
			call void @print(i32 %1, i32 %2)
			ret void
		}
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, x86ExternalCallFixOnMultiplePlaces)
{
	parseInput(R"(
		declare void @print()
		define void @fnc1() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
			store i32 123, i32* %stack_-4
			store i32 456, i32* %stack_-8
			call void @print()
			ret void
		}
		define void @fnc2() {
			%stack_-16 = alloca i32
			%stack_-20 = alloca i32
			%stack_-24 = alloca i32
			store i32 456, i32* %stack_-20
			store i32 123, i32* %stack_-16
			store i32 123, i32* %stack_-24
			call void @print()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "little",
			"name" : "x86"
		},
		"functions" : [
			{
				"name" : "fnc1",
				"locals" : [
					{
						"name" : "stack_-4",
						"storage" : { "type" : "stack", "value" : -4 }
					},
					{
						"name" : "stack_-8",
						"storage" : { "type" : "stack", "value" : -8 }
					}
				]
			},
			{
				"name" : "fnc2",
				"locals" : [
					{
						"name" : "stack_-16",
						"storage" : { "type" : "stack", "value" : -16 }
					},
					{
						"name" : "stack_-20",
						"storage" : { "type" : "stack", "value" : -20 }
					},
					{
						"name" : "stack_-24",
						"storage" : { "type" : "stack", "value" : -24 }
					}
				]
			}
		]
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		declare void @print(i32, i32)
		declare void @0()
		define void @fnc1() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
			store i32 123, i32* %stack_-4
			store i32 456, i32* %stack_-8
			%1 = load i32, i32* %stack_-8
			%2 = load i32, i32* %stack_-4
			call void @print(i32 %1, i32 %2)
			ret void
		}
		define void @fnc2() {
			%stack_-16 = alloca i32
			%stack_-20 = alloca i32
			%stack_-24 = alloca i32
			store i32 456, i32* %stack_-20
			store i32 123, i32* %stack_-16
			store i32 123, i32* %stack_-24
			%1 = load i32, i32* %stack_-24
			%2 = load i32, i32* %stack_-20
			call void @print(i32 %1, i32 %2)
			ret void
		}
	)";
	checkModuleAgainstExpectedIr(exp);
}

//TEST_F(ParamReturnTests, x86ExternalCallSomeFunctionCallsAreNotModified)
//{
//	parseInput(R"(
//		@r = global i32 0
//		define void @fnc() {
//			%stack_-4 = alloca i32
//			%stack_-8 = alloca i32
//			store i32 123, i32* %stack_-4
//			store i32 456, i32* %stack_-8
//			%a = bitcast i32* @r to void()*
//			call void %a()
//			ret void
//		}
//	)");
//	auto config = Config::fromJsonString(module.get(), R"({
//		"architecture" : {
//			"bitSize" : 32,
//			"endian" : "little",
//			"name" : "x86"
//		},
//		"functions" : [
//			{
//				"name" : "fnc",
//				"locals" : [
//					{
//						"name" : "stack_-4",
//						"storage" : { "type" : "stack", "value" : -4 }
//					},
//					{
//						"name" : "stack_-8",
//						"storage" : { "type" : "stack", "value" : -8 }
//					}
//				]
//			}
//		]
//	})");
//	auto abi = AbiProvider::addAbi(module.get(), &config);
//
//	pass.runOnModuleCustom(*module, &config, abi);
//
//	std::string exp = R"(
//		@r = global i32 0
//		define void @fnc() {
//			%stack_-4 = alloca i32
//			%stack_-8 = alloca i32
//			store i32 123, i32* %stack_-4
//			store i32 456, i32* %stack_-8
//			%a = bitcast i32* @r to void()*
//			%1 = load i32, i32* %stack_-8
//			%2 = load i32, i32* %stack_-4
//			%3 = bitcast void ()* %a to void (i32, i32)*
//			call void %3(i32 %1, i32 %2)
//			ret void
//		}
//	)";
//	checkModuleAgainstExpectedIr(exp);
//}
//
//TEST_F(ParamReturnTests, x86PtrCallPrevBbIsUsedOnlyIfItIsASinglePredecessor)
//{
//	parseInput(R"(
//		@r = global i32 0
//		define void @fnc() {
//			%stack_-4 = alloca i32
//			%stack_-8 = alloca i32
//		br label %lab1
//		lab1:
//			store i32 123, i32* %stack_-4
//		br label %lab2
//		lab2:
//			store i32 456, i32* %stack_-8
//			%a = bitcast i32* @r to void()*
//			call void %a()
//			ret void
//		}
//	)");
//	auto config = Config::fromJsonString(module.get(), R"({
//		"architecture" : {
//			"bitSize" : 32,
//			"endian" : "little",
//			"name" : "x86"
//		},
//		"functions" : [
//			{
//				"name" : "fnc",
//				"locals" : [
//					{
//						"name" : "stack_-4",
//						"storage" : { "type" : "stack", "value" : -4 }
//					},
//					{
//						"name" : "stack_-8",
//						"storage" : { "type" : "stack", "value" : -8 }
//					}
//				]
//			}
//		]
//	})");
//	auto abi = AbiProvider::addAbi(module.get(), &config);
//
//	pass.runOnModuleCustom(*module, &config, abi);
//
//	std::string exp = R"(
//		@r = global i32 0
//		define void @fnc() {
//			%stack_-4 = alloca i32
//			%stack_-8 = alloca i32
//		br label %lab1
//		lab1:
//			store i32 123, i32* %stack_-4
//		br label %lab2
//		lab2:
//			store i32 456, i32* %stack_-8
//			%a = bitcast i32* @r to void()*
//			%1 = load i32, i32* %stack_-8
//			%2 = load i32, i32* %stack_-4
//			%3 = bitcast void ()* %a to void (i32, i32)*
//			call void %3(i32 %1, i32 %2)
//			ret void
//		}
//	)";
//	checkModuleAgainstExpectedIr(exp);
//}
//
//TEST_F(ParamReturnTests, x86PtrCallPrevBbIsNotUsedIfItIsNotASinglePredecessor)
//{
//	parseInput(R"(
//		@r = global i32 0
//		define void @fnc() {
//			%stack_-4 = alloca i32
//			%stack_-8 = alloca i32
//		br label %lab1
//		lab1:
//			store i32 123, i32* %stack_-4
//		br label %lab2
//		lab2:
//			store i32 456, i32* %stack_-8
//			%a = bitcast i32* @r to void()*
//			call void %a()
//			br label %lab2
//			ret void
//		}
//	)");
//	auto config = Config::fromJsonString(module.get(), R"({
//		"architecture" : {
//			"bitSize" : 32,
//			"endian" : "little",
//			"name" : "x86"
//		},
//		"functions" : [
//			{
//				"name" : "fnc",
//				"locals" : [
//					{
//						"name" : "stack_-4",
//						"storage" : { "type" : "stack", "value" : -4 }
//					},
//					{
//						"name" : "stack_-8",
//						"storage" : { "type" : "stack", "value" : -8 }
//					}
//				]
//			}
//		]
//	})");
//	auto abi = AbiProvider::addAbi(module.get(), &config);
//
//	pass.runOnModuleCustom(*module, &config, abi);
//
//	std::string exp = R"(
//		@r = global i32 0
//		define void @fnc() {
//			%stack_-4 = alloca i32
//			%stack_-8 = alloca i32
//		br label %lab1
//		lab1:
//			store i32 123, i32* %stack_-4
//		br label %lab2
//		lab2:
//			store i32 456, i32* %stack_-8
//			%a = bitcast i32* @r to void()*
//			%1 = load i32, i32* %stack_-8
//			%2 = bitcast void ()* %a to void (i32)*
//			call void %2(i32 %1)
//			br label %lab2
//			ret void
//		}
//	)";
//	checkModuleAgainstExpectedIr(exp);
//}
//
//TEST_F(ParamReturnTests, x86PtrCallOnlyStackStoresAreUsed)
//{
//	parseInput(R"(
//		@eax = global i32 0
//		@r = global i32 0
//		define void @fnc() {
//			%stack_-4 = alloca i32
//			%local = alloca i32
//			store i32 123, i32* %stack_-4
//			store i32 456, i32* %local
//			store i32 789, i32* @eax
//			%a = bitcast i32* @r to void()*
//			call void %a()
//			ret void
//		}
//	)");
//	auto config = Config::fromJsonString(module.get(), R"({
//		"architecture" : {
//			"bitSize" : 32,
//			"endian" : "little",
//			"name" : "x86"
//		},
//		"functions" : [
//			{
//				"name" : "fnc",
//				"locals" : [
//					{
//						"name" : "stack_-4",
//						"storage" : { "type" : "stack", "value" : -4 }
//					}
//				]
//			}
//		],
//		"registers" : [
//			{
//				"name" : "eax",
//				"storage" : { "type" : "register", "value" : "eax",
//							"registerClass" : "gpr", "registerNumber" : 0 }
//			}
//		]
//	})");
//	auto abi = AbiProvider::addAbi(module.get(), &config);
//
//	pass.runOnModuleCustom(*module, &config, abi);
//
//	std::string exp = R"(
//		@eax = global i32 0
//		@r = global i32 0
//		define void @fnc() {
//			%stack_-4 = alloca i32
//			%local = alloca i32
//			store i32 123, i32* %stack_-4
//			store i32 456, i32* %local
//			store i32 789, i32* @eax
//			%a = bitcast i32* @r to void()*
//			%1 = load i32, i32* %stack_-4
//			%2 = bitcast void ()* %a to void (i32)*
//			call void %2(i32 %1)
//			ret void
//		}
//	)";
//	checkModuleAgainstExpectedIr(exp);
//}
//
//TEST_F(ParamReturnTests, x86PtrCallStackAreUsedAsArgumentsInCorrectOrder)
//{
//	parseInput(R"(
//		@r = global i32 0
//		define void @fnc() {
//			%stack_-4 = alloca i32
//			%stack_-8 = alloca i32
//			store i32 456, i32* %stack_-8
//			store i32 123, i32* %stack_-4
//			%a = bitcast i32* @r to void()*
//			call void %a()
//			ret void
//		}
//	)");
//	auto config = Config::fromJsonString(module.get(), R"({
//		"architecture" : {
//			"bitSize" : 32,
//			"endian" : "little",
//			"name" : "x86"
//		},
//		"functions" : [
//			{
//				"name" : "fnc",
//				"locals" : [
//					{
//						"name" : "stack_-4",
//						"storage" : { "type" : "stack", "value" : -4 }
//					},
//					{
//						"name" : "stack_-8",
//						"storage" : { "type" : "stack", "value" : -8 }
//					}
//				]
//			}
//		]
//	})");
//	auto abi = AbiProvider::addAbi(module.get(), &config);
//
//	pass.runOnModuleCustom(*module, &config, abi);
//
//	std::string exp = R"(
//		@r = global i32 0
//		define void @fnc() {
//			%stack_-4 = alloca i32
//			%stack_-8 = alloca i32
//			store i32 456, i32* %stack_-8
//			store i32 123, i32* %stack_-4
//			%a = bitcast i32* @r to void()*
//			%1 = load i32, i32* %stack_-8
//			%2 = load i32, i32* %stack_-4
//			%3 = bitcast void ()* %a to void (i32, i32)*
//			call void %3(i32 %1, i32 %2)
//			ret void
//		}
//	)";
//	checkModuleAgainstExpectedIr(exp);
//}

TEST_F(ParamReturnTests, x86_64PtrCallBasicFunctionality)
{
	parseInput(R"(
		target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

		@r = global i64 0
		@rdi = global i64 0
		@rsi = global i64 0
		@rax = global i64 0
		define void @fnc() {
			store i64 123, i64* @rdi
			store i64 456, i64* @rsi
			%a = bitcast i64* @r to void()*
			call void %a()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 64,
			"endian" : "little",
			"name" : "x86"
		}
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(X86_REG_RDI, getGlobalByName("rdi"));
	abi->addRegister(X86_REG_RSI, getGlobalByName("rsi"));
	abi->addRegister(X86_REG_RAX, getGlobalByName("rax"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

		@r = global i64 0
		@rdi = global i64 0
		@rsi = global i64 0
		@rax = global i64 0

		define i64 @fnc() {
			store i64 123, i64* @rdi
			store i64 456, i64* @rsi
			%a = bitcast i64* @r to void()*
			%1 = load i64, i64* @rdi
			%2 = load i64, i64* @rsi
			%3 = bitcast void ()* %a to void (i64, i64)*
			call void %3(i64 %1, i64 %2)
			%4 = load i64, i64* @rax
			ret i64 %4
		}

		declare void @0()
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, x86_64PtrCallPrevBbIsUsedOnlyIfItIsASinglePredecessor)
{
	parseInput(R"(
		target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

		@r = global i64 0
		@rdi = global i64 0
		@rsi = global i64 0
		@rax = global i64 0

		define void @fnc() {
		br label %lab1
		lab1:
			store i64 123, i64* @rdi
		br label %lab2
		lab2:
			store i64 456, i64* @rsi
			%a = bitcast i64* @r to void()*
			call void %a()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 64,
			"endian" : "little",
			"name" : "x86"
		}
	})");

	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(X86_REG_RDI, getGlobalByName("rdi"));
	abi->addRegister(X86_REG_RSI, getGlobalByName("rsi"));
	abi->addRegister(X86_REG_RAX, getGlobalByName("rax"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

		@r = global i64 0
		@rdi = global i64 0
		@rsi = global i64 0
		@rax = global i64 0

		define i64 @fnc() {
			br label %lab1

		lab1:
			store i64 123, i64* @rdi
			br label %lab2

		lab2:
			store i64 456, i64* @rsi
			%a = bitcast i64* @r to void ()*
			%1 = load i64, i64* @rdi
			%2 = load i64, i64* @rsi
			%3 = bitcast void ()* %a to void (i64, i64)*
			call void %3(i64 %1, i64 %2)
			%4 = load i64, i64* @rax
			ret i64 %4
		}

		declare void @0()
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, x86_64ExternalCallUseStacksIf6RegistersUsed)
{
	parseInput(R"(
		target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

		@rdi = global i64 0
		@rsi = global i64 0
		@rcx = global i64 0
		@rdx = global i64 0
		@r8 = global i64 0
		@r9 = global i64 0
		@r10 = global i64 0
		@rax = global i64 0
		declare void @print()
		define void @fnc() {
			store i64 1, i64* @rdi
			%stack_-8 = alloca i64
			%stack_-16 = alloca i64
			store i64 1, i64* @r9
			store i64 2, i64* @r10
			store i64 1, i64* @r8
			store i64 1, i64* @rsi
			store i64 2, i64* %stack_-8
			store i64 1, i64* @rdx
			store i64 2, i64* %stack_-16
			store i64 1, i64* @rcx
			call void @print()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 64,
			"endian" : "little",
			"name" : "x86"
		},
		"functions" : [
			{
				"name" : "fnc",
				"locals" : [
					{
						"name" : "stack_-8",
						"storage" : { "type" : "stack", "value" : -8 }
					},
					{
						"name" : "stack_-16",
						"storage" : { "type" : "stack", "value" : -16 }
					}

				]
			}
		]
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(X86_REG_RAX, getGlobalByName("rax"));
	abi->addRegister(X86_REG_RDI, getGlobalByName("rdi"));
	abi->addRegister(X86_REG_RSI, getGlobalByName("rsi"));
	abi->addRegister(X86_REG_RCX, getGlobalByName("rcx"));
	abi->addRegister(X86_REG_RDX, getGlobalByName("rdx"));
	abi->addRegister(X86_REG_R8, getGlobalByName("r8"));
	abi->addRegister(X86_REG_R9, getGlobalByName("r9"));
	abi->addRegister(X86_REG_R10, getGlobalByName("r10"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

		@rdi = global i64 0
		@rsi = global i64 0
		@rcx = global i64 0
		@rdx = global i64 0
		@r8 = global i64 0
		@r9 = global i64 0
		@r10 = global i64 0
		@rax = global i64 0

		declare i64 @print(i64, i64, i64, i64, i64, i64, i64, i64)

		declare void @0()

		define i64 @fnc() {
			store i64 1, i64* @rdi
			%stack_-8 = alloca i64
			%stack_-16 = alloca i64
			store i64 1, i64* @r9
			store i64 2, i64* @r10
			store i64 1, i64* @r8
			store i64 1, i64* @rsi
			store i64 2, i64* %stack_-8
			store i64 1, i64* @rdx
			store i64 2, i64* %stack_-16
			store i64 1, i64* @rcx
			%1 = load i64, i64* @rdi
			%2 = load i64, i64* @rsi
			%3 = load i64, i64* @rdx
			%4 = load i64, i64* @rcx
			%5 = load i64, i64* @r8
			%6 = load i64, i64* @r9
			%7 = load i64, i64* %stack_-16
			%8 = load i64, i64* %stack_-8
			%9 = call i64 @print(i64 %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, i64 %7, i64 %8)
			store i64 %9, i64* @rax
			%10 = load i64, i64* @rax
			ret i64 %10
		}

		declare void @1()
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, x86_64ExternalCallUsesFPRegisters)
{
	parseInput(R"(
		target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

		@rdi = global i64 0
		@rsi = global i64 0
		@rcx = global i64 0
		@rdx = global i64 0
		@r8 = global i64 0
		@r9 = global i64 0
		@r10 = global i64 0
		@rax = global i64 0
		@xmm0 = global double 0.0
		@xmm1 = global double 0.0

		declare void @print()
		define void @fnc() {
			store i64 1, i64* @rdi
			store i64 1, i64* @r9
			store i64 2, i64* @r10
			store i64 1, i64* @r8
			store i64 1, i64* @rsi
			store double 2.0, double* @xmm1
			store i64 1, i64* @rdx
			store double 2.0, double* @xmm0
			store i64 1, i64* @rcx
			call void @print()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 64,
			"endian" : "little",
			"name" : "x86"
		}
	})");

	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(X86_REG_RAX, getGlobalByName("rax"));
	abi->addRegister(X86_REG_RDI, getGlobalByName("rdi"));
	abi->addRegister(X86_REG_RSI, getGlobalByName("rsi"));
	abi->addRegister(X86_REG_RCX, getGlobalByName("rcx"));
	abi->addRegister(X86_REG_RDX, getGlobalByName("rdx"));
	abi->addRegister(X86_REG_R8, getGlobalByName("r8"));
	abi->addRegister(X86_REG_R9, getGlobalByName("r9"));
	abi->addRegister(X86_REG_R10, getGlobalByName("r10"));
	abi->addRegister(X86_REG_XMM0, getGlobalByName("xmm0"));
	abi->addRegister(X86_REG_XMM1, getGlobalByName("xmm1"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
	target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

	@rdi = global i64 0
	@rsi = global i64 0
	@rcx = global i64 0
	@rdx = global i64 0
	@r8 = global i64 0
	@r9 = global i64 0
	@r10 = global i64 0
	@rax = global i64 0
	@xmm0 = global double 0.000000e+00
	@xmm1 = global double 0.000000e+00

	declare i64 @print(i64, i64, i64, i64, i64, i64, float, float)

	declare void @0()

	define i64 @fnc() {
		store i64 1, i64* @rdi
		store i64 1, i64* @r9
		store i64 2, i64* @r10
		store i64 1, i64* @r8
		store i64 1, i64* @rsi
		store double 2.000000e+00, double* @xmm1
		store i64 1, i64* @rdx
		store double 2.000000e+00, double* @xmm0
		store i64 1, i64* @rcx
		%1 = load i64, i64* @rdi
		%2 = load i64, i64* @rsi
		%3 = load i64, i64* @rdx
		%4 = load i64, i64* @rcx
		%5 = load i64, i64* @r8
		%6 = load i64, i64* @r9
		%7 = load double, double* @xmm0
		%8 = load double, double* @xmm1
		%9 = fptrunc double %7 to float
		%10 = fptrunc double %8 to float
		%11 = call i64 @print(i64 %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, float %9, float %10)
		store i64 %11, i64* @rax
		%12 = load i64, i64* @rax
		ret i64 %12
	}

	declare void @1()

	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, x86_64UsesJustContinuousSequenceOfRegisters)
{
	parseInput(R"(
		target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

		@rax = global i64 0
		@rdi = global i64 0
		@rsi = global i64 0
		@rcx = global i64 0
		@rdx = global i64 0

		declare void @print()
		define void @fnc() {
			store i64 1, i64* @rdi
			store i64 1, i64* @rdx
			store i64 1, i64* @rcx
			call void @print()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 64,
			"endian" : "little",
			"name" : "x86"
		}
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(X86_REG_RAX, getGlobalByName("rax"));
	abi->addRegister(X86_REG_RDI, getGlobalByName("rdi"));
	abi->addRegister(X86_REG_RSI, getGlobalByName("rsi"));
	abi->addRegister(X86_REG_RCX, getGlobalByName("rcx"));
	abi->addRegister(X86_REG_RDX, getGlobalByName("rdx"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

		@rax = global i64 0
		@rdi = global i64 0
		@rsi = global i64 0
		@rcx = global i64 0
		@rdx = global i64 0

		declare i64 @print(i64)

		declare void @0()

		define i64 @fnc() {
			store i64 1, i64* @rdi
			store i64 1, i64* @rdx
			store i64 1, i64* @rcx
			%1 = load i64, i64* @rdi
			%2 = call i64 @print(i64 %1)
			store i64 %2, i64* @rax
			%3 = load i64, i64* @rax
			ret i64 %3
		}

		declare void @1()
	)";
	checkModuleAgainstExpectedIr(exp);
}

//
//TEST_F(ParamReturnTests, x86PtrCallOnlyContinuousStackOffsetsAreUsed)
//{
//	parseInput(R"(
//		@r = global i32 0
//		define void @fnc() {
//			%stack_-4 = alloca i32
//			%stack_-16 = alloca i32
//			%stack_-20 = alloca i32
//			%stack_-24 = alloca i32
//			store i32 1, i32* %stack_-16
//			store i32 2, i32* %stack_-20
//			store i32 3, i32* %stack_-24
//			store i32 4, i32* %stack_-4
//			%a = bitcast i32* @r to void()*
//			call void %a()
//			ret void
//		}
//	)");
//	auto config = Config::fromJsonString(module.get(), R"({
//		"architecture" : {
//			"bitSize" : 32,
//			"endian" : "little",
//			"name" : "x86"
//		},
//		"functions" : [
//			{
//				"name" : "fnc",
//				"locals" : [
//					{
//						"name" : "stack_-4",
//						"storage" : { "type" : "stack", "value" : -4 }
//					},
//					{
//						"name" : "stack_-16",
//						"storage" : { "type" : "stack", "value" : -16 }
//					},
//					{
//						"name" : "stack_-20",
//						"storage" : { "type" : "stack", "value" : -20 }
//					},
//					{
//						"name" : "stack_-24",
//						"storage" : { "type" : "stack", "value" : -24 }
//					}
//				]
//			}
//		]
//	})");
//	auto abi = AbiProvider::addAbi(module.get(), &config);
//

TEST_F(ParamReturnTests, ppcPtrCallBasicFunctionality)
{
	parseInput(R"(
		@r = global i32 0
		@r3 = global i32 0
		@r4 = global i32 0
		define void @fnc() {
			store i32 123, i32* @r3
			store i32 456, i32* @r4
			%a = bitcast i32* @r to void()*
			call void %a()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "big",
			"name" : "powerpc"
		}
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(PPC_REG_R3, getGlobalByName("r3"));
	abi->addRegister(PPC_REG_R4, getGlobalByName("r4"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r = global i32 0
		@r3 = global i32 0
		@r4 = global i32 0

		define i32 @fnc() {
			store i32 123, i32* @r3
			store i32 456, i32* @r4
			%a = bitcast i32* @r to void ()*
			%1 = load i32, i32* @r3
			%2 = load i32, i32* @r4
			%3 = bitcast void ()* %a to void (i32, i32)*
			call void %3(i32 %1, i32 %2)
			%4 = load i32, i32* @r3
			ret i32 %4
		}

		declare void @0()
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, ppcExternalCallBasicFunctionality)
{
	parseInput(R"(
		@r3 = global i32 0
		@r4 = global i32 0
		declare void @print()
		define void @fnc() {
			store i32 123, i32* @r3
			store i32 456, i32* @r4
			call void @print()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "big",
			"name" : "powerpc"
		}
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(PPC_REG_R3, getGlobalByName("r3"));
	abi->addRegister(PPC_REG_R4, getGlobalByName("r4"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r3 = global i32 0
		@r4 = global i32 0

		declare i32 @print(i32, i32)
		declare void @0()

		define i32 @fnc() {
			store i32 123, i32* @r3
			store i32 456, i32* @r4
			%1 = load i32, i32* @r3
			%2 = load i32, i32* @r4
			%3 = call i32 @print(i32 %1, i32 %2)
			store i32 %3, i32* @r3
			%4 = load i32, i32* @r3
			ret i32 %4
		}

		declare void @1()
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, ppcExternalCallDoNotUseObjectsIfTheyAreNotRegisters)
{
	parseInput(R"(
		@r3 = global i32 0
		declare void @print()
		define void @fnc() {
			store i32 123, i32* @r3
			call void @print()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "big",
			"name" : "powerpc"
		}
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r3 = global i32 0
		declare void @print()
		define void @fnc() {
			store i32 123, i32* @r3
			call void @print()
			ret void
		}
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, ppcExternalCallFilterRegistersOnMultiplePlaces)
{
	parseInput(R"(
		@r3 = global i32 0
		@r4 = global i32 0
		@r5 = global i32 0
		declare void @print()
		define void @fnc1() {
			store i32 123, i32* @r3
			store i32 456, i32* @r4
			call void @print()
			ret void
		}
		define void @fnc2() {
			store i32 123, i32* @r3
			store i32 456, i32* @r5
			call void @print()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "big",
			"name" : "powerpc"
		}
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);
	abi->addRegister(PPC_REG_R3, getGlobalByName("r3"));
	abi->addRegister(PPC_REG_R4, getGlobalByName("r4"));
	abi->addRegister(PPC_REG_R5, getGlobalByName("r5"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r3 = global i32 0
		@r4 = global i32 0
		@r5 = global i32 0

		declare i32 @print(i32)

		declare void @0()

		define i32 @fnc1() {
			store i32 123, i32* @r3
			store i32 456, i32* @r4
			%1 = load i32, i32* @r3
			%2 = call i32 @print(i32 %1)
			store i32 %2, i32* @r3
			%3 = load i32, i32* @r3
			ret i32 %3
		}

		declare void @1()

		define i32 @fnc2() {
			store i32 123, i32* @r3
			store i32 456, i32* @r5
			%1 = load i32, i32* @r3
			%2 = call i32 @print(i32 %1)
			store i32 %2, i32* @r3
			%3 = load i32, i32* @r3
			ret i32 %3
		}

		declare void @2()
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, ppcExternalCallDoNotUseAllRegisters)
{
	parseInput(R"(
		@r1 = global i32 0
		@r2 = global i32 0
		@r3 = global i32 0
		declare void @print()
		define void @fnc() {
			store i32 123, i32* @r1
			store i32 456, i32* @r3
			store i32 789, i32* @r2
			call void @print()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "big",
			"name" : "powerpc"
		}
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(PPC_REG_R1, getGlobalByName("r1"));
	abi->addRegister(PPC_REG_R2, getGlobalByName("r2"));
	abi->addRegister(PPC_REG_R3, getGlobalByName("r3"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r1 = global i32 0
		@r2 = global i32 0
		@r3 = global i32 0

		declare i32 @print(i32)

		declare void @0()

		define i32 @fnc() {
			store i32 123, i32* @r1
			store i32 456, i32* @r3
			store i32 789, i32* @r2
			%1 = load i32, i32* @r3
			%2 = call i32 @print(i32 %1)
			store i32 %2, i32* @r3
			%3 = load i32, i32* @r3
			ret i32 %3
		}

		declare void @1()
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, ppcExternalCallSortRegistersIntoCorrectOrder)
{
	parseInput(R"(
		@r3 = global i32 0
		@r4 = global i32 0
		@r5 = global i32 0
		declare void @print()
		define void @fnc() {
			store i32 123, i32* @r5
			store i32 456, i32* @r3
			store i32 789, i32* @r4
			call void @print()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "big",
			"name" : "powerpc"
		}
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(PPC_REG_R3, getGlobalByName("r3"));
	abi->addRegister(PPC_REG_R4, getGlobalByName("r4"));
	abi->addRegister(PPC_REG_R5, getGlobalByName("r5"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r3 = global i32 0
		@r4 = global i32 0
		@r5 = global i32 0

		declare i32 @print(i32, i32, i32)

		declare void @0()

		define i32 @fnc() {
			store i32 123, i32* @r5
			store i32 456, i32* @r3
			store i32 789, i32* @r4
			%1 = load i32, i32* @r3
			%2 = load i32, i32* @r4
			%3 = load i32, i32* @r5
			%4 = call i32 @print(i32 %1, i32 %2, i32 %3)
			store i32 %4, i32* @r3
			%5 = load i32, i32* @r3
			ret i32 %5
		}

		declare void @1()
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, ppcExternalCallDoNotUseStacksIfLessThan7RegistersUsed)
{
	parseInput(R"(
		@r3 = global i32 0
		declare void @print()
		define void @fnc() {
			%stack_-4 = alloca i32
			store i32 123, i32* @r3
			store i32 456, i32* %stack_-4
			call void @print()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "big",
			"name" : "powerpc"
		},
		"functions" : [
			{
				"name" : "fnc",
				"locals" : [
					{
						"name" : "stack_-4",
						"storage" : { "type" : "stack", "value" : -4 }
					}
				]
			}
		]
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(PPC_REG_R3, getGlobalByName("r3"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r3 = global i32 0

		declare i32 @print(i32)

		declare void @0()

		define i32 @fnc() {
			%stack_-4 = alloca i32
			store i32 123, i32* @r3
			store i32 456, i32* %stack_-4
			%1 = load i32, i32* @r3
			%2 = call i32 @print(i32 %1)
			store i32 %2, i32* @r3
			%3 = load i32, i32* @r3
			ret i32 %3
		}

		declare void @1()
	)";
	checkModuleAgainstExpectedIr(exp);
}

//
//	std::string exp = R"(
//		@r = global i32 0
//		define void @fnc() {
//			%stack_-4 = alloca i32
//			%stack_-16 = alloca i32
//			%stack_-20 = alloca i32
//			%stack_-24 = alloca i32
//			store i32 1, i32* %stack_-16
//			store i32 2, i32* %stack_-20
//			store i32 3, i32* %stack_-24
//			store i32 4, i32* %stack_-4
//			%a = bitcast i32* @r to void()*
//			%1 = load i32, i32* %stack_-24
//			%2 = load i32, i32* %stack_-20
//			%3 = load i32, i32* %stack_-16
//			%4 = bitcast void ()* %a to void (i32, i32, i32)*
//			call void %4(i32 %1, i32 %2, i32 %3)
//			ret void
//		}
//	)";
//	checkModuleAgainstExpectedIr(exp);
//}
//

TEST_F(ParamReturnTests, armPtrCallBasicFunctionality)
{
	parseInput(R"(
		@r = global i32 0
		@r0 = global i32 0
		@r1 = global i32 0
		define void @fnc() {
			store i32 123, i32* @r0
			store i32 456, i32* @r1
			%a = bitcast i32* @r to void()*
			call void %a()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "little",
			"name" : "arm"
		}
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(ARM_REG_R0, getGlobalByName("r0"));
	abi->addRegister(ARM_REG_R1, getGlobalByName("r1"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r = global i32 0
		@r0 = global i32 0
		@r1 = global i32 0

		define i32 @fnc() {
			store i32 123, i32* @r0
			store i32 456, i32* @r1
			%a = bitcast i32* @r to void ()*
			%1 = load i32, i32* @r0
			%2 = load i32, i32* @r1
			%3 = bitcast void ()* %a to void (i32, i32)*
			call void %3(i32 %1, i32 %2)
			%4 = load i32, i32* @r0
			ret i32 %4
		}

		declare void @0()
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, armExternalCallBasicFunctionality)
{
	parseInput(R"(
		@r0 = global i32 0
		@r1 = global i32 0
		declare void @print()
		define void @fnc() {
			store i32 123, i32* @r0
			store i32 456, i32* @r1
			call void @print()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "little",
			"name" : "arm"
		}
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(ARM_REG_R0, getGlobalByName("r0"));
	abi->addRegister(ARM_REG_R1, getGlobalByName("r1"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r0 = global i32 0
		@r1 = global i32 0

		declare i32 @print(i32, i32)

		declare void @0()

		define i32 @fnc() {
			store i32 123, i32* @r0
			store i32 456, i32* @r1
			%1 = load i32, i32* @r0
			%2 = load i32, i32* @r1
			%3 = call i32 @print(i32 %1, i32 %2)
			store i32 %3, i32* @r0
			%4 = load i32, i32* @r0
			ret i32 %4
		}

		declare void @1()

	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, armExternalCallUseStacksIf4RegistersUsed)
{
	parseInput(R"(
		@r0 = global i32 0
		@r1 = global i32 0
		@r2 = global i32 0
		@r3 = global i32 0
		@r4 = global i32 0
		declare void @print()
		define void @fnc() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
			store i32 1, i32* @r2
			store i32 1, i32* @r1
			store i32 2, i32* %stack_-4
			store i32 1, i32* @r4
			store i32 1, i32* @r0
			store i32 2, i32* %stack_-8
			store i32 1, i32* @r3
			call void @print()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "little",
			"name" : "arm"
		},
		"functions" : [
			{
				"name" : "fnc",
				"locals" : [
					{
						"name" : "stack_-4",
						"storage" : { "type" : "stack", "value" : -4 }
					},
					{
						"name" : "stack_-8",
						"storage" : { "type" : "stack", "value" : -8 }
					}
				]
			}
		]
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(ARM_REG_R0, getGlobalByName("r0"));
	abi->addRegister(ARM_REG_R1, getGlobalByName("r1"));
	abi->addRegister(ARM_REG_R2, getGlobalByName("r2"));
	abi->addRegister(ARM_REG_R3, getGlobalByName("r3"));
	abi->addRegister(ARM_REG_R4, getGlobalByName("r4"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r0 = global i32 0
		@r1 = global i32 0
		@r2 = global i32 0
		@r3 = global i32 0
		@r4 = global i32 0

		declare i32 @print(i32, i32, i32, i32, i32, i32)

		declare void @0()

		define i32 @fnc() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
			store i32 1, i32* @r2
			store i32 1, i32* @r1
			store i32 2, i32* %stack_-4
			store i32 1, i32* @r4
			store i32 1, i32* @r0
			store i32 2, i32* %stack_-8
			store i32 1, i32* @r3
			%1 = load i32, i32* @r0
			%2 = load i32, i32* @r1
			%3 = load i32, i32* @r2
			%4 = load i32, i32* @r3
			%5 = load i32, i32* %stack_-8
			%6 = load i32, i32* %stack_-4
			%7 = call i32 @print(i32 %1, i32 %2, i32 %3, i32 %4, i32 %5, i32 %6)
			store i32 %7, i32* @r0
			%8 = load i32, i32* @r0
			ret i32 %8
		}

		declare void @1()
	)";
	checkModuleAgainstExpectedIr(exp);
}

//
//TEST_F(ParamReturnTests, ppcExternalCallUseStacksIf7RegistersUsed)
//{
//	parseInput(R"(
//		@r3 = global i32 0
//		@r4 = global i32 0
//		@r5 = global i32 0
//		@r6 = global i32 0
//		@r7 = global i32 0
//		@r8 = global i32 0
//		@r9 = global i32 0
//		@r10 = global i32 0
//		declare void @print()
//		define void @fnc() {
//			%stack_-4 = alloca i32
//			%stack_-8 = alloca i32
//			store i32 1, i32* @r3
//			store i32 1, i32* @r4
//			store i32 1, i32* @r5
//			store i32 2, i32* %stack_-4
//			store i32 1, i32* @r6
//			store i32 1, i32* @r7
//			store i32 1, i32* @r8
//			store i32 2, i32* %stack_-8
//			store i32 1, i32* @r9
//			store i32 1, i32* @r10
//			call void @print()
//			ret void
//		}
//	)");
//	auto config = Config::fromJsonString(module.get(), R"({
//		"architecture" : {
//			"bitSize" : 32,
//			"endian" : "big",
//			"name" : "powerpc"
//		},
//		"functions" : [
//			{
//				"name" : "fnc",
//				"locals" : [
//					{
//						"name" : "stack_-4",
//						"storage" : { "type" : "stack", "value" : -4 }
//					},
//					{
//						"name" : "stack_-8",
//						"storage" : { "type" : "stack", "value" : -8 }
//					}
//				]
//			}
//		],
//		"registers" : [
//			{
//				"name" : "r3",
//				"storage" : { "type" : "register", "value" : "r3",
//							"registerClass" : "gpregs", "registerNumber" : 3 }
//			},
//			{
//				"name" : "r4",
//				"storage" : { "type" : "register", "value" : "r4",
//							"registerClass" : "gpregs", "registerNumber" : 4 }
//			},
//			{
//				"name" : "r5",
//				"storage" : { "type" : "register", "value" : "r5",
//							"registerClass" : "gpregs", "registerNumber" : 5 }
//			},
//			{
//				"name" : "r6",
//				"storage" : { "type" : "register", "value" : "r6",
//							"registerClass" : "gpregs", "registerNumber" : 6 }
//			},
//			{
//				"name" : "r7",
//				"storage" : { "type" : "register", "value" : "r7",
//							"registerClass" : "gpregs", "registerNumber" : 7 }
//			},
//			{
//				"name" : "r8",
//				"storage" : { "type" : "register", "value" : "r8",
//							"registerClass" : "gpregs", "registerNumber" : 8 }
//			},
//			{
//				"name" : "r9",
//				"storage" : { "type" : "register", "value" : "r9",
//							"registerClass" : "gpregs", "registerNumber" : 9 }
//			},
//			{
//				"name" : "r10",
//				"storage" : { "type" : "register", "value" : "r10",
//							"registerClass" : "gpregs", "registerNumber" : 10 }
//			}
//		]
//	})");
//	auto abi = AbiProvider::addAbi(module.get(), &config);
//

TEST_F(ParamReturnTests, mipsPtrCallBasicFunctionality)
{
	parseInput(R"(
		@r = global i32 0
		@a0 = global i32 0
		@a1 = global i32 0
		define void @fnc() {
			store i32 123, i32* @a0
			store i32 456, i32* @a1
			%a = bitcast i32* @r to void()*
			call void %a()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "little",
			"name" : "mips"
		}
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(MIPS_REG_A0, getGlobalByName("a0"));
	abi->addRegister(MIPS_REG_A1, getGlobalByName("a1"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@r = global i32 0
		@a0 = global i32 0
		@a1 = global i32 0
		define void @fnc() {
			store i32 123, i32* @a0
			store i32 456, i32* @a1
			%a = bitcast i32* @r to void()*
			%1 = load i32, i32* @a0
			%2 = load i32, i32* @a1
			%3 = bitcast void ()* %a to void (i32, i32)*
			call void %3(i32 %1, i32 %2)
			ret void
		}
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, mipsExternalCallBasicFunctionality)
{
	parseInput(R"(
		@a0 = global i32 0
		@a1 = global i32 0
		declare void @print()
		define void @fnc() {
			store i32 123, i32* @a0
			store i32 456, i32* @a1
			call void @print()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "little",
			"name" : "mips"
		}
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(MIPS_REG_A0, getGlobalByName("a0"));
	abi->addRegister(MIPS_REG_A1, getGlobalByName("a1"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@a0 = global i32 0
		@a1 = global i32 0
		declare void @print(i32, i32)
		declare void @0()
		define void @fnc() {
			store i32 123, i32* @a0
			store i32 456, i32* @a1
			%1 = load i32, i32* @a0
			%2 = load i32, i32* @a1
			call void @print(i32 %1, i32 %2)
			ret void
		}
	)";
	checkModuleAgainstExpectedIr(exp);
}

TEST_F(ParamReturnTests, mipsExternalCallUseStacksIf4RegistersUsed)
{
	parseInput(R"(
		@a0 = global i32 0
		@a1 = global i32 0
		@a2 = global i32 0
		@a3 = global i32 0
		@t0 = global i32 0
		declare void @print()
		define void @fnc() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
			store i32 1, i32* @a2
			store i32 1, i32* @a1
			store i32 2, i32* %stack_-4
			store i32 1, i32* @t0
			store i32 1, i32* @a0
			store i32 2, i32* %stack_-8
			store i32 1, i32* @a3
			call void @print()
			ret void
		}
	)");
	auto config = Config::fromJsonString(module.get(), R"({
		"architecture" : {
			"bitSize" : 32,
			"endian" : "little",
			"name" : "mips"
		},
		"functions" : [
			{
				"name" : "fnc",
				"locals" : [
					{
						"name" : "stack_-4",
						"storage" : { "type" : "stack", "value" : -4 }
					},
					{
						"name" : "stack_-8",
						"storage" : { "type" : "stack", "value" : -8 }
					}
				]
			}
		]
	})");
	auto abi = AbiProvider::addAbi(module.get(), &config);

	abi->addRegister(MIPS_REG_A0, getGlobalByName("a0"));
	abi->addRegister(MIPS_REG_A1, getGlobalByName("a1"));
	abi->addRegister(MIPS_REG_A2, getGlobalByName("a2"));
	abi->addRegister(MIPS_REG_A3, getGlobalByName("a3"));
	abi->addRegister(MIPS_REG_T0, getGlobalByName("t0"));

	pass.runOnModuleCustom(*module, &config, abi);

	std::string exp = R"(
		@a0 = global i32 0
		@a1 = global i32 0
		@a2 = global i32 0
		@a3 = global i32 0
		@t0 = global i32 0
		declare void @print(i32, i32, i32, i32, i32, i32)
		declare void @0()
		define void @fnc() {
			%stack_-4 = alloca i32
			%stack_-8 = alloca i32
			store i32 1, i32* @a2
			store i32 1, i32* @a1
			store i32 2, i32* %stack_-4
			store i32 1, i32* @t0
			store i32 1, i32* @a0
			store i32 2, i32* %stack_-8
			store i32 1, i32* @a3
			%1 = load i32, i32* @a0
			%2 = load i32, i32* @a1
			%3 = load i32, i32* @a2
			%4 = load i32, i32* @a3
			%5 = load i32, i32* %stack_-8
			%6 = load i32, i32* %stack_-4
			call void @print(i32 %1, i32 %2, i32 %3, i32 %4, i32 %5, i32 %6)
			ret void
		}
	)";
	checkModuleAgainstExpectedIr(exp);
}

} // namespace tests
} // namespace bin2llvmir
} // namespace retdec
