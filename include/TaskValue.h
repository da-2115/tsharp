// TaskValue.h
// T# v3.0.0-alpha1
// Dylan Armstrong, 2026

#pragma once

#include "Value.h"

#include <future>
#include <memory>
#include <utility>

namespace tsharp {

struct TaskValue {
	std::shared_future<Value> future;

	explicit TaskValue(std::future<Value>&& f) : future(f.share()) {
	}
};

using TaskHandle = std::shared_ptr<TaskValue>;

}