#pragma once

/// Variadic min().
template<typename T>
T&& vmin(T&& val)
{
    return std::forward<T>(val);
}

template<typename T0, typename T1, typename... Ts>
auto vmin(T0&& val1, T1&& val2, Ts&&... vs)
{
    return (val1 < val2) ?
        vmin(val1, std::forward<Ts>(vs)...) :
        vmin(val2, std::forward<Ts>(vs)...);
}

template<class T>
const T& min(const T& a, const T& b)
{
    return (b < a) ? b : a;
}

template<class T>
const T& max(const T& a, const T& b)
{
    return (a < b) ? b : a;
}
