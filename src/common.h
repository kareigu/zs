#pragma once
#include <godot_cpp/variant/utility_functions.hpp>

#define SGL(CLASS) CLASS::get_singleton()

#define PRINTLN(...) UtilityFunctions::print(__VA_ARGS__)
