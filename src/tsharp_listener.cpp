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
tsharp_listener::tsharp_listener() : 
    ints{}, strings{}, bools{}, chars{}, doubles{}, floats{}, shorts{}, longs{}, executing(false) {
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

            // If accessing an object's property within println
            else if(ctx->OBJ_NAME) {
                std::shared_ptr<tsharp_class> object = objects.at(ctx->OBJ_NAME->getText());

                tsharp_function f = object->get_method(ctx->PROPERTY_NAME->getText());

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
            }

            // Else if the passed variable name to println is NOT a nullptr
            else if(ctx->VAR) {
                // Check if the integer exists, and is not positioned at the end of the iterator of the map
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

        else if(strings.find(ctx->VAR->getText()) != strings.end()) {
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
    std::vector<tsharp_argument> args;

    for (auto* arg_ctx : ctx->ARGS) {
        tsharp_argument arg{};
        arg.var_name = arg_ctx->NAME->getText();
        arg.type = arg_ctx->TYPE->getText();
        args.push_back(arg);
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
        tsharp_value field_value(0);  // Default to int(0)
        
        if (field->TYPE && field->TYPE->getText() == "int") {
            field_value = tsharp_value(0);
        } else if (field->TYPE && field->TYPE->getText() == "float") {
            field_value = tsharp_value(0.0f);
        } else if (field->TYPE && field->TYPE->getText() == "string") {
            field_value = tsharp_value(std::string(""));
        } else if (field->TYPE && field->TYPE->getText() == "bool") {
            field_value = tsharp_value(false);
        }
        
        c.add_field(field->NAME->getText(), std::move(field_value), field->ACCESS_IDENTIFIER->getText() == "private" ? true : false);
    }

    for (auto* constructor : ctx->CONSTRUCTORS) {
        if (!constructor || !constructor->NAME) continue;
        
        std::vector<tsharp_argument> args;
        if (!constructor->ARGS.empty()) {
            for (auto* arg : constructor->ARGS) {
                if (!arg || !arg->TYPE || !arg->NAME) continue;
                
                tsharp_argument tsharp_arg{};
                tsharp_arg.type = arg->TYPE->getText();
                tsharp_arg.var_name = arg->NAME->getText();
                args.push_back(tsharp_arg);
            }
        }
        c.add_constructor(constructor->NAME->getText(), constructor->NAME->getText(), args, "");
    }

    std::vector<tsharp_argument> args;
    std::string return_val;
    std::string func_type;

    for (auto* method : ctx->METHODS) {
        tsharp_argument arg{};
        arg.var_name = method->NAME->getText();
        arg.type = method->TYPE->getText();
        args.push_back(arg);

        c.add_method(method->NAME->getText(), method->TYPE->getText(), args, method->BODY->return_statement()->VAL->getText());
    }

    classes.emplace(ctx->NAME->getText(), c);
}

void tsharp_listener::enterMain_function(tsharp_parser::Main_functionContext* ctx) {
    executing = true;
}

void tsharp_listener::exitMain_function(tsharp_parser::Main_functionContext* ctx) {
    executing = false;
}

void tsharp_listener::enterObject_inst(tsharp_parser::Object_instContext* ctx) {
    //executing = false;

    std::string class_name = ctx->NAME->getText();
    std::string var_name = ctx->VAR->getText();
    
    objects.emplace(var_name, std::make_shared<tsharp_class>(classes.at(class_name)));
    
    std::shared_ptr<tsharp_class> object = objects.at(var_name);
    tsharp_constructor constructor = object->get_constructor(class_name);
    
    std::vector<tsharp_field> fields = object->get_fields();

    std::vector<tsharp_value> args;
    if (!ctx->ARGS.empty()) {
        for (auto* arg : ctx->ARGS) {
            args.push_back(arg->getText());
        }
    }
    
    constructor.execute(args, *object);
    //executing = true;

    auto* main_fn = dynamic_cast<tsharp_parser::Main_functionContext*>(ctx->parent);
    auto* program = dynamic_cast<tsharp_parser::ProgramContext*>(main_fn->parent);
    
    tsharp_parser::ClassContext* matching_class;
    std::vector<tsharp_parser::ClassContext*> classes_vec = program->class_();

    size_t i = 0;
    for (std::map<std::string, tsharp_class>::iterator iter = classes.begin(); iter != classes.end(); ++iter) {
        if (iter->first == classes_vec.at(i)->NAME->getText()) {
           matching_class = dynamic_cast<tsharp_parser::ClassContext*>(program->class_(i));
        }

        ++i;
    }
    
    tsharp_parser::ConstructorContext* matching_constructor;
    for (size_t j = 0; j < object->get_constructors().size(); j++) {
        if (object->get_constructors().at(j) == constructor) {
            matching_constructor = dynamic_cast<tsharp_parser::ConstructorContext*>(matching_class->constructor(j));
        }
    }

    auto* matching_con_body = dynamic_cast<tsharp_parser::Constructor_bodyContext*>(matching_constructor->constructor_body());

    for (auto* println_statement : matching_con_body->println_statement()) {
        enterPrintln_statement(println_statement);
    }
}