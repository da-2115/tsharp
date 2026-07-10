// Interpreter.cpp
// T# v1.0.0-betarc (v1.0.0 Beta Release Candidate/Final Beta)
// Dylan Armstrong, 2026

#include "Interpreter.h"

#include <algorithm>
#include <any>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>

#include "TSharpLexer.h"
#include "TSharpParser.h"
#include "antlr4-runtime.h"

#include "FunctionValue.h"
#include "InstanceValue.h"

// Use the antlr4 namespace
using namespace antlr4;

namespace tsharp {

// Interpreter constructor implementation - uses constructor body instead of member initializer list
Interpreter::Interpreter() {
	globals = std::make_shared<Environment>();
	env = globals;
	install_builtins(globals);
}

// Execute the interpreter - load the program and then run the main function
void Interpreter::execute(TSharpParser::ProgramContext* program) {
	load(program);
	run_main();
}

void Interpreter::load(TSharpParser::ProgramContext* program) {
	visit(program);
}

// Run the main function
void Interpreter::run_main() {
	// Get the main function from the global environment
	auto main_value = globals->get("main");

	// Call the main function, empty set of arguments - as the T# main function currently does not support any arguments
	call_function(main_value, {});
}

static std::string parameter_type_name(TSharpParser::ParameterContext* p) {
	std::string type_name = p->typeRef()->getText();

	if (p->arrayDeclarator()) {
		type_name += "[]";
	}

	return type_name;
}

// Get base type name as string
static std::string base_type_name(const std::string& type_name) {
	auto pos = type_name.find('<');

	if (pos == std::string::npos) {
		return type_name;
	}

	return type_name.substr(0, pos);
}

static std::string trim_copy(const std::string& text) {
	size_t start = 0;
	while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
		start++;
	}

	size_t end = text.size();
	while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
		end--;
	}

	return text.substr(start, end - start);
}

static std::string remove_import_lines(const std::string& source) {
	std::istringstream input(source);
	std::ostringstream output;
	std::string line;

	while (std::getline(input, line)) {
		std::string trimmed = trim_copy(line);

		if (trimmed.rfind("import ", 0) == 0) {
			continue;
		}

		output << line << '\n';
	}

	return output.str();
}

std::string Interpreter::read_file(const std::string& file_path) {
	std::ifstream file(file_path);

	if (!file.is_open()) {
		throw RuntimeError("Could not open file: " + file_path);
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

// Extract imports (when using import) for importing std T# library classes
std::vector<std::string> Interpreter::extract_imports(const std::string& source) {
	std::vector<std::string> imports;
	std::istringstream stream(source);
	std::string line;

	while (std::getline(stream, line)) {
		std::string trimmed = trim_copy(line);

		if (trimmed.rfind("import ", 0) != 0) {
			continue;
		}

		std::string import_name = trim_copy(trimmed.substr(7));

		if (!import_name.empty() && import_name.back() == ';') {
			import_name.pop_back();
			import_name = trim_copy(import_name);
		}

		if (!import_name.empty()) {
			imports.push_back(import_name);
		}
	}

	return imports;
}

// Resolve imports
std::string Interpreter::resolve_import(const std::string& import_name) {
	std::string relative_path = import_name;
	for (char& c : relative_path) {
		if (c == '.') {
			c = std::filesystem::path::preferred_separator;
		}
	}

	std::filesystem::path stdlib_path = std::filesystem::path("stdlib") / (relative_path + ".tsharp");

	if (std::filesystem::exists(stdlib_path)) {
		return std::filesystem::absolute(stdlib_path).string();
	}

	std::filesystem::path local_path = relative_path + ".tsharp";

	if (std::filesystem::exists(local_path)) {
		return std::filesystem::absolute(local_path).string();
	}

	throw RuntimeError("Could not resolve import: " + import_name);
}

void Interpreter::parse_and_execute_declarations(const std::string& source, const std::string& file_path) {
	auto unit = std::make_unique<ParsedUnit>();

	unit->source = remove_import_lines(source);
	unit->input = std::make_unique<antlr4::ANTLRInputStream>(unit->source);
	unit->lexer = std::make_unique<TSharpLexer>(unit->input.get());
	unit->tokens = std::make_unique<antlr4::CommonTokenStream>(unit->lexer.get());
	unit->parser = std::make_unique<TSharpParser>(unit->tokens.get());

	unit->parser->removeErrorListeners();
	unit->parser->addErrorListener(new antlr4::ConsoleErrorListener());

	unit->program = unit->parser->program();

	if (unit->parser->getNumberOfSyntaxErrors() > 0) {
		throw RuntimeError("Syntax error while parsing: " + file_path);
	}

	load(unit->program);

	parsed_units.push_back(std::move(unit));
}

// Load program files - from the main function
void Interpreter::load_program_files(const std::vector<std::string>& user_files) {
	std::unordered_set<std::string> loaded_files;

	// Iterate over each file
	for (const auto& file : user_files) {
		load_file_with_imports(file, loaded_files);
	}

	// Run main function
	run_main();
}

void Interpreter::load_file_with_imports(const std::string& file_path, std::unordered_set<std::string>& loaded_files) {
	std::string full_path = std::filesystem::absolute(file_path).string();

	if (loaded_files.contains(full_path)) {
		return;
	}

	loaded_files.insert(full_path);

	std::string source = read_file(full_path);

	auto imports = extract_imports(source);

	for (const auto& import_name : imports) {
		std::string import_path = resolve_import(import_name);

		load_file_with_imports(import_path, loaded_files);
	}

	parse_and_execute_declarations(source, full_path);
}

// Call a function in T#
Value Interpreter::call_function(const Value& callee, const std::vector<Value>& args, const Value& thisValue) {
	auto fn = callee.as_function();

	// Native/built-in function
	if (fn->is_native) {
		return fn->native(*this, args, thisValue);
	}

	// Argument count validation
	if (args.size() != fn->params.size()) {
		throw RuntimeError("Function " + fn->name + " expected " + std::to_string(fn->params.size()) +
						   " arguments but got " + std::to_string(args.size()));
	}

	auto local = std::make_shared<Environment>(fn->closure ? fn->closure : globals);

	// Bind this/base for instance methods
	if (!thisValue.is_null()) {
		local->define("this", thisValue);

		if (thisValue.is_instance()) {
			auto inst = thisValue.as_instance();

			if (!inst->class_val->base_class_name.empty()) {
				local->define("base", thisValue);
			}
		}
	}

	// Bind parameters
	for (size_t i = 0; i < fn->params.size(); ++i) {
		const auto& param = fn->params.at(i);

		require_assignable(args.at(i), param.type_name, "argument '" + param.name + "' of function " + fn->name);

		local->define(param.name, args.at(i));
	}

	auto previous = env;
	env = local;

	try {
		auto* block = static_cast<TSharpParser::BlockContext*>(fn->body_node);

		exec_result.flow = ControlFlow::NORMAL;
		exec_result.value = Value();

		visit(block);

		Value result_value;

		if (exec_result.flow == ControlFlow::RETURN_VALUE) {
			result_value = exec_result.value;
			exec_result.flow = ControlFlow::NORMAL;
			exec_result.value = Value();
		} else {
			result_value = Value();
		}

		bool is_generic_return =
			fn->return_type.size() == 1 && std::isupper(static_cast<unsigned char>(fn->return_type[0]));

		if (fn->return_type == "void") {
			if (!result_value.is_null()) {
				throw RuntimeError("Void function " + fn->name + " cannot return a value");
			}
		} else {
			if (result_value.is_null() && !is_generic_return) {
				throw RuntimeError("Function " + fn->name + " must return " + fn->return_type);
			}

			if (!result_value.is_null()) {
				require_assignable(result_value, fn->return_type, "return value of function " + fn->name);
			}
		}

		env = previous;
		return result_value;
	}

	catch (const ReturnSignal& r) {
		env = previous;
		return r.value;
	}

	catch (...) {
		env = previous;
		throw;
	}
}

// Evaluate parse tree node
Value Interpreter::evaluate(tree::ParseTree* node) {
	// If node is a nullptr
	if (!node) {
		return Value();
	}

	// Otherwise
	return std::any_cast<Value>(visit(node));
}

// Globals getter
std::shared_ptr<Environment> Interpreter::get_globals() const {
	return globals;
}

// Environment getter
std::shared_ptr<Environment> Interpreter::get_environment() const {
	return env;
}

// Environment setter
void Interpreter::set_environment(const std::shared_ptr<Environment>& env) {
	this->env = env;
}

// Get class by name
std::shared_ptr<ClassValue> Interpreter::get_class_by_name(const std::string& name) const {
	// Find class
	auto it = classes.find(name);

	// If no classes
	if (it == classes.end()) {
		return nullptr;
	}

	// Return class name
	return it->second;
}

// Register class
void Interpreter::register_class(const std::shared_ptr<ClassValue>& class_val) {
	classes[class_val->name] = class_val;
	globals->define(class_val->name, Value(class_val));
}

std::shared_ptr<FunctionValue> Interpreter::build_function(TSharpParser::FunctionDeclContext* ctx, bool is_method) {
	auto fn = std::make_shared<FunctionValue>();

	fn->name = ctx->IDENTIFIER()->getText();
	fn->return_type = ctx->returnType()->getText();
	fn->closure = env;
	fn->body_node = ctx->block();
	fn->is_method = is_method;

	if (ctx->parameterList()) {
		for (auto* p : ctx->parameterList()->parameter()) {
			fn->params.push_back({parameter_type_name(p), p->IDENTIFIER()->getText()});
		}
	}

	if (ctx->modifiers()) {
		for (auto* mod : ctx->modifiers()->modifier()) {
			auto text = mod->getText();

			fn->is_static = fn->is_static || text == "static";
			fn->is_virtual = fn->is_virtual || text == "virtual";
			fn->is_override = fn->is_override || text == "override";
			fn->is_abstract = fn->is_abstract || text == "abstract";
		}
	}

	return fn;
}

std::shared_ptr<FunctionValue> Interpreter::build_method(TSharpParser::MethodDeclContext* ctx) {
	auto fn = std::make_shared<FunctionValue>();

	fn->name = ctx->IDENTIFIER()->getText();
	fn->return_type = ctx->returnType()->getText();
	fn->closure = env;
	fn->body_node = ctx->block();
	fn->is_method = true;

	if (ctx->parameterList()) {
		for (auto* p : ctx->parameterList()->parameter()) {
			fn->params.push_back({parameter_type_name(p), p->IDENTIFIER()->getText()});
		}
	}

	if (ctx->modifiers()) {
		for (auto* mod : ctx->modifiers()->modifier()) {
			auto text = mod->getText();

			fn->is_static = fn->is_static || text == "static";
			fn->is_virtual = fn->is_virtual || text == "virtual";
			fn->is_override = fn->is_override || text == "override";
			fn->is_abstract = fn->is_abstract || text == "abstract";

			if (text == "private") {
				fn->is_private = true;
				fn->is_public = false;
			}

			if (text == "protected") {
				fn->is_protected = true;
				fn->is_public = false;
			}
		}
	}

	return fn;
}

// Evaluate arguments
void Interpreter::eval_arguments(TSharpParser::ArgumentListContext* ctx, std::vector<Value>& out_args) {
	if (!ctx) {
		return;
	}

	for (auto* expr : ctx->expression()) {
		out_args.push_back(evaluate(expr));
	}
}

// Get a member
Value Interpreter::get_member(const Value& target, const std::string& name, bool from_base) {
	// Handle enum member access
	if (target.is_class()) {
		auto class_val = target.as_class();

		if (class_val->is_enum) {
			auto it = class_val->static_fields.find(name);

			if (it != class_val->static_fields.end()) {
				return it->second;
			}

			throw RuntimeError("Unknown enum member: " + name);
		}
	}

	if (target.is_instance()) {
		auto inst = target.as_instance();

		// Check if accessing from within the same instance (internal access allowed)
		bool is_internal_access = false;
		try {
			Value this_val = env->get("this");
			if (this_val.is_instance() && this_val.as_instance().get() == inst.get()) {
				is_internal_access = true;
			}
		} catch (...) {
		}

		auto fit = inst->fields.find(name);

		if (fit != inst->fields.end()) {
			// Check field access modifiers (but allow private access from within the class)
			auto meta_it = inst->class_val->field_metadata.find(name);

			if (meta_it != inst->class_val->field_metadata.end()) {
				if (meta_it->second.is_private && !is_internal_access) {
					throw RuntimeError("Cannot access private field: " + name);
				}
			}

			return fit->second;
		}

		std::shared_ptr<ClassValue> class_val = inst->class_val;

		if (from_base) {
			if (class_val->base_class_name.empty()) {
				throw RuntimeError("No base class_val for member lookup: " + name);
			}

			auto it = classes.find(class_val->base_class_name);

			if (it == classes.end()) {
				throw RuntimeError("Unknown base class_val: " + class_val->base_class_name);
			}

			class_val = it->second;
		}

		while (class_val) {
			// Use unified member lookup instead of separate method/property searches
			auto mem_it = class_val->member_lookup.find(name);
			if (mem_it != class_val->member_lookup.end()) {
				const MemberInfo& info = mem_it->second;

				// Check access modifiers
				if (info.field_meta.is_private && !is_internal_access) {
					throw RuntimeError("Cannot access private member: " + name);
				}

				if (info.type == MemberInfo::METHOD) {
					return Value(info.method);
				} else if (info.type == MemberInfo::PROPERTY) {
					auto previous = env;
					auto prop_env = std::make_shared<Environment>(globals);
					prop_env->define("this", target);

					if (!inst->class_val->base_class_name.empty()) {
						prop_env->define("base", target);
					}
					env = prop_env;

					try {
						Value result;
						if (info.property.is_arrow) {
							result = evaluate(static_cast<antlr4::tree::ParseTree*>(info.property.expr_node));
						} else {
							try {
								visit(static_cast<TSharpParser::BlockContext*>(info.property.body_node));
								result = Value();
							} catch (const ReturnSignal& r) {
								result = r.value;
							}
						}

						env = previous;
						return result;
					} catch (...) {
						env = previous;
						throw;
					}
				}
			}

			if (class_val->base_class_name.empty()) {
				break;
			}
			auto it = classes.find(class_val->base_class_name);
			if (it == classes.end()) {
				break;
			}
			class_val = it->second;
		}
	}

	throw RuntimeError("Unknown member: " + name);
}

// Construct object (call constructor)
Value Interpreter::construct_object(const std::string& type_name, const std::vector<Value>& args) {
	std::string runtime_type_name = base_type_name(type_name);
	Value class_val_val = env->get(runtime_type_name);
	auto class_val = class_val_val.as_class();

	auto instance = std::make_shared<InstanceValue>(class_val);

	// Initialise instance fields from class_val defaults
	copy_field_defaults(class_val, instance);

	auto ctor_it = class_val->constructors.find(args.size());

	if (ctor_it == class_val->constructors.end()) {
		if (args.empty() && class_val->constructors.empty()) {
			return Value(instance);
		}

		throw RuntimeError("No matching constructor for " + runtime_type_name + " with " + std::to_string(args.size()) +
						   " arguments");
	}

	call_function(Value(ctor_it->second), args, Value(instance));

	return Value(instance);
}

// Visitor implementation
// Start with the root AST node of program via visitProgram
antlrcpp::Any Interpreter::visitProgram(TSharpParser::ProgramContext* ctx) {
	for (auto* decl : ctx->topLevelDecl()) {
		visit(decl);
	}
	return Value();
}

// Visit function declaration
antlrcpp::Any Interpreter::visitFunctionDecl(TSharpParser::FunctionDeclContext* ctx) {
	auto fn = build_function(ctx, false);
	functions[fn->name] = fn;
	globals->define(fn->name, Value(fn));
	return Value(fn);
}

// Visit class declaration
antlrcpp::Any Interpreter::visitClassDecl(TSharpParser::ClassDeclContext* ctx) {
	auto class_val = std::make_shared<ClassValue>();
	class_val->name = ctx->IDENTIFIER()->getText();
	if (ctx->modifiers()) {
		for (auto* mod : ctx->modifiers()->modifier()) {
			if (mod->getText() == "abstract") {
				class_val->is_abstract = true;
			}
		}
	}

	if (ctx->genericParams()) {
		for (auto* id : ctx->genericParams()->IDENTIFIER()) {
			class_val->generic_names.push_back(id->getText());
		}
	}

	if (ctx->inheritanceClause()) {
		auto types = ctx->inheritanceClause()->typeRef();

		if (!types.empty()) {
			class_val->base_class_name = types[0]->getText();
			for (size_t i = 1; i < types.size(); ++i) {
				class_val->interfaces.push_back(types[i]->getText());
			}
		}
	}

	register_class(class_val);

	for (auto* member : ctx->classBody()->classMember()) {
		if (auto* field = member->fieldDecl()) {
			auto name = field->IDENTIFIER()->getText();

			Value initial;

			if (field->arrayDeclarator()) {
				int size = 0;

				if (field->arrayDeclarator()->expression()) {
					size = evaluate(field->arrayDeclarator()->expression()).as_int();
				}

				auto arr = std::make_shared<Array>();

				if (size > 0) {
					arr->resize(size, Value());
				}

				initial = Value(arr);
			} else {
				initial = field->expression() ? evaluate(field->expression()) : Value();
			}
			bool is_static = false;
			bool is_private = false;
			bool is_protected = false;
			if (field->modifiers()) {
				for (auto* mod : field->modifiers()->modifier()) {
					auto text = mod->getText();
					if (text == "static") {
						is_static = true;
					}
					if (text == "private") {
						is_private = true;
					}
					if (text == "protected") {
						is_protected = true;
					}
				}
			}
			if (is_static) {
				class_val->static_fields[name] = initial;
			} else {
				class_val->field_defaults[name] = initial;
			}

			FieldInfo field_info;
			field_info.type_name = field->typeRef()->getText();

			if (field->arrayDeclarator()) {
				field_info.type_name += "[]";
			}
			field_info.is_static = is_static;
			field_info.is_private = is_private;
			field_info.is_protected = is_protected;
			field_info.is_public = !is_private && !is_protected;
			class_val->field_metadata[name] = field_info;
		}

		if (auto* prop = member->propertyDecl()) {
			PropertyValue p;
			p.name = prop->IDENTIFIER()->getText();
			p.type_name = prop->typeRef()->getText();

			if (prop->ARROW()) {
				p.is_arrow = true;
				p.expr_node = prop->expression();
			}

			else {
				p.body_node = prop->block();
			}

			if (prop->modifiers()) {
				for (auto* mod : prop->modifiers()->modifier()) {
					auto text = mod->getText();

					if (text == "private") {
						p.is_private = true;
						p.is_public = false;
					}

					if (text == "protected") {
						p.is_protected = true;
						p.is_public = false;
					}
				}
			}

			class_val->properties[p.name] = p;

			MemberInfo member_info;
			member_info.type = MemberInfo::PROPERTY;
			member_info.property = p;
			member_info.field_meta.type_name = p.type_name;
			member_info.field_meta.is_private = p.is_private;
			member_info.field_meta.is_protected = p.is_protected;
			member_info.field_meta.is_public = p.is_public;
			class_val->member_lookup[p.name] = member_info;
		}

		if (auto* method = member->methodDecl()) {
			auto fn = build_method(method);
			class_val->methods[fn->name] = fn;

			MemberInfo member_info;
			member_info.type = MemberInfo::METHOD;
			member_info.method = fn;
			member_info.field_meta.is_private = fn->is_private;
			member_info.field_meta.is_protected = fn->is_protected;
			member_info.field_meta.is_public = fn->is_public;
			class_val->member_lookup[fn->name] = member_info;
		}

		if (auto* ctor = member->constructorDecl()) {
			auto fn = std::make_shared<FunctionValue>();
			fn->name = "__ctor";
			fn->return_type = "void";
			fn->closure = env;
			fn->body_node = ctor->block();
			fn->is_method = true;
			if (ctor->parameterList()) {
				for (auto* p : ctor->parameterList()->parameter()) {
					fn->params.push_back({parameter_type_name(p), p->IDENTIFIER()->getText()});
				}
			}

			class_val->constructors[fn->params.size()] = fn;
		}
	}

	return Value(class_val);
}

// Visit interface declaration
antlrcpp::Any Interpreter::visitInterfaceDecl(TSharpParser::InterfaceDeclContext* ctx) {
	auto class_val = std::make_shared<ClassValue>();
	class_val->name = ctx->IDENTIFIER()->getText();
	class_val->is_interface = true;
	register_class(class_val);
	return Value(class_val);
}

// Visit enum declaration
antlrcpp::Any Interpreter::visitEnumDecl(TSharpParser::EnumDeclContext* ctx) {
	auto enum_val = std::make_shared<ClassValue>();
	enum_val->name = ctx->IDENTIFIER()->getText();
	enum_val->is_enum = true;

	// Parse enum members with optional explicit values
	int next_value = 0;
	for (auto* member : ctx->enumMember()) {
		std::string member_name = member->IDENTIFIER()->getText();
		int member_value = next_value;

		// Check if member has explicit value assignment (INTEGER_LITERAL)
		if (member->INTEGER_LITERAL()) {
			member_value = std::stoi(member->INTEGER_LITERAL()->getText());
		}

		// Store enum member as static field with integer value
		enum_val->static_fields[member_name] = Value(member_value);
		next_value = member_value + 1;
	}

	// Register enum as a type
	register_class(enum_val);

	return Value(enum_val);
}

// Visit variable statement
antlrcpp::Any Interpreter::visitVariableStatement(TSharpParser::VariableStatementContext* ctx) {
	return visit(ctx->variableDecl());
}

// Visit variable declaration
antlrcpp::Any Interpreter::visitVariableDecl(TSharpParser::VariableDeclContext* ctx) {
	std::string type_name = ctx->typeRef()->getText();

	if (ctx->arrayDeclarator()) {
		type_name += "[]";
	}

	std::string var_name = ctx->IDENTIFIER()->getText();

	Value value;

	// Static array declaration like: int arr[4]
	if (ctx->arrayDeclarator()) {
		int size = 0;
		if (ctx->arrayDeclarator()->expression()) {
			size = evaluate(ctx->arrayDeclarator()->expression()).as_int();
		}

		auto arr = std::make_shared<Array>();
		if (size > 0) {
			arr->resize(size, Value());
		}

		value = Value(arr);

		// Optional initializer wins if present
		if (ctx->variableInitializer()) {
			auto* init = ctx->variableInitializer();

			if (init->ASSIGN()) {
				value = evaluate(init->expression());
			} else {
				std::vector<Value> args;
				if (init->argumentList()) {
					for (auto* expr : init->argumentList()->expression()) {
						args.push_back(evaluate(expr));
					}
				}
				value = construct_object(type_name, args);
			}
		}
	} else if (ctx->variableInitializer()) {
		auto* init = ctx->variableInitializer();

		if (init->ASSIGN()) {
			value = evaluate(init->expression());
		} else {
			std::vector<Value> args;
			if (init->argumentList()) {
				for (auto* expr : init->argumentList()->expression()) {
					args.push_back(evaluate(expr));
				}
			}
			value = construct_object(type_name, args);
		}
	} else {
		value = Value();
	}

	if (!value.is_null()) {
		require_assignable(value, type_name, "variable '" + var_name + "'");
	}

	env->define(var_name, value);
	return value;
}

// Visit assignment
antlrcpp::Any Interpreter::visitAssignment(TSharpParser::AssignmentContext* ctx) {
	auto rhs = evaluate(ctx->expression());
	std::string op = ctx->assignmentOperator()->getText();

	auto* lv = ctx->lvalue();

	// Simple variable assignment OR implicit this-field assignment
	if (lv->lvalueSuffix().empty()) {
		std::string name = lv->primaryLValue()->getText();

		bool is_this_field = false;
		Value this_value;
		Value current;

		// Prefer current-instance fields over captured/global variables
		try {
			this_value = env->get("this");

			if (this_value.is_instance()) {
				current = get_member(this_value, name);
				is_this_field = true;
			}
		} catch (const RuntimeError&) {
		}

		// If no this-field was found, use normal variable assignment
		if (!is_this_field) {
			try {
				current = env->get(name);
			} catch (const RuntimeError&) {
				throw RuntimeError("Undefined variable: " + name);
			}
		}

		auto assign_result = [&](const Value& result) -> Value {
			if (is_this_field) {
				set_member(this_value, name, result);
			} else {
				env->assign(name, result);
			}

			return result;
		};

		if (op == "=") {
			return assign_result(rhs);
		}

		if (op == "+=") {
			if (current.is_string() || rhs.is_string()) {
				return assign_result(Value(current.as_string() + rhs.as_string()));
			}

			if (current.is_number() && rhs.is_number()) {
				if (current.is_double() || current.is_float() || rhs.is_double() || rhs.is_float()) {
					return assign_result(Value(current.as_double() + rhs.as_double()));
				}

				return assign_result(Value(current.as_int() + rhs.as_int()));
			}

			throw RuntimeError("Invalid operands for +=");
		}

		if (op == "-=") {
			if (current.is_number() && rhs.is_number()) {
				if (current.is_double() || current.is_float() || rhs.is_double() || rhs.is_float()) {
					return assign_result(Value(current.as_double() - rhs.as_double()));
				}

				return assign_result(Value(current.as_int() - rhs.as_int()));
			}

			throw RuntimeError("Invalid operands for -=");
		}

		if (op == "*=") {
			if (current.is_number() && rhs.is_number()) {
				if (current.is_double() || current.is_float() || rhs.is_double() || rhs.is_float()) {
					return assign_result(Value(current.as_double() * rhs.as_double()));
				}

				return assign_result(Value(current.as_int() * rhs.as_int()));
			}

			throw RuntimeError("Invalid operands for *=");
		}

		if (op == "/=") {
			if (!current.is_number() || !rhs.is_number()) {
				throw RuntimeError("Invalid operands for /=");
			}

			if (rhs.as_double() == 0.0) {
				throw RuntimeError("Division by zero");
			}

			if (current.is_double() || current.is_float() || rhs.is_double() || rhs.is_float()) {
				return assign_result(Value(current.as_double() / rhs.as_double()));
			}

			return assign_result(Value(current.as_int() / rhs.as_int()));
		}

		if (op == "%=") {
			if (!current.is_number() || !rhs.is_number()) {
				throw RuntimeError("Invalid operands for %=");
			}

			if (current.is_double() || current.is_float() || rhs.is_double() || rhs.is_float()) {
				if (rhs.as_double() == 0.0) {
					throw RuntimeError("Modulo by zero");
				}

				return assign_result(Value(std::fmod(current.as_double(), rhs.as_double())));
			}

			if (rhs.as_int() == 0) {
				throw RuntimeError("Modulo by zero");
			}

			return assign_result(Value(current.as_int() % rhs.as_int()));
		}

		throw RuntimeError("Unsupported assignment operator: " + op);
	}

	// Resolve base target for member/index assignment
	Value target;
	std::string root = lv->primaryLValue()->getText();

	if (root == "this") {
		target = env->get("this");
	} else if (root == "base") {
		target = env->get("base");
	} else {
		try {
			target = env->get(root);
		} catch (const RuntimeError&) {
			Value this_value = env->get("this");
			target = get_member(this_value, root);
		}
	}

	auto suffixes = lv->lvalueSuffix();

	for (size_t i = 0; i + 1 < suffixes.size(); ++i) {
		auto* part = suffixes[i];

		if (part->DOT()) {
			target = get_member(target, part->IDENTIFIER()->getText());
		} else if (part->LBRACK()) {
			auto arr = target.as_array();
			int idx = evaluate(part->expression()).as_int();

			if (idx < 0 || idx >= static_cast<int>(arr->size())) {
				throw RuntimeError("Array index out of range");
			}

			target = arr->at(idx);
		}
	}

	auto* last = suffixes.back();

	if (last->DOT()) {
		std::string member_name = last->IDENTIFIER()->getText();

		auto assign_member_result = [&](const Value& result) -> Value {
			set_member(target, member_name, result);
			return result;
		};

		if (op == "=") {
			return assign_member_result(rhs);
		}

		Value current = get_member(target, member_name);

		if (op == "+=") {
			if (current.is_string() || rhs.is_string()) {
				return assign_member_result(Value(current.as_string() + rhs.as_string()));
			}

			if (current.is_number() && rhs.is_number()) {
				if (current.is_double() || current.is_float() || rhs.is_double() || rhs.is_float()) {
					return assign_member_result(Value(current.as_double() + rhs.as_double()));
				}

				return assign_member_result(Value(current.as_int() + rhs.as_int()));
			}

			throw RuntimeError("Invalid operands for +=");
		}

		if (op == "-=") {
			if (current.is_number() && rhs.is_number()) {
				if (current.is_double() || current.is_float() || rhs.is_double() || rhs.is_float()) {
					return assign_member_result(Value(current.as_double() - rhs.as_double()));
				}

				return assign_member_result(Value(current.as_int() - rhs.as_int()));
			}

			throw RuntimeError("Invalid operands for -=");
		}

		if (op == "*=") {
			if (current.is_number() && rhs.is_number()) {
				if (current.is_double() || current.is_float() || rhs.is_double() || rhs.is_float()) {
					return assign_member_result(Value(current.as_double() * rhs.as_double()));
				}

				return assign_member_result(Value(current.as_int() * rhs.as_int()));
			}

			throw RuntimeError("Invalid operands for *=");
		}

		if (op == "/=") {
			if (!current.is_number() || !rhs.is_number()) {
				throw RuntimeError("Invalid operands for /=");
			}

			if (rhs.as_double() == 0.0) {
				throw RuntimeError("Division by zero");
			}

			if (current.is_double() || current.is_float() || rhs.is_double() || rhs.is_float()) {
				return assign_member_result(Value(current.as_double() / rhs.as_double()));
			}

			return assign_member_result(Value(current.as_int() / rhs.as_int()));
		}

		if (op == "%=") {
			if (!current.is_number() || !rhs.is_number()) {
				throw RuntimeError("Invalid operands for %=");
			}

			if (current.is_double() || current.is_float() || rhs.is_double() || rhs.is_float()) {
				if (rhs.as_double() == 0.0) {
					throw RuntimeError("Modulo by zero");
				}

				return assign_member_result(Value(std::fmod(current.as_double(), rhs.as_double())));
			}

			if (rhs.as_int() == 0) {
				throw RuntimeError("Modulo by zero");
			}

			return assign_member_result(Value(current.as_int() % rhs.as_int()));
		}

		throw RuntimeError("Unsupported member assignment operator: " + op);
	}

	if (last->LBRACK()) {
		auto arr = target.as_array();
		int idx = evaluate(last->expression()).as_int();

		if (idx < 0 || idx >= static_cast<int>(arr->size())) {
			throw RuntimeError("Array index out of range");
		}

		auto assign_array_result = [&](const Value& result) -> Value {
			(*arr)[idx] = result;
			return result;
		};

		if (op == "=") {
			return assign_array_result(rhs);
		}

		Value current = (*arr)[idx];

		if (op == "+=") {
			if (current.is_number() && rhs.is_number()) {
				if (current.is_double() || current.is_float() || rhs.is_double() || rhs.is_float()) {
					return assign_array_result(Value(current.as_double() + rhs.as_double()));
				}

				return assign_array_result(Value(current.as_int() + rhs.as_int()));
			}

			throw RuntimeError("Invalid operands for +=");
		}

		if (op == "-=") {
			if (current.is_number() && rhs.is_number()) {
				if (current.is_double() || current.is_float() || rhs.is_double() || rhs.is_float()) {
					return assign_array_result(Value(current.as_double() - rhs.as_double()));
				}

				return assign_array_result(Value(current.as_int() - rhs.as_int()));
			}

			throw RuntimeError("Invalid operands for -=");
		}

		if (op == "*=") {
			if (current.is_number() && rhs.is_number()) {
				if (current.is_double() || current.is_float() || rhs.is_double() || rhs.is_float()) {
					return assign_array_result(Value(current.as_double() * rhs.as_double()));
				}

				return assign_array_result(Value(current.as_int() * rhs.as_int()));
			}

			throw RuntimeError("Invalid operands for *=");
		}

		if (op == "/=") {
			if (!current.is_number() || !rhs.is_number()) {
				throw RuntimeError("Invalid operands for /=");
			}

			if (rhs.as_double() == 0.0) {
				throw RuntimeError("Division by zero");
			}

			if (current.is_double() || current.is_float() || rhs.is_double() || rhs.is_float()) {
				return assign_array_result(Value(current.as_double() / rhs.as_double()));
			}

			return assign_array_result(Value(current.as_int() / rhs.as_int()));
		}

		if (op == "%=") {
			if (!current.is_number() || !rhs.is_number()) {
				throw RuntimeError("Invalid operands for %=");
			}

			if (current.is_double() || current.is_float() || rhs.is_double() || rhs.is_float()) {
				if (rhs.as_double() == 0.0) {
					throw RuntimeError("Modulo by zero");
				}

				return assign_array_result(Value(std::fmod(current.as_double(), rhs.as_double())));
			}

			if (rhs.as_int() == 0) {
				throw RuntimeError("Modulo by zero");
			}

			return assign_array_result(Value(current.as_int() % rhs.as_int()));
		}

		throw RuntimeError("Unsupported array assignment operator: " + op);
	}

	throw RuntimeError("Unsupported assignment target");
}
// Visit a block in T#
antlrcpp::Any Interpreter::visitBlock(TSharpParser::BlockContext* ctx) {
	auto previous = env;
	env = std::make_shared<Environment>(previous);
	try {
		for (auto* stmt : ctx->statement()) {
			visit(stmt);

			if (exec_result.flow != ControlFlow::NORMAL) {
				break;
			}
		}

		env = previous;

		return Value();
	} catch (...) {
		env = previous;
		throw;
	}
}

// Set a member
void Interpreter::set_member(const Value& target, const std::string& name, const Value& value) {
	if (target.is_instance()) {
		auto inst = target.as_instance();

		auto fit = inst->fields.find(name);
		if (fit != inst->fields.end()) {
			auto meta_it = inst->class_val->field_metadata.find(name);

			if (meta_it != inst->class_val->field_metadata.end()) {
				require_assignable(value, meta_it->second.type_name, "field '" + name + "'");
			}
			fit->second = value;
			return;
		}

		std::shared_ptr<ClassValue> class_val = inst->class_val;
		while (class_val) {
			auto pit = class_val->properties.find(name);
			if (pit != class_val->properties.end()) {
				throw RuntimeError("Property is read-only: " + name);
			}

			if (class_val->base_class_name.empty()) {
				break;
			}

			auto it = classes.find(class_val->base_class_name);
			if (it == classes.end()) {
				break;
			}

			class_val = it->second;
		}

		throw RuntimeError("Unknown field: " + name);
	}

	if (target.is_class()) {
		auto class_val = target.as_class();

		auto fit = class_val->static_fields.find(name);
		if (fit != class_val->static_fields.end()) {
			fit->second = value;
			return;
		}

		throw RuntimeError("Unknown static member: " + name);
	}

	throw RuntimeError("Cannot assign member on non-object target: " + name);
}

// Visit if statement
antlrcpp::Any Interpreter::visitIfStatement(TSharpParser::IfStatementContext* ctx) {
	size_t block_index = 0;
	if (evaluate(ctx->expression(0)).as_bool()) {
		return visit(ctx->block(block_index));
	}
	++block_index;
	for (size_t i = 1; i < ctx->expression().size(); ++i, ++block_index) {
		if (evaluate(ctx->expression(i)).as_bool()) {
			return visit(ctx->block(block_index));
		}
	}
	if (!ctx->ELSE().empty()) {
		return visit(ctx->block(block_index));
	}
	return Value();
}

// Visit while statement
antlrcpp::Any Interpreter::visitWhileStatement(TSharpParser::WhileStatementContext* ctx) {
	while (evaluate(ctx->expression()).as_bool()) {
		exec_result.flow = ControlFlow::NORMAL;
		visit(ctx->block());
		// Check status after block
		if (exec_result.flow == ControlFlow::BREAK) {
			exec_result.flow = ControlFlow::NORMAL;
			break;
		}
		if (exec_result.flow == ControlFlow::CONTINUE) {
			exec_result.flow = ControlFlow::NORMAL;
			continue;
		}
		if (exec_result.flow == ControlFlow::RETURN_VALUE) {
			break; // Return out of loop
		}
	}
	return Value();
}

// Visit do while statement
antlrcpp::Any Interpreter::visitDoWhileStatement(TSharpParser::DoWhileStatementContext* ctx) {
	do {
		exec_result.flow = ControlFlow::NORMAL;

		visit(ctx->block());

		if (exec_result.flow == ControlFlow::BREAK) {
			exec_result.flow = ControlFlow::NORMAL;
			break;
		}

		if (exec_result.flow == ControlFlow::CONTINUE) {
			exec_result.flow = ControlFlow::NORMAL;
		}

		if (exec_result.flow == ControlFlow::RETURN_VALUE) {
			break;
		}
	} while (evaluate(ctx->expression()).as_bool());
	return Value();
}

// Visit for statement/loop
antlrcpp::Any Interpreter::visitForStatement(TSharpParser::ForStatementContext* ctx) {
	auto previous = env;
	env = std::make_shared<Environment>(previous);

	if (ctx->forInit()) {
		visit(ctx->forInit());
	}

	while (!ctx->expression() || evaluate(ctx->expression()).as_bool()) {
		exec_result.flow = ControlFlow::NORMAL; // Reset flow status
		visit(ctx->block());
		//  Check status after block
		if (exec_result.flow == ControlFlow::BREAK) {
			exec_result.flow = ControlFlow::NORMAL;
			break;
		}
		if (exec_result.flow == ControlFlow::CONTINUE) {
			exec_result.flow = ControlFlow::NORMAL;
			// Fall through to update
		}
		if (exec_result.flow == ControlFlow::RETURN_VALUE) {
			break; // Return out of loop
		}
		if (ctx->forUpdate()) {
			visit(ctx->forUpdate());
		}
	}
	env = previous;
	return Value();
}

// Visit switch statement
antlrcpp::Any Interpreter::visitSwitchStatement(TSharpParser::SwitchStatementContext* ctx) {
	Value subject = evaluate(ctx->expression());
	bool matched = false;
	for (auto* section : ctx->switchSection()) {
		bool is_default = section->DEFAULT() != nullptr;
		if (!matched && !is_default && values_equal(subject, evaluate(section->expression()))) {
			matched = true;
		}
		if (!matched && is_default) {
			matched = true;
		}
		if (matched) {
			exec_result.flow = ControlFlow::NORMAL; // Reset flow status
			for (auto* stmt : section->statement()) {
				visit(stmt);
			}
			// Check status after statements
			if (exec_result.flow == ControlFlow::BREAK) {
				exec_result.flow = ControlFlow::NORMAL;
				break;
			}
			if (exec_result.flow == ControlFlow::RETURN_VALUE) {
				break; // Return out of switch
			}
		}
	}
	return Value();
}

// Visit break statement
antlrcpp::Any Interpreter::visitBreakStatement(TSharpParser::BreakStatementContext*) {
	exec_result.flow = ControlFlow::BREAK;

	return Value();
}

// Visit continue statement
antlrcpp::Any Interpreter::visitContinueStatement(TSharpParser::ContinueStatementContext*) {
	exec_result.flow = ControlFlow::CONTINUE;

	return Value();
}

// Visit return statement
antlrcpp::Any Interpreter::visitReturnStatement(TSharpParser::ReturnStatementContext* ctx) {
	exec_result.value = ctx->expression() ? evaluate(ctx->expression()) : Value();

	exec_result.flow = ControlFlow::RETURN_VALUE;

	return Value();
}

// Visit throw statement
antlrcpp::Any Interpreter::visitThrowStatement(TSharpParser::ThrowStatementContext* ctx) {
	throw ThrowSignal(evaluate(ctx->expression()));
}

// Visit try statement
antlrcpp::Any Interpreter::visitTryStatement(TSharpParser::TryStatementContext* ctx) {
	try {
		visit(ctx->block());
	} catch (const ThrowSignal& ts) {
		bool handled = false;
		for (auto* c : ctx->catchClause()) {
			auto local = std::make_shared<Environment>(env);
			local->define(c->IDENTIFIER()->getText(), ts.value);
			auto previous = env;
			env = local;
			try {
				visit(c->block());
				handled = true;
			} catch (...) {
				env = previous;
				throw;
			}
			env = previous;
			if (handled) {
				break;
			}
		}
		if (!handled) {
			throw;
		}
	}
	if (ctx->finallyClause()) {
		visit(ctx->finallyClause()->block());
	}
	return Value();
}

// Visit expression statement
antlrcpp::Any Interpreter::visitExpressionStatement(TSharpParser::ExpressionStatementContext* ctx) {
	return evaluate(ctx->expression());
}

// Visit expression
antlrcpp::Any Interpreter::visitExpression(TSharpParser::ExpressionContext* ctx) {
	return visit(ctx->logicalOrExpression());
}

// Visit OR expression
antlrcpp::Any Interpreter::visitLogicalOrExpression(TSharpParser::LogicalOrExpressionContext* ctx) {
	Value result = evaluate(ctx->logicalAndExpression(0));
	for (size_t i = 1; i < ctx->logicalAndExpression().size(); ++i) {
		if (result.as_bool()) {
			return Value(true);
		}
		result = Value(result.as_bool() || evaluate(ctx->logicalAndExpression(i)).as_bool());
	}
	return result;
}

// Visit AND expression
antlrcpp::Any Interpreter::visitLogicalAndExpression(TSharpParser::LogicalAndExpressionContext* ctx) {
	Value result = evaluate(ctx->equalityExpression(0));
	for (size_t i = 1; i < ctx->equalityExpression().size(); ++i) {
		if (!result.as_bool()) {
			return Value(false);
		}
		result = Value(result.as_bool() && evaluate(ctx->equalityExpression(i)).as_bool());
	}
	return result;
}

// Visit equality expression
antlrcpp::Any Interpreter::visitEqualityExpression(TSharpParser::EqualityExpressionContext* ctx) {
	Value result = evaluate(ctx->relationalExpression(0));
	for (size_t i = 1; i < ctx->relationalExpression().size(); ++i) {
		std::string op = ctx->children[2 * i - 1]->getText();
		Value rhs = evaluate(ctx->relationalExpression(i));
		result = Value(op == "==" ? values_equal(result, rhs) : !values_equal(result, rhs));
	}
	return result;
}

// Visit relational expression
antlrcpp::Any Interpreter::visitRelationalExpression(TSharpParser::RelationalExpressionContext* ctx) {
	Value result = evaluate(ctx->additiveExpression(0));
	for (size_t i = 1; i < ctx->additiveExpression().size(); ++i) {
		std::string op = ctx->children[2 * i - 1]->getText();
		Value rhs = evaluate(ctx->additiveExpression(i));
		if (op == "<") {
			result = Value(result.as_double() < rhs.as_double());
		} else if (op == "<=") {
			result = Value(result.as_double() <= rhs.as_double());
		} else if (op == ">") {
			result = Value(result.as_double() > rhs.as_double());
		} else if (op == ">=") {
			result = Value(result.as_double() >= rhs.as_double());
		}
	}
	return result;
}

// Visit additive expression
antlrcpp::Any Interpreter::visitAdditiveExpression(TSharpParser::AdditiveExpressionContext* ctx) {
	Value result = evaluate(ctx->multiplicativeExpression(0));

	for (size_t i = 1; i < ctx->multiplicativeExpression().size(); ++i) {
		std::string op = ctx->children[2 * i - 1]->getText();
		Value rhs = evaluate(ctx->multiplicativeExpression(i));

		if (op == "+") {
			if (result.is_string() || rhs.is_string()) {
				result = Value(result.as_string() + rhs.as_string());
			} else if (result.is_number() && rhs.is_number()) {
				if (result.is_double() || result.is_float() || rhs.is_double() || rhs.is_float()) {
					result = Value(result.as_double() + rhs.as_double());
				} else {
					result = Value(result.as_int() + rhs.as_int());
				}
			} else {
				throw RuntimeError("Invalid operands for +");
			}
		} else {
			if (result.is_number() && rhs.is_number()) {
				if (result.is_double() || result.is_float() || rhs.is_double() || rhs.is_float()) {
					result = Value(result.as_double() - rhs.as_double());
				} else {
					result = Value(result.as_int() - rhs.as_int());
				}
			} else {
				throw RuntimeError("Invalid operands for -");
			}
		}
	}

	return result;
}

// Visit multiplicative expression
antlrcpp::Any Interpreter::visitMultiplicativeExpression(TSharpParser::MultiplicativeExpressionContext* ctx) {
	Value result = evaluate(ctx->unaryExpression(0));

	for (size_t i = 1; i < ctx->unaryExpression().size(); ++i) {
		std::string op = ctx->children[2 * i - 1]->getText();
		Value rhs = evaluate(ctx->unaryExpression(i));

		if (op == "*") {
			if (result.is_number() && rhs.is_number()) {
				// If either operand is float/double, use double multiplication
				if (result.is_double() || result.is_float() || rhs.is_double() || rhs.is_float()) {
					result = Value(result.as_double() * rhs.as_double());
				} else {
					result = Value(result.as_int() * rhs.as_int());
				}
			} else {
				result = Value(result.as_double() * rhs.as_double());
			}
		} else if (op == "/") {
			if (rhs.as_double() == 0.0) {
				throw RuntimeError("Division by zero");
			}

			// If either operand is float/double, use double division
			if (result.is_double() || result.is_float() || rhs.is_double() || rhs.is_float()) {
				result = Value(result.as_double() / rhs.as_double());
			} else {
				result = Value(result.as_int() / rhs.as_int());
			}
		} else if (op == "%") {
			if (result.is_double() || result.is_float() || rhs.is_double() || rhs.is_float()) {
				if (rhs.as_double() == 0.0) {
					throw RuntimeError("Modulo by zero");
				}
				result = Value(std::fmod(result.as_double(), rhs.as_double()));
			} else {
				if (rhs.as_int() == 0) {
					throw RuntimeError("Modulo by zero");
				}
				result = Value(result.as_int() % rhs.as_int());
			}
		}
	}

	return result;
}

// Visit unary expression
antlrcpp::Any Interpreter::visitUnaryExpression(TSharpParser::UnaryExpressionContext* ctx) {
	if (ctx->postfixExpression()) {
		return visit(ctx->postfixExpression());
	}

	std::string op = ctx->children[0]->getText();

	// PREFIX ++ / --
	if (op == "++" || op == "--") {
		auto* inner = ctx->unaryExpression();

		// Only support simple variables for now
		if (inner->postfixExpression() && inner->postfixExpression()->primary()->IDENTIFIER()) {
			std::string name = inner->postfixExpression()->primary()->IDENTIFIER()->getText();

			Value current = env->get(name);

			Value result;
			if (op == "++") {
				result = current.is_int() ? Value(current.as_int() + 1) : Value(current.as_double() + 1);
			} else {
				result = current.is_int() ? Value(current.as_int() - 1) : Value(current.as_double() - 1);
			}

			env->assign(name, result);
			return result;
		}

		throw RuntimeError("Invalid operand for prefix " + op);
	}

	// Existing unary ops
	Value value = evaluate(ctx->unaryExpression());

	if (op == "-") {
		return value.is_int() ? Value(-value.as_int()) : Value(-value.as_double());
	}
	if (op == "+") {
		return value.is_int() ? Value(+value.as_int()) : Value(+value.as_double());
	}

	return Value(!value.as_bool());
}

// Visit postfix expression
antlrcpp::Any Interpreter::visitPostfixExpression(TSharpParser::PostfixExpressionContext* ctx) {
	Value current = evaluate(ctx->primary());
	Value pending_this = Value();
	bool from_base = ctx->primary()->BASE() != nullptr;

	auto suffixes = ctx->postfixSuffix();
	for (size_t i = 0; i < suffixes.size(); ++i) {
		auto* part = suffixes[i];

		if (part->DOT() && part->IDENTIFIER()) {
			std::string member_name = part->IDENTIFIER()->getText();
			// Check if next suffix is a function call (parentheses)
			bool is_method_call = false;
			if ((i + 1) < suffixes.size() && suffixes[i + 1]->LPAREN()) {
				is_method_call = true;
				i++; // Skip the next suffix since we're handling it here
			}

			// Handle cast methods on primitives
			if (current.is_number()) {
				if (member_name == "to_int") {
					current = Value(current.as_int());
					continue;
				} else if (member_name == "to_float") {
					current = Value(current.as_float());
					continue;
				} else if (member_name == "to_double") {
					current = Value(current.as_double());
					continue;
				} else if (member_name == "to_string") {
					current = Value(current.as_string());
					continue;
				} else if (member_name == "to_bool") {
					current = Value(current.as_double() != 0.0);
					continue;
				}
			} else if (current.is_bool()) {
				if (member_name == "to_int") {
					current = Value(current.as_bool() ? 1 : 0);
					continue;
				} else if (member_name == "to_string") {
					current = Value(current.as_bool() ? std::string("true") : std::string("false"));
					continue;
				}
			} else if (current.is_string()) {
				if (member_name == "to_int") {
					try {
						current = Value(std::stoi(current.as_string()));
					} catch (...) {
						throw RuntimeError("Cannot convert string to int");
					}
					continue;
				}
			}

			pending_this = current;
			current = get_member(current, member_name, from_base);
			from_base = false;

			if (is_method_call) {
				std::vector<Value> args;
				eval_arguments(suffixes[i]->argumentList(), args);

				current = call_function(current, args, pending_this);
				pending_this = Value();
			}
		} else if (part->LPAREN()) {
			if (from_base && current.is_instance()) {
				// Base constructor call
				auto inst = current.as_instance();
				auto class_val = inst->class_val;
				if (class_val->base_class_name.empty()) {
					throw RuntimeError("No base class to call constructor for");
				}

				auto base_class_it = classes.find(class_val->base_class_name);
				if (base_class_it == classes.end()) {
					throw RuntimeError("Unknown base class: " + class_val->base_class_name);
				}

				auto base_class = base_class_it->second;
				std::vector<Value> args;
				eval_arguments(part->argumentList(), args); // Fill by reference

				// O(1) constructor lookup by arity
				auto ctor_it = base_class->constructors.find(args.size());
				if (ctor_it == base_class->constructors.end()) {
					throw RuntimeError("No matching base constructor with " + std::to_string(args.size()) +
									   " arguments");
				}

				// Call the base constructor with the instance as 'this'
				call_function(Value(ctor_it->second), args, current);
				current = Value();
			} else {
				std::vector<Value> args;
				eval_arguments(part->argumentList(), args);

				if (current.is_class()) {
					auto class_val = current.as_class();
					current = construct_object(class_val->name, args);
				} else {
					current = call_function(current, args, pending_this);
				}
			}
			pending_this = Value();
			from_base = false;
		} else if (part->LBRACK()) {
			int idx = evaluate(part->expression()).as_int();
			auto arr = current.as_array();
			if (idx < 0 || idx >= static_cast<int>(arr->size())) {
				throw RuntimeError("Array index out of range");
			}
			current = (*arr)[idx];
			pending_this = Value();
			from_base = false;
		} else if (part->INC() || part->DEC()) {
			bool is_inc = part->INC() != nullptr;

			Value old_value;
			Value new_value;

			// Case 1: simple variable or implicit this-field: count++
			if (ctx->primary()->IDENTIFIER() && ctx->postfixSuffix().size() == 1) {
				std::string name = ctx->primary()->IDENTIFIER()->getText();

				try {
					old_value = env->get(name);

					if (is_inc) {
						new_value =
							old_value.is_int() ? Value(old_value.as_int() + 1) : Value(old_value.as_double() + 1);
					} else {
						new_value =
							old_value.is_int() ? Value(old_value.as_int() - 1) : Value(old_value.as_double() - 1);
					}

					env->assign(name, new_value);
					current = old_value;
				} catch (const RuntimeError&) {
					// fallback: this.name++
					Value this_value = env->get("this");
					old_value = get_member(this_value, name);

					if (is_inc) {
						new_value =
							old_value.is_int() ? Value(old_value.as_int() + 1) : Value(old_value.as_double() + 1);
					} else {
						new_value =
							old_value.is_int() ? Value(old_value.as_int() - 1) : Value(old_value.as_double() - 1);
					}

					set_member(this_value, name, new_value);
					current = old_value;
				}
			}

			// Case 2: this.count++ or obj.count++
			else if (ctx->postfixSuffix().size() == 2 && ctx->postfixSuffix(0)->DOT()) {
				Value target = evaluate(ctx->primary());
				std::string member_name = ctx->postfixSuffix(0)->IDENTIFIER()->getText();

				old_value = get_member(target, member_name);

				if (is_inc) {
					new_value = old_value.is_int() ? Value(old_value.as_int() + 1) : Value(old_value.as_double() + 1);
				} else {
					new_value = old_value.is_int() ? Value(old_value.as_int() - 1) : Value(old_value.as_double() - 1);
				}

				set_member(target, member_name, new_value);
				current = old_value;
			} else {
				throw RuntimeError("Invalid operand for postfix increment/decrement");
			}

			pending_this = Value();
			from_base = false;
		}
	}

	return current;
}

// Visit primary
antlrcpp::Any Interpreter::visitPrimary(TSharpParser::PrimaryContext* ctx) {
	if (ctx->literal()) {
		return visit(ctx->literal());
	}
	if (ctx->THIS()) {
		return env->get("this");
	}
	if (ctx->BASE()) {
		return env->get("base");
	}
	if (ctx->arrayLiteral()) {
		return visit(ctx->arrayLiteral());
	}
	if (ctx->expression()) {
		return evaluate(ctx->expression());
	}

	if (ctx->IDENTIFIER()) {
		std::string name = ctx->IDENTIFIER()->getText();

		try {
			return env->get(name);
		} catch (const RuntimeError&) {
			// Fallback to instance member lookup via this
			try {
				Value thisValue = env->get("this");
				return get_member(thisValue, name);
			} catch (const RuntimeError& e) {
				// Only convert "Unknown member" to "Undefined variable"
				// Allow access errors to propagate
				std::string msg = e.what();
				if (msg.find("Cannot access") != std::string::npos) {
					throw;
				}
				throw RuntimeError("Undefined variable: " + name);
			}
		}
	}

	return Value();
}

// Visit array lit
antlrcpp::Any Interpreter::visitArrayLiteral(TSharpParser::ArrayLiteralContext* ctx) {
	auto arr = std::make_shared<Array>();
	for (auto* expr : ctx->expression()) {
		arr->push_back(evaluate(expr));
	}
	return Value(arr);
}

// Visit literal
antlrcpp::Any Interpreter::visitLiteral(TSharpParser::LiteralContext* ctx) {
	if (ctx->INTEGER_LITERAL()) {
		return Value(std::stoi(ctx->INTEGER_LITERAL()->getText()));
	}
	if (ctx->FLOAT_LITERAL()) {
		std::string text = ctx->FLOAT_LITERAL()->getText();

		if (!text.empty() && (text.back() == 'f' || text.back() == 'F')) {
			text.pop_back();
			return Value(std::stof(text)); // float
		}

		return Value(std::stod(text)); // double
	}
	if (ctx->STRING_LITERAL()) {
		auto text = ctx->STRING_LITERAL()->getText();
		return Value(text.substr(1, text.size() - 2));
	}
	if (ctx->CHAR_LITERAL()) {
		auto text = ctx->CHAR_LITERAL()->getText();
		return Value(text.size() >= 3 ? text[1] : '\0');
	}
	if (ctx->TRUE()) {
		return Value(true);
	}
	if (ctx->FALSE()) {
		return Value(false);
	}
	return Value();
}

// Evaluate binary chain
Value Interpreter::eval_binary_chain(const std::vector<TSharpParser::AdditiveExpressionContext*>&) {
	return Value();
}

void Interpreter::copy_field_defaults(const std::shared_ptr<ClassValue>& class_val,
									  const std::shared_ptr<InstanceValue>& instance) {
	if (!class_val->base_class_name.empty()) {
		auto it = classes.find(class_val->base_class_name);
		if (it != classes.end()) {
			copy_field_defaults(it->second, instance);
		}
	}

	for (const auto& [name, value] : class_val->field_defaults) {
		instance->fields[name] = value;
	}
}
static std::string runtime_type_of(const Value& v) {
	if (v.is_null()) {
		return "null";
	}
	if (v.is_int()) {
		return "int";
	}
	if (v.is_float()) {
		return "float";
	}
	if (v.is_double()) {
		return "double";
	}
	if (v.is_bool()) {
		return "bool";
	}
	if (v.is_char()) {
		return "char";
	}
	if (v.is_string()) {
		return "string";
	}
	if (v.is_array()) {
		return "array";
	}
	if (v.is_function()) {
		return "function";
	}
	if (v.is_class()) {
		return "class";
	}

	if (v.is_instance()) {
		return v.as_instance()->class_val->name;
	}

	return "unknown";
}

// Static boolean helper for determining whether a given T# type is a numeric type e.g. int, float or double
static bool is_numeric_type(const std::string& t) {
	return t == "int" || t == "float" || t == "double";
}

// Is assignable to - static boolean helper
// This determines whether or not a value is assignable to a given type
// This is (for now) the type checking system
static bool is_assignable_to(const Value& value, const std::string& declared_type) {
	std::string target = base_type_name(declared_type);

	if (target == "any") {
		return true;
	}

	if (declared_type.size() == 1 && std::isupper(static_cast<unsigned char>(declared_type[0]))) {
		return true;
	}

	// arrays: int[], string[], DictionaryEntry<K,V>[], etc.
	if (declared_type.ends_with("[]")) {
		return value.is_array();
	}

	if (value.is_null()) {
		return !(target == "int" || target == "float" || target == "double" || target == "bool" || target == "char");
	}

	if (target == "int") {
		return value.is_int();
	}

	if (target == "float") {
		return value.is_float() || value.is_int();
	}

	if (target == "double") {
		return value.is_double() || value.is_float() || value.is_int();
	}

	if (target == "bool") {
		return value.is_bool();
	}
	if (target == "char") {
		return value.is_char();
	}
	if (target == "string") {
		return value.is_string();
	}

	if (target == "array") {
		return value.is_array();
	}

	if (value.is_instance()) {
		return value.as_instance()->class_val->name == target;
	}

	if (value.is_class()) {
		return value.as_class()->name == target;
	}

	return false;
}

void Interpreter::require_assignable(const Value& value, const std::string& declared_type, const std::string& where) {
	std::string target = base_type_name(declared_type);

	// Allow enum variables/fields/params to store enum integer values
	if (value.is_int()) {
		auto it = classes.find(target);

		if (it != classes.end() && it->second->is_enum) {
			return;
		}
	}

	if (!is_assignable_to(value, declared_type)) {
		throw RuntimeError("Type error in " + where + ": expected " + declared_type + ", got " +
						   runtime_type_of(value));
	}
}
}