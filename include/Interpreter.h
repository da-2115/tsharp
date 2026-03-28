// Interpreter.h
// Dylan Armstrong, 2026

#pragma once

#include "Builtins.h"
#include "RuntimeModel.h"
#include "Value.h"
#include "antlr4-runtime.h"
#include "TSharpBaseVisitor.h"
#include "TSharpLexer.h"
#include "TSharpParser.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace tsharp {

// The all mighty interpreter of T#
class Interpreter : public TSharpBaseVisitor {
public:
    // Interpreter constructor
    Interpreter();

    // Execute method, takes an argument of program, which is a pointer of the TSharpParser::ProgramContext type
    void execute(TSharpParser::ProgramContext* program);

    // Call function - return a value
    Value call_function(const Value& callee, const std::vector<Value>& args, const Value& thisValue = Value());

    // Evaluation function - takes an ANTLR parsetree node as an argument
    Value evaluate(antlr4::tree::ParseTree* node);

    // Get globals, environment and set environment
    std::shared_ptr<Environment> get_globals() const;
    std::shared_ptr<Environment> get_environment() const;
    void set_environment(const std::shared_ptr<Environment>& env);

    // Get and register classes
    std::shared_ptr<ClassValue> get_class_by_name(const std::string& name) const;
    void register_class(const std::shared_ptr<ClassValue>& class_val);

    // Visitor method implementations - from ANTLR generated visitor
    antlrcpp::Any visitProgram(TSharpParser::ProgramContext* ctx) override;
    antlrcpp::Any visitFunctionDecl(TSharpParser::FunctionDeclContext* ctx) override;
    antlrcpp::Any visitClassDecl(TSharpParser::ClassDeclContext* ctx) override;
    antlrcpp::Any visitInterfaceDecl(TSharpParser::InterfaceDeclContext* ctx) override;
    antlrcpp::Any visitEnumDecl(TSharpParser::EnumDeclContext* ctx) override;
    antlrcpp::Any visitVariableStatement(TSharpParser::VariableStatementContext* ctx) override;
    antlrcpp::Any visitVariableDecl(TSharpParser::VariableDeclContext* ctx) override;
    antlrcpp::Any visitAssignment(TSharpParser::AssignmentContext* ctx) override;
    antlrcpp::Any visitBlock(TSharpParser::BlockContext* ctx) override;
    antlrcpp::Any visitIfStatement(TSharpParser::IfStatementContext* ctx) override;
    antlrcpp::Any visitWhileStatement(TSharpParser::WhileStatementContext* ctx) override;
    antlrcpp::Any visitDoWhileStatement(TSharpParser::DoWhileStatementContext* ctx) override;
    antlrcpp::Any visitForStatement(TSharpParser::ForStatementContext* ctx) override;
    antlrcpp::Any visitSwitchStatement(TSharpParser::SwitchStatementContext* ctx) override;
    antlrcpp::Any visitBreakStatement(TSharpParser::BreakStatementContext* ctx) override;
    antlrcpp::Any visitContinueStatement(TSharpParser::ContinueStatementContext* ctx) override;
    antlrcpp::Any visitReturnStatement(TSharpParser::ReturnStatementContext* ctx) override;
    antlrcpp::Any visitThrowStatement(TSharpParser::ThrowStatementContext* ctx) override;
    antlrcpp::Any visitTryStatement(TSharpParser::TryStatementContext* ctx) override;
    antlrcpp::Any visitExpressionStatement(TSharpParser::ExpressionStatementContext* ctx) override;
    antlrcpp::Any visitExpression(TSharpParser::ExpressionContext* ctx) override;
    antlrcpp::Any visitLogicalOrExpression(TSharpParser::LogicalOrExpressionContext* ctx) override;
    antlrcpp::Any visitLogicalAndExpression(TSharpParser::LogicalAndExpressionContext* ctx) override;
    antlrcpp::Any visitEqualityExpression(TSharpParser::EqualityExpressionContext* ctx) override;
    antlrcpp::Any visitRelationalExpression(TSharpParser::RelationalExpressionContext* ctx) override;
    antlrcpp::Any visitAdditiveExpression(TSharpParser::AdditiveExpressionContext* ctx) override;
    antlrcpp::Any visitMultiplicativeExpression(TSharpParser::MultiplicativeExpressionContext* ctx) override;
    antlrcpp::Any visitUnaryExpression(TSharpParser::UnaryExpressionContext* ctx) override;
    antlrcpp::Any visitPostfixExpression(TSharpParser::PostfixExpressionContext* ctx) override;
    antlrcpp::Any visitPrimary(TSharpParser::PrimaryContext* ctx) override;
    antlrcpp::Any visitArrayLiteral(TSharpParser::ArrayLiteralContext* ctx) override;
    antlrcpp::Any visitLiteral(TSharpParser::LiteralContext* ctx) override;

private:
    // T# state member variables
    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> env;
    std::unordered_map<std::string, std::shared_ptr<ClassValue>> classes;
    std::unordered_map<std::string, std::shared_ptr<FunctionValue>> functions;

    // Private member functions for building functions, methods...
    std::shared_ptr<FunctionValue> build_function(TSharpParser::FunctionDeclContext* ctx, bool is_method);
    std::shared_ptr<FunctionValue> build_method(TSharpParser::MethodDeclContext* ctx);

    // ...for evalulating arguments, getting and setting member variables, constructing objects and evaluating binary chain
    std::vector<Value> eval_arguments(TSharpParser::ArgumentListContext* ctx);
    Value get_member(const Value& target, const std::string& name, bool from_base = false);
    void set_member(const Value& target, const std::string& name, const Value& value);
    Value construct_object(const std::string& type_name, const std::vector<Value>& args);
    Value eval_binary_chain(const std::vector<TSharpParser::AdditiveExpressionContext*>&);
};

}
