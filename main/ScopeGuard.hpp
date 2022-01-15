#pragma once

#include "Util.hpp"

#include <utility>

template <class F>
class ScopeGuard {
private:
	F function;
	bool cancelled = false;

public:
	// Delibrately not explicit to allow usages like: ScopeGuard var = lambda;
	ScopeGuard(F&& function) noexcept
		: function{ std::move(function) } {
	}

	~ScopeGuard() noexcept {
		if (!cancelled) {
			function();
		}
	}

	ScopeGuard(const ScopeGuard&) = delete;
	ScopeGuard& operator=(const ScopeGuard&) = delete;

	ScopeGuard(ScopeGuard&& that) noexcept
		: function{ std::move(that.function) } {
		that.Cancel();
	}

	ScopeGuard& operator=(ScopeGuard&& that) noexcept {
		if (!cancelled) {
			function();
		}
		this->function = std::move(that.function);
		this->cancelled = std::exchange(that.cancelled, true);
	}

	auto Cancel() -> void {
		this->cancelled = true;
	}
};

#define SCOPE_GUARD(name) ScopeGuard name = [&]()
#define DEFER ScopeGuard UNIQUE_NAME(scopeGuard) = [&]()
