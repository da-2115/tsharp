// tsharp_listener.cpp
// Dylan Armstrong, 2026

#include "tsharp_listener.h"
#include "tsharp_types.h"
#include "tsharp_math.h"

#include <algorithm>
#include <cstddef>

// Constructor, initialize all member variables with sensible default values.
tsharp_listener::tsharp_listener() : 
    ints{}, strings{}, bools{}, chars{}, doubles{}, floats{}, shorts{}, longs{} {
}

// Integer methods
const int tsharp_listener::get_int(const std::string& index) const {
    return ints.at(index);
}

void tsharp_listener::add_int(const std::string& index, const int value) {
    ints.emplace(index, value);
}

// String methods
const std::string tsharp_listener::get_string(const std::string& index) const {
    return strings.at(index);
}

void tsharp_listener::add_string(const std::string& index, const std::string& value) {
    strings.emplace(index, value);
}

const float tsharp_listener::get_float(const std::string& index) const {
    return floats.at(index);
}

void tsharp_listener::add_float(const std::string& index, const float value) {
    floats.emplace(index, value);
}

// Println statement
void tsharp_listener::enterPrintln_statement(tsharp_parser::Println_statementContext* ctx) {
    // If the message is NOT a nullptr
    if (ctx->MESSAGE) {
        std::string message = ctx->MESSAGE->getText();

        message.erase(std::remove(message.begin(), message.end(), '\"'), message.end());

        std::cout << message << std::endl;
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
        std::cout << tsharp_math::pi<double>() << std::endl;
    }
}

// Print statement
void tsharp_listener::enterPrint_statement(tsharp_parser::Print_statementContext* ctx) {
    // If the message is NOT a nullptr
    if (ctx->MESSAGE) {
        std::string message = ctx->MESSAGE->getText();

        message.erase(std::remove(message.begin(), message.end(), '\"'), message.end());

        std::cout << message << std::flush;
    }

    // Else if the passed variable name to println is NOT a nullptr
    else if (ctx->VAR) {
        if (ints.find(ctx->VAR->getText()) != ints.end()) {
            std::cout << ints.at(ctx->VAR->getText()) << std::flush;
        }

        else if(strings.find(ctx->VAR->getText()) != strings.end()) {
            std::cout << strings.at(ctx->VAR->getText()) << std::flush;
        }
    }
}

// Variable assignment
void tsharp_listener::enterAssignment(tsharp_parser::AssignmentContext* ctx) {
    if (!ctx || !ctx->TYPE || !ctx->NAME || !ctx->VALUE) {
        return;
    }

    // If type is int
    if (ctx->TYPE->getText() == type_to_string(tsharp_types::INT)) {    
        ints.emplace(ctx->NAME->getText(), std::stoi(ctx->VALUE->getText()));
    }

    // Else if type is string
    else if (ctx->TYPE->getText() == type_to_string(tsharp_types::STRING)) {
        std::string value = ctx->VALUE->getText();

        // Get rid of quotes from string
        value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());

        strings.emplace(ctx->NAME->getText(), value);
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
        if (assignment->TYPE->getText() == type_to_string(tsharp_types::INT)) {
            ints.emplace(assignment->NAME->getText(), tsharp_math::sqrt<int>(std::stoi(ctx->NUMBER->getText())));
        }

        else if (assignment->TYPE->getText() == type_to_string(tsharp_types::FLOAT)) {
            floats.emplace(assignment->NAME->getText(), tsharp_math::sqrt<float>(std::stof(ctx->NUMBER->getText())));
        }
    }
}
