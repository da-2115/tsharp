// Builtins.cpp
// Dylan Armstrong, 2026

#include "Builtins.h"
#include "Interpreter.h"
#include "FunctionValue.h"
#include "Value.h"

#include <cmath>
#include <iostream>
#include <sstream>

static std::string value_to_string(const tsharp::Value& v) {
    if (v.is_null()) {
		return "null";
	}

    if (v.is_int()) {
        return std::to_string(v.as_int());
    }

    if (v.is_double()) {
        std::ostringstream oss;

        oss << v.as_double();

        return oss.str();
    }

    if (v.is_float()) {
        std::ostringstream oss;

        oss << v.as_float();

        return oss.str();
    }

    if (v.is_bool()) {
        return v.as_bool() ? "true" : "false";
    }

    if (v.is_string()) {
        return v.as_string();
    }

    if (v.is_char()) {
        return std::string(1, v.as_char());
    }

    if (v.is_array()) {
        return v.as_string();
    }

    if (v.is_instance()) {
        return "<instance>";
    }

    if (v.is_class()) {
        return "<class>";
    }

    if (v.is_function()) {
        return "<function>";
    }

    return "<value>";
}

namespace tsharp {

static std::shared_ptr<FunctionValue> native_fn(const std::string& name, const std::function<Value(Interpreter&, const std::vector<Value>&, const Value&)>& fn) {
	auto f = std::make_shared<FunctionValue>();

	f->name = name;
	f->is_native = true;
	f->native = fn;

	return f;
}

// Defines all builtin functions in T#
// Including: println, print, math functions and math constants
// Does these as C++ lambda expressions
void install_builtins(const std::shared_ptr<Environment>& global) {
	global->define("pi", Value(static_cast<double>(M_PI)));
	global->define("e", Value(static_cast<double>(std::exp(1.0))));
	global->define("tau", Value(static_cast<double>(2.0 * M_PI)));
	global->define("golden_ratio", Value(1.618033988749895));

	global->define("print", Value(native_fn("print", [](Interpreter&, const std::vector<Value>& args, const Value&) {
		for (const auto& a : args) {
			std::cout << value_to_string(args[0]);	
		}

		return Value();
	})));

	global->define("println", Value(native_fn("println", [](Interpreter&, const std::vector<Value>& args, const Value&) {
		for (const auto& a : args) {
			std::cout << value_to_string(args[0]) << std::endl;
		}

		return Value();
	})));

	auto unary_math = [&](const std::string& name, auto fn) {
		global->define(name, Value(native_fn(name, [fn](Interpreter&, const std::vector<Value>& args, const Value&) {

			if (args.size() != 1) {
				throw RuntimeError("Function expects 1 argument");
			}

			return Value(fn(args[0].as_double()));
		})));
	};

	unary_math("abs", static_cast<double (*)(double)>(std::fabs));
	unary_math("sqrt", static_cast<double (*)(double)>(std::sqrt));
	unary_math("cube_root", static_cast<double (*)(double)>(std::cbrt));
	unary_math("exp", static_cast<double (*)(double)>(std::exp));
	unary_math("log", static_cast<double (*)(double)>(std::log));
	unary_math("sin", static_cast<double (*)(double)>(std::sin));
	unary_math("cos", static_cast<double (*)(double)>(std::cos));
	unary_math("tan", static_cast<double (*)(double)>(std::tan));
	unary_math("floor", static_cast<double (*)(double)>(std::floor));
	unary_math("ceil", static_cast<double (*)(double)>(std::ceil));
	unary_math("round", static_cast<double (*)(double)>(std::round));

	global->define("factorial", Value(native_fn("factorial", [](Interpreter&, const std::vector<Value>& args, const Value&) {
		if (args.size() != 1) {
			throw RuntimeError("factorial expects 1 argument");
		}

		int n = args[0].as_int();
		
		if (n < 0) {
			throw RuntimeError("factorial requires non-negative integer");
		}

		double result = 1;
		for (int i = 2; i <= n; ++i) {
			result *= i;  
		}
      
		return Value(result);
	})));

	global->define("x_root", Value(native_fn("x_root", [](Interpreter&, const std::vector<Value>& args, const Value&) {
		if (args.size() != 2) {
			throw RuntimeError("x_root expects 2 arguments: value, root");
		}
			               
		return Value(std::pow(args[0].as_double(), 1.0 / args[1].as_double()));
	})));

	global->define("min", Value(native_fn("min", [](Interpreter&, const std::vector<Value>& args, const Value&) {
		if (args.size() != 2) {
			throw RuntimeError("min expects 2 arguments");
		}
			               
		return Value(std::min(args[0].as_double(), args[1].as_double()));
	})));

	global->define("max", Value(native_fn("max", [](Interpreter&, const std::vector<Value>& args, const Value&) {
		if (args.size() != 2) {
			throw RuntimeError("max expects 2 arguments");
		}
			               
		return Value(std::max(args[0].as_double(), args[1].as_double()));
	})));
    
    global->define("pow", Value(native_fn("pow", [](Interpreter&, const std::vector<Value>& args, const Value&) {
		if (args.size() != 2) {
			throw RuntimeError("pow expects 2 arguments");
		}
			               
		return Value(std::pow(args[0].as_double(), args[1].as_double()));
	})));

	global->define("size", Value(native_fn("size", [](Interpreter&, const std::vector<Value>& args, const Value&) {
        if (args.size() != 1) {
            throw RuntimeError("size expects 1 argument");
        }

        const Value& v = args[0];

        if (v.is_string()) {
            return Value(static_cast<int>(v.as_string().size()));
        }

        if (v.is_array()) {
            return Value(static_cast<int>(v.as_array()->size()));
        }

        return Value(static_cast<int>(sizeof(v)));
    })));

	global->define("typeof", Value(native_fn("typeof", [](Interpreter&, const std::vector<Value>& args, const Value&) {
        if (args.size() != 1) {
            throw RuntimeError("typeof expects 1 argument");
        }

        const Value& v = args[0];
        if (v.is_null()) return Value("null");
        if (v.is_int()) return Value("int");
        if (v.is_float()) return Value("float");
        if (v.is_double()) return Value("double");
        if (v.is_bool()) return Value("bool");
        if (v.is_char()) return Value("char");
        if (v.is_string()) return Value("string");
        if (v.is_array()) return Value("array");
        if (v.is_instance()) return Value("instance");
        if (v.is_class()) return Value("class");
        if (v.is_function()) return Value("function");
        return Value("unknown");
    })));

    global->define("push", Value(native_fn("push", [](Interpreter&, const std::vector<Value>& args, const Value&) {
        if (args.size() != 2) {
            throw RuntimeError("push expects 2 arguments");
        }

        if (!args[0].is_array()) {
            throw RuntimeError("push expects first argument to be an array");
        }

        args[0].as_array()->push_back(args[1]);
        return Value();
    })));

    global->define("pop", Value(native_fn("pop", [](Interpreter&, const std::vector<Value>& args, const Value&) {
        if (args.size() != 1) {
            throw RuntimeError("pop expects 1 argument");
        }

        if (!args[0].is_array()) {
            throw RuntimeError("pop expects argument to be an array");
        }

        auto arr = args[0].as_array();
        if (arr->empty()) {
            throw RuntimeError("pop on empty array");
        }

        Value last = arr->back();
        arr->pop_back();
        return last;
    })));

    global->define("sort", Value(native_fn("sort", [](Interpreter&, const std::vector<Value>& args, const Value&) {
        if (args.size() != 1) {
            throw RuntimeError("sort expects 1 argument");
        }

        if (!args[0].is_array()) {
            throw RuntimeError("sort expects argument to be an array");
        }

        auto arr = args[0].as_array();
        std::sort(arr->begin(), arr->end(), [](const Value& a, const Value& b) {
            if (a.is_number() && b.is_number()) {
                return a.as_double() < b.as_double();
            }
            if (a.is_string() && b.is_string()) {
                return a.as_string() < b.as_string();
            }
            return false;
        });

        return Value();
    })));
}

}
