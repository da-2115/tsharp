// Builtins.h
// T# v1.0.0-betarc (v1.0.0 Beta Release Candidate/Final Beta)
// Dylan Armstrong, 2026

#pragma once

#include "Environment.h"

namespace tsharp {
void install_builtins(const std::shared_ptr<Environment>& global);
}
