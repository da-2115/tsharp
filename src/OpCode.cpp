// OpCode.cpp
// T# v2.1.1
// Dylan Armstrong, 2026

#include "OpCode.h"

namespace tsharp {

const char* opcode_name(OpCode opcode) {
	switch (opcode) {
		// Constants
		case OpCode::Constant:
			return "CONSTANT";
		case OpCode::Null:
			return "NULL";
		case OpCode::True:
			return "TRUE";
		case OpCode::False:
			return "FALSE";

		// Stack
		case OpCode::Pop:
			return "POP";
		case OpCode::Duplicate:
			return "DUPLICATE";

		// Locals
		case OpCode::LoadLocal:
			return "LOAD_LOCAL";
		case OpCode::StoreLocal:
			return "STORE_LOCAL";
		case OpCode::IncrementLocal:
			return "INCREMENT_LOCAL";
		case OpCode::DecrementLocal:
			return "DECREMENT_LOCAL";
		case OpCode::IncrementValue:
			return "INCREMENT_VALUE";
		case OpCode::DecrementValue:
			return "DECREMENT_VALUE";

		// Globals
		case OpCode::LoadGlobal:
			return "LOAD_GLOBAL";
		case OpCode::StoreGlobal:
			return "STORE_GLOBAL";

		// Objects / fields
		case OpCode::LoadThis:
			return "LOAD_THIS";
		case OpCode::NewObject:
			return "NEW_OBJECT";
		case OpCode::LoadField:
			return "LOAD_FIELD";
		case OpCode::StoreField:
			return "STORE_FIELD";
		case OpCode::IncrementField:
			return "INCREMENT_FIELD";
		case OpCode::DecrementField:
			return "DECREMENT_FIELD";
		case OpCode::StoreFieldDynamic:
			return "STORE_FIELD_DYNAMIC";
		case OpCode::LoadMember:
			return "LOAD_MEMBER";
		case OpCode::GetProperty:
			return "GET_PROPERTY";

		// Arrays
		case OpCode::NewArray:
			return "NEW_ARRAY";
		case OpCode::ArrayLiteral:
			return "ARRAY_LITERAL";
		case OpCode::LoadIndex:
			return "LOAD_INDEX";
		case OpCode::StoreIndex:
			return "STORE_INDEX";
		case OpCode::ArrayLength:
			return "ARRAY_LENGTH";
		case OpCode::SetArrayType:
			return "SET_ARRAY_TYPE";

		// Generic arithmetic
		case OpCode::Add:
			return "ADD";
		case OpCode::Subtract:
			return "SUBTRACT";
		case OpCode::Multiply:
			return "MULTIPLY";
		case OpCode::Divide:
			return "DIVIDE";
		case OpCode::Modulo:
			return "MODULO";

		// Typed addition
		case OpCode::AddInt:
			return "ADD_INT";
		case OpCode::AddFloat:
			return "ADD_FLOAT";
		case OpCode::AddDouble:
			return "ADD_DOUBLE";
		case OpCode::AddString:
			return "ADD_STRING";

		// Typed subtraction
		case OpCode::SubtractInt:
			return "SUBTRACT_INT";
		case OpCode::SubtractFloat:
			return "SUBTRACT_FLOAT";
		case OpCode::SubtractDouble:
			return "SUBTRACT_DOUBLE";

		// Typed multiplication
		case OpCode::MultiplyInt:
			return "MULTIPLY_INT";
		case OpCode::MultiplyFloat:
			return "MULTIPLY_FLOAT";
		case OpCode::MultiplyDouble:
			return "MULTIPLY_DOUBLE";

		// Typed division
		case OpCode::DivideInt:
			return "DIVIDE_INT";
		case OpCode::DivideFloat:
			return "DIVIDE_FLOAT";
		case OpCode::DivideDouble:
			return "DIVIDE_DOUBLE";

		// Typed modulo
		case OpCode::ModuloInt:
			return "MODULO_INT";

		// Unary
		case OpCode::Negate:
			return "NEGATE";
		case OpCode::NegateInt:
			return "NEGATE_INT";
		case OpCode::NegateFloat:
			return "NEGATE_FLOAT";
		case OpCode::NegateDouble:
			return "NEGATE_DOUBLE";

		// Long arithmetic
		case OpCode::AddLong:
			return "ADD_LONG";
		case OpCode::SubtractLong:
			return "SUBTRACT_LONG";
		case OpCode::MultiplyLong:
			return "MULTIPLY_LONG";
		case OpCode::DivideLong:
			return "DIVIDE_LONG";
		case OpCode::ModuloLong:
			return "MODULO_LONG";

		// Long comparisons
		case OpCode::LessLong:
			return "LESS_LONG";
		case OpCode::LessEqualLong:
			return "LESS_EQUAL_LONG";
		case OpCode::GreaterLong:
			return "GREATER_LONG";
		case OpCode::GreaterEqualLong:
			return "GREATER_EQUAL_LONG";

		// Long unary / mutation
		case OpCode::NegateLong:
			return "NEGATE_LONG";
		case OpCode::IncrementLong:
			return "INCREMENT_LONG";
		case OpCode::DecrementLong:
			return "DECREMENT_LONG";

		// Equality
		case OpCode::Equal:
			return "EQUAL";
		case OpCode::NotEqual:
			return "NOT_EQUAL";

		// Generic comparisons
		case OpCode::Less:
			return "LESS";
		case OpCode::LessEqual:
			return "LESS_EQUAL";
		case OpCode::Greater:
			return "GREATER";
		case OpCode::GreaterEqual:
			return "GREATER_EQUAL";

		// Integer comparisons
		case OpCode::LessInt:
			return "LESS_INT";
		case OpCode::LessEqualInt:
			return "LESS_EQUAL_INT";
		case OpCode::GreaterInt:
			return "GREATER_INT";
		case OpCode::GreaterEqualInt:
			return "GREATER_EQUAL_INT";

		// Float comparisons
		case OpCode::LessFloat:
			return "LESS_FLOAT";
		case OpCode::LessEqualFloat:
			return "LESS_EQUAL_FLOAT";
		case OpCode::GreaterFloat:
			return "GREATER_FLOAT";
		case OpCode::GreaterEqualFloat:
			return "GREATER_EQUAL_FLOAT";

		// Double comparisons
		case OpCode::LessDouble:
			return "LESS_DOUBLE";
		case OpCode::LessEqualDouble:
			return "LESS_EQUAL_DOUBLE";
		case OpCode::GreaterDouble:
			return "GREATER_DOUBLE";
		case OpCode::GreaterEqualDouble:
			return "GREATER_EQUAL_DOUBLE";

		// Logical
		case OpCode::LogicalAnd:
			return "LOGICAL_AND";
		case OpCode::LogicalOr:
			return "LOGICAL_OR";
		case OpCode::LogicalNot:
			return "LOGICAL_NOT";

		// Casts / conversions
		case OpCode::Cast:
			return "CAST";
		case OpCode::ToInt:
			return "TO_INT";
		case OpCode::ToFloat:
			return "TO_FLOAT";
		case OpCode::ToDouble:
			return "TO_DOUBLE";
		case OpCode::ToString:
			return "TO_STRING";
		case OpCode::ToBool:
			return "TO_BOOL";
		case OpCode::ToChar:
			return "TO_CHAR";
		case OpCode::TypeOf:
			return "TYPE_OF";

		// Control flow
		case OpCode::Jump:
			return "JUMP";
		case OpCode::JumpIfFalse:
			return "JUMP_IF_FALSE";
		case OpCode::JumpIfTrue:
			return "JUMP_IF_TRUE";
		case OpCode::Loop:
			return "LOOP";

		// Calls
		case OpCode::Call:
			return "CALL";
		case OpCode::CallNative:
			return "CALL_NATIVE";
		case OpCode::CallValue:
			return "CALL_VALUE";
		case OpCode::Return:
			return "RETURN";

		// Methods / constructors
		case OpCode::CallMethod:
			return "CALL_METHOD";
		case OpCode::CallVirtual:
			return "CALL_VIRTUAL";
		case OpCode::CallConstructor:
			return "CALL_CONSTRUCTOR";

		// Enums
		case OpCode::LoadEnumValue:
			return "LOAD_ENUM_VALUE";

		// Exceptions
		case OpCode::PushExceptionHandler:
			return "PUSH_EXCEPTION_HANDLER";
		case OpCode::PopExceptionHandler:
			return "POP_EXCEPTION_HANDLER";
		case OpCode::Throw:
			return "THROW";

		// Switch
		case OpCode::Switch:
			return "SWITCH";

		// IO
		case OpCode::Print:
			return "PRINT";
		case OpCode::PrintLine:
			return "PRINT_LINE";

		// Optimised / fused operations
		case OpCode::LessLocalLongConstant:
			return "LESS_LOCAL_LONG_CONSTANT";
		case OpCode::LessLocalIntConstant:
			return "LESS_LOCAL_INT_CONSTANT";
		case OpCode::AddLocalLong:
			return "ADD_LOCAL_LONG";
		case OpCode::AddLocalInt:
			return "ADD_LOCAL_INT";

		// VM
		case OpCode::Halt:
			return "HALT";
	}

	return "UNKNOWN";
}

}