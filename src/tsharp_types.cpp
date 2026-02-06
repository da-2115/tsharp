#include "tsharp_types.h"

std::string type_to_string(tsharp_types type) {
    switch (type) {
        case tsharp_types::INT:
            return "int";
        case tsharp_types::STRING:
            return "string";
    }
}