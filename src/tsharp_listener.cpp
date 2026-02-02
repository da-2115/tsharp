#include "tsharp_listener.h"

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
    std::cout << ctx->MESSAGE->getText() << std::endl;
}