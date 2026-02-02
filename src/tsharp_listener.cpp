#include "tsharp_listener.h"

#include <algorithm>
#include <cstddef>

tsharp_listener::tsharp_listener() : 
    ints{}, strings{}, bools{}, chars{}, doubles{}, floats{}, shorts{}, longs{} {
}

const int tsharp_listener::get_int(const std::string& index) const {
    return ints.at(index);
}

void tsharp_listener::add_int(const std::string& index, const int value) {
    ints.emplace(index, value);
}

const std::string tsharp_listener::get_string(const std::string& index) const {
    return strings.at(index);
}

void tsharp_listener::add_string(const std::string& index, const std::string& value) {
    strings.emplace(index, value);
}

void tsharp_listener::enterPrintln_statement(tsharp_parser::Println_statementContext* ctx) {
    if (ctx->MESSAGE) {
        std::string message = ctx->MESSAGE->getText();

        message.erase(std::remove(message.begin(), message.end(), '\"'), message.end());

        std::cout << message << std::endl;
    }

    else if(ctx->VAR) {
        if (ints.find(ctx->VAR->getText()) != ints.end()) {
            std::cout << ints.at(ctx->VAR->getText()) << std::endl;
        }

        else if(strings.find(ctx->VAR->getText()) != strings.end()) {
            std::cout << strings.at(ctx->VAR->getText()) << std::endl;
        }
    }
}

void tsharp_listener::enterPrint_statement(tsharp_parser::Print_statementContext* ctx) {
    if (ctx->MESSAGE) {
        std::string message = ctx->MESSAGE->getText();

        message.erase(std::remove(message.begin(), message.end(), '\"'), message.end());

        std::cout << message << std::ends;
    }

    else if(ctx->VAR) {
        if (ints.find(ctx->VAR->getText()) != ints.end()) {
            std::cout << ints.at(ctx->VAR->getText()) << std::ends;
        }

        else if(strings.find(ctx->VAR->getText()) != strings.end()) {
            std::cout << strings.at(ctx->VAR->getText()) << std::ends;
        }
    }
}

void tsharp_listener::enterAssignment(tsharp_parser::AssignmentContext* ctx) {
    if (ctx->TYPE->getText() == "int") {
        ints.emplace(ctx->NAME->getText(), std::stoi(ctx->VALUE->getText()));
    }

    else if (ctx->TYPE->getText() == "string") {
        std::string value = ctx->VALUE->getText();

        value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());

        strings.emplace(ctx->NAME->getText(), value);
    }
}
