// Builtins.cpp
// Dylan Armstrong, 2026

#include "Builtins.h"
#include "FunctionValue.h"
#include "Interpreter.h"
#include "Value.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

// value_to_string helper function
// Gets the value as a string - depending on what type it is, for example:
// If it's null: return a string that says "null"
// If it's an actual type e.g. a boolean - return true or false from the Value object
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

// Native function implementation
// Native functions are functions that are native to the language itself (not the std library - as that is written in T# itself)
static std::shared_ptr<FunctionValue> native_fn(const std::string& name, const std::function<Value(Interpreter&, const std::vector<Value>&, const Value&)>& fn) {
	auto f = std::make_shared<FunctionValue>();

	f->name = name;
	f->is_native = true;
	f->native = fn;

	return f;
}

// Defines all builtin functions in T# as globals
// Including: println, print, math functions and math constants
// Does these as C++ lambda expressions
void install_builtins(const std::shared_ptr<Environment>& global) {
	// Math constants in T#
	// Static cast to double
	// pi, e, tau, golden_ratio (phi)
	global->define("pi", Value(static_cast<double>(M_PI)));
	global->define("e", Value(static_cast<double>(std::exp(1.0))));
	global->define("tau", Value(static_cast<double>(2.0 * M_PI)));
	global->define("golden_ratio", Value(static_cast<double>(1.618033988749895)));

	// Print function
	global->define("print", Value(native_fn("print", [](Interpreter&, const std::vector<Value>& args, const Value&) {
					   for (const auto& a : args) {
						   std::cout << value_to_string(a);
					   }

					   return Value();
				   })));

	// Println function - exact same thing as print but has a new line at the end via std::endl
	global->define("println",
				   Value(native_fn("println", [](Interpreter&, const std::vector<Value>& args, const Value&) {
					   for (const auto& a : args) {
						   std::cout << value_to_string(a) << std::endl;
					   }

					   return Value();
				   })));

	// Unary math lambda C++ expression
	// This defines a common implementation for math functions that require a single argument
	auto unary_math = [&](const std::string& name, auto fn) {
		global->define(name, Value(native_fn(name, [fn](Interpreter&, const std::vector<Value>& args, const Value&) {
						   // If EXACTLY one argument is NOT provided - throw a runtime error
						   if (args.size() != 1) {
							   throw RuntimeError("Function expects 1 argument");
						   }

						   // Otherwise, return a Value object with the fn arguments as a double
						   return Value(fn(args[0].as_double()));
					   })));
	};

	// Unary math functions
	// maps to standard library C++ math functions - that can be used in T#
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

	// atan2, takes two arguments - maps to std::atan2 and returns a double
	global->define("atan2", Value(native_fn("atan2", [](Interpreter&, const std::vector<Value>& args, const Value&) {
					   if (args.size() != 2) {
						   throw RuntimeError("atan2 expects 2 arguments");
					   }

					   return Value(std::atan2(args[0].as_double(), args[1].as_double()));
				   })));

	// factorial, takes ONE argument - simple for loop and increase the result - does not map to standard library C++ function
	global->define("factorial",
				   Value(native_fn("factorial", [](Interpreter&, const std::vector<Value>& args, const Value&) {
					   if (args.size() != 1) {
						   throw RuntimeError("factorial expects 1 argument");
					   }

					   int n = args[0].as_int();

					   // Only supports non-negative integer parameters
					   if (n < 0) {
						   throw RuntimeError("factorial requires non-negative integer");
					   }

					   double result = 1;
					   for (int i = 2; i <= n; ++i) {
						   result *= i;
					   }

					   return Value(result);
				   })));

	// x_root - the xth root of any number - takes two arguments: the value inside the root - and the root e.g. 3 is cubed root, 4 is fourth root, 10 is the tenth root, etc.
	global->define("x_root",
				   Value(native_fn("x_root", [](Interpreter&, const std::vector<Value>& args, const Value&) {
					   if (args.size() != 2) {
						   throw RuntimeError("x_root expects 2 arguments: value, root");
					   }

					   return Value(std::pow(args[0].as_double(), 1.0 / args[1].as_double()));
				   })));

	// min - takes two arguments uses std::min - gets the minimum of a set of two numbers
	global->define("min", Value(native_fn("min", [](Interpreter&, const std::vector<Value>& args, const Value&) {
					   if (args.size() != 2) {
						   throw RuntimeError("min expects 2 arguments");
					   }

					   return Value(std::min(args[0].as_double(), args[1].as_double()));
				   })));

	// max - takes two arguments uses std::max - gets the maximum of a set of two numbers
	global->define("max", Value(native_fn("max", [](Interpreter&, const std::vector<Value>& args, const Value&) {
					   if (args.size() != 2) {
						   throw RuntimeError("max expects 2 arguments");
					   }

					   return Value(std::max(args[0].as_double(), args[1].as_double()));
				   })));

	// pow - raise one number to the power of another using std::pow
	global->define("pow", Value(native_fn("pow", [](Interpreter&, const std::vector<Value>& args, const Value&) {
					   if (args.size() != 2) {
						   throw RuntimeError("pow expects 2 arguments");
					   }

					   return Value(std::pow(args[0].as_double(), args[1].as_double()));
				   })));

	// Non-math and non-file built in T# functions
	// Contains the size, address, array functions and the sort function for arrays

	// size function - gets the length of a string, number of items in Array, otherwise will return the size of the object - returns an integer
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

	// address function - returns a hexadecimal address (as a string) to the user of the object in memory passed to the address function
	global->define("address",
				   Value(native_fn("address", [](Interpreter&, const std::vector<Value>& args, const Value&) {
					   if (args.size() != 1) {
						   throw RuntimeError("address expects 1 argument");
					   }

					   const Value& v = args[0];

					   // Use string stream
					   std::ostringstream oss;
					   oss << "0x" << std::hex;

					   if (v.is_instance()) {
						   oss << reinterpret_cast<std::uintptr_t>(v.as_instance().get());
					   } else if (v.is_array()) {
						   oss << reinterpret_cast<std::uintptr_t>(v.as_array().get());
					   } else if (v.is_function()) {
						   oss << reinterpret_cast<std::uintptr_t>(v.as_function().get());
					   } else if (v.is_class()) {
						   oss << reinterpret_cast<std::uintptr_t>(v.as_class().get());
					   } else {
						   // For primitives, this is the address of the Value wrapper copy.
						   oss << reinterpret_cast<std::uintptr_t>(&v);
					   }

					   return Value(oss.str());
				   })));

	// typeof function - return the type of a T# object
	global->define("typeof",
				   Value(native_fn("typeof", [](Interpreter&, const std::vector<Value>& args, const Value&) {
					   if (args.size() != 1) {
						   throw RuntimeError("typeof expects 1 argument");
					   }

					   const Value& v = args[0];
					   if (v.is_null()) {
						   return Value("null");
					   }
					   if (v.is_int()) {
						   return Value("int");
					   }
					   if (v.is_float()) {
						   return Value("float");
					   }
					   if (v.is_double()) {
						   return Value("double");
					   }
					   if (v.is_bool()) {
						   return Value("bool");
					   }
					   if (v.is_char()) {
						   return Value("char");
					   }
					   if (v.is_string()) {
						   return Value("string");
					   }
					   if (v.is_array()) {
						   return Value("array");
					   }
					   if (v.is_instance()) {
						   return Value("instance");
					   }
					   if (v.is_class()) {
						   return Value("class");
					   }
					   if (v.is_function()) {
						   return Value("function");
					   }

					   // By default, return unknown if the type is unknown
					   return Value("unknown");
				   })));

	// Push - push items to an array
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

	// Pop - pop items from an array, array must NOT be empty
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

	// Sort - sorts an array via std::sort
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

	// File I/O Functions
	// Used heavily in the T# std library under the File class

	// file_read_all_text - reads all text data from a file and returns a string with the file text data
	global->define(
		"file_read_all_text",
		Value(native_fn("file_read_all_text", [](Interpreter&, const std::vector<Value>& args, const Value&) {
			if (args.size() != 1) {
				throw RuntimeError("file_read_all_text expects 1 argument");
			}

			std::ifstream file(args[0].as_string());

			if (!file) {
				throw RuntimeError("Could not open file: " + args[0].as_string());
			}

			std::ostringstream buffer;
			buffer << file.rdbuf();

			return Value(buffer.str());
		})));

	// file_write_all_text - writes all data to a file
	global->define(
		"file_write_all_text",
		Value(native_fn("file_write_all_text", [](Interpreter&, const std::vector<Value>& args, const Value&) {
			if (args.size() != 2) {
				throw RuntimeError("file_write_all_text expects 2 arguments");
			}

			std::ofstream file(args[0].as_string());

			if (!file) {
				throw RuntimeError("Could not write file: " + args[0].as_string());
			}

			file << args[1].as_string();

			return Value();
		})));

	// Append data to file
	global->define(
		"file_append_all_text",
		Value(native_fn("file_append_all_text", [](Interpreter&, const std::vector<Value>& args, const Value&) {
			if (args.size() != 2) {
				throw RuntimeError("file_append_all_text expects 2 arguments");
			}

			std::ofstream file(args[0].as_string(), std::ios::app);

			if (!file) {
				throw RuntimeError("Could not append file: " + args[0].as_string());
			}

			file << args[1].as_string();

			return Value();
		})));

	// Does the file exist? Returns a boolean
	global->define("file_exists",
				   Value(native_fn("file_exists", [](Interpreter&, const std::vector<Value>& args, const Value&) {
					   if (args.size() != 1) {
						   throw RuntimeError("file_exists expects 1 argument");
					   }

					   return Value(std::filesystem::exists(args[0].as_string()));
				   })));

	// Delete a file
	global->define("file_delete",
				   Value(native_fn("file_delete", [](Interpreter&, const std::vector<Value>& args, const Value&) {
					   if (args.size() != 1) {
						   throw RuntimeError("file_delete expects 1 argument");
					   }

					   std::filesystem::remove(args[0].as_string());

					   return Value();
				   })));

	// Copy a file into another
	global->define("file_copy",
				   Value(native_fn("file_copy", [](Interpreter&, const std::vector<Value>& args, const Value&) {
					   if (args.size() != 2) {
						   throw RuntimeError("file_copy expects 2 arguments");
					   }

					   std::filesystem::copy_file(args[0].as_string(), args[1].as_string(),
												  std::filesystem::copy_options::overwrite_existing);

					   return Value();
				   })));

	// Move a file into another
	global->define("file_move",
				   Value(native_fn("file_move", [](Interpreter&, const std::vector<Value>& args, const Value&) {
					   if (args.size() != 2) {
						   throw RuntimeError("file_move expects 2 arguments");
					   }

					   std::filesystem::rename(args[0].as_string(), args[1].as_string());

					   return Value();
				   })));
}

}
