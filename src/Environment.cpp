// Environment.cpp
// Dylan Armstrong, 2026

#include "Environment.h"

namespace tsharp {

// Environment constructor
Environment::Environment(std::shared_ptr<Environment> parent) : parent_(std::move(parent)) {}

// Define environment
void Environment::define(const std::string& name, const Value& value) {
    values_.emplace(name, value);
}

// Assign environment
void Environment::assign(const std::string& name, const Value& value) {
    if (values_.contains(name)) {
        values_.at(name) = value;
        return;
    }

    if (parent_) {
        parent_->assign(name, value);
        return;
    }
    
    throw RuntimeError("Undefined variable: " + name);
}

// Get environment value
Value Environment::get(const std::string& name) const {
    auto it = values_.find(name);

    if (it != values_.end()) {
        return it->second;
    }
    
    if (parent_) {
        return parent_->get(name);
    }

    throw RuntimeError("Undefined variable: " + name);
}

// Does a local exist with the same name?
bool Environment::exists_local(const std::string& name) const {
    return values_.contains(name);
}

std::shared_ptr<Environment> Environment::parent() const { 
    return parent_;
}

}
