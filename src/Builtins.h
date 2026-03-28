// Builtins.h
// Dylan Armstrong, 2026

#pragma once

#include "Environment.h"

namespace tsharp {
    void install_builtins(const std::shared_ptr<Environment>& global);
}
