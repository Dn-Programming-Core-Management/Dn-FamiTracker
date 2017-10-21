#pragma once

#include <iterator>
#include <utility>
#include <type_traits>

namespace details {

template <typename T>
constexpr auto begin_expr(T&& x) {
	using std::begin;
	return begin(std::forward<T>(x));
}

template <typename T>
constexpr auto end_expr(T&& x) {
	using std::end;
	return end(std::forward<T>(x));
}

template <typename T, typename F>
struct begin_iterator {
	decltype(begin_expr(std::declval<T>())) it_;
	F f_;
	decltype(auto) operator*() {
		return f_(*it_);
	}
	begin_iterator &operator++() {
		++it_;
		return *this;
	}
};
template <typename T, typename F>
struct end_iterator {
	decltype(end_expr(std::declval<T>())) it_;
	F f_;
	decltype(auto) operator*() {
		return f_(*it_);
	}
	end_iterator &operator++() {
		++it_;
		return *this;
	}
};
template <typename T, typename F>
bool operator!=(const begin_iterator<T, F> &b, const end_iterator<T, F> &e) {
	return b.it_ != e.it_;
}

/*constexpr*/ const auto identity = [] (auto&& x) -> decltype(auto) {
	return std::forward<decltype(x)>(x);
};

template <typename F, typename G>
constexpr auto compose(F&& f, G&& g) noexcept {
	return [f = std::forward<F>(f), g = std::forward<G>(g)] (auto&& x) -> decltype(auto) {
		return f(g(std::forward<decltype(x)>(x)));
	};
}

} // namespace details

template <typename T, typename F>
struct proj_t {
	template <typename U>
	proj_t(U&& x, F f) :
		x_(std::forward<T>(x)),
		b_ {details::begin_expr(x_), f},
		e_ {details::end_expr(x_), f}
	{
	}
	auto begin() {
		return b_;
	}
	auto end() {
		return e_;
	}
	template <typename G>
	auto operator|(G&& g) &noexcept {
		auto gf = details::compose(g, b_.f_);
		return proj_t<T, decltype(gf)>(x_, gf);
	}
	template <typename G>
	auto operator|(G&& g) &&noexcept {
		auto gf = details::compose(g, b_.f_);
		return proj_t<T, decltype(gf)>(std::move(x_), gf);
	}
private:
	T x_;
	details::begin_iterator<T&&, F> b_;
	details::end_iterator<T&&, F> e_;
};

template <typename T, typename F>
auto proj(T&& x, F&& f) {
	return proj_t<T, F>(std::forward<T>(x), std::forward<F>(f));
}

template <typename T>
auto proj(T&& x) {
	return proj_t<T, decltype(details::identity)>(std::forward<T>(x), details::identity);
}

template <typename T>
auto proj_deref(T&& x) {
	return proj(std::forward<T>(x), [] (const auto &x) -> decltype(auto) { return *x; });
}

template <typename T>
auto proj_with_index(T&& x) {
	return proj(std::forward<T>(x), [i = 0] (const auto &x) mutable {
		return std::pair<decltype((x)), int>(x, i++);
	});
}
