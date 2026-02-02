// tsharp_listener.h
// Dylan Armstrong, 2026

#pragma once

#include <map>
#include <string>

#include "tsharp_parserBaseListener.h"

class tsharp_listener : public tsharp_parserBaseListener {
private:
    std::map<std::string, int> ints;
    std::map<std::string, std::string> strings;
    std::map<std::string, bool> bools;
    std::map<std::string, char> chars;
    std::map<std::string, double> doubles;
    std::map<std::string, float> floats;
    std::map<std::string, short> shorts;
    std::map<std::string, long> longs;
    
public:
    tsharp_listener();

    const int get_int(const std::string& index) const;
    void add_int(const std::string& index, const int value);

    const std::string get_string(const std::string& index) const;
    void add_string(const std::string& index, const std::string& value);

    void enterPrintln_statement(tsharp_parser::Println_statementContext* ctx) override;
};