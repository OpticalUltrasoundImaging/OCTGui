#pragma once

#include <type_traits>


namespace OCT {

using Float = float;

template <typename T>
concept Floating = std::is_same_v<T, double> || std::is_same_v<T, float>;

}