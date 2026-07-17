// OpCode.h
// T# v2.0.0-beta1
// Dylan Armstrong, 2026

#pragma once

#include <cstdint>

namespace tsharp {

// OpCode enum
// Contains all VM opcodes
enum class OpCode : uint8_t {
	// Constants / literals
	Constant,

	Null,
	True,
	False,

	// Stack operations
	Pop,
	Duplicate,

	// Local variables
	LoadLocal,
	StoreLocal,

	IncrementLocal,
	DecrementLocal,

	IncrementValue,
	DecrementValue,

	// Global variables
	LoadGlobal,
	StoreGlobal,

	// Object / instance access
	LoadThis,

	NewObject,

	LoadField,
	StoreField,
	IncrementField,
	DecrementField,
	StoreFieldDynamic,

	LoadMember,
	GetProperty,

	// Arrays
	NewArray,
	ArrayLiteral,

	LoadIndex,
	StoreIndex,

	ArrayLength,

	// Arithmetic
	Add,
	Subtract,
	Multiply,
	Divide,
	Modulo,

	AddInt,
	AddFloat,
	AddDouble,
	AddString,

	SubtractInt,
	SubtractFloat,
	SubtractDouble,

	MultiplyInt,
	MultiplyFloat,
	MultiplyDouble,

	DivideInt,
	DivideFloat,
	DivideDouble,

	ModuloInt,

	Negate,
	NegateInt,
	NegateFloat,
	NegateDouble,

	AddLong,
	SubtractLong,
	MultiplyLong,
	DivideLong,
	ModuloLong,

	LessLong,
	LessEqualLong,
	GreaterLong,
	GreaterEqualLong,

	NegateLong,

	IncrementLong,
	DecrementLong,

	// Comparison
	Equal,
	NotEqual,

	Less,
	LessEqual,
	Greater,
	GreaterEqual,

	LessInt,
	LessEqualInt,
	GreaterInt,
	GreaterEqualInt,

	LessFloat,
	LessEqualFloat,
	GreaterFloat,
	GreaterEqualFloat,

	LessDouble,
	LessEqualDouble,
	GreaterDouble,
	GreaterEqualDouble,

	// Boolean / logical operations
	LogicalAnd,
	LogicalOr,
	LogicalNot,

	// Type operations
	Cast,

	ToInt,
	ToFloat,
	ToDouble,
	ToString,
	ToBool,
	ToChar,

	TypeOf,

	// Control flow
	Jump,
	JumpIfFalse,
	JumpIfTrue,

	Loop,

	// Functions
	Call,
	CallNative,
	CallValue,

	Return,

	// Methods / constructors
	CallMethod,
	CallVirtual,
	CallConstructor,

	// Enums
	LoadEnumValue,

	// Exceptions
	PushExceptionHandler,
	PopExceptionHandler,

	Throw,

	// Switch
	Switch,

	// Built-in IO
	Print,
	PrintLine,
	LessLocalLongConstant,
	LessLocalIntConstant,

	// Program control
	AddLocalLong,
	AddLocalInt,
	Halt
};

}