// Compiler.cpp
// T# v2.0.0-beta1
// Dylan Armstrong, 2026

#include "Compiler.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace tsharp {

static std::string base_type_name(const std::string& type_name) {
	auto pos = type_name.find('<');

	if (pos == std::string::npos) {
		return type_name;
	}

	return type_name.substr(0, pos);
}

static std::string simple_identifier_from_expression(TSharpParser::ExpressionContext* expression) {
	if (!expression) {
		return {};
	}

	auto* logical_or = expression->logicalOrExpression();

	if (!logical_or || logical_or->logicalAndExpression().size() != 1) {
		return {};
	}

	auto* logical_and = logical_or->logicalAndExpression()[0];

	if (logical_and->equalityExpression().size() != 1) {
		return {};
	}

	auto* equality = logical_and->equalityExpression()[0];

	if (equality->relationalExpression().size() != 1) {
		return {};
	}

	auto* relational = equality->relationalExpression()[0];

	if (relational->additiveExpression().size() != 1) {
		return {};
	}

	auto* additive = relational->additiveExpression()[0];

	if (additive->multiplicativeExpression().size() != 1) {
		return {};
	}

	auto* multiplicative = additive->multiplicativeExpression()[0];

	if (multiplicative->unaryExpression().size() != 1) {
		return {};
	}

	auto* unary = multiplicative->unaryExpression()[0];

	if (!unary->postfixExpression()) {
		return {};
	}

	auto* postfix = unary->postfixExpression();

	if (!postfix->primary() || !postfix->primary()->IDENTIFIER() || !postfix->postfixSuffix().empty()) {
		return {};
	}

	return postfix->primary()->IDENTIFIER()->getText();
}

// Public compile entry point

BytecodeModule Compiler::compile(const std::vector<TSharpParser::ProgramContext*>& programs, TSharpParser::ProgramContext* entry_program) {
	if (programs.empty() || !entry_program) {
		throw std::runtime_error("Cannot compile an empty program set");
	}

	module = BytecodeModule();

	register_natives();

	current_function = nullptr;
	current_class = nullptr;

	function_contexts.clear();
	loop_contexts.clear();

	// IMPORTANT:
	// Declare everything in every imported module first.
	for (auto* program : programs) {
		if (program) {
			declare_top_level(program);
		}
	}

	// Then compile all bodies.
	for (auto* program : programs) {
		if (program) {
			compile_top_level(program);
		}
	}

	auto main_it = module.function_lookup.find("main");

	if (main_it == module.function_lookup.end()) {
		throw std::runtime_error("No main() function was defined");
	}

	module.main_function = main_it->second;

	return std::move(module);
}

// Top-level declaration pass

void Compiler::declare_top_level(TSharpParser::ProgramContext* program) {
	for (auto* top_level : program->topLevelDecl()) {
		if (!top_level) {
			continue;
		}

		if (auto* function = top_level->functionDecl()) {
			declare_function(function);

			continue;
		}

		if (auto* class_decl = top_level->classDecl()) {
			declare_class(class_decl);

			continue;
		}

		if (auto* interface_decl = top_level->interfaceDecl()) {
			declare_interface(interface_decl);

			continue;
		}

		if (auto* enum_decl = top_level->enumDecl()) {
			declare_enum(enum_decl);

			continue;
		}

		if (auto* global_decl = top_level->globalVarDecl()) {
			declare_global(global_decl);

			continue;
		}
	}
}

// Top-level compilation pass

void Compiler::compile_top_level(TSharpParser::ProgramContext* program) {
	for (auto* top_level : program->topLevelDecl()) {
		if (!top_level) {
			continue;
		}

		if (auto* function = top_level->functionDecl()) {
			compile_function(function);

			continue;
		}

		if (auto* class_decl = top_level->classDecl()) {
			compile_class(class_decl);

			continue;
		}

		if (auto* global_decl = top_level->globalVarDecl()) {
			compile_global_initializer(global_decl);

			continue;
		}
	}
}

// Function declaration

void Compiler::declare_function(TSharpParser::FunctionDeclContext* ctx) {
	const std::string name = ctx->IDENTIFIER()->getText();

	if (module.function_lookup.contains(name)) {
		throw std::runtime_error("Function already defined: " + name);
	}

	BytecodeFunction function;

	function.name = name;

	function.is_method = false;

	function.is_constructor = false;

	if (ctx->parameterList()) {
		for (auto* parameter : ctx->parameterList()->parameter()) {
			function.parameter_names.push_back(parameter->IDENTIFIER()->getText());

			function.parameter_types.push_back(base_type_name(parameter->typeRef()->getText()));
		}
	}

	function.arity = function.parameter_names.size();

	const std::size_t index = module.functions.size();

	module.function_lookup[name] = index;

	module.functions.push_back(std::move(function));
}

// Class declaration

void Compiler::declare_class(TSharpParser::ClassDeclContext* ctx) {
	const std::string name = ctx->IDENTIFIER()->getText();

	if (module.class_lookup.contains(name)) {
		throw std::runtime_error("Class already defined: " + name);
	}

	ClassInfo class_info;

	class_info.name = name;

	class_info.is_abstract = has_modifier(ctx->modifiers(), "abstract");

	if (ctx->inheritanceClause() && !ctx->inheritanceClause()->typeRef().empty()) {
		class_info.base_class = ctx->inheritanceClause()->typeRef()[0]->getText();
	}

	const std::size_t class_index = module.classes.size();

	module.class_lookup[name] = class_index;

	module.classes.push_back(std::move(class_info));

	ClassInfo& declared_class = module.classes[class_index];

	// Inherit fields and methods.
	if (!declared_class.base_class.empty()) {
		auto base_it = module.class_lookup.find(declared_class.base_class);

		if (base_it != module.class_lookup.end()) {
			const ClassInfo& base = module.classes[base_it->second];

			declared_class.fields = base.fields;

			declared_class.field_slots = base.field_slots;

			declared_class.methods = base.methods;

			declared_class.properties = base.properties;
		}
	}

	for (auto* member : ctx->classBody()->classMember()) {
		if (auto* field = member->fieldDecl()) {
			const std::string field_name = field->IDENTIFIER()->getText();

			if (declared_class.field_slots.contains(field_name)) {
				throw std::runtime_error("Field already defined: " + name + "." + field_name);
			}

			const std::size_t slot = declared_class.fields.size();

			BytecodeFieldInfo info;

			info.name = field_name;

			info.slot = slot;

			declared_class.field_slots[field_name] = slot;

			declared_class.fields.push_back(std::move(info));

			continue;
		}

		if (auto* method = member->methodDecl()) {
			declare_method(class_index, method);

			continue;
		}

		if (auto* constructor = member->constructorDecl()) {
			declare_constructor(class_index, constructor);

			continue;
		}

		if (auto* property = member->propertyDecl()) {
			declare_property(class_index, property);

			continue;
		}
	}
}

// Interface declaration

void Compiler::declare_interface(TSharpParser::InterfaceDeclContext* ctx) {
	// Interfaces are primarily compile-time metadata for now.
	//
	// Runtime method calls still use normal virtual dispatch.
	//
	// A full semantic analyser can later validate that classes
	// implement every interface member.

	const std::string name = ctx->IDENTIFIER()->getText();

	module.interfaces.push_back(name);
}

// Enum declaration

void Compiler::declare_enum(TSharpParser::EnumDeclContext* ctx) {
	EnumInfo info;

	info.name = ctx->IDENTIFIER()->getText();

	int next_value = 0;

	for (auto* member : ctx->enumMember()) {
		int value = next_value;

		if (member->INTEGER_LITERAL()) {
			value = std::stoi(member->INTEGER_LITERAL()->getText());
		}

		info.values[member->IDENTIFIER()->getText()] = value;

		next_value = value + 1;
	}

	const std::size_t index = module.enums.size();

	module.enum_lookup[info.name] = index;

	module.enums.push_back(std::move(info));
}

// Globals

void Compiler::declare_global(TSharpParser::GlobalVarDeclContext* ctx) {
	auto* variable = ctx->variableDecl();

	const std::string name = variable->IDENTIFIER()->getText();

	if (module.global_lookup.contains(name)) {
		throw std::runtime_error("Global variable already defined: " + name);
	}

	const std::size_t slot = module.globals.size();

	module.global_lookup[name] = slot;

	module.globals.push_back(Value());
}

void Compiler::compile_global_initializer(TSharpParser::GlobalVarDeclContext* ctx) {
	// Global initialisers should ultimately be compiled into
	// a module initialisation function.
	//
	// For constant literals, initialise directly now.

	auto* variable = ctx->variableDecl();

	const std::string name = variable->IDENTIFIER()->getText();

	const std::size_t slot = resolve_global(name);

	if (!variable->variableInitializer()) {
		return;
	}

	auto* initializer = variable->variableInitializer();

	if (!initializer->expression()) {
		return;
	}

	if (Value value; try_get_constant_expression(initializer->expression(), value)) {
		module.globals[slot] = std::move(value);
	}
}

// Methods

void Compiler::declare_method(std::size_t class_index, TSharpParser::MethodDeclContext* ctx) {
	ClassInfo& class_info = module.classes[class_index];

	const std::string method_name = ctx->IDENTIFIER()->getText();

	const std::string qualified_name = class_info.name + "." + method_name;

	BytecodeFunction function;

	function.name = qualified_name;

	function.is_method = true;

	function.is_constructor = false;

	// Slot 0 is reserved for this.
	function.parameter_names.push_back("this");

	if (ctx->parameterList()) {
		for (auto* parameter : ctx->parameterList()->parameter()) {
			function.parameter_names.push_back(parameter->IDENTIFIER()->getText());

			function.parameter_types.push_back(base_type_name(parameter->typeRef()->getText()));
		}
	}

	function.arity = function.parameter_names.size() - 1;

	const std::size_t function_index = module.functions.size();

	module.function_lookup[qualified_name] = function_index;

	module.functions.push_back(std::move(function));

	class_info.methods[method_name] = function_index;
}

// Constructors

void Compiler::declare_constructor(std::size_t class_index, TSharpParser::ConstructorDeclContext* ctx) {
	ClassInfo& class_info = module.classes[class_index];

	BytecodeFunction function;

	function.name = class_info.name + ".__ctor_" + std::to_string(class_info.constructors.size());

	function.is_method = true;

	function.is_constructor = true;

	function.parameter_names.push_back("this");

	if (ctx->parameterList()) {
		for (auto* parameter : ctx->parameterList()->parameter()) {
			function.parameter_names.push_back(parameter->IDENTIFIER()->getText());

			function.parameter_types.push_back(base_type_name(parameter->typeRef()->getText()));
		}
	}

	function.arity = function.parameter_names.size() - 1;

	const std::size_t function_index = module.functions.size();

	module.function_lookup[function.name] = function_index;

	module.functions.push_back(std::move(function));

	class_info.constructors.push_back(function_index);
}

// Properties

void Compiler::declare_property(std::size_t class_index, TSharpParser::PropertyDeclContext* ctx) {
	ClassInfo& class_info = module.classes[class_index];

	const std::string property_name = ctx->IDENTIFIER()->getText();

	const std::string function_name = class_info.name + ".__property_" + property_name;

	BytecodeFunction function;

	function.name = function_name;

	function.is_method = true;

	function.parameter_names.push_back("this");

	function.arity = 0;

	const std::size_t function_index = module.functions.size();

	module.function_lookup[function_name] = function_index;

	module.functions.push_back(std::move(function));

	class_info.properties[property_name] = function_index;
}

// Compile top-level function

void Compiler::compile_function(TSharpParser::FunctionDeclContext* ctx) {
	const std::string name = ctx->IDENTIFIER()->getText();

	const std::size_t function_index = resolve_function(name);

	BytecodeFunction& function = module.functions[function_index];

	begin_function(function);

	for (std::size_t i = 0; i < function.parameter_names.size(); i++) {
		const std::string type = i < function.parameter_types.size() ? function.parameter_types[i] : std::string{};

		declare_local(function.parameter_names[i], type);
	}

	if (ctx->block()) {
		visit(ctx->block());
	}

	// Implicit return for void functions.
	emit_opcode(OpCode::Null);

	emit_opcode(OpCode::Return);

	function.local_count = current_context().next_local;

	end_function();
}

// Compile class bodies

void Compiler::compile_class(TSharpParser::ClassDeclContext* ctx) {
	const std::string name = ctx->IDENTIFIER()->getText();

	const std::size_t class_index = resolve_class(name);

	current_class = &module.classes[class_index];

	std::size_t constructor_index = 0;

	for (auto* member : ctx->classBody()->classMember()) {
		if (auto* method = member->methodDecl()) {
			compile_method(class_index, method);

			continue;
		}

		if (auto* constructor = member->constructorDecl()) {
			compile_constructor(class_index, constructor, constructor_index++);

			continue;
		}

		if (auto* property = member->propertyDecl()) {
			compile_property(class_index, property);

			continue;
		}
	}

	current_class = nullptr;
}

// Compile method

void Compiler::compile_method(std::size_t class_index, TSharpParser::MethodDeclContext* ctx) {
	ClassInfo& class_info = module.classes[class_index];

	const std::string qualified_name = class_info.name + "." + ctx->IDENTIFIER()->getText();

	const std::size_t function_index = resolve_function(qualified_name);

	BytecodeFunction& function = module.functions[function_index];

	begin_function(function);

	for (std::size_t i = 0; i < function.parameter_names.size(); i++) {
		const std::string type = i < function.parameter_types.size() ? function.parameter_types[i] : std::string{};

		declare_local(function.parameter_names[i], type);
	}

	if (ctx->block()) {
		visit(ctx->block());

		emit_opcode(OpCode::Null);

		emit_opcode(OpCode::Return);
	}

	function.local_count = current_context().next_local;

	end_function();
}

// Compile constructor

void Compiler::compile_constructor(std::size_t class_index, TSharpParser::ConstructorDeclContext* ctx, std::size_t constructor_index) {
	ClassInfo& class_info = module.classes[class_index];

	const std::size_t function_index = class_info.constructors[constructor_index];

	BytecodeFunction& function = module.functions[function_index];

	begin_function(function);

	for (std::size_t i = 0; i < function.parameter_names.size(); i++) {
		const std::string type = i < function.parameter_types.size() ? function.parameter_types[i] : std::string{};

		declare_local(function.parameter_names[i], type);
	}

	visit(ctx->block());

	// Constructors return this.
	emit_load_local(0);

	emit_opcode(OpCode::Return);

	function.local_count = current_context().next_local;

	end_function();
}

// Compile property

void Compiler::compile_property(std::size_t class_index, TSharpParser::PropertyDeclContext* ctx) {
	ClassInfo& class_info = module.classes[class_index];

	const std::size_t function_index = class_info.properties[ctx->IDENTIFIER()->getText()];

	BytecodeFunction& function = module.functions[function_index];

	begin_function(function);

	declare_local("this");

	if (ctx->expression()) {
		visit(ctx->expression());
	} else if (ctx->block()) {
		visit(ctx->block());

		emit_opcode(OpCode::Null);
	}

	emit_opcode(OpCode::Return);

	function.local_count = current_context().next_local;

	end_function();
}

// Function compiler context

void Compiler::begin_scope() {
	current_context().scopes.emplace_back();
}

void Compiler::end_scope() {
	auto& context = current_context();

	if (context.scopes.size() <= 1) {
		// Never remove the function-level scope.
		return;
	}

	context.scopes.pop_back();
}
void Compiler::begin_function(BytecodeFunction& function) {
	FunctionCompilerContext context;

	context.function = &function;

	context.next_local = 0;

	// Function-level scope.
	context.scopes.emplace_back();

	function_contexts.push_back(std::move(context));

	current_function = &function;
}

void Compiler::end_function() {
	function_contexts.pop_back();

	current_function = function_contexts.empty() ? nullptr : function_contexts.back().function;
}

Compiler::FunctionCompilerContext& Compiler::current_context() {
	if (function_contexts.empty()) {
		throw std::runtime_error("No active function compilation context");
	}

	return function_contexts.back();
}

Chunk& Compiler::chunk() {
	if (!current_function) {
		throw std::runtime_error("No active bytecode function");
	}

	return current_function->chunk;
}

// Local/global resolution

std::size_t Compiler::declare_local(const std::string& name, const std::string& type) {
	auto& context = current_context();

	if (context.scopes.empty()) {
		context.scopes.emplace_back();
	}

	auto& current_scope = context.scopes.back();

	if (current_scope.contains(name)) {
		throw std::runtime_error("Local variable already declared in this scope: " + name);
	}

	const std::size_t slot = context.next_local++;

	current_scope[name] = LocalInfo{.slot = slot, .type = type};

	return slot;
}

std::size_t Compiler::resolve_local(const std::string& name) const {
	if (function_contexts.empty()) {
		throw std::runtime_error("No active function context");
	}

	const auto& scopes = function_contexts.back().scopes;

	for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) {
		auto local = scope->find(name);

		if (local != scope->end()) {
			return local->second.slot;
		}
	}

	throw std::runtime_error("Undefined local variable: " + name);
}

std::string Compiler::resolve_local_type(const std::string& name) const {
	if (function_contexts.empty()) {
		return {};
	}

	const auto& scopes = function_contexts.back().scopes;

	for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) {
		auto local = scope->find(name);

		if (local != scope->end()) {
			return local->second.type;
		}
	}

	return {};
}

bool Compiler::has_local(const std::string& name) const {
	if (function_contexts.empty()) {
		return false;
	}

	const auto& scopes = function_contexts.back().scopes;

	for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
		if (it->contains(name)) {
			return true;
		}
	}

	return false;
}

std::size_t Compiler::resolve_global(const std::string& name) const {
	auto it = module.global_lookup.find(name);

	if (it == module.global_lookup.end()) {
		throw std::runtime_error("Undefined global variable: " + name);
	}

	return it->second;
}

std::size_t Compiler::resolve_function(const std::string& name) const {
	auto it = module.function_lookup.find(name);

	if (it == module.function_lookup.end()) {
		throw std::runtime_error("Undefined function: " + name);
	}

	return it->second;
}

std::size_t Compiler::resolve_class(const std::string& name) const {
	auto it = module.class_lookup.find(name);

	if (it == module.class_lookup.end()) {
		throw std::runtime_error("Undefined class: " + name);
	}

	return it->second;
}

// Bytecode emit helpers

void Compiler::emit_opcode(OpCode opcode) {
	chunk().write_opcode(opcode);
}

void Compiler::emit_byte(std::uint8_t value) {
	chunk().write_byte(value);
}

void Compiler::emit_u16(std::uint16_t value) {
	emit_byte(static_cast<std::uint8_t>(value & 0xff));

	emit_byte(static_cast<std::uint8_t>((value >> 8) & 0xff));
}

void Compiler::emit_constant(Value value) {
	const std::size_t index = chunk().add_constant(std::move(value));

	if (index > std::numeric_limits<std::uint16_t>::max()) {
		throw std::runtime_error("Too many constants");
	}

	emit_opcode(OpCode::Constant);

	emit_u16(static_cast<std::uint16_t>(index));
}

void Compiler::emit_load_local(std::size_t slot) {
	emit_opcode(OpCode::LoadLocal);

	emit_u16(static_cast<std::uint16_t>(slot));
}

void Compiler::emit_store_local(std::size_t slot) {
	emit_opcode(OpCode::StoreLocal);

	emit_u16(static_cast<std::uint16_t>(slot));
}

std::size_t Compiler::emit_jump(OpCode opcode) {
	emit_opcode(opcode);

	emit_u16(0xffff);

	return (chunk().code_size() - 2);
}

void Compiler::patch_jump(std::size_t operand_position) {
	const std::size_t offset = chunk().code_size() - operand_position - 2;

	if (offset > std::numeric_limits<std::uint16_t>::max()) {
		throw std::runtime_error("Jump offset too large");
	}

	auto& code = chunk().mutable_code();

	code[operand_position] = static_cast<std::uint8_t>(offset & 0xff);

	code[operand_position + 1] = static_cast<std::uint8_t>((offset >> 8) & 0xff);
}

void Compiler::emit_loop(std::size_t loop_start) {
	emit_opcode(OpCode::Loop);

	const std::size_t offset = chunk().code_size() - loop_start + 2;

	if (offset > std::numeric_limits<std::uint16_t>::max()) {
		throw std::runtime_error("Loop body too large");
	}

	emit_u16(static_cast<std::uint16_t>(offset));
}

// Block

antlrcpp::Any Compiler::visitBlock(TSharpParser::BlockContext* ctx) {
	begin_scope();

	for (auto* statement : ctx->statement()) {
		visit(statement);
	}

	end_scope();

	return {};
}

// Variable declarations

antlrcpp::Any Compiler::visitVariableStatement(TSharpParser::VariableStatementContext* ctx) {
	return visit(ctx->variableDecl());
}

antlrcpp::Any Compiler::visitVariableDecl(TSharpParser::VariableDeclContext* ctx) {
	const std::string name = ctx->IDENTIFIER()->getText();

	const std::string declared_type = base_type_name(ctx->typeRef()->getText());

	const std::size_t slot = declare_local(name, declared_type);

	auto* array_decl = ctx->arrayDeclarator();

	auto* initializer = ctx->variableInitializer();

	// Initializer first
	//
	// int values[] = { 1, 2, 3 }
	// int x = 10

	if (initializer) {
		if (initializer->expression()) {
			visit(initializer->expression());

			emit_store_local(slot);

			return {};
		}

		if (initializer->LPAREN()) {
			const std::string runtime_type = base_type_name(ctx->typeRef()->getText());

			if (module.class_lookup.contains(runtime_type)) {
				std::size_t argument_count = 0;

				if (initializer->argumentList()) {
					for (auto* argument : initializer->argumentList()->expression()) {
						visit(argument);

						argument_count++;
					}
				}

				emit_opcode(OpCode::NewObject);
				emit_u16(static_cast<std::uint16_t>(module.class_lookup.at(runtime_type)));
				emit_u16(static_cast<std::uint16_t>(argument_count));

				emit_store_local(slot);

				return {};
			}
		}

		// Keep your constructor-style initialization here if
		// argumentList() represents:
		//
		// Counter counter()
		// Counter counter(100)
		//
		// Do NOT treat an array literal as a zero-sized NewArray.
	}

	// Fixed-size array without initializer
	//
	// int values[5]

	if (array_decl) {
		if (array_decl->expression()) {
			visit(array_decl->expression());

			emit_opcode(OpCode::NewArray);

			emit_store_local(slot);

			return {};
		}

		// Unsized [] with no initializer.
		//
		// int values[]
		//
		// Represent as empty array.
		emit_constant(Value(0));

		emit_opcode(OpCode::NewArray);

		emit_store_local(slot);

		return {};
	}

	// No initializer

	if (!initializer) {
		emit_opcode(OpCode::Null);

		emit_store_local(slot);

		return {};
	}

	// Your constructor-style initialization logic continues here.

	return {};
}

// Assignment statements

antlrcpp::Any Compiler::visitAssignmentStatement(TSharpParser::AssignmentStatementContext* ctx) {
	return visit(ctx->assignment());
}

antlrcpp::Any Compiler::visitAssignment(TSharpParser::AssignmentContext* ctx) {
	auto* lvalue = ctx->lvalue();

	const std::string op = ctx->assignmentOperator()->getText();

	const auto& suffixes = lvalue->lvalueSuffix();

	const std::string primary_name = lvalue->primaryLValue()->getText();

	// Simple identifier
	//
	// x = value
	// x += value
	//
	// Also supports implicit this fields and globals.

	if (suffixes.empty()) {
		const std::string name = primary_name;

		// Implicit this field FIRST.
		if (current_class && current_class->field_slots.contains(name)) {
			const std::size_t field_slot = current_class->field_slots[name];

			emit_load_local(0);

			if (op != "=") {
				emit_opcode(OpCode::Duplicate);

				emit_opcode(OpCode::LoadField);

				emit_u16(static_cast<std::uint16_t>(field_slot));
			}

			visit(ctx->expression());

			if (op != "=") {
				emit_compound_operator(op);
			}

			emit_opcode(OpCode::StoreField);

			emit_u16(static_cast<std::uint16_t>(field_slot));

			return {};
		}

		if (has_local(name)) {
			const std::size_t slot = resolve_local(name);

			
			// Fused local addition
			//
			// result += i
			

			if (op == "+=") {
				const std::string rhs_name = simple_identifier_from_expression(ctx->expression());

				if (!rhs_name.empty() && has_local(rhs_name)) {
					const std::string lhs_type = resolve_local_type(name);

					const std::string rhs_type = resolve_local_type(rhs_name);

					if (lhs_type == "int" && rhs_type == "int") {
						emit_opcode(OpCode::AddLocalInt);

						emit_u16(static_cast<std::uint16_t>(slot));

						emit_u16(static_cast<std::uint16_t>(resolve_local(rhs_name)));

						return {};
					}

					if (lhs_type == "long" && rhs_type == "long") {
						emit_opcode(OpCode::AddLocalLong);

						emit_u16(static_cast<std::uint16_t>(slot));

						emit_u16(static_cast<std::uint16_t>(resolve_local(rhs_name)));

						return {};
					}
				}
			}

			
			// Normal assignment
			

			if (op == "=") {
				visit(ctx->expression());

				emit_store_local(slot);

				return {};
			}

			
			// Generic compound assignment fallback
			

			emit_load_local(slot);

			visit(ctx->expression());

			emit_compound_operator(op);

			emit_store_local(slot);

			return {};
		}

		// --------------------------------------------------------
		// Global
		// --------------------------------------------------------

		if (module.global_lookup.contains(name)) {
			const std::size_t global = resolve_global(name);

			if (op != "=") {
				emit_opcode(OpCode::LoadGlobal);

				emit_u16(static_cast<std::uint16_t>(global));
			}

			visit(ctx->expression());

			if (op != "=") {
				emit_compound_operator(op);
			}

			emit_opcode(OpCode::StoreGlobal);

			emit_u16(static_cast<std::uint16_t>(global));

			return {};
		}

		throw std::runtime_error("Undefined assignment target: " + name);
	}

	// Chained l-value
	//
	// object.field = value
	// array[i] = value
	// entries[i].value = value
	// object.items[i] = value
	// object.items[i].value = value
	//
	// Compile the primary and every suffix EXCEPT the final one.
	// The final suffix determines the store operation.

	emit_variable_load(primary_name);

	for (std::size_t i = 0; i + 1 < suffixes.size(); i++) {
		auto* suffix = suffixes[i];

		if (suffix->LBRACK()) {
			visit(suffix->expression());

			emit_opcode(OpCode::LoadIndex);

			continue;
		}

		if (suffix->DOT()) {
			const std::string member = suffix->IDENTIFIER()->getText();

			emit_opcode(OpCode::LoadMember);

			emit_string_operand(member);

			continue;
		}

		throw std::runtime_error("Unsupported intermediate assignment target: " + lvalue->getText());
	}

	auto* final_suffix = suffixes.back();

	// Final member
	//
	// object.field = value
	// entries[i].value = value

	if (final_suffix->DOT()) {
		const std::string field_name = final_suffix->IDENTIFIER()->getText();

		if (op == "=") {
			visit(ctx->expression());

			emit_opcode(OpCode::StoreFieldDynamic);

			emit_string_operand(field_name);

			return {};
		}

		// For compound assignment:
		//
		// object.field += value
		//
		// Stack:
		// receiver
		// receiver receiver
		// receiver old_value
		// receiver old_value rhs
		// receiver result
		emit_opcode(OpCode::Duplicate);

		emit_opcode(OpCode::LoadMember);

		emit_string_operand(field_name);

		visit(ctx->expression());

		emit_compound_operator(op);

		emit_opcode(OpCode::StoreFieldDynamic);

		emit_string_operand(field_name);

		return {};
	}

	// Final index
	//
	// array[i] = value
	// object.items[i] = value

	if (final_suffix->LBRACK()) {
		visit(final_suffix->expression());

		if (op == "=") {
			visit(ctx->expression());

			emit_opcode(OpCode::StoreIndex);

			return {};
		}

		// Compound indexed assignments require preserving both the
		// array and index while loading the existing value.
		//
		// Store them temporarily to avoid requiring a Duplicate2
		// opcode.

		const std::size_t index_slot = declare_temporary();

		emit_store_local(index_slot);

		const std::size_t receiver_slot = declare_temporary();

		emit_store_local(receiver_slot);

		// Load current value.
		emit_load_local(receiver_slot);

		emit_load_local(index_slot);

		emit_opcode(OpCode::LoadIndex);

		// Apply RHS.
		visit(ctx->expression());

		emit_compound_operator(op);

		// Save result temporarily.
		const std::size_t value_slot = declare_temporary();

		emit_store_local(value_slot);

		// Perform the actual store.
		emit_load_local(receiver_slot);

		emit_load_local(index_slot);

		emit_load_local(value_slot);

		emit_opcode(OpCode::StoreIndex);

		return {};
	}

	throw std::runtime_error("Unsupported assignment target: " + lvalue->getText());
}

// Expression statement

antlrcpp::Any Compiler::visitExpressionStatement(TSharpParser::ExpressionStatementContext* ctx) {
	visit(ctx->expression());

	emit_opcode(OpCode::Pop);

	return {};
}

// Expressions

antlrcpp::Any Compiler::visitExpression(TSharpParser::ExpressionContext* ctx) {
	return visit(ctx->logicalOrExpression());
}

antlrcpp::Any Compiler::visitLogicalOrExpression(TSharpParser::LogicalOrExpressionContext* ctx) {
	const auto& values = ctx->logicalAndExpression();

	visit(values[0]);

	for (std::size_t i = 1; i < values.size(); i++) {
		visit(values[i]);

		emit_opcode(OpCode::LogicalOr);
	}

	return {};
}

antlrcpp::Any Compiler::visitLogicalAndExpression(TSharpParser::LogicalAndExpressionContext* ctx) {
	const auto& values = ctx->equalityExpression();

	visit(values[0]);

	for (std::size_t i = 1; i < values.size(); i++) {
		visit(values[i]);

		emit_opcode(OpCode::LogicalAnd);
	}

	return {};
}

antlrcpp::Any Compiler::visitEqualityExpression(TSharpParser::EqualityExpressionContext* ctx) {
	const auto& values = ctx->relationalExpression();

	visit(values[0]);

	for (std::size_t i = 1; i < values.size(); i++) {
		visit(values[i]);

		const std::string op = ctx->children[i * 2 - 1]->getText();

		emit_opcode(op == "==" ? OpCode::Equal : OpCode::NotEqual);
	}

	return {};
}

antlrcpp::Any Compiler::visitRelationalExpression(TSharpParser::RelationalExpressionContext* ctx) {
	const auto& values = ctx->additiveExpression();

	visit(values[0]);

	for (std::size_t i = 1; i < values.size(); i++) {
		visit(values[i]);

		const std::string op = ctx->children[i * 2 - 1]->getText();

		if (op == "<") {
			emit_opcode(OpCode::Less);
		} else if (op == "<=") {
			emit_opcode(OpCode::LessEqual);
		} else if (op == ">") {
			emit_opcode(OpCode::Greater);
		} else if (op == ">=") {
			emit_opcode(OpCode::GreaterEqual);
		}
	}

	return {};
}

antlrcpp::Any Compiler::visitAdditiveExpression(TSharpParser::AdditiveExpressionContext* ctx) {
	const auto& values = ctx->multiplicativeExpression();

	visit(values[0]);

	for (std::size_t i = 1; i < values.size(); i++) {
		visit(values[i]);

		const std::string op = ctx->children[i * 2 - 1]->getText();

		emit_opcode(op == "+" ? OpCode::Add : OpCode::Subtract);
	}

	return {};
}

antlrcpp::Any Compiler::visitMultiplicativeExpression(TSharpParser::MultiplicativeExpressionContext* ctx) {
	const auto& values = ctx->unaryExpression();

	visit(values[0]);

	for (std::size_t i = 1; i < values.size(); i++) {
		visit(values[i]);

		const std::string op = ctx->children[i * 2 - 1]->getText();

		if (op == "*") {
			emit_opcode(OpCode::Multiply);
		} else if (op == "/") {
			emit_opcode(OpCode::Divide);
		} else {
			emit_opcode(OpCode::Modulo);
		}
	}

	return {};
}

// Unary

antlrcpp::Any Compiler::visitUnaryExpression(TSharpParser::UnaryExpressionContext* ctx) {
	if (ctx->postfixExpression()) {
		return visit(ctx->postfixExpression());
	}

	const std::string op = ctx->children[0]->getText();

	if (op == "++" || op == "--") {
		auto* inner = ctx->unaryExpression();

		auto* postfix = inner->postfixExpression();

		if (!postfix || !postfix->primary() || !postfix->primary()->IDENTIFIER()) {
			throw std::runtime_error("Prefix increment/decrement requires a variable");
		}

		const std::string name = postfix->primary()->IDENTIFIER()->getText();

		const std::size_t slot = resolve_local(name);

		emit_opcode(op == "++" ? OpCode::IncrementLocal : OpCode::DecrementLocal);

		emit_u16(static_cast<std::uint16_t>(slot));

		emit_load_local(slot);

		return {};
	}

	visit(ctx->unaryExpression());

	if (op == "!") {
		emit_opcode(OpCode::LogicalNot);
	} else if (op == "-") {
		emit_opcode(OpCode::Negate);
	}

	return {};
}

// Postfix expressions

antlrcpp::Any Compiler::visitPostfixExpression(TSharpParser::PostfixExpressionContext* ctx) {
	auto* primary = ctx->primary();

	if (!primary) {
		throw std::runtime_error("Postfix expression has no primary");
	}

	const auto& suffixes = ctx->postfixSuffix();

	// Postfix increment/decrement
	//
	// i++
	// i--
	//
	// count++      // implicit this field
	// count--

	if (primary->IDENTIFIER() && suffixes.size() == 1 && (suffixes[0]->INC() || suffixes[0]->DEC())) {
		const std::string name = primary->IDENTIFIER()->getText();

		const bool increment = suffixes[0]->INC() != nullptr;

		// ------------------------------------------------------------
		// Local variable
		// ------------------------------------------------------------

		if (has_local(name)) {
			const std::size_t slot = resolve_local(name);

			// Postfix result is the old value.
			emit_load_local(slot);

			// Actually mutate the local variable.
			emit_opcode(increment ? OpCode::IncrementLocal : OpCode::DecrementLocal);

			emit_u16(static_cast<std::uint16_t>(slot));

			return {};
		}

		// ------------------------------------------------------------
		// Implicit this field
		//
		// class Counter {
		//     private int count
		//
		//     void Increment() {
		//         count++
		//     }
		// }
		// ------------------------------------------------------------

		if (current_class && current_class->field_slots.contains(name)) {
			const std::size_t field_slot = current_class->field_slots[name];

			// Load old value as the postfix expression result.
			emit_load_local(0);

			emit_opcode(OpCode::LoadField);

			emit_u16(static_cast<std::uint16_t>(field_slot));

			// Mutate this.field directly.
			emit_load_local(0);

			emit_opcode(increment ? OpCode::IncrementField : OpCode::DecrementField);

			emit_u16(static_cast<std::uint16_t>(field_slot));

			return {};
		}

		throw std::runtime_error("Postfix increment/decrement requires "
								 "a variable or field: " +
								 name);
	}
	// Simple local postfix increment/decrement:
	//
	// i++
	// i--
	if (primary->IDENTIFIER() && suffixes.size() == 1 && (suffixes[0]->INC() || suffixes[0]->DEC())) {
		const std::string name = primary->IDENTIFIER()->getText();

		if (!has_local(name)) {
			throw std::runtime_error("Postfix increment/decrement currently "
									 "requires a local variable: " +
									 name);
		}

		const std::size_t slot = resolve_local(name);

		// Postfix semantics: leave old value on the stack.
		emit_load_local(slot);

		// Actually mutate the local.
		emit_opcode(suffixes[0]->INC() ? OpCode::IncrementLocal : OpCode::DecrementLocal);

		emit_u16(static_cast<std::uint16_t>(slot));

		return {};
	}

	// Enum member access:
	//
	// Color.BLUE
	if (primary->IDENTIFIER() && !suffixes.empty() && suffixes[0]->DOT() && suffixes[0]->IDENTIFIER()) {
		const std::string type_name = primary->IDENTIFIER()->getText();

		auto enum_it = module.enum_lookup.find(type_name);

		if (enum_it != module.enum_lookup.end()) {
			const std::string member_name = suffixes[0]->IDENTIFIER()->getText();

			const EnumInfo& enum_info = module.enums[enum_it->second];

			auto value_it = enum_info.values.find(member_name);

			if (value_it == enum_info.values.end()) {
				throw std::runtime_error("Enum " + type_name + " has no member " + member_name);
			}

			emit_constant(Value(value_it->second));

			for (std::size_t i = 1; i < suffixes.size(); i++) {
				compile_postfix_suffix(suffixes[i]);
			}

			return {};
		}
	}

	// Direct function call:
	//
	// println(...)
	// Add(...)
	if (primary->IDENTIFIER() && !suffixes.empty()) {
		auto* first_suffix = suffixes.front();

		if (first_suffix && first_suffix->LPAREN()) {
			const std::string name = primary->IDENTIFIER()->getText();

			compile_call(name, first_suffix->argumentList());

			for (std::size_t i = 1; i < suffixes.size(); i++) {
				compile_postfix_suffix(suffixes[i]);
			}

			return {};
		}
	}

	// ------------------------------------------------------------
	// Normal/native function call
	// ------------------------------------------------------------

	if (primary->IDENTIFIER() != nullptr && !suffixes.empty() && suffixes[0]->LPAREN() != nullptr) {
		const std::string name = primary->IDENTIFIER()->getText();

		compile_call(name, suffixes[0]->argumentList());

		for (std::size_t i = 1; i < suffixes.size(); i++) {
			compile_postfix_suffix(suffixes[i]);
		}

		return {};
	}

	visit(primary);

	for (auto* suffix : suffixes) {
		compile_postfix_suffix(suffix);
	}

	return {};
}

// Primary

antlrcpp::Any Compiler::visitPrimary(TSharpParser::PrimaryContext* ctx) {
	if (ctx->literal()) {
		return visit(ctx->literal());
	}

	if (ctx->THIS()) {
		emit_load_local(0);

		return {};
	}

	if (ctx->BASE()) {
		emit_load_local(0);

		return {};
	}

	if (ctx->expression()) {
		return visit(ctx->expression());
	}

	if (ctx->arrayLiteral()) {
		return visit(ctx->arrayLiteral());
	}

	if (ctx->IDENTIFIER()) {
		const std::string name = ctx->IDENTIFIER()->getText();

		emit_variable_load(name);

		return {};
	}

	throw std::runtime_error("Unsupported primary expression");
}

// Literals

antlrcpp::Any Compiler::visitLiteral(TSharpParser::LiteralContext* ctx) {

	if (ctx->LONG_LITERAL()) {
		std::string text = ctx->LONG_LITERAL()->getText();

		text.pop_back();

		emit_constant(Value(static_cast<std::int64_t>(std::stoll(text))));

		return {};
	}

	if (ctx->INTEGER_LITERAL()) {
		emit_constant(Value(std::stoi(ctx->INTEGER_LITERAL()->getText())));

		return {};
	}

	if (ctx->FLOAT_LITERAL()) {
		std::string text = ctx->FLOAT_LITERAL()->getText();

		const bool is_float = text.ends_with("f") || text.ends_with("F");

		if (is_float) {
			text.pop_back();

			emit_constant(Value(std::stof(text)));
		} else {
			emit_constant(Value(std::stod(text)));
		}

		return {};
	}

	if (ctx->STRING_LITERAL()) {
		std::string value = ctx->STRING_LITERAL()->getText();

		value = value.substr(1, value.size() - 2);

		emit_constant(Value(value));

		return {};
	}

	if (ctx->CHAR_LITERAL()) {
		const std::string text = ctx->CHAR_LITERAL()->getText();

		emit_constant(Value(text[1]));

		return {};
	}

	if (ctx->TRUE()) {
		emit_opcode(OpCode::True);

		return {};
	}

	if (ctx->FALSE()) {
		emit_opcode(OpCode::False);

		return {};
	}

	emit_opcode(OpCode::Null);

	return {};
}

// Array literal

antlrcpp::Any Compiler::visitArrayLiteral(TSharpParser::ArrayLiteralContext* ctx) {
	std::size_t count = 0;

	for (auto* expression : ctx->expression()) {
		visit(expression);

		count++;
	}

	emit_opcode(OpCode::ArrayLiteral);

	emit_u16(static_cast<std::uint16_t>(count));

	return {};
}

// If / else

antlrcpp::Any Compiler::visitIfStatement(TSharpParser::IfStatementContext* ctx) {
	const auto& expressions = ctx->expression();

	const auto& blocks = ctx->block();

	std::vector<std::size_t> end_jumps;

	for (std::size_t i = 0; i < expressions.size(); i++) {
		visit(expressions[i]);

		const std::size_t false_jump = emit_jump(OpCode::JumpIfFalse);

		visit(blocks[i]);

		end_jumps.push_back(emit_jump(OpCode::Jump));

		patch_jump(false_jump);
	}

	if (blocks.size() > expressions.size()) {
		visit(blocks.back());
	}

	for (const std::size_t jump : end_jumps) {
		patch_jump(jump);
	}

	return {};
}

// While

antlrcpp::Any Compiler::visitWhileStatement(TSharpParser::WhileStatementContext* ctx) {
	const std::size_t loop_start = chunk().code_size();

	visit(ctx->expression());

	const std::size_t exit_jump = emit_jump(OpCode::JumpIfFalse);

	begin_loop(loop_start);

	visit(ctx->block());

	patch_continue_jumps(loop_start);

	emit_loop(loop_start);

	patch_jump(exit_jump);

	end_loop();

	return {};
}

// Do / while

antlrcpp::Any Compiler::visitDoWhileStatement(TSharpParser::DoWhileStatementContext* ctx) {
	const std::size_t loop_start = chunk().code_size();

	begin_loop(loop_start);

	visit(ctx->block());

	const std::size_t condition_start = chunk().code_size();

	patch_continue_jumps(condition_start);

	visit(ctx->expression());

	const std::size_t exit = emit_jump(OpCode::JumpIfFalse);

	emit_loop(loop_start);

	patch_jump(exit);

	end_loop();

	return {};
}

// For

antlrcpp::Any Compiler::visitForStatement(TSharpParser::ForStatementContext* ctx) {
	begin_scope();

	if (ctx->forInit()) {
		auto* init = ctx->forInit();

		if (init->variableDecl()) {
			visit(init->variableDecl());
		} else if (init->assignment()) {
			visit(init->assignment());
		} else if (init->expression()) {
			visit(init->expression());

			emit_opcode(OpCode::Pop);
		}
	}

	const std::size_t condition_start = chunk().code_size();

	std::size_t exit_jump = std::numeric_limits<std::size_t>::max();

	if (ctx->expression()) {
		visit(ctx->expression());

		exit_jump = emit_jump(OpCode::JumpIfFalse);
	}

	begin_loop(condition_start);

	visit(ctx->block());

	const std::size_t update_start = chunk().code_size();

	patch_continue_jumps(update_start);

	if (ctx->forUpdate()) {
		auto* update = ctx->forUpdate();

		for (auto* assignment : update->assignment()) {
			visit(assignment);
		}

		for (auto* expression : update->expression()) {
			visit(expression);

			emit_opcode(OpCode::Pop);
		}
	}

	emit_loop(condition_start);

	if (exit_jump != std::numeric_limits<std::size_t>::max()) {
		patch_jump(exit_jump);
	}

	end_loop();

	end_scope();

	return {};
}

// Break / continue

antlrcpp::Any Compiler::visitBreakStatement(TSharpParser::BreakStatementContext*) {
	if (loop_contexts.empty()) {
		throw std::runtime_error("break used outside a loop or switch");
	}

	loop_contexts.back().break_jumps.push_back(emit_jump(OpCode::Jump));

	return {};
}

antlrcpp::Any Compiler::visitContinueStatement(TSharpParser::ContinueStatementContext*) {
	if (loop_contexts.empty()) {
		throw std::runtime_error("continue used outside a loop");
	}

	loop_contexts.back().continue_jumps.push_back(emit_jump(OpCode::Jump));

	return {};
}

// Switch

antlrcpp::Any Compiler::visitSwitchStatement(TSharpParser::SwitchStatementContext* ctx) {
	// Store switch value in a temporary local.
	visit(ctx->expression());

	const std::size_t switch_slot = declare_temporary();

	emit_store_local(switch_slot);

	begin_loop(chunk().code_size());

	TSharpParser::SwitchSectionContext* default_section = nullptr;

	std::vector<std::size_t> end_jumps;

	for (auto* section : ctx->switchSection()) {
		if (section->DEFAULT()) {
			default_section = section;

			continue;
		}

		emit_load_local(switch_slot);

		visit(section->expression());

		emit_opcode(OpCode::Equal);

		const std::size_t next_case = emit_jump(OpCode::JumpIfFalse);

		compile_switch_section(section);

		end_jumps.push_back(emit_jump(OpCode::Jump));

		patch_jump(next_case);
	}

	if (default_section) {
		compile_switch_section(default_section);
	}

	for (const std::size_t jump : end_jumps) {
		patch_jump(jump);
	}

	end_loop();

	return {};
}

// Return

antlrcpp::Any Compiler::visitReturnStatement(TSharpParser::ReturnStatementContext* ctx) {
	if (ctx->expression()) {
		visit(ctx->expression());
	} else {
		emit_opcode(OpCode::Null);
	}

	emit_opcode(OpCode::Return);

	return {};
}

// Try / catch / finally

antlrcpp::Any Compiler::visitTryStatement(TSharpParser::TryStatementContext* ctx) {
	const std::size_t handler = emit_exception_handler_placeholder();

	visit(ctx->block());

	emit_opcode(OpCode::PopExceptionHandler);

	const std::size_t end_try = emit_jump(OpCode::Jump);

	patch_exception_handler(handler, chunk().code_size());

	for (auto* catch_clause : ctx->catchClause()) {
		const std::size_t exception_slot = declare_local(catch_clause->IDENTIFIER()->getText());

		emit_store_local(exception_slot);

		visit(catch_clause->block());
	}

	if (ctx->finallyClause()) {
		visit(ctx->finallyClause()->block());
	}

	patch_jump(end_try);

	return {};
}

// Throw

antlrcpp::Any Compiler::visitThrowStatement(TSharpParser::ThrowStatementContext* ctx) {
	visit(ctx->expression());

	emit_opcode(OpCode::Throw);

	return {};
}

// Call compilation

void Compiler::compile_call(const std::string& name, TSharpParser::ArgumentListContext* arguments) {
	std::size_t argument_count = 0;

	if (arguments) {
		for (auto* argument : arguments->expression()) {
			visit(argument);

			argument_count++;
		}
	}

	// Constructor call:
	//
	// Vector2(...)
	// Dictionary(...)
	if (module.class_lookup.contains(name)) {
		emit_opcode(OpCode::NewObject);

		emit_u16(static_cast<std::uint16_t>(module.class_lookup.at(name)));

		emit_u16(static_cast<std::uint16_t>(argument_count));

		return;
	}

	if (is_builtin(name)) {
		emit_opcode(OpCode::CallNative);

		emit_u16(static_cast<std::uint16_t>(resolve_native(name)));

		emit_u16(static_cast<std::uint16_t>(argument_count));

		return;
	}

	if (module.function_lookup.contains(name)) {
		emit_opcode(OpCode::Call);

		emit_u16(static_cast<std::uint16_t>(resolve_function(name)));

		emit_u16(static_cast<std::uint16_t>(argument_count));

		return;
	}

	// Implicit method call:
	//
	// magnitude()
	// to_string()
	//
	// inside a class means:
	//
	// this.magnitude()
	// this.to_string()
	if (current_class && current_class->methods.contains(name)) {
		// Pass `this` as the implicit receiver.
		emit_load_local(0);

		emit_opcode(OpCode::Call);

		emit_u16(static_cast<std::uint16_t>(current_class->methods.at(name)));

		emit_u16(static_cast<std::uint16_t>(argument_count + 1));

		return;
	}

	throw std::runtime_error("Undefined callable: " + name);
}

// Postfix suffix

void Compiler::compile_postfix_suffix(TSharpParser::PostfixSuffixContext* ctx) {
	if (ctx->LBRACK()) {
		visit(ctx->expression());

		emit_opcode(OpCode::LoadIndex);

		return;
	}

	if (ctx->DOT()) {
		const std::string member = ctx->IDENTIFIER()->getText();

		emit_opcode(OpCode::LoadMember);

		emit_string_operand(member);

		return;
	}

	if (ctx->LPAREN()) {
		std::size_t argument_count = 0;

		if (ctx->argumentList()) {
			for (auto* argument : ctx->argumentList()->expression()) {
				visit(argument);

				argument_count++;
			}
		}

		emit_opcode(OpCode::CallValue);

		emit_u16(static_cast<std::uint16_t>(argument_count));

		return;
	}

	if (ctx->INC() || ctx->DEC()) {
		emit_opcode(ctx->INC() ? OpCode::IncrementValue : OpCode::DecrementValue);

		return;
	}
}

// Variable load

void Compiler::emit_variable_load(const std::string& name) {
	if (has_local(name)) {
		emit_load_local(resolve_local(name));

		return;
	}

	if (current_class && current_class->field_slots.contains(name)) {
		emit_load_local(0);

		emit_opcode(OpCode::LoadField);

		emit_u16(static_cast<std::uint16_t>(current_class->field_slots[name]));

		return;
	}

	if (module.global_lookup.contains(name)) {
		emit_opcode(OpCode::LoadGlobal);

		emit_u16(static_cast<std::uint16_t>(resolve_global(name)));

		return;
	}

	if (is_builtin_constant(name)) {
		emit_constant(get_builtin_constant(name));

		return;
	}

	throw std::runtime_error("Undefined variable: " + name);
}

// Compound operator

void Compiler::emit_compound_operator(const std::string& op) {
	if (op == "+=") {
		emit_opcode(OpCode::Add);
	} else if (op == "-=") {
		emit_opcode(OpCode::Subtract);
	} else if (op == "*=") {
		emit_opcode(OpCode::Multiply);
	} else if (op == "/=") {
		emit_opcode(OpCode::Divide);
	} else if (op == "%=") {
		emit_opcode(OpCode::Modulo);
	}
}

// Loop helpers

void Compiler::begin_loop(std::size_t continue_target) {
	LoopCompilerContext context;

	context.continue_target = continue_target;

	loop_contexts.push_back(std::move(context));
}

void Compiler::end_loop() {
	if (loop_contexts.empty()) {
		return;
	}

	auto context = std::move(loop_contexts.back());

	loop_contexts.pop_back();

	for (const std::size_t jump : context.break_jumps) {
		patch_jump(jump);
	}
}

void Compiler::patch_continue_jumps(std::size_t target) {
	if (loop_contexts.empty()) {
		return;
	}

	for (const std::size_t position : loop_contexts.back().continue_jumps) {
		patch_absolute_jump(position, target);
	}

	loop_contexts.back().continue_jumps.clear();
}

// Switch helper

void Compiler::compile_switch_section(TSharpParser::SwitchSectionContext* ctx) {
	for (auto* child : ctx->children) {
		auto* statement = dynamic_cast<TSharpParser::StatementContext*>(child);

		if (statement) {
			visit(statement);
		}
	}
}

// Utility helpers

bool Compiler::has_modifier(TSharpParser::ModifiersContext* ctx, const std::string& modifier) const {
	if (!ctx) {
		return false;
	}

	for (auto* item : ctx->modifier()) {
		if (item->getText() == modifier) {
			return true;
		}
	}

	return false;
}

std::size_t Compiler::declare_temporary() {
	const std::string name = "$tmp_" + std::to_string(current_context().next_local);

	return declare_local(name);
}

void Compiler::emit_string_operand(const std::string& value) {
	const std::size_t index = chunk().add_constant(Value(value));

	emit_u16(static_cast<std::uint16_t>(index));
}

bool Compiler::try_get_constant_expression(TSharpParser::ExpressionContext* expression, Value& value) {
	if (!expression) {
		return false;
	}

	auto* logical_or = expression->logicalOrExpression();

	if (!logical_or || logical_or->logicalAndExpression().size() != 1) {
		return false;
	}

	auto* logical_and = logical_or->logicalAndExpression()[0];

	if (logical_and->equalityExpression().size() != 1) {
		return false;
	}

	auto* equality = logical_and->equalityExpression()[0];

	if (equality->relationalExpression().size() != 1) {
		return false;
	}

	auto* relational = equality->relationalExpression()[0];

	if (relational->additiveExpression().size() != 1) {
		return false;
	}

	auto* additive = relational->additiveExpression()[0];

	if (additive->multiplicativeExpression().size() != 1) {
		return false;
	}

	auto* multiplicative = additive->multiplicativeExpression()[0];

	if (multiplicative->unaryExpression().size() != 1) {
		return false;
	}

	auto* unary = multiplicative->unaryExpression()[0];

	if (!unary->postfixExpression()) {
		return false;
	}

	auto* postfix = unary->postfixExpression();

	if (!postfix->postfixSuffix().empty() || !postfix->primary()) {
		return false;
	}

	auto* primary = postfix->primary();

	if (!primary->literal()) {
		return false;
	}

	auto* literal = primary->literal();

	if (literal->LONG_LITERAL()) {
		std::string text = literal->LONG_LITERAL()->getText();

		text.pop_back();

		value = Value(static_cast<std::int64_t>(std::stoll(text)));

		return true;
	}

	if (literal->INTEGER_LITERAL()) {
		value = Value(std::stoi(literal->INTEGER_LITERAL()->getText()));

		return true;
	}

	if (literal->FLOAT_LITERAL()) {
		std::string text = literal->FLOAT_LITERAL()->getText();

		const bool is_float = text.ends_with("f") || text.ends_with("F");

		if (is_float) {
			text.pop_back();

			value = Value(std::stof(text));
		} else {
			value = Value(std::stod(text));
		}

		return true;
	}

	if (literal->STRING_LITERAL()) {
		std::string text = literal->STRING_LITERAL()->getText();

		if (text.size() >= 2) {
			text = text.substr(1, text.size() - 2);
		}

		value = Value(std::move(text));

		return true;
	}

	if (literal->CHAR_LITERAL()) {
		const std::string text = literal->CHAR_LITERAL()->getText();

		if (text.size() < 3) {
			return false;
		}

		value = Value(text[1]);

		return true;
	}

	if (literal->TRUE()) {
		value = Value(true);

		return true;
	}

	if (literal->FALSE()) {
		value = Value(false);

		return true;
	}

	if (literal->isEmpty()) {
		value = Value();

		return true;
	}

	return false;
}

bool Compiler::is_builtin(const std::string& name) const {
	return (name == "print" || name == "println" || name == "sqrt" || name == "abs" || name == "cube_root" || name == "exp" || name == "log" || name == "sin" || name == "cos" || name == "tan" ||
			name == "factorial" || name == "x_root" || name == "pow" || name == "floor" || name == "ceil" || name == "round" || name == "min" || name == "max" || name == "address" || name == "size" ||
			name == "typeof" || name == "push" || name == "pop" || name == "sort" || name == "file_read_all_text" || name == "file_write_all_text" || name == "file_append_all_text" ||
			name == "file_exists" || name == "file_delete" || name == "file_copy" || name == "file_move" || name == "atan2");
}

std::size_t Compiler::resolve_native(const std::string& name) const {
	auto it = module.native_lookup.find(name);

	if (it == module.native_lookup.end()) {
		throw std::runtime_error("Unknown native function: " + name);
	}

	return it->second;
}

bool Compiler::is_builtin_constant(const std::string& name) const {
	return (name == "pi" || name == "e" || name == "tau" || name == "golden_ratio");
}

Value Compiler::get_builtin_constant(const std::string& name) const {
	if (name == "pi") {
		return Value(3.14159265358979323846);
	}

	if (name == "e") {
		return Value(2.71828182845904523536);
	}

	if (name == "tau") {
		return Value(6.28318530717958647692);
	}

	if (name == "golden_ratio") {
		return Value(1.61803398874989484820);
	}

	throw std::runtime_error("Unknown builtin constant: " + name);
}

void Compiler::patch_absolute_jump(std::size_t operand_position, std::size_t target) {
	if (operand_position + 1 >= chunk().code_size()) {
		throw std::runtime_error("Invalid jump operand position");
	}

	if (target < operand_position + 2) {
		throw std::runtime_error("patch_absolute_jump currently only "
								 "supports forward jumps");
	}

	const std::size_t offset = target - operand_position - 2;

	if (offset > std::numeric_limits<std::uint16_t>::max()) {
		throw std::runtime_error("Jump offset too large");
	}

	auto& code = chunk().mutable_code();

	code[operand_position] = static_cast<std::uint8_t>(offset & 0xff);

	code[operand_position + 1] = static_cast<std::uint8_t>((offset >> 8) & 0xff);
}
std::size_t Compiler::emit_exception_handler_placeholder() {
	emit_opcode(OpCode::PushExceptionHandler);

	emit_u16(0xffff);

	return (chunk().code_size() - 2);
}

void Compiler::patch_exception_handler(std::size_t handler_position, std::size_t target) {
	if (target < handler_position + 2) {
		throw std::runtime_error("Invalid exception handler target");
	}

	const std::size_t offset = target - handler_position - 2;

	if (offset > std::numeric_limits<std::uint16_t>::max()) {
		throw std::runtime_error("Exception handler offset too large");
	}

	auto& code = chunk().mutable_code();

	code[handler_position] = static_cast<std::uint8_t>(offset & 0xff);

	code[handler_position + 1] = static_cast<std::uint8_t>((offset >> 8) & 0xff);
}

void Compiler::register_natives() {
	auto register_native = [this](const std::string& name, std::size_t arity) {
		const std::size_t index = module.natives.size();

		NativeFunctionInfo info;

		info.name = name;
		info.arity = arity;

		module.native_lookup[name] = index;

		module.natives.push_back(std::move(info));
	};

	register_native("print", 1);
	register_native("println", 1);

	register_native("sqrt", 1);
	register_native("abs", 1);
	register_native("cube_root", 1);
	register_native("exp", 1);
	register_native("log", 1);
	register_native("sin", 1);
	register_native("cos", 1);
	register_native("tan", 1);
	register_native("factorial", 1);
	register_native("x_root", 2);
	register_native("atan2", 2);

	register_native("pow", 2);

	register_native("floor", 1);
	register_native("ceil", 1);
	register_native("round", 1);

	register_native("min", 2);
	register_native("max", 2);

	register_native("address", 1);
	register_native("size", 1);
	register_native("typeof", 1);
	register_native("push", 2);
	register_native("pop", 1);
	register_native("sort", 1);

	register_native("file_read_all_text", 1);
	register_native("file_write_all_text", 2);
	register_native("file_append_all_text", 2);
	register_native("file_exists", 1);
	register_native("file_delete", 1);
	register_native("file_copy", 2);
	register_native("file_move", 2);
}
}