// tsharp_listener.cpp
// Dylan Armstrong, 2026

#include "tsharp_listener.h"
#include "tsharp_math.h"

#include <algorithm>
#include <chrono>
#include <cstddef>

std::string tsharp_listener::remove_quotes_from_string(std::string& str) const {
	str.erase(std::remove(str.begin(), str.end(), '\"'), str.end());

	return str;
}

// Constructor, initialize all member variables with sensible default values.
tsharp_listener::tsharp_listener() : ints{}, strings{}, bools{}, chars{}, doubles{}, floats{}, shorts{}, longs{}, executing(false) {
}

// Destructor, safely clear maps.
tsharp_listener::~tsharp_listener() {
	ints.clear();
	strings.clear();
	bools.clear();
	chars.clear();
	doubles.clear();
	floats.clear();
	shorts.clear();
	longs.clear();
	functions.clear();
	classes.clear();
	objects.clear();
}

// Integer methods
const int& tsharp_listener::get_int(const std::string& index) const {
	return ints.at(index);
}

void tsharp_listener::add_int(const std::string& index, const int value) {
	ints.emplace(index, value);
}

// String methods
const std::string& tsharp_listener::get_string(const std::string& index) const {
	return strings.at(index);
}

void tsharp_listener::add_string(const std::string& index, const std::string& value) {
	strings.emplace(index, value);
}

const float& tsharp_listener::get_float(const std::string& index) const {
	return floats.at(index);
}

void tsharp_listener::add_float(const std::string& index, const float value) {
	floats.emplace(index, value);
}

// Println statement
void tsharp_listener::enterPrintln_statement(tsharp_parser::Println_statementContext* ctx) {
	if (executing) {
		// If the message is NOT a nullptr
		if (ctx->MESSAGE) {
			std::string message = ctx->MESSAGE->getText();
			std::cout << remove_quotes_from_string(message) << std::endl;
		}

		// If accessing an object's property/method within println
		else if (ctx->OBJ_NAME) {
			try {
				tsharp_object object = objects.at(ctx->OBJ_NAME->getText());

				std::string method_name = ctx->PROPERTY_NAME->getText();
				
				tsharp_function f = object->get_method(method_name);
				std::string return_spec = f.get_ret_value();
				
				// Check if return value is a method call (contains a dot)
				size_t dot_pos = return_spec.find('.');
				if (dot_pos != std::string::npos) {
					// This is a nested method call like "_typeOfHuman.Type()"
					std::string field_name = return_spec.substr(0, dot_pos);
					std::string nested_method_full = return_spec.substr(dot_pos + 1);
					
					// Remove trailing () from method name if present
					std::string nested_method = nested_method_full;
					if (nested_method.length() >= 2 && nested_method.substr(nested_method.length() - 2) == "()") {
						nested_method = nested_method.substr(0, nested_method.length() - 2);
					}
					
					try {
						// Get the field object - the field contains a variable name string
						tsharp_field field = object->get_field(field_name);
						
						// Extract the variable name from the field using the same method as strings
						std::string obj_var_name = std::get<std::string>(field.get_value().get_value());
						
						// Look up the actual object from the objects map
						if (objects.find(obj_var_name) == objects.end()) {
							throw std::runtime_error("Object not found: " + obj_var_name);
						}
						
						tsharp_object field_obj = objects.at(obj_var_name);
						
						tsharp_function nested_f = field_obj->get_method(nested_method);
						
						// Call the nested method and print result
						if (nested_f.get_type() == STRING_TYPE) {
							std::string result = std::get<std::string>(field_obj->get_field(nested_f.get_ret_value()).get_value().get_value());
							std::cout << remove_quotes_from_string(result) << std::endl;
						} else if (nested_f.get_type() == INT_TYPE) {
							std::cout << std::get<int>(field_obj->get_field(nested_f.get_ret_value()).get_value().get_value()) << std::endl;
						} else if (nested_f.get_type() == FLOAT_TYPE) {
							std::cout << std::get<float>(field_obj->get_field(nested_f.get_ret_value()).get_value().get_value()) << std::endl;
						}
					} catch (const std::exception& ex) {
						std::cerr << "ERROR in println object access: " << ex.what() << std::endl;
						throw;
					}
				} else {
					// Simple field reference - original code
					if (f.get_type() == INT_TYPE) {
						std::cout << std::get<int>(object->get_field(f.get_ret_value()).get_value().get_value()) << std::endl;
					}

					else if (f.get_type() == STRING_TYPE) {
						std::string message = std::get<std::string>(object->get_field(f.get_ret_value()).get_value().get_value());
						std::cout << remove_quotes_from_string(message) << std::endl;
					}

					else if (f.get_type() == FLOAT_TYPE) {
						std::cout << std::get<float>(object->get_field(f.get_ret_value()).get_value().get_value()) << std::endl;
					}

                    else {
                        std::cout << ctx->OBJ_NAME->getText() << std::endl;
                    }
				}
			} catch (const std::exception& e) {
				std::cerr << "ERROR in println object access: " << e.what() << std::endl;
			}
		}

		// Else if the passed variable name to println is NOT a nullptr
		else if (ctx->VAR) {
			// Check if the integer exists, and is not positioned at the end of
			// the iterator of the map
			if (ints.find(ctx->VAR->getText()) != ints.end()) {
				std::cout << ints.at(ctx->VAR->getText()) << std::endl;
			}

			else if (strings.find(ctx->VAR->getText()) != strings.end()) {
				std::cout << strings.at(ctx->VAR->getText()) << std::endl;
			}

			else if (floats.find(ctx->VAR->getText()) != floats.end()) {
				std::cout << floats.at(ctx->VAR->getText()) << std::endl;
			}
		}

		// Println for pi (3.14159). NOTE: Prints as type double by default
		else if (ctx->PI_CONST) {
			std::cout << tsharp_math::pi<double> << std::endl;
		}
	}
}

// Print statement
void tsharp_listener::enterPrint_statement(tsharp_parser::Print_statementContext* ctx) {
	// If the message is NOT a nullptr
	if (ctx->MESSAGE) {
		std::string message = ctx->MESSAGE->getText();
		std::cout << remove_quotes_from_string(message) << std::flush;
	}

	// Else if the passed variable name to println is NOT a nullptr
	else if (ctx->VAR) {
		if (ints.find(ctx->VAR->getText()) != ints.end()) {
			std::cout << ints.at(ctx->VAR->getText()) << std::flush;
		}

		else if (strings.find(ctx->VAR->getText()) != strings.end()) {
			std::cout << strings.at(ctx->VAR->getText()) << std::flush;
		}

		else if (floats.find(ctx->VAR->getText()) != floats.end()) {
			std::cout << floats.at(ctx->VAR->getText()) << std::flush;
		}
	}

	// Print for pi (3.14159). NOTE: Prints as type double by default
	else if (ctx->PI_CONST) {
		std::cout << tsharp_math::pi<double> << std::flush;
	}
}

// Variable assignment
void tsharp_listener::enterAssignment(tsharp_parser::AssignmentContext* ctx) {
	if (!ctx || !ctx->TYPE || !ctx->NAME || !ctx->VALUE) {
		return;
	}

	// If type is int
	if (ctx->TYPE->getText() == INT_TYPE) {
		ints.emplace(ctx->NAME->getText(), std::stoi(ctx->VALUE->getText()));
	}

	// Else if type is string
	else if (ctx->TYPE->getText() == STRING_TYPE) {
		std::string message = ctx->VALUE->getText();

		strings.emplace(ctx->NAME->getText(), remove_quotes_from_string(message));
	}

	// Else if type is float
	else if (ctx->TYPE->getText() == FLOAT_TYPE) {
		floats.emplace(ctx->NAME->getText(), std::stof(ctx->VALUE->getText()));
	}
}

// Square root function
void tsharp_listener::enterSquare_root(tsharp_parser::Square_rootContext* ctx) {
	// If has come from println statement
	if (dynamic_cast<tsharp_parser::Println_statementContext*>(ctx->parent)) {
		std::string number = ctx->NUMBER->getText();

		if (ctx->NUMBER && number.find('f') != std::string::npos) {
			std::cout << tsharp_math::sqrt<float>(std::stof(number)) << std::endl;
		}

		else if (ctx->NUMBER) {
			std::cout << tsharp_math::sqrt<int>(std::stoi(number)) << std::endl;
		}

		else if (ctx->VAR) {
			std::cout << tsharp_math::sqrt<int>(ints.at(ctx->VAR->getText())) << std::endl;
		}
	}

	// If being assigned to a variable
	else if (auto* assignment = dynamic_cast<tsharp_parser::AssignmentContext*>(ctx->parent)) {
		if (assignment->TYPE->getText() == INT_TYPE) {
			ints.emplace(assignment->NAME->getText(), tsharp_math::sqrt<int>(std::stoi(ctx->NUMBER->getText())));
		}

		else if (assignment->TYPE->getText() == FLOAT_TYPE) {
			floats.emplace(assignment->NAME->getText(), tsharp_math::sqrt<float>(std::stof(ctx->NUMBER->getText())));
		}
	}
}

void tsharp_listener::enterAbsolute_value(tsharp_parser::Absolute_valueContext* ctx) {
	// If has come from println statement
	if (dynamic_cast<tsharp_parser::Println_statementContext*>(ctx->parent)) {
		std::string number = ctx->NUMBER->getText();

		if (ctx->NUMBER && number.find('f') != std::string::npos) {
			std::cout << tsharp_math::abs<float>(std::stof(number)) << std::endl;
		}

		else if (ctx->NUMBER) {
			std::cout << tsharp_math::abs<int>(std::stoi(number)) << std::endl;
		}

		else if (ctx->VAR) {
			std::cout << tsharp_math::abs<int>(ints.at(ctx->VAR->getText())) << std::endl;
		}
	}

	// If being assigned to a variable
	else if (auto* assignment = dynamic_cast<tsharp_parser::AssignmentContext*>(ctx->parent)) {
		if (assignment->TYPE->getText() == INT_TYPE) {
			ints.emplace(assignment->NAME->getText(), tsharp_math::abs<int>(std::stoi(ctx->NUMBER->getText())));
		}

		else if (assignment->TYPE->getText() == FLOAT_TYPE) {
			floats.emplace(assignment->NAME->getText(), tsharp_math::abs<float>(std::stof(ctx->NUMBER->getText())));
		}
	}
}

void tsharp_listener::enterExpression(tsharp_parser::ExpressionContext* ctx) {
	if (ctx->PLUS()) {
		if (ints.find(ctx->NAME->getText()) != ints.end()) {
			ints.at(ctx->NAME->getText()) = ints.at(ctx->VAR->getText()) + std::stoi(ctx->VALUE->getText());
		}

		else if (floats.find(ctx->NAME->getText()) != floats.end()) {
			floats.at(ctx->NAME->getText()) = floats.at(ctx->VAR->getText()) + std::stof(ctx->VALUE->getText());
		}
	}

	else if (ctx->MINUS()) {
		if (ints.find(ctx->NAME->getText()) != ints.end()) {
			ints.at(ctx->NAME->getText()) = ints.at(ctx->VAR->getText()) - std::stoi(ctx->VALUE->getText());
		}

		else if (floats.find(ctx->NAME->getText()) != floats.end()) {
			floats.at(ctx->NAME->getText()) = floats.at(ctx->VAR->getText()) - std::stof(ctx->VALUE->getText());
		}
	}
}

void tsharp_listener::enterFunction(tsharp_parser::FunctionContext* ctx) {
	// Skip if this function is actually a method inside a class
	auto class_parent = dynamic_cast<tsharp_parser::ClassContext*>(ctx->parent);
	if (class_parent) {
		return;
	}
	
	std::vector<tsharp_argument> args;

	for (auto* arg_ctx : ctx->ARGS) {
		tsharp_argument arg{};
		arg.var_name = arg_ctx->NAME->getText();
		arg.type = arg_ctx->TYPE->getText();
		args.push_back(arg);
	}

	if (!ctx->TYPE || !ctx->BODY || !ctx->BODY->return_statement() || !ctx->BODY->return_statement()->VAL) {
		return;
	}

	tsharp_function function = tsharp_function(ctx->TYPE->getText(), args, ctx->BODY->return_statement()->VAL->getText());

	functions.emplace(ctx->NAME->getText(), function);
}

void tsharp_listener::enterFunc_call(tsharp_parser::Func_callContext* ctx) {
	if (dynamic_cast<tsharp_parser::Println_statementContext*>(ctx->parent) && functions.at(ctx->NAME->getText()).get_type() == INT_TYPE) {
		std::cout << functions.at(ctx->NAME->getText()).func_return<int>(functions.at(ctx->NAME->getText()).get_ret_value()) << std::endl;
	}
}

void tsharp_listener::enterClass(tsharp_parser::ClassContext* ctx) {
	executing = false;
	tsharp_class c;
	for (auto* field : ctx->FIELDS) {
		// Create field with proper type based on field declaration
		tsharp_value field_value(0); // Default to int(0)

		if (field->TYPE && field->TYPE->getText() == "int") {
			field_value = tsharp_value(0);
		} else if (field->TYPE && field->TYPE->getText() == "float") {
			field_value = tsharp_value(0.0f);
		} else if (field->TYPE && field->TYPE->getText() == "string") {
			field_value = tsharp_value(std::string(""));
		} else if (field->TYPE && field->TYPE->getText() == "bool") {
			field_value = tsharp_value(false);
		} else if (field->TYPE) {
			// For complex types, use std::any with nullptr placeholder
			field_value = tsharp_value(std::any());
		}

		c.add_field(field->NAME->getText(), std::move(field_value), field->ACCESS_IDENTIFIER->getText() == "private" ? true : false);
	}

	for (auto* constructor : ctx->CONSTRUCTORS) {
		if (!constructor || !constructor->NAME)
			continue;

		std::vector<tsharp_argument> args;
		if (!constructor->ARGS.empty()) {
			for (auto* arg : constructor->ARGS) {
				if (!arg || !arg->TYPE || !arg->NAME)
					continue;

				tsharp_argument tsharp_arg{};
				tsharp_arg.type = arg->TYPE->getText();
				tsharp_arg.var_name = arg->NAME->getText();
				args.push_back(tsharp_arg);
			}
		}
		c.add_constructor(constructor->NAME->getText(), constructor->NAME->getText(), args, "");
	}

	for (auto* method : ctx->METHODS) {
		if (!method->TYPE) {
			continue;
		}
		
		std::string method_type = method->TYPE->getText();
		auto ret_stmt = method->BODY->return_statement();
		
		std::string return_value = "";
		if (ret_stmt->VAL) {
			return_value = ret_stmt->VAL->getText();
		} else if (ret_stmt->OBJ_NAME) {
			// For method calls like obj.method(), construct the return value
			return_value = ret_stmt->OBJ_NAME->getText() + "." + ret_stmt->PROPERTY_NAME->getText();
		}
		
		std::vector<tsharp_argument> empty_args;
		c.add_method(method->NAME->getText(), method_type, empty_args, return_value);
	}

	classes.emplace(ctx->NAME->getText(), c);
}
// End of enterClass function

void tsharp_listener::enterMain_function(tsharp_parser::Main_functionContext* ctx) {
	try {
		executing = true;
	} catch (const std::exception& e) {
		std::cerr << "ERROR in enterMain_function: " << e.what() << std::endl;
	}
}

void tsharp_listener::exitMain_function(tsharp_parser::Main_functionContext* ctx) {
	executing = false;
}

void tsharp_listener::enterObject_inst(tsharp_parser::Object_instContext* ctx) {
	try {
		std::string class_name = ctx->NAME->getText();
		std::string var_name = ctx->VAR->getText();

		objects.emplace(var_name, std::make_shared<tsharp_class>(classes.at(class_name)));

		tsharp_object object = objects.at(var_name);
		tsharp_constructor constructor = object->get_constructor(class_name);

		std::vector<tsharp_field> fields = object->get_fields();

		std::vector<tsharp_value> args;
		if (!ctx->ARGS.empty()) {
			for (auto* arg : ctx->ARGS) {
				std::string arg_text = arg->VALUE->getText();
				
				// Check if this is a variable reference to an object
				if (objects.find(arg_text) != objects.end()) {
					// It's an object variable - store the variable name as a string
					// The field will contain the name, which we can resolve later
					args.push_back(arg_text);
				} else {
					// It's a literal value
					args.push_back(arg_text);
				}
			}
		}

		constructor.execute(args, *object);

		auto* main_fn = dynamic_cast<tsharp_parser::Main_functionContext*>(ctx->parent);
		auto* program = dynamic_cast<tsharp_parser::ProgramContext*>(main_fn->parent);

		std::vector<tsharp_parser::ClassContext*> classes_vec = program->class_();

		size_t i = 0;
		tsharp_parser::ClassContext* matching_class = nullptr;
		for (std::map<std::string, tsharp_class>::iterator iter = classes.begin(); iter != classes.end(); ++iter) {
			if (iter->first == class_name) {
				// Found our class
				if (i < classes_vec.size()) {
					matching_class = dynamic_cast<tsharp_parser::ClassContext*>(program->class_(i));
				}
				break;
			}
			++i;
		}
		
		if (matching_class) {
			tsharp_parser::ConstructorContext* matching_constructor = nullptr;
			for (size_t j = 0; j < matching_class->constructor().size(); j++) {
				auto* ctor = matching_class->constructor(j);
				if (ctor && ctor->NAME->getText() == class_name) {
					matching_constructor = ctor;
					break;
				}
			}

			if (matching_constructor) {
				auto* matching_con_body = dynamic_cast<tsharp_parser::Constructor_bodyContext*>(matching_constructor->constructor_body());
				if (matching_con_body) {
					for (auto* println_statement : matching_con_body->println_statement()) {
						enterPrintln_statement(println_statement);
					}
				}
			}
		}
	} catch (const std::exception& e) {
		std::cerr << "ERROR in enterObject_inst: " << e.what() << std::endl;
	} catch (...) {
		std::cerr << "ERROR: Unknown exception in enterObject_inst" << std::endl;
	}
}