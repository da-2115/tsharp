// Compiler.cpp
// T# v3.0.0-alpha1
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
	const auto generic_pos = type_name.find('<');

	if (generic_pos != std::string::npos) {
		return type_name.substr(0, generic_pos);
	}

	// Preserve nullable markers, but strip array suffixes.
	const auto array_pos = type_name.find('[');

	if (array_pos != std::string::npos) {
		return type_name.substr(0, array_pos);
	}

	return type_name;
}

static void check_nullable_assignment(const std::string& target_name, const std::string& target_type, TSharpParser::ExpressionContext* expression, const std::string& source_type = "") {
	if (target_type.empty() || target_type == "any") {
		return;
	}

	const bool target_nullable = target_type.find('?') != std::string::npos;

	if (expression->getText() == "null" && !target_nullable) {
		throw std::runtime_error("Cannot assign null to non-nullable variable '" + target_name + "' of type '" + target_type + "'");
	}

	if (!source_type.empty() && source_type.find('?') != std::string::npos && !target_nullable) {
		throw std::runtime_error("Cannot assign nullable type '" + source_type + "' to non-nullable variable '" + target_name + "'");
	}
}

static std::string scalar_literal_type_from_text(const std::string& value) {
	if (value == "true" || value == "false") {
		return "bool";
	}
	if (value == "null") {
		return "null";
	}
	if (value.size() >= 2 && value.front() == '\"' && value.back() == '\"') {
		return "string";
	}
	if (value.size() >= 3 && value.front() == '\'' && value.back() == '\'') {
		return "char";
	}
	if (value.empty()) {
		return {};
	}
	size_t i = (value[0] == '-' || value[0] == '+') ? 1 : 0;
	bool digits = false, dot = false, exponent = false;
	for (; i < value.size(); ++i) {
		const char c = value[i];
		if (c >= '0' && c <= '9') {
			digits = true;
			continue;
		}
		if (c == '.' && !dot && !exponent) {
			dot = true;
			continue;
		}
		if ((c == 'e' || c == 'E') && digits && !exponent) {
			exponent = true;
			if (i + 1 < value.size() && (value[i + 1] == '+' || value[i + 1] == '-')) {
				++i;
			}
			if (i + 1 == value.size()) {
				return {};
			}
			continue;
		}
		if (i + 1 == value.size() && digits) {
			if (c == 'f' || c == 'F') {
				return "float";
			}
			if ((c == 'L' || c == 'l') && !dot && !exponent) {
				return "long";
			}
		}
		return {};
	}
	if (!digits) {
		return {};
	}
	return dot || exponent ? "double" : "int";
}

static std::string scalar_literal_type(TSharpParser::ExpressionContext* expression) {
	return expression ? scalar_literal_type_from_text(expression->getText()) : std::string{};
}

static bool is_array_type(const std::string& t);

static void check_scalar_compatibility(const std::string& target_name, const std::string& target, const std::string& source) {
	if (source.empty() || target.empty() || target == "any" || source == "any") {
		return;
	}
	if (source == "null") {
		return; // Existing nullability checker handles this.
	}

	// A value of type T can be assigned to T?.
	if (target.size() > 1 && target.back() == '?') {
		const std::string underlying_type = target.substr(0, target.size() - 1);

		if (source == underlying_type) {
			return;
		}
	}

	if (is_array_type(target) || is_array_type(source)) {
		if (target == source) {
			return;
		}
		throw std::runtime_error("Type error: cannot assign '" + source + "' to '" + target + "' in '" + target_name + "'");
	}
	if (target == source) {
		return;
	}
	if (source == "int" && (target == "long" || target == "float" || target == "double")) {
		return;
	}
	if (source == "long" && (target == "float" || target == "double")) {
		return;
	}
	if (source == "float" && target == "double") {
		return;
	}
	throw std::runtime_error("Type error: cannot assign '" + source + "' to '" + target + "' in '" + target_name + "'");
}

static void check_condition_type(const std::string& construct, const std::string& type) {
	if (!type.empty() && type != "any" && type != "bool") {
		throw std::runtime_error("Type error: " + construct + " condition must be bool, got '" + type + "'");
	}
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

static bool is_array_type(const std::string& t) {
	return t.size() >= 2 && t.compare(t.size() - 2, 2, "[]") == 0;
}

static std::string array_element_type(const std::string& t) {
	return is_array_type(t) ? t.substr(0, t.size() - 2) : std::string{};
}

static bool is_numeric_type(const std::string& t) {
	return t == "int" || t == "long" || t == "float" || t == "double";
}

static std::string promote_numeric_type(const std::string& a, const std::string& b) {
	if (a == "double" || b == "double") {
		return "double";
	}
	if (a == "float" || b == "float") {
		return "float";
	}
	if (a == "long" || b == "long") {
		return "long";
	}
	return "int";
}

bool Compiler::is_subtype_of(const std::string& derived, const std::string& base) const {
	if (derived == base) {
		return true;
	}

	if (derived.empty() || base.empty() || is_array_type(derived) || is_array_type(base)) {
		return false;
	}

	auto it = module.class_lookup.find(derived);
	if (it == module.class_lookup.end() || !module.class_lookup.contains(base)) {
		return false;
	}

	// Guard against malformed cyclic inheritance metadata.
	size_t remaining = module.classes.size();
	std::string current = derived;

	while (remaining-- > 0) {
		auto found = module.class_lookup.find(current);
		if (found == module.class_lookup.end()) {
			return false;
		}
		const std::string& parent = module.classes.at(found->second).base_class;
		if (parent.empty()) {
			return false;
		}
		if (parent == base) {
			return true;
		}
		current = parent;
	}
	return false;
}

void Compiler::check_type_compatibility(const std::string& context, const std::string& target, const std::string& source) const {

	if (target == "T" && source == "int") {
		if (auto it = class_generic_parameters.find(current_class->name); it != class_generic_parameters.end()) {
		}

		if (current_class && is_numeric_type(source)) {
			const auto it = class_generic_parameters.find(current_class->name);

			if (it != class_generic_parameters.end()) {
				const bool is_generic_parameter = std::find(it->second.begin(), it->second.end(), target) != it->second.end();

				if (is_generic_parameter) {
					return;
				}
			}
		}
	}
	if (is_subtype_of(source, target)) {
		return;
	}
	check_scalar_compatibility(context, target, source);
}

std::string Compiler::infer_expression_type(TSharpParser::ExpressionContext* expression) const {
	return expression ? infer_node_type(expression) : std::string{};
}

std::string Compiler::infer_node_type(antlr4::ParserRuleContext* node) const {
	if (!node) {
		return {};
	}

	if (auto* primary = dynamic_cast<TSharpParser::PrimaryContext*>(node)) {
		if (primary->IDENTIFIER()) {
			const std::string name = primary->IDENTIFIER()->getText();
			if (has_local(name)) {
				return resolve_local_type(name);
			}
			if (auto it = global_types.find(name); it != global_types.end()) {
				return it->second;
			}
			return {};
		}
		if (primary->expression()) {
			return infer_expression_type(primary->expression());
		}
		if (primary->literal()) {
			return scalar_literal_type_from_text(primary->literal()->getText());
		}
		if (primary->arrayLiteral()) {
			std::string element;
			for (auto* e : primary->arrayLiteral()->expression()) {
				const auto next = infer_expression_type(e);
				if (next.empty() || next == "any") {
					return {};
				}
				if (element.empty()) {
					element = next;
				} else if (is_numeric_type(element) && is_numeric_type(next)) {
					element = promote_numeric_type(element, next);
				} else if (element != next) {
					throw std::runtime_error("Type error: incompatible array literal elements '" + element + "' and '" + next + "'");
				}
			}
			return element.empty() ? std::string{} : element + "[]";
		}
		return {};
	}

	if (auto* postfix = dynamic_cast<TSharpParser::PostfixExpressionContext*>(node)) {
		const auto& suffixes = postfix->postfixSuffix();
		if (suffixes.empty()) {
			return infer_node_type(postfix->primary());
		}
		std::string type = infer_node_type(postfix->primary());
		size_t start_index = 0;

		if (postfix->primary() && postfix->primary()->IDENTIFIER() && suffixes.front()->LPAREN()) {
			const std::string name = postfix->primary()->IDENTIFIER()->getText();

			if (auto it = module.function_lookup.find(name); it != module.function_lookup.end()) {

				type = module.functions.at(it->second).return_type;
				start_index = 1;

			} else if (current_class && current_class->methods.contains(name)) {

				type = module.functions.at(current_class->methods.at(name)).return_type;

				start_index = 1;

			} else if (module.class_lookup.contains(name)) {

				type = name;

				const std::string constructor_text = postfix->getText();
				const size_t generic_start = name.size();

				if (constructor_text.size() > generic_start && constructor_text[generic_start] == '<') {

					int depth = 0;

					for (size_t i = generic_start; i < constructor_text.size(); ++i) {

						if (constructor_text[i] == '<') {
							++depth;
						} else if (constructor_text[i] == '>') {
							--depth;

							if (depth == 0) {
								type = constructor_text.substr(0, i + 1);
								break;
							}
						}
					}
				}

				start_index = 1;
			}
		}

		const auto& parts = suffixes;

		for (size_t i = start_index; i < parts.size(); ++i) {
			auto* suffix = parts[i];
			if (suffix->LBRACK()) {
				if (suffix->expression()) {
					const auto index = infer_expression_type(suffix->expression());
					if (!index.empty() && index != "any" && index != "int" && index != "long") {
						throw std::runtime_error("Type error: array index must be an integer, got '" + index + "'");
					}
				}
				if (!type.empty() && type != "any" && !is_array_type(type)) {
					throw std::runtime_error("Type error: cannot index non-array type '" + type + "'");
				}
				type = array_element_type(type);
			} else if (suffix->DOT()) {
				const std::string owner = type;
				const std::string class_name = base_type_name(owner);
				const std::string member = suffix->IDENTIFIER()->getText();
				const auto class_it = module.class_lookup.find(class_name);
				if (class_it != module.class_lookup.end()) {
					const auto& cls = module.classes.at(class_it->second);
					const auto method_it = cls.methods.find(member);
					if (method_it != cls.methods.end() && i + 1 < parts.size() && parts[i + 1]->LPAREN()) {
						const auto& fn = module.functions.at(method_it->second);
						auto* args = parts[i + 1]->argumentList();
						const size_t count = args ? args->expression().size() : 0;
						if (count != fn.arity) {
							throw std::runtime_error("Type error: method '" + owner + "." + member + "' expects " + std::to_string(fn.arity) + " arguments, got " + std::to_string(count));
						}

						std::unordered_map<std::string, std::string> substitutions;

						if (auto it = class_generic_parameters.find(class_name); it != class_generic_parameters.end()) {

							const auto& parameters = it->second;

							const size_t left = owner.find('<');
							const size_t right = owner.rfind('>');

							if (left != std::string::npos && right != std::string::npos && right > left) {

								const std::string arguments = owner.substr(left + 1, right - left - 1);

								std::vector<std::string> concrete_types;
								std::string current;
								int depth = 0;

								for (char c : arguments) {
									if (c == '<') {
										++depth;
									} else if (c == '>') {
										--depth;
									}

									if (c == ',' && depth == 0) {
										concrete_types.push_back(current);
										current.clear();
									} else {
										current += c;
									}
								}

								if (!current.empty()) {
									concrete_types.push_back(current);
								}

								if (parameters.size() == concrete_types.size()) {
									for (size_t k = 0; k < parameters.size(); ++k) {
										substitutions[parameters[k]] = concrete_types[k];
									}
								}
							}
						}

						const auto substitute_type = [&](const std::string& original) {
							std::string result;
							result.reserve(original.size());

							for (size_t i = 0; i < original.size();) {
								const unsigned char c = static_cast<unsigned char>(original[i]);

								if (std::isalpha(c) || original[i] == '_') {
									const size_t start = i++;

									while (i < original.size()) {
										const unsigned char next = static_cast<unsigned char>(original[i]);

										if (!std::isalnum(next) && original[i] != '_') {
											break;
										}

										++i;
									}

									const std::string identifier = original.substr(start, i - start);

									if (auto it = substitutions.find(identifier); it != substitutions.end()) {
										result += it->second;
									} else {
										result += identifier;
									}
								} else {
									result += original[i++];
								}
							}

							return result;
						};

						for (size_t j = 0; args && j < count && j < fn.parameter_types.size(); ++j) {

							const std::string expected = substitute_type(fn.parameter_types[j]);

							const std::string actual = infer_expression_type(args->expression()[j]);

							const std::string context = owner + "." + member + " argument " + std::to_string(j + 1);

							check_nullable_assignment(context, expected, args->expression()[j], actual);

							check_type_compatibility(context, expected, actual);
						}

						type = substitute_type(fn.return_type);
						++i; // consume call suffix
					} else if (auto fields = class_field_types.find(base_type_name(owner)); fields != class_field_types.end() && fields->second.contains(member)) {
						type = fields->second.at(member);
					} else if (method_it != cls.methods.end() || cls.properties.contains(member)) {
						type.clear(); // method reference/property type not yet modelled
					} else {
						throw std::runtime_error("Type error: class '" + owner + "' has no member '" + member + "'");
					}
				} else {
					type.clear(); // dynamic or unresolved receiver
				}
			} else {
				type.clear();
			}
		}
		return type;
	}

	if (auto* unary = dynamic_cast<TSharpParser::UnaryExpressionContext*>(node)) {
		if (unary->postfixExpression()) {
			return infer_node_type(unary->postfixExpression());
		}
		if (!unary->unaryExpression()) {
			return {};
		}
		const std::string operand = infer_node_type(unary->unaryExpression());
		const std::string op = unary->children.empty() ? "" : unary->children.front()->getText();

		if (op == "await") {
			if (operand.empty() || operand == "any") {
				return {};
			}

			if (operand.size() >= 6 && operand.compare(0, 5, "task<") == 0 && operand.back() == '>') {
				return operand.substr(5, operand.size() - 6);
			}

			throw std::runtime_error("Type error: await requires task<T>, got '" + operand + "'");
		}

		if (op == "!") {
			if (!operand.empty() && operand != "any" && operand != "bool") {
				throw std::runtime_error("Type error: logical '!' requires bool, got '" + operand + "'");
			}
			return "bool";
		}
		if (op == "-" || op == "+" || op == "++" || op == "--") {
			if (!operand.empty() && operand != "any" && !is_numeric_type(operand)) {
				throw std::runtime_error("Type error: unary '" + op + "' requires a numeric operand, got '" + operand + "'");
			}
		}
		return operand;
	}

	// Each binary-precedence level has child rule contexts alternating with
	// operator terminals. Work directly on the parse tree: no runtime cost.
	std::string result;
	bool first = true;
	std::string pending_op;
	for (auto* child : node->children) {
		if (auto* terminal = dynamic_cast<antlr4::tree::TerminalNode*>(child)) {
			const std::string op = terminal->getText();
			if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%" || op == "<" || op == "<=" || op == ">" || op == ">=" || op == "==" || op == "!=" || op == "&&" || op == "||") {
				pending_op = op;
			}
			continue;
		}
		auto* sub = dynamic_cast<antlr4::ParserRuleContext*>(child);
		if (!sub) {
			continue;
		}
		const std::string rhs = infer_node_type(sub);
		if (first) {
			result = rhs;
			first = false;
			continue;
		}
		if (pending_op.empty()) {
			result = rhs;
			continue;
		}
		const std::string op = pending_op;
		pending_op.clear();
		const bool known = !result.empty() && !rhs.empty() && result != "any" && rhs != "any";
		if (op == "&&" || op == "||") {
			if (known && (result != "bool" || rhs != "bool")) {
				throw std::runtime_error("Type error: operator '" + op + "' requires bool operands");
			}
			result = "bool";
		} else if (op == "==" || op == "!=") {
			bool generic_numeric_comparison = false;

			if (current_class) {
				const auto it = class_generic_parameters.find(current_class->name);

				if (it != class_generic_parameters.end()) {
					const auto& parameters = it->second;

					const auto is_generic_parameter = [&](const std::string& type) { return std::find(parameters.begin(), parameters.end(), type) != parameters.end(); };

					generic_numeric_comparison = (is_generic_parameter(result) && is_numeric_type(rhs)) || (is_numeric_type(result) && is_generic_parameter(rhs));
				}
			}

			const bool nullable_null_comparison = (result == "null" && rhs.size() > 1 && rhs.back() == '?') || (rhs == "null" && result.size() > 1 && result.back() == '?');

			if (known && result != rhs && !generic_numeric_comparison && !nullable_null_comparison && !(is_numeric_type(result) && is_numeric_type(rhs))) {
				throw std::runtime_error("Type error: cannot compare '" + result + "' and '" + rhs + "'");
			}

			result = "bool";
		} else if (op == "<" || op == "<=" || op == ">" || op == ">=") {
			if (known && !(is_numeric_type(result) && is_numeric_type(rhs))) {
				throw std::runtime_error("Type error: operator '" + op + "' requires numeric operands");
			}
			result = "bool";
		} else if (known) {
			if (op == "+" && result == "string" && rhs == "string") {
				result = "string";
			} else if (is_numeric_type(result) && is_numeric_type(rhs)) {
				result = promote_numeric_type(result, rhs);
			} else {
				throw std::runtime_error("Type error: invalid operands '" + result + "' and '" + rhs + "' for '" + op + "'");
			}
		} else {
			result.clear();
		}
	}
	return result;
}

static bool statement_guarantees_exit(TSharpParser::StatementContext* statement);

static bool block_guarantees_exit(TSharpParser::BlockContext* block) {
	if (!block) {
		return false;
	}

	for (auto* statement : block->statement()) {
		if (statement_guarantees_exit(statement)) {
			return true;
		}
	}

	return false;
}

static bool statement_guarantees_exit(TSharpParser::StatementContext* statement) {
	if (!statement) {
		return false;
	}
	if (statement->returnStatement() || statement->throwStatement()) {
		return true;
	}
	if (statement->block()) {
		return block_guarantees_exit(statement->block());
	}
	if (auto* branch = statement->ifStatement()) {
		const auto& conditions = branch->expression();
		const auto& blocks = branch->block();
		// An else branch is mandatory; every if/else-if branch must exit.
		if (blocks.size() != conditions.size() + 1) {
			return false;
		}
		for (auto* body : blocks) {
			if (!block_guarantees_exit(body)) {
				return false;
			}
		}
		return true;
	}
	return false;
}

// Public compile entry point

BytecodeModule Compiler::compile(const std::vector<TSharpParser::ProgramContext*>& programs, TSharpParser::ProgramContext* entry_program) {
	if (programs.empty() || !entry_program) {
		throw std::runtime_error("Cannot compile an empty program set");
	}

	module = BytecodeModule();
	global_types.clear();
	class_field_types.clear();

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

			std::string parameter_type = parameter->typeRef()->getText();

			if (parameter->arrayDeclarator() != nullptr) {
				parameter_type += "[]";
			}

			function.parameter_types.push_back(parameter_type);
		}
	}

	function.arity = function.parameter_names.size();

	const size_t index = module.functions.size();

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

	const size_t class_index = module.classes.size();

	module.class_lookup[name] = class_index;

	module.classes.push_back(std::move(class_info));

	ClassInfo& declared_class = module.classes[class_index];

	// Register generic parameters during class declaration.
	if (ctx->genericParams()) {
		auto& parameters = class_generic_parameters[name];

		parameters.clear();

		for (auto* identifier : ctx->genericParams()->IDENTIFIER()) {
			parameters.push_back(identifier->getText());
		}
	}

	// Inherit fields and methods.
	if (!declared_class.base_class.empty()) {
		auto base_it = module.class_lookup.find(declared_class.base_class);

		if (base_it != module.class_lookup.end()) {
			const ClassInfo& base = module.classes[base_it->second];

			declared_class.fields = base.fields;
			class_field_types[name] = class_field_types[declared_class.base_class];

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

			const size_t slot = declared_class.fields.size();

			BytecodeFieldInfo info;

			info.name = field_name;

			info.slot = slot;

			declared_class.field_slots[field_name] = slot;
			class_field_types[name][field_name] = field->typeRef()->getText() + (field->arrayDeclarator() && !is_array_type(field->typeRef()->getText()) ? "[]" : "");

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

	const size_t index = module.enums.size();

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

	const size_t slot = module.globals.size();

	module.global_lookup[name] = slot;

	module.globals.push_back(Value());
	global_types[name] = variable->typeRef()->getText() + (variable->arrayDeclarator() && !is_array_type(variable->typeRef()->getText()) ? "[]" : "");
}

void Compiler::compile_global_initializer(TSharpParser::GlobalVarDeclContext* ctx) {
	// Global initialisers should ultimately be compiled into
	// a module initialisation function.
	//
	// For constant literals, initialise directly now.

	auto* variable = ctx->variableDecl();

	const std::string name = variable->IDENTIFIER()->getText();

	const size_t slot = resolve_global(name);

	if (!variable->variableInitializer()) {
		return;
	}

	auto* initializer = variable->variableInitializer();

	if (!initializer->expression()) {
		return;
	}

	const auto* expression = initializer->expression();
	const auto source_type = infer_expression_type(initializer->expression());
	check_nullable_assignment(name, global_types.at(name), initializer->expression(), source_type);
	check_type_compatibility(name, global_types.at(name), source_type);

	if (Value value; try_get_constant_expression(initializer->expression(), value)) {
		module.globals[slot] = std::move(value);
	}
}

// Methods

void Compiler::declare_method(size_t class_index, TSharpParser::MethodDeclContext* ctx) {
	ClassInfo& class_info = module.classes[class_index];

	const std::string method_name = ctx->IDENTIFIER()->getText();

	const std::string qualified_name = class_info.name + "." + method_name;

	BytecodeFunction function;

	function.name = qualified_name;

	function.is_method = true;

	function.is_constructor = false;
	function.return_type = ctx->returnType()->getText();

	// Slot 0 is reserved for this.
	function.parameter_names.push_back("this");

	if (ctx->parameterList()) {
		for (auto* parameter : ctx->parameterList()->parameter()) {
			function.parameter_names.push_back(parameter->IDENTIFIER()->getText());

			std::string parameter_type = parameter->typeRef()->getText();

			if (parameter->arrayDeclarator() != nullptr) {
				parameter_type += "[]";
			}

			function.parameter_types.push_back(parameter_type);
		}
	}

	function.arity = function.parameter_names.size() - 1;

	// A method replacing an inherited method must preserve its signature.
	// Do not silently change a base method's parameter or return contract.
	if (auto inherited = class_info.methods.find(method_name); inherited != class_info.methods.end()) {
		const auto& base = module.functions.at(inherited->second);
		if (base.arity != function.arity || base.parameter_types != function.parameter_types || base.return_type != function.return_type) {
			throw std::runtime_error("Type error: incompatible override '" + qualified_name + "' of '" + base.name + "'");
		}
	}

	const size_t function_index = module.functions.size();

	module.function_lookup[qualified_name] = function_index;

	module.functions.push_back(std::move(function));

	class_info.methods[method_name] = function_index;
}

// Constructors

void Compiler::declare_constructor(size_t class_index, TSharpParser::ConstructorDeclContext* ctx) {
	ClassInfo& class_info = module.classes[class_index];

	BytecodeFunction function;

	function.name = class_info.name + ".__ctor_" + std::to_string(class_info.constructors.size());

	function.is_method = true;

	function.is_constructor = true;

	function.parameter_names.push_back("this");

	if (ctx->parameterList()) {
		for (auto* parameter : ctx->parameterList()->parameter()) {
			function.parameter_names.push_back(parameter->IDENTIFIER()->getText());

			std::string parameter_type = parameter->typeRef()->getText();

			if (parameter->arrayDeclarator() != nullptr) {
				parameter_type += "[]";
			}

			function.parameter_types.push_back(parameter_type);
		}
	}

	function.arity = function.parameter_names.size() - 1;

	const size_t function_index = module.functions.size();

	module.function_lookup[function.name] = function_index;

	module.functions.push_back(std::move(function));

	class_info.constructors.push_back(function_index);
}

// Properties

void Compiler::declare_property(size_t class_index, TSharpParser::PropertyDeclContext* ctx) {
	ClassInfo& class_info = module.classes[class_index];

	const std::string property_name = ctx->IDENTIFIER()->getText();

	const std::string function_name = class_info.name + ".__property_" + property_name;

	BytecodeFunction function;

	function.name = function_name;

	function.is_method = true;

	function.parameter_names.push_back("this");

	function.arity = 0;

	const size_t function_index = module.functions.size();

	module.function_lookup[function_name] = function_index;

	module.functions.push_back(std::move(function));

	class_info.properties[property_name] = function_index;
}

// Compile top-level function
void Compiler::compile_function(TSharpParser::FunctionDeclContext* ctx) {
	const std::string name = ctx->IDENTIFIER()->getText();

	const size_t function_index = resolve_function(name);

	BytecodeFunction& function = module.functions[function_index];

	function.return_type = ctx->returnType()->getText();

	begin_function(function);

	current_return_type = function.return_type;

	for (size_t i = 0; i < function.parameter_names.size(); i++) {
		const std::string type = i < function.parameter_types.size() ? function.parameter_types[i] : std::string{};

		declare_local(function.parameter_names[i], type);
	}

	if (function.return_type != "void" && !block_guarantees_exit(ctx->block())) {
		throw std::runtime_error("Type error: function '" + name + "' may finish without returning '" + function.return_type + "'");
	}

	if (ctx->block()) {
		visit(ctx->block());
	}

	// Implicit return for void functions.
	emit_opcode(OpCode::Null);

	emit_opcode(OpCode::Return);

	function.local_count = current_context().next_local;

	end_function();

	current_return_type.clear();
}

// Compile class bodies

void Compiler::compile_class(TSharpParser::ClassDeclContext* ctx) {
	const std::string name = ctx->IDENTIFIER()->getText();

	const size_t class_index = resolve_class(name);

	current_class = &module.classes[class_index];

	if (ctx->genericParams()) {
		auto& parameters = class_generic_parameters[current_class->name];

		parameters.clear();

		for (auto* identifier : ctx->genericParams()->IDENTIFIER()) {
			parameters.push_back(identifier->getText());
		}
	}

	if (ctx->genericParams() != nullptr) {
		auto& parameters = class_generic_parameters[name];

		for (auto* identifier : ctx->genericParams()->IDENTIFIER()) {
			const std::string parameter = identifier->getText();

			if (std::find(parameters.begin(), parameters.end(), parameter) == parameters.end()) {
				parameters.push_back(parameter);
			}
		}
	}

	size_t constructor_index = 0;

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

void Compiler::compile_method(size_t class_index, TSharpParser::MethodDeclContext* ctx) {
	ClassInfo& class_info = module.classes[class_index];

	const std::string qualified_name = class_info.name + "." + ctx->IDENTIFIER()->getText();

	const size_t function_index = resolve_function(qualified_name);

	BytecodeFunction& function = module.functions[function_index];

	begin_function(function);
	function.return_type = ctx->returnType()->getText();
	current_return_type = function.return_type;

	for (size_t i = 0; i < function.parameter_names.size(); i++) {
		const std::string type = i == 0 ? class_info.name : (i - 1 < function.parameter_types.size() ? function.parameter_types[i - 1] : std::string{});
		declare_local(function.parameter_names[i], type);
	}

	if (ctx->block() && function.return_type != "void" && !block_guarantees_exit(ctx->block())) {
		throw std::runtime_error("Type error: method '" + qualified_name + "' may finish without returning '" + function.return_type + "'");
	}

	if (ctx->block()) {
		visit(ctx->block());

		emit_opcode(OpCode::Null);

		emit_opcode(OpCode::Return);
	}

	function.local_count = current_context().next_local;

	end_function();
	current_return_type.clear();
}

// Compile constructor

void Compiler::compile_constructor(size_t class_index, TSharpParser::ConstructorDeclContext* ctx, size_t constructor_index) {
	ClassInfo& class_info = module.classes[class_index];

	const size_t function_index = class_info.constructors[constructor_index];

	BytecodeFunction& function = module.functions[function_index];

	begin_function(function);

	for (size_t i = 0; i < function.parameter_names.size(); i++) {
		const std::string type = i == 0 ? class_info.name : (i - 1 < function.parameter_types.size() ? function.parameter_types[i - 1] : std::string{});

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

void Compiler::compile_property(size_t class_index, TSharpParser::PropertyDeclContext* ctx) {
	ClassInfo& class_info = module.classes[class_index];

	const size_t function_index = class_info.properties[ctx->IDENTIFIER()->getText()];

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

void Compiler::emit_default_value(const std::string& type) {
	if (type.find('?') != std::string::npos) {
		emit_opcode(OpCode::Null);
		return;
	}

	if (type == "int") {
		emit_constant(Value(0));
		return;
	}

	if (type == "long") {
		emit_constant(Value(static_cast<std::int64_t>(0)));
		return;
	}

	if (type == "float") {
		emit_constant(Value(0.0f));
		return;
	}

	if (type == "double") {
		emit_constant(Value(0.0));
		return;
	}

	if (type == "bool") {
		emit_opcode(OpCode::False);
		return;
	}

	if (type == "char") {
		emit_constant(Value('\0'));
		return;
	}

	emit_opcode(OpCode::Null);
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

size_t Compiler::declare_local(const std::string& name, const std::string& type) {
	auto& context = current_context();

	if (context.scopes.empty()) {
		context.scopes.emplace_back();
	}

	auto& current_scope = context.scopes.back();

	if (current_scope.contains(name)) {
		throw std::runtime_error("Local variable already declared in this scope: " + name);
	}

	const size_t slot = context.next_local++;

	current_scope[name] = LocalInfo{.slot = slot, .type = type};

	return slot;
}

size_t Compiler::resolve_local(const std::string& name) const {
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

size_t Compiler::resolve_global(const std::string& name) const {
	auto it = module.global_lookup.find(name);

	if (it == module.global_lookup.end()) {
		throw std::runtime_error("Undefined global variable: " + name);
	}

	return it->second;
}

size_t Compiler::resolve_function(const std::string& name) const {
	auto it = module.function_lookup.find(name);

	if (it == module.function_lookup.end()) {
		throw std::runtime_error("Undefined function: " + name);
	}

	return it->second;
}

size_t Compiler::resolve_class(const std::string& name) const {
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

void Compiler::emit_byte(uint8_t value) {
	chunk().write_byte(value);
}

void Compiler::emit_u16(uint16_t value) {
	emit_byte(static_cast<uint8_t>(value & 0xff));

	emit_byte(static_cast<uint8_t>((value >> 8) & 0xff));
}

void Compiler::emit_constant(Value value) {
	const size_t index = chunk().add_constant(std::move(value));

	if (index > std::numeric_limits<uint16_t>::max()) {
		throw std::runtime_error("Too many constants");
	}

	emit_opcode(OpCode::Constant);

	emit_u16(static_cast<uint16_t>(index));
}

void Compiler::emit_load_local(size_t slot) {
	emit_opcode(OpCode::LoadLocal);

	emit_u16(static_cast<uint16_t>(slot));
}

void Compiler::emit_store_local(size_t slot) {
	emit_opcode(OpCode::StoreLocal);

	emit_u16(static_cast<uint16_t>(slot));
}

size_t Compiler::emit_jump(OpCode opcode) {
	emit_opcode(opcode);

	emit_u16(0xffff);

	return (chunk().code_size() - 2);
}

void Compiler::patch_jump(size_t operand_position) {
	const size_t offset = chunk().code_size() - operand_position - 2;

	if (offset > std::numeric_limits<uint16_t>::max()) {
		throw std::runtime_error("Jump offset too large");
	}

	auto& code = chunk().mutable_code();

	code[operand_position] = static_cast<uint8_t>(offset & 0xff);

	code[operand_position + 1] = static_cast<uint8_t>((offset >> 8) & 0xff);
}

void Compiler::emit_loop(size_t loop_start) {
	emit_opcode(OpCode::Loop);

	const size_t offset = chunk().code_size() - loop_start + 2;

	if (offset > std::numeric_limits<uint16_t>::max()) {
		throw std::runtime_error("Loop body too large");
	}

	emit_u16(static_cast<uint16_t>(offset));
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

	const std::string declared_type = ctx->typeRef()->getText();
	const std::string static_type = ctx->typeRef()->getText() + (ctx->arrayDeclarator() ? "[]" : "");

	const size_t slot = declare_local(name, static_type);

	auto* array_decl = ctx->arrayDeclarator();

	auto* initializer = ctx->variableInitializer();

	// Initializer first
	//
	// int values[] = { 1, 2, 3 }
	// int x = 10

	if (initializer) {
		if (initializer->expression()) {
			auto* expression = initializer->expression();

			const std::string source_name = simple_identifier_from_expression(expression);

			const std::string source_type = infer_expression_type(expression);

			check_nullable_assignment(name, declared_type, expression, source_type);
			check_type_compatibility(name, static_type, source_type);

			visit(expression);

			if (array_decl) {
				emit_array_type(declared_type);
			}

			emit_store_local(slot);

			return {};
		}

		if (initializer->LPAREN()) {
			const std::string runtime_type = base_type_name(ctx->typeRef()->getText());

			if (module.class_lookup.contains(runtime_type)) {
				size_t argument_count = 0;

				if (initializer->argumentList()) {
					for (auto* argument : initializer->argumentList()->expression()) {
						visit(argument);
						argument_count++;
					}
				}

				emit_opcode(OpCode::NewObject);
				emit_u16(static_cast<uint16_t>(module.class_lookup.at(runtime_type)));
				emit_u16(static_cast<uint16_t>(argument_count));

				emit_store_local(slot);

				return {};
			}

			throw std::runtime_error("Cannot construct unknown type '" + runtime_type + "'");
		}
	}

	// Fixed-size array without initializer
	// int values[5]

	if (array_decl) {
		if (array_decl->expression()) {
			visit(array_decl->expression());

			emit_opcode(OpCode::NewArray);

			emit_array_type(declared_type);

			emit_store_local(slot);

			return {};
		}

		// Unsized [] with no initializer.
		// int values[]
		// Represent as empty array.

		emit_constant(Value(0));
		emit_opcode(OpCode::NewArray);
		emit_array_type(declared_type);
		emit_store_local(slot);
	}

	// No initializer
	if (!initializer) {
		emit_default_value(declared_type);

		emit_store_local(slot);

		return {};
	}

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
			if (op == "=") {
				auto cls = class_field_types.find(current_class->name);
				if (cls != class_field_types.end() && cls->second.contains(name)) {
					check_type_compatibility(name, cls->second.at(name), infer_expression_type(ctx->expression()));
				}
			}
			const size_t field_slot = current_class->field_slots[name];

			emit_load_local(0);

			if (op != "=") {
				emit_opcode(OpCode::Duplicate);

				emit_opcode(OpCode::LoadField);

				emit_u16(static_cast<uint16_t>(field_slot));
			}

			visit(ctx->expression());

			if (op != "=") {
				emit_compound_operator(op);
			}

			emit_opcode(OpCode::StoreField);

			emit_u16(static_cast<uint16_t>(field_slot));

			return {};
		}

		if (has_local(name)) {
			const size_t slot = resolve_local(name);

			if (op == "=") {
				const std::string target_type = resolve_local_type(name);

				const std::string source_name = simple_identifier_from_expression(ctx->expression());

				const std::string source_type = infer_expression_type(ctx->expression());

				check_nullable_assignment(name, target_type, ctx->expression(), source_type);
				check_type_compatibility(name, target_type, source_type);
			}

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

						emit_u16(static_cast<uint16_t>(slot));

						emit_u16(static_cast<uint16_t>(resolve_local(rhs_name)));

						return {};
					}

					if (lhs_type == "long" && rhs_type == "long") {
						emit_opcode(OpCode::AddLocalLong);

						emit_u16(static_cast<uint16_t>(slot));

						emit_u16(static_cast<uint16_t>(resolve_local(rhs_name)));

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

		// Global

		if (module.global_lookup.contains(name)) {
			const size_t global = resolve_global(name);
			if (op == "=" && global_types.contains(name)) {
				const auto source_name = simple_identifier_from_expression(ctx->expression());
				const auto source_type = infer_expression_type(ctx->expression());
				check_nullable_assignment(name, global_types.at(name), ctx->expression(), source_type);
				check_type_compatibility(name, global_types.at(name), source_type);
			}

			if (op != "=") {
				emit_opcode(OpCode::LoadGlobal);

				emit_u16(static_cast<uint16_t>(global));
			}

			visit(ctx->expression());

			if (op != "=") {
				emit_compound_operator(op);
			}

			emit_opcode(OpCode::StoreGlobal);

			emit_u16(static_cast<uint16_t>(global));

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

	for (size_t i = 0; i + 1 < suffixes.size(); i++) {
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
		if (op == "=" && suffixes.size() == 1) {
			std::string owner_type;
			if (has_local(primary_name)) {
				owner_type = resolve_local_type(primary_name);
			} else if (auto it = global_types.find(primary_name); it != global_types.end()) {
				owner_type = it->second;
			}
			if (auto cls = class_field_types.find(owner_type); cls != class_field_types.end()) {
				if (auto field = cls->second.find(field_name); field != cls->second.end()) {
					const auto actual = infer_expression_type(ctx->expression());
					check_nullable_assignment(primary_name + "." + field_name, field->second, ctx->expression(), actual);
					check_type_compatibility(primary_name + "." + field_name, field->second, actual);
				} else if (module.class_lookup.contains(owner_type)) {
					throw std::runtime_error("Type error: class '" + owner_type + "' has no field '" + field_name + "'");
				}
			}
		}

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
		std::string container_type;
		if (has_local(primary_name)) {
			container_type = resolve_local_type(primary_name);
		} else if (auto it = global_types.find(primary_name); it != global_types.end()) {
			container_type = it->second;
		}
		if (suffixes.size() == 1 && !container_type.empty() && container_type != "any") {
			if (!is_array_type(container_type)) {
				throw std::runtime_error("Type error: cannot index non-array type '" + container_type + "'");
			}
			const auto index_type = infer_expression_type(final_suffix->expression());
			if (!index_type.empty() && index_type != "any" && index_type != "int" && index_type != "long") {
				throw std::runtime_error("Type error: array index must be an integer, got '" + index_type + "'");
			}
			if (op == "=") {
				check_type_compatibility(primary_name + "[]", array_element_type(container_type), infer_expression_type(ctx->expression()));
			}
		}
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

		const size_t index_slot = declare_temporary();

		emit_store_local(index_slot);

		const size_t receiver_slot = declare_temporary();

		emit_store_local(receiver_slot);

		// Load current value.
		emit_load_local(receiver_slot);

		emit_load_local(index_slot);

		emit_opcode(OpCode::LoadIndex);

		// Apply RHS.
		visit(ctx->expression());

		emit_compound_operator(op);

		// Save result temporarily.
		const size_t value_slot = declare_temporary();

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

	for (size_t i = 1; i < values.size(); i++) {
		visit(values[i]);

		emit_opcode(OpCode::LogicalOr);
	}

	return {};
}

antlrcpp::Any Compiler::visitLogicalAndExpression(TSharpParser::LogicalAndExpressionContext* ctx) {
	const auto& values = ctx->equalityExpression();

	visit(values[0]);

	for (size_t i = 1; i < values.size(); i++) {
		visit(values[i]);

		emit_opcode(OpCode::LogicalAnd);
	}

	return {};
}

antlrcpp::Any Compiler::visitEqualityExpression(TSharpParser::EqualityExpressionContext* ctx) {
	const auto& values = ctx->relationalExpression();

	visit(values[0]);

	size_t value_index = 1;

	for (auto* child : ctx->children) {
		auto* terminal = dynamic_cast<antlr4::tree::TerminalNode*>(child);

		if (!terminal) {
			continue;
		}

		const int token_type = terminal->getSymbol()->getType();

		if (token_type != TSharpParser::EQ && token_type != TSharpParser::NEQ) {
			continue;
		}

		visit(values[value_index++]);

		emit_opcode(token_type == TSharpParser::EQ ? OpCode::Equal : OpCode::NotEqual);
	}

	return {};
}

antlrcpp::Any Compiler::visitRelationalExpression(TSharpParser::RelationalExpressionContext* ctx) {
	const auto& values = ctx->additiveExpression();

	visit(values[0]);

	size_t value_index = 1;

	for (auto* child : ctx->children) {
		auto* terminal = dynamic_cast<antlr4::tree::TerminalNode*>(child);

		if (!terminal) {
			continue;
		}

		const int token_type = terminal->getSymbol()->getType();

		if (token_type != TSharpParser::LT && token_type != TSharpParser::LTE && token_type != TSharpParser::GT && token_type != TSharpParser::GTE) {
			continue;
		}

		visit(values[value_index++]);

		switch (token_type) {
			case TSharpParser::LT:
				emit_opcode(OpCode::Less);
				break;

			case TSharpParser::LTE:
				emit_opcode(OpCode::LessEqual);
				break;

			case TSharpParser::GT:
				emit_opcode(OpCode::Greater);
				break;

			case TSharpParser::GTE:
				emit_opcode(OpCode::GreaterEqual);
				break;

			default:
				break;
		}
	}

	return {};
}

antlrcpp::Any Compiler::visitAdditiveExpression(TSharpParser::AdditiveExpressionContext* ctx) {
	const auto& values = ctx->multiplicativeExpression();

	visit(values[0]);

	size_t value_index = 1;

	for (auto* child : ctx->children) {
		auto* terminal = dynamic_cast<antlr4::tree::TerminalNode*>(child);

		if (!terminal) {
			continue;
		}

		const int token_type = terminal->getSymbol()->getType();

		if (token_type != TSharpParser::ADD && token_type != TSharpParser::SUB) {
			continue;
		}

		visit(values[value_index++]);

		emit_opcode(token_type == TSharpParser::ADD ? OpCode::Add : OpCode::Subtract);
	}

	return {};
}

antlrcpp::Any Compiler::visitMultiplicativeExpression(TSharpParser::MultiplicativeExpressionContext* ctx) {
	const auto& values = ctx->unaryExpression();

	visit(values[0]);

	size_t value_index = 1;

	for (auto* child : ctx->children) {
		auto* terminal = dynamic_cast<antlr4::tree::TerminalNode*>(child);

		if (!terminal) {
			continue;
		}

		const int token_type = terminal->getSymbol()->getType();

		if (token_type != TSharpParser::MUL && token_type != TSharpParser::DIV && token_type != TSharpParser::MOD) {
			continue;
		}

		visit(values[value_index++]);

		switch (token_type) {
			case TSharpParser::MUL:
				emit_opcode(OpCode::Multiply);
				break;

			case TSharpParser::DIV:
				emit_opcode(OpCode::Divide);
				break;

			case TSharpParser::MOD:
				emit_opcode(OpCode::Modulo);
				break;

			default:
				break;
		}
	}

	return {};
}

// Unary
antlrcpp::Any Compiler::visitUnaryExpression(TSharpParser::UnaryExpressionContext* ctx) {
	if (auto* spawn = ctx->spawnExpression()) {
		compile_spawn(spawn->IDENTIFIER()->getText(), spawn->argumentList());

		return {};
	}

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

		const size_t slot = resolve_local(name);

		emit_opcode(op == "++" ? OpCode::IncrementLocal : OpCode::DecrementLocal);

		emit_u16(static_cast<uint16_t>(slot));
		emit_load_local(slot);

		return {};
	}

	visit(ctx->unaryExpression());

	if (op == "await") {
		emit_opcode(OpCode::Await);
	} else if (op == "!") {
		emit_opcode(OpCode::LogicalNot);
	} else if (op == "-") {
		emit_opcode(OpCode::Negate);
	}

	return {};
}

// Postfix expressions

antlrcpp::Any Compiler::visitPostfixExpression(TSharpParser::PostfixExpressionContext* ctx) {
	// Validate statically resolvable member calls before emitting bytecode.
	(void)infer_node_type(ctx);
	auto* primary = ctx->primary();

	if (!primary) {
		throw std::runtime_error("Postfix expression has no primary");
	}

	const auto& suffixes = ctx->postfixSuffix();

	// Explicit base constructor invocation: base(...)
	if (primary->BASE() && !suffixes.empty() && suffixes.front()->LPAREN()) {

		if (!current_class || current_class->base_class.empty()) {
			throw std::runtime_error("base(...) requires a derived class");
		}

		const std::string& base_name = current_class->base_class;
		const size_t base_index = resolve_class(base_name);
		const auto& base_class = module.classes.at(base_index);

		auto* args = suffixes.front()->argumentList();
		const size_t argc = args ? args->expression().size() : 0;

		size_t constructor_index = module.functions.size();

		for (size_t candidate : base_class.constructors) {
			if (module.functions.at(candidate).arity == argc) {
				constructor_index = candidate;
				break;
			}
		}

		if (constructor_index == module.functions.size()) {
			throw std::runtime_error("No matching base constructor for '" + base_name + "' with " + std::to_string(argc) + " arguments");
		}

		// Use the existing derived instance.
		emit_load_local(0);

		if (args) {
			for (auto* argument : args->expression()) {
				visit(argument);
			}
		}

		emit_opcode(OpCode::CallMethod);
		emit_u16(static_cast<uint16_t>(constructor_index));
		emit_u16(static_cast<uint16_t>(argc));

		return {};
	}

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

		if (has_local(name)) {
			const size_t slot = resolve_local(name);

			// Postfix result is the old value.
			emit_load_local(slot);

			// Actually mutate the local variable.
			emit_opcode(increment ? OpCode::IncrementLocal : OpCode::DecrementLocal);

			emit_u16(static_cast<uint16_t>(slot));

			return {};
		}

		if (current_class && current_class->field_slots.contains(name)) {
			const size_t field_slot = current_class->field_slots[name];

			// Load old value as the postfix expression result.
			emit_load_local(0);

			emit_opcode(OpCode::LoadField);

			emit_u16(static_cast<uint16_t>(field_slot));

			// Mutate this.field directly.
			emit_load_local(0);

			emit_opcode(increment ? OpCode::IncrementField : OpCode::DecrementField);

			emit_u16(static_cast<uint16_t>(field_slot));

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

		const size_t slot = resolve_local(name);

		// Postfix semantics: leave old value on the stack.
		emit_load_local(slot);

		// Actually mutate the local.
		emit_opcode(suffixes[0]->INC() ? OpCode::IncrementLocal : OpCode::DecrementLocal);

		emit_u16(static_cast<uint16_t>(slot));

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

			for (size_t i = 1; i < suffixes.size(); i++) {
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

			for (size_t i = 1; i < suffixes.size(); i++) {
				compile_postfix_suffix(suffixes[i]);
			}

			return {};
		}
	}

	if (primary->IDENTIFIER() != nullptr && !suffixes.empty() && suffixes[0]->LPAREN() != nullptr) {
		const std::string name = primary->IDENTIFIER()->getText();

		compile_call(name, suffixes[0]->argumentList());

		for (size_t i = 1; i < suffixes.size(); i++) {
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
	size_t count = 0;

	for (auto* expression : ctx->expression()) {
		visit(expression);

		count++;
	}

	emit_opcode(OpCode::ArrayLiteral);

	emit_u16(static_cast<uint16_t>(count));

	return {};
}

// If / else

antlrcpp::Any Compiler::visitIfStatement(TSharpParser::IfStatementContext* ctx) {
	const auto& expressions = ctx->expression();

	const auto& blocks = ctx->block();

	std::vector<size_t> end_jumps;

	for (size_t i = 0; i < expressions.size(); i++) {
		check_condition_type("if", infer_expression_type(expressions[i]));
		visit(expressions[i]);

		const size_t false_jump = emit_jump(OpCode::JumpIfFalse);

		visit(blocks[i]);

		end_jumps.push_back(emit_jump(OpCode::Jump));

		patch_jump(false_jump);
	}

	if (blocks.size() > expressions.size()) {
		visit(blocks.back());
	}

	for (const size_t jump : end_jumps) {
		patch_jump(jump);
	}

	return {};
}

// While

antlrcpp::Any Compiler::visitWhileStatement(TSharpParser::WhileStatementContext* ctx) {
	const size_t loop_start = chunk().code_size();

	check_condition_type("while", infer_expression_type(ctx->expression()));
	visit(ctx->expression());

	const size_t exit_jump = emit_jump(OpCode::JumpIfFalse);

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
	const size_t loop_start = chunk().code_size();

	begin_loop(loop_start);

	visit(ctx->block());

	const size_t condition_start = chunk().code_size();

	patch_continue_jumps(condition_start);

	check_condition_type("do-while", infer_expression_type(ctx->expression()));
	visit(ctx->expression());

	const size_t exit = emit_jump(OpCode::JumpIfFalse);

	emit_loop(loop_start);

	patch_jump(exit);

	end_loop();

	return {};
}

// For

antlrcpp::Any Compiler::visitForStatement(TSharpParser::ForStatementContext* ctx) {
	begin_scope();
	// for item in items
	if (ctx->foreachClause()) {
		auto* foreach_clause = ctx->foreachClause();

		// Evaluate the array expression once.
		visit(foreach_clause->expression());

		const size_t array_slot = declare_temporary();

		emit_store_local(array_slot);

		// Hidden index variable starts at zero.
		emit_constant(Value(0));

		const size_t index_slot = declare_temporary();

		emit_store_local(index_slot);

		// The range variable is visible inside the loop body.
		const std::string item_name = foreach_clause->IDENTIFIER()->getText();

		const size_t item_slot = declare_local(item_name, "any");

		const size_t condition_start = chunk().code_size();

		// index < array.length
		emit_load_local(index_slot);

		emit_load_local(array_slot);

		emit_opcode(OpCode::ArrayLength);

		emit_opcode(OpCode::Less);

		const size_t exit_jump = emit_jump(OpCode::JumpIfFalse);

		// item = array[index]
		emit_load_local(array_slot);

		emit_load_local(index_slot);

		emit_opcode(OpCode::LoadIndex);

		emit_store_local(item_slot);

		begin_loop(condition_start);

		visit(ctx->block());

		// continue jumps must run the index increment first.
		const size_t update_start = chunk().code_size();

		patch_continue_jumps(update_start);

		// index = index + 1
		emit_load_local(index_slot);

		emit_constant(Value(1));

		emit_opcode(OpCode::Add);

		emit_store_local(index_slot);

		emit_loop(condition_start);

		patch_jump(exit_jump);

		end_loop();

		end_scope();

		return {};
	}

	// Classic for loop.
	auto* classic = ctx->classicForClause();

	if (classic->forInit()) {
		auto* init = classic->forInit();

		if (init->variableDecl()) {
			visit(init->variableDecl());
		} else if (init->assignment()) {
			visit(init->assignment());
		} else if (init->expression()) {
			visit(init->expression());

			emit_opcode(OpCode::Pop);
		}
	}

	const size_t condition_start = chunk().code_size();

	size_t exit_jump = std::numeric_limits<size_t>::max();

	if (classic->expression()) {
		visit(classic->expression());

		exit_jump = emit_jump(OpCode::JumpIfFalse);
	}

	begin_loop(condition_start);

	visit(ctx->block());

	const size_t update_start = chunk().code_size();

	patch_continue_jumps(update_start);

	if (classic->forUpdate()) {
		auto* update = classic->forUpdate();

		for (auto* assignment : update->assignment()) {
			visit(assignment);
		}

		for (auto* expression : update->expression()) {
			visit(expression);

			emit_opcode(OpCode::Pop);
		}
	}

	emit_loop(condition_start);

	if (exit_jump != std::numeric_limits<size_t>::max()) {
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

	const size_t switch_slot = declare_temporary();

	emit_store_local(switch_slot);

	begin_loop(chunk().code_size());

	TSharpParser::SwitchSectionContext* default_section = nullptr;

	std::vector<size_t> end_jumps;

	for (auto* section : ctx->switchSection()) {
		if (section->DEFAULT()) {
			default_section = section;

			continue;
		}

		emit_load_local(switch_slot);

		visit(section->expression());

		emit_opcode(OpCode::Equal);

		const size_t next_case = emit_jump(OpCode::JumpIfFalse);

		compile_switch_section(section);

		end_jumps.push_back(emit_jump(OpCode::Jump));

		patch_jump(next_case);
	}

	if (default_section) {
		compile_switch_section(default_section);
	}

	for (const size_t jump : end_jumps) {
		patch_jump(jump);
	}

	end_loop();

	return {};
}

// Return

antlrcpp::Any Compiler::visitReturnStatement(TSharpParser::ReturnStatementContext* ctx) {
	if (ctx->expression()) {
		if (current_return_type == "void") {
			throw std::runtime_error("Type error: cannot return a value from void function");
		}
		auto* expression = ctx->expression();

		const std::string source_name = simple_identifier_from_expression(expression);

		const std::string source_type = infer_expression_type(expression);

		check_nullable_assignment("return value", current_return_type, expression, source_type);

		check_type_compatibility("return value", current_return_type, source_type);

		visit(expression);
	} else {
		if (!current_return_type.empty() && current_return_type != "void") {
			throw std::runtime_error("Cannot return without a value from function returning '" + current_return_type + "'");
		}

		emit_opcode(OpCode::Null);
	}

	emit_opcode(OpCode::Return);
	return {};
}

// Try / catch / finally

antlrcpp::Any Compiler::visitTryStatement(TSharpParser::TryStatementContext* ctx) {
	const size_t handler = emit_exception_handler_placeholder();

	visit(ctx->block());

	emit_opcode(OpCode::PopExceptionHandler);

	const size_t end_try = emit_jump(OpCode::Jump);

	patch_exception_handler(handler, chunk().code_size());

	for (auto* catch_clause : ctx->catchClause()) {
		const size_t exception_slot = declare_local(catch_clause->IDENTIFIER()->getText());

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
	size_t argument_count = 0;

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
		const auto& cls = module.classes.at(module.class_lookup.at(name));
		if (cls.constructors.size() == 1) {
			const auto& fn = module.functions.at(cls.constructors.front());
			if (argument_count != fn.arity) {
				throw std::runtime_error("Type error: constructor '" + name + "' expects " + std::to_string(fn.arity) + " arguments, got " + std::to_string(argument_count));
			}
			if (arguments) {
				const auto& args = arguments->expression();
				for (size_t i = 0; i < args.size() && i < fn.parameter_types.size(); ++i) {
					const auto actual = infer_expression_type(args[i]);
					check_nullable_assignment(name + " constructor argument " + std::to_string(i + 1), fn.parameter_types[i], args[i], actual);
					check_type_compatibility(name + " constructor argument " + std::to_string(i + 1), fn.parameter_types[i], actual);
				}
			}
		}
		emit_opcode(OpCode::NewObject);

		emit_u16(static_cast<uint16_t>(module.class_lookup.at(name)));

		emit_u16(static_cast<uint16_t>(argument_count));

		return;
	}

	if (is_builtin(name)) {
		emit_opcode(OpCode::CallNative);

		emit_u16(static_cast<uint16_t>(resolve_native(name)));

		emit_u16(static_cast<uint16_t>(argument_count));

		return;
	}

	if (module.function_lookup.contains(name)) {
		const auto& fn = module.functions.at(module.function_lookup.at(name));
		if (argument_count != fn.arity) {
			throw std::runtime_error("Type error: function '" + name + "' expects " + std::to_string(fn.arity) + " arguments, got " + std::to_string(argument_count));
		}
		if (arguments) {
			const auto& args = arguments->expression();
			for (size_t i = 0; i < args.size() && i < fn.parameter_types.size(); ++i) {
				const std::string actual = infer_expression_type(args[i]);
				check_nullable_assignment(name + " argument " + std::to_string(i + 1), fn.parameter_types[i], args[i], actual);
				check_type_compatibility(name + " argument " + std::to_string(i + 1), fn.parameter_types[i], actual);
			}
		}
		emit_opcode(OpCode::Call);

		emit_u16(static_cast<uint16_t>(resolve_function(name)));

		emit_u16(static_cast<uint16_t>(argument_count));

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
		const auto& fn = module.functions.at(current_class->methods.at(name));
		if (argument_count != fn.arity) {
			throw std::runtime_error("Type error: method '" + name + "' expects " + std::to_string(fn.arity) + " arguments, got " + std::to_string(argument_count));
		}
		if (arguments) {
			const auto& args = arguments->expression();
			for (size_t i = 0; i < args.size() && i < fn.parameter_types.size(); ++i) {
				const auto actual = infer_expression_type(args[i]);
				check_nullable_assignment(name + " argument " + std::to_string(i + 1), fn.parameter_types[i], args[i], actual);
				check_type_compatibility(name + " argument " + std::to_string(i + 1), fn.parameter_types[i], actual);
			}
		}
		// Pass `this` as the implicit receiver.
		emit_load_local(0);

		emit_opcode(OpCode::Call);

		emit_u16(static_cast<uint16_t>(current_class->methods.at(name)));

		emit_u16(static_cast<uint16_t>(argument_count + 1));

		return;
	}

	throw std::runtime_error("Undefined callable: " + name);
}

void Compiler::compile_spawn(const std::string& name, TSharpParser::ArgumentListContext* arguments) {
	if (!module.function_lookup.contains(name)) {
		throw std::runtime_error("Cannot spawn unknown function: " + name);
	}

	const size_t function_index = resolve_function(name);
	const auto& function = module.functions[function_index];

	if (function.is_method || function.is_constructor) {
		throw std::runtime_error("Cannot spawn methods or constructors yet");
	}

	const size_t argument_count = arguments ? arguments->expression().size() : 0;

	if (argument_count != function.arity) {
		throw std::runtime_error("Incorrect argument count for spawned function: " + name);
	}

	if (function_index > UINT16_MAX || argument_count > UINT16_MAX) {
		throw std::runtime_error("SPAWN operands exceed bytecode limits");
	}

	if (arguments) {
		for (auto* argument : arguments->expression()) {
			visit(argument);
		}
	}

	emit_opcode(OpCode::Spawn);

	emit_u16(static_cast<uint16_t>(function_index));
	emit_u16(static_cast<uint16_t>(argument_count));
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
		size_t argument_count = 0;

		if (ctx->argumentList()) {
			for (auto* argument : ctx->argumentList()->expression()) {
				visit(argument);

				argument_count++;
			}
		}

		emit_opcode(OpCode::CallValue);

		emit_u16(static_cast<uint16_t>(argument_count));

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

		emit_u16(static_cast<uint16_t>(current_class->field_slots[name]));

		return;
	}

	if (module.global_lookup.contains(name)) {
		emit_opcode(OpCode::LoadGlobal);

		emit_u16(static_cast<uint16_t>(resolve_global(name)));

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

void Compiler::begin_loop(size_t continue_target) {
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

	for (const size_t jump : context.break_jumps) {
		patch_jump(jump);
	}
}

void Compiler::patch_continue_jumps(size_t target) {
	if (loop_contexts.empty()) {
		return;
	}

	for (const size_t position : loop_contexts.back().continue_jumps) {
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

size_t Compiler::declare_temporary() {
	const std::string name = "$tmp_" + std::to_string(current_context().next_local);

	return declare_local(name);
}

void Compiler::emit_string_operand(const std::string& value) {
	const size_t index = chunk().add_constant(Value(value));

	emit_u16(static_cast<uint16_t>(index));
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
	return module.native_lookup.contains(name);
}

size_t Compiler::resolve_native(const std::string& name) const {
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

void Compiler::patch_absolute_jump(size_t operand_position, size_t target) {
	if (operand_position + 1 >= chunk().code_size()) {
		throw std::runtime_error("Invalid jump operand position");
	}

	if (target < operand_position + 2) {
		throw std::runtime_error("patch_absolute_jump currently only "
								 "supports forward jumps");
	}

	const size_t offset = target - operand_position - 2;

	if (offset > std::numeric_limits<uint16_t>::max()) {
		throw std::runtime_error("Jump offset too large");
	}

	auto& code = chunk().mutable_code();

	code[operand_position] = static_cast<uint8_t>(offset & 0xff);

	code[operand_position + 1] = static_cast<uint8_t>((offset >> 8) & 0xff);
}
size_t Compiler::emit_exception_handler_placeholder() {
	emit_opcode(OpCode::PushExceptionHandler);

	emit_u16(0xffff);

	return (chunk().code_size() - 2);
}

void Compiler::patch_exception_handler(size_t handler_position, size_t target) {
	if (target < handler_position + 2) {
		throw std::runtime_error("Invalid exception handler target");
	}

	const size_t offset = target - handler_position - 2;

	if (offset > std::numeric_limits<uint16_t>::max()) {
		throw std::runtime_error("Exception handler offset too large");
	}

	auto& code = chunk().mutable_code();

	code[handler_position] = static_cast<uint8_t>(offset & 0xff);

	code[handler_position + 1] = static_cast<uint8_t>((offset >> 8) & 0xff);
}

void Compiler::register_natives() {
	auto register_native = [this](const std::string& name, size_t arity) {
		const size_t index = module.natives.size();

		NativeFunctionInfo info;

		info.name = name;
		info.arity = arity;

		module.native_lookup[name] = index;

		module.natives.push_back(std::move(info));
	};

	register_native("print", 1);
	register_native("println", 1);
	register_native("readln", 0);

	// DateTime
	register_native("datetime_now", 0);
	register_native("datetime_format", 3);
	register_native("datetime_add_days", 2);

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

void Compiler::emit_array_type(const std::string& type) {
	const size_t index = chunk().add_constant(Value(type));

	if (index > std::numeric_limits<uint16_t>::max()) {
		throw std::runtime_error("Too many constants");
	}

	emit_opcode(OpCode::SetArrayType);
	emit_u16(static_cast<uint16_t>(index));
}
}