// Interpreter.cpp
// Dylan Armstrong, 2026

#include "Interpreter.h"
#include <any>
#include <iostream>

using namespace antlr4;

namespace tsharp {

// Interpreter constructor implementation - uses constructor body instead of member initializer list
Interpreter::Interpreter() {
	globals = std::make_shared<Environment>();
	env = globals;
	install_builtins(globals);
}

// Execute method
// Call ANTLR visit method, with the program (root AST node in T#)
void Interpreter::execute(TSharpParser::ProgramContext* program) {
	visit(program);
	auto main_value = globals->get("main");
	call_function(main_value, {});
}

// Get base type name as string
static std::string base_type_name(const std::string& type_name) {
	auto pos = type_name.find('<');
	if (pos == std::string::npos) {
		return type_name;
	}
	return type_name.substr(0, pos);
}

// Call a function in T#
Value Interpreter::call_function(const Value& callee, const std::vector<Value>& args, const Value& thisValue) {
	// Get callee function as a function
	auto fn = callee.as_function();

	// If it's a native function, call the function!
	if (fn->is_native) {
		return fn->native(*this, args, thisValue);
	}

	// Otherwise define a local function
	auto local = std::make_shared<Environment>(fn->closure ? fn->closure : globals);

	if (!thisValue.is_null()) {
		local->define("this", thisValue);

		// If it's an instance function
		if (thisValue.is_instance()) {
			auto inst = thisValue.as_instance();
			if (!inst->class_val->base_class_name.empty()) {
				local->define("base", thisValue);
			}
		}
	}

	// Set parameters of function to arguments
	for (size_t i = 0; i < fn->params.size(); ++i) {
		local->define(fn->params[i].name, i < args.size() ? args[i] : Value());
	}

	auto previous = env;
	env = local;
	try {
		auto* block = static_cast<TSharpParser::BlockContext*>(fn->body_node);
		visit(block);
		env = previous;
		return Value();
	} catch (const ReturnSignal& r) {
		env = previous;
		return r.value;
	} catch (...) {
		env = previous;
		throw;
	}
}

// Evaluate parse tree node
Value Interpreter::evaluate(tree::ParseTree* node) {
	// If node is a nullptr
	if (!node)
		return Value();
	// Otherwise...
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
			fn->params.push_back({p->typeRef()->getText(), p->IDENTIFIER()->getText()});
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
			fn->params.push_back({p->typeRef()->getText(), p->IDENTIFIER()->getText()});
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

std::vector<Value> Interpreter::eval_arguments(TSharpParser::ArgumentListContext* ctx) {
	std::vector<Value> args;
	if (!ctx)
		return args;
	for (auto* expr : ctx->expression())
		args.push_back(evaluate(expr));
	return args;
}

// Get a member
Value Interpreter::get_member(const Value& target, const std::string& name, bool from_base) {
	
	if (target.is_instance()) {
		auto inst = target.as_instance();

		// Check if accessing from within the same instance (internal access allowed)
		bool is_internal_access = false;
		try {
			Value this_val = env->get("this");
			if (this_val.is_instance() && this_val.as_instance().get() == inst.get()) {
				is_internal_access = true;
			}
		} catch (...) {}

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
			auto mit = class_val->methods.find(name);
			if (mit != class_val->methods.end()) {
				if (mit->second->is_private && !is_internal_access) {
					throw RuntimeError("Cannot access private method: " + name);
				}
				return Value(mit->second);
			}

			auto pit = class_val->properties.find(name);
			if (pit != class_val->properties.end()) {
				if (pit->second.is_private && !is_internal_access) {
					throw RuntimeError("Cannot access private property: " + name);
				}
				auto previous = env;
				auto prop_env = std::make_shared<Environment>(env);
				prop_env->define("this", target);

				if (!inst->class_val->base_class_name.empty()) {
					prop_env->define("base", target);
				}
				env = prop_env;

				try {
					Value result;
					if (pit->second.is_arrow) {
						result = evaluate(static_cast<antlr4::tree::ParseTree*>(pit->second.expr_node));
					} else {
						try {
							visit(static_cast<TSharpParser::BlockContext*>(pit->second.body_node));
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

			if (class_val->base_class_name.empty())
				break;
			auto it = classes.find(class_val->base_class_name);
			if (it == classes.end())
				break;
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
	for (const auto& [name, value] : class_val->field_defaults) {
		instance->fields[name] = value;
	}

	// Find constructor by arity
	std::shared_ptr<FunctionValue> ctor = nullptr;
	for (const auto& [name, fn] : class_val->constructors) {
		if (fn && fn->params.size() == args.size()) {
			ctor = fn;
			break;
		}
	}

	if (ctor) {
		call_function(Value(ctor), args, Value(instance));
	}

	return Value(instance);
}

// Visitor implementation
// Start with the root AST node of program via visitProgram
antlrcpp::Any Interpreter::visitProgram(TSharpParser::ProgramContext* ctx) {
	for (auto* decl : ctx->topLevelDecl())
		visit(decl);
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
			if (mod->getText() == "abstract")
				class_val->is_abstract = true;
		}
	}
	if (ctx->genericParams()) {
		for (auto* id : ctx->genericParams()->IDENTIFIER())
			class_val->generic_names.push_back(id->getText());
	}
	if (ctx->inheritanceClause()) {
		auto types = ctx->inheritanceClause()->typeRef();
		if (!types.empty()) {
			class_val->base_class_name = types[0]->getText();
			for (size_t i = 1; i < types.size(); ++i)
				class_val->interfaces.push_back(types[i]->getText());
		}
	}

	register_class(class_val);

	for (auto* member : ctx->classBody()->classMember()) {
		if (auto* field = member->fieldDecl()) {
			auto name = field->IDENTIFIER()->getText();
			Value initial = field->expression() ? evaluate(field->expression()) : Value();
			bool is_static = false;
			bool is_private = false;
			bool is_protected = false;
			if (field->modifiers()) {
				for (auto* mod : field->modifiers()->modifier()) {
					auto text = mod->getText();
					if (text == "static")
						is_static = true;
					if (text == "private")
						is_private = true;
					if (text == "protected")
						is_protected = true;
				}
			}
			if (is_static)
				class_val->static_fields[name] = initial;
			else
				class_val->field_defaults[name] = initial;
			
			FieldInfo field_info;
			field_info.type_name = field->typeRef()->getText();
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
			} else {
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
		}
		if (auto* method = member->methodDecl()) {
			auto fn = build_method(method);
			class_val->methods[fn->name] = fn;
		}
		if (auto* ctor = member->constructorDecl()) {
			auto fn = std::make_shared<FunctionValue>();
			fn->name = "__ctor_" + std::to_string(class_val->constructors.size());
			fn->return_type = "void";
			fn->closure = env;
			fn->body_node = ctor->block();
			fn->is_method = true;
			if (ctor->parameterList()) {
				for (auto* p : ctor->parameterList()->parameter()) {
					fn->params.push_back({p->typeRef()->getText(), p->IDENTIFIER()->getText()});
				}
			}
			class_val->constructors[fn->name] = fn;
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
	// TODO: Implement enum declarations
	// For now, just parse and discard
	return Value();
}

// Visit variable statement
antlrcpp::Any Interpreter::visitVariableStatement(TSharpParser::VariableStatementContext* ctx) {
	return visit(ctx->variableDecl());
}

// Visit variable declaration
antlrcpp::Any Interpreter::visitVariableDecl(TSharpParser::VariableDeclContext* ctx) {
	std::string type_name = ctx->typeRef()->getText();
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

	env->define(var_name, value);
	return value;
}

// Visit assignment
antlrcpp::Any Interpreter::visitAssignment(TSharpParser::AssignmentContext* ctx) {
	auto rhs = evaluate(ctx->expression());
	std::string op = ctx->assignmentOperator()->getText();

	auto* lv = ctx->lvalue();

	// Simple variable assignment
	if (lv->lvalueSuffix().empty()) {
		std::string name = lv->primaryLValue()->getText();
		Value current = env->get(name);

		if (op == "=") {
			env->assign(name, rhs);
		} else if (op == "+=") {
			if (current.is_number() && rhs.is_number())
				env->assign(name, Value(current.as_int() + rhs.as_int()));
			else
				env->assign(name, Value(current.as_double() + rhs.as_double()));
		} else if (op == "-=") {
			if (current.is_number() && rhs.is_number())
				env->assign(name, Value(current.as_int() - rhs.as_int()));
			else
				env->assign(name, Value(current.as_double() - rhs.as_double()));
		} else if (op == "*=") {
			if (current.is_number() && rhs.is_number())
				env->assign(name, Value(current.as_int() * rhs.as_int()));
			else
				env->assign(name, Value(current.as_double() * rhs.as_double()));
		} else if (op == "/=") {
			if (rhs.as_double() == 0.0)
				throw RuntimeError("Division by zero");

			if (current.is_number() && rhs.is_number())
				env->assign(name, Value(static_cast<double>(current.as_int()) / rhs.as_int()));
			else
				env->assign(name, Value(current.as_double() / rhs.as_double()));
		} else if (op == "%=") {
			if (current.is_number() && rhs.is_number()) {
				if (rhs.as_int() == 0)
					throw RuntimeError("Modulo by zero");
				env->assign(name, Value(current.as_int() % rhs.as_int()));
			} else {
				env->assign(name, Value(std::fmod(current.as_double(), rhs.as_double())));
			}
		}

		return env->get(name);
	}

	// Resolve base target
	Value target;
	std::string root = lv->primaryLValue()->getText();
	target = env->get(root);

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
			target = (*arr)[idx];
		}
	}

	auto* last = suffixes.back();

	if (last->DOT()) {
		std::string member_name = last->IDENTIFIER()->getText();

		if (op == "=") {
			set_member(target, member_name, rhs);
			return rhs;
		}

		Value current = get_member(target, member_name);

		if (op == "+=") {
			if (current.is_string() || rhs.is_string()) {
				Value result(current.as_string() + rhs.as_string());
				set_member(target, member_name, result);
				return result;
			} else if (current.is_int() && rhs.is_int()) {
				Value result(current.as_int() + rhs.as_int());
				set_member(target, member_name, result);
				return result;
			} else {
				Value result(current.as_double() + rhs.as_double());
				set_member(target, member_name, result);
				return result;
			}
		} else if (op == "-=") {
			Value result = (current.is_int() && rhs.is_int()) ? Value(current.as_int() - rhs.as_int()) : Value(current.as_double() - rhs.as_double());
			set_member(target, member_name, result);
			return result;
		} else if (op == "*=") {
			Value result = (current.is_int() && rhs.is_int()) ? Value(current.as_int() * rhs.as_int()) : Value(current.as_double() * rhs.as_double());
			set_member(target, member_name, result);
			return result;
		} else if (op == "/=") {
			if (rhs.as_double() == 0.0) {
				throw RuntimeError("Division by zero");
			}
			Value result =
			    (current.is_int() && rhs.is_int()) ? Value(static_cast<double>(current.as_int()) / rhs.as_int()) : Value(current.as_double() / rhs.as_double());
			set_member(target, member_name, result);
			return result;
		} else if (op == "%=") {
			Value result;
			if (current.is_int() && rhs.is_int()) {
				if (rhs.as_int() == 0) {
					throw RuntimeError("Modulo by zero");
				}
				result = Value(current.as_int() % rhs.as_int());
			} else {
				result = Value(std::fmod(current.as_double(), rhs.as_double()));
			}
			set_member(target, member_name, result);
			return result;
		}

		throw RuntimeError("Unsupported member assignment operator: " + op);
	}

	if (last->LBRACK()) {
		auto arr = target.as_array();
		int idx = evaluate(last->expression()).as_int();
		if (idx < 0 || idx >= static_cast<int>(arr->size())) {
			throw RuntimeError("Array index out of range");
		}

		if (op == "=") {
			(*arr)[idx] = rhs;
			return rhs;
		}

		Value current = (*arr)[idx];

		if (op == "+=") {
			(*arr)[idx] = (current.is_int() && rhs.is_int()) ? Value(current.as_int() + rhs.as_int()) : Value(current.as_double() + rhs.as_double());
		} else if (op == "-=") {
			(*arr)[idx] = (current.is_int() && rhs.is_int()) ? Value(current.as_int() - rhs.as_int()) : Value(current.as_double() - rhs.as_double());
		} else if (op == "*=") {
			(*arr)[idx] = (current.is_int() && rhs.is_int()) ? Value(current.as_int() * rhs.as_int()) : Value(current.as_double() * rhs.as_double());
		} else if (op == "/=") {
			if (rhs.as_double() == 0.0) {
				throw RuntimeError("Division by zero");
			}
			(*arr)[idx] =
			    (current.is_int() && rhs.is_int()) ? Value(static_cast<double>(current.as_int()) / rhs.as_int()) : Value(current.as_double() / rhs.as_double());
		} else if (op == "%=") {
			if (current.is_int() && rhs.is_int()) {
				if (rhs.as_int() == 0) {
					throw RuntimeError("Modulo by zero");
				}
				(*arr)[idx] = Value(current.as_int() % rhs.as_int());
			} else {
				(*arr)[idx] = Value(std::fmod(current.as_double(), rhs.as_double()));
			}
		} else {
			throw RuntimeError("Unsupported array assignment operator: " + op);
		}

		return (*arr)[idx];
	}

	throw RuntimeError("Unsupported assignment target");
}

// Visit a block in T#
antlrcpp::Any Interpreter::visitBlock(TSharpParser::BlockContext* ctx) {
	auto previous = env;
	env = std::make_shared<Environment>(previous);
	try {
		for (auto* stmt : ctx->statement())
			visit(stmt);
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

		// if field not found, allow dynamic instance field creation
		inst->fields[name] = value;
		return;
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
	if (evaluate(ctx->expression(0)).as_bool())
		return visit(ctx->block(block_index));
	++block_index;
	for (size_t i = 1; i < ctx->expression().size(); ++i, ++block_index) {
		if (evaluate(ctx->expression(i)).as_bool())
			return visit(ctx->block(block_index));
	}
	if (!ctx->ELSE().empty())
		return visit(ctx->block(block_index));
	return Value();
}

// Visit while statement
antlrcpp::Any Interpreter::visitWhileStatement(TSharpParser::WhileStatementContext* ctx) {
	while (evaluate(ctx->expression()).as_bool()) {
		try {
			visit(ctx->block());
		} catch (const BreakSignal&) {
			break;
		} catch (const ContinueSignal&) {
			continue;
		}
	}
	return Value();
}

// Visit do while statement
antlrcpp::Any Interpreter::visitDoWhileStatement(TSharpParser::DoWhileStatementContext* ctx) {
	do {
		try {
			visit(ctx->block());
		} catch (const BreakSignal&) {
			break;
		} catch (const ContinueSignal&) {
		}
	} while (evaluate(ctx->expression()).as_bool());
	return Value();
}

// Visit for statement/loop
antlrcpp::Any Interpreter::visitForStatement(TSharpParser::ForStatementContext* ctx) {
	auto previous = env;
	env = std::make_shared<Environment>(previous);
	if (ctx->forInit())
		visit(ctx->forInit());
	while (!ctx->expression() || evaluate(ctx->expression()).as_bool()) {
		try {
			visit(ctx->block());
		} catch (const BreakSignal&) {
			break;
		} catch (const ContinueSignal&) {
		}
		if (ctx->forUpdate())
			visit(ctx->forUpdate());
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
		if (!matched && !is_default && values_equal(subject, evaluate(section->expression())))
			matched = true;
		if (!matched && is_default)
			matched = true;
		if (matched) {
			try {
				for (auto* stmt : section->statement())
					visit(stmt);
			} catch (const BreakSignal&) {
				break;
			}
		}
	}
	return Value();
}

// Visit break statement
antlrcpp::Any Interpreter::visitBreakStatement(TSharpParser::BreakStatementContext*) {
	throw BreakSignal();
}
// Visit continue statement
antlrcpp::Any Interpreter::visitContinueStatement(TSharpParser::ContinueStatementContext*) {
	throw ContinueSignal();
}

// Visit return statement
antlrcpp::Any Interpreter::visitReturnStatement(TSharpParser::ReturnStatementContext* ctx) {
	throw ReturnSignal(ctx->expression() ? evaluate(ctx->expression()) : Value());
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
			if (handled)
				break;
		}
		if (!handled)
			throw;
	}
	if (ctx->finallyClause())
		visit(ctx->finallyClause()->block());
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
		if (result.as_bool())
			return Value(true);
		result = Value(result.as_bool() || evaluate(ctx->logicalAndExpression(i)).as_bool());
	}
	return result;
}

// Visit AND expression
antlrcpp::Any Interpreter::visitLogicalAndExpression(TSharpParser::LogicalAndExpressionContext* ctx) {
	Value result = evaluate(ctx->equalityExpression(0));
	for (size_t i = 1; i < ctx->equalityExpression().size(); ++i) {
		if (!result.as_bool())
			return Value(false);
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
		if (op == "<")
			result = Value(result.as_double() < rhs.as_double());
		else if (op == "<=")
			result = Value(result.as_double() <= rhs.as_double());
		else if (op == ">")
			result = Value(result.as_double() > rhs.as_double());
		else if (op == ">=")
			result = Value(result.as_double() >= rhs.as_double());
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
				result = Value(result.as_double() + rhs.as_double());
			}
		} else {
			if (result.is_number() && rhs.is_number()) {
				result = Value(result.as_int() - rhs.as_int());
			} else {
				result = Value(result.as_double() - rhs.as_double());
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
				result = Value(result.as_int() * rhs.as_int());
			} else {
				result = Value(result.as_double() * rhs.as_double());
			}
		} else if (op == "/") {
			if (rhs.as_double() == 0.0) {
				throw RuntimeError("Division by zero");
			}

			// Keep this as real division
			if (result.is_number() && rhs.is_number()) {
				result = Value(static_cast<double>(result.as_int()) / rhs.as_int());
			} else {
				result = Value(result.as_double() / rhs.as_double());
			}
		} else {
			if (result.is_number() && rhs.is_number()) {
				if (rhs.as_int() == 0) {
					throw RuntimeError("Modulo by zero");
				}
				result = Value(result.as_int() % rhs.as_int());
			} else {
				result = Value(std::fmod(result.as_double(), rhs.as_double()));
			}
		}
	}

	return result;
}

// Visit unary expression
antlrcpp::Any Interpreter::visitUnaryExpression(TSharpParser::UnaryExpressionContext* ctx) {
    if (ctx->postfixExpression())
        return visit(ctx->postfixExpression());

    std::string op = ctx->children[0]->getText();

    // PREFIX ++ / --
    if (op == "++" || op == "--") {
        auto* inner = ctx->unaryExpression();

        // Only support simple variables for now
        if (inner->postfixExpression() &&
            inner->postfixExpression()->primary()->IDENTIFIER()) {

            std::string name = inner->postfixExpression()->primary()->IDENTIFIER()->getText();

            Value current = env->get(name);

            Value result;
            if (op == "++") {
                result = current.is_int()
                    ? Value(current.as_int() + 1)
                    : Value(current.as_double() + 1);
            } else {
                result = current.is_int()
                    ? Value(current.as_int() - 1)
                    : Value(current.as_double() - 1);
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

    for (auto* part : ctx->postfixSuffix()) {
        if (part->DOT() && part->IDENTIFIER()) {
            pending_this = current;
            current = get_member(current, part->IDENTIFIER()->getText(), from_base);
            from_base = false;
        } else if (part->LPAREN()) {
            if (from_base && current.is_instance()) {
                // base() constructor call
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
                auto args = eval_arguments(part->argumentList());
                
                // Find matching constructor by arity
                std::shared_ptr<FunctionValue> base_ctor = nullptr;
                for (const auto& [name, fn] : base_class->constructors) {
                    if (fn && fn->params.size() == args.size()) {
                        base_ctor = fn;
                        break;
                    }
                }
                
                if (!base_ctor) {
                    throw RuntimeError("No matching base constructor with " + std::to_string(args.size()) + " arguments");
                }
                
                // Call the base constructor with the instance as 'this'
                call_function(Value(base_ctor), args, current);
                current = Value();
            } else {
                current = call_function(current, eval_arguments(part->argumentList()), pending_this);
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

            // simple variable: i++ / i--
            if (ctx->primary()->IDENTIFIER() && ctx->postfixSuffix().size() == 1) {
                std::string name = ctx->primary()->IDENTIFIER()->getText();
                Value old_value = env->get(name);

                Value new_value;
                if (is_inc) {
                    new_value = old_value.is_int()
                        ? Value(old_value.as_int() + 1)
                        : Value(old_value.as_double() + 1);
                } else {
                    new_value = old_value.is_int()
                        ? Value(old_value.as_int() - 1)
                        : Value(old_value.as_double() - 1);
                }

                env->assign(name, new_value);

                // postfix returns old value
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
	if (ctx->literal())
		return visit(ctx->literal());
	if (ctx->THIS())
		return env->get("this");
	if (ctx->BASE())
		return env->get("base");
	if (ctx->arrayLiteral())
		return visit(ctx->arrayLiteral());
	if (ctx->expression())
		return evaluate(ctx->expression());

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
	for (auto* expr : ctx->expression())
		arr->push_back(evaluate(expr));
	return Value(arr);
}

// Visit literal
antlrcpp::Any Interpreter::visitLiteral(TSharpParser::LiteralContext* ctx) {
	if (ctx->INTEGER_LITERAL())
		return Value(std::stoi(ctx->INTEGER_LITERAL()->getText()));
	if (ctx->FLOAT_LITERAL())
		return Value(std::stod(ctx->FLOAT_LITERAL()->getText()));
	if (ctx->STRING_LITERAL()) {
		auto text = ctx->STRING_LITERAL()->getText();
		return Value(text.substr(1, text.size() - 2));
	}
	if (ctx->CHAR_LITERAL()) {
		auto text = ctx->CHAR_LITERAL()->getText();
		return Value(text.size() >= 3 ? text[1] : '\0');
	}
	if (ctx->TRUE())
		return Value(true);
	if (ctx->FALSE())
		return Value(false);
	return Value();
}

// Evaluate binary chain
Value Interpreter::eval_binary_chain(const std::vector<TSharpParser::AdditiveExpressionContext*>&) {
	return Value();
}

}