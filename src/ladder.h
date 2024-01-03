#pragma once

#include "godot_cpp/classes/static_body3d.hpp"
namespace godot {
class Ladder : public StaticBody3D {
  GDCLASS(Ladder, StaticBody3D)
public:
private:
protected:
  static void _bind_methods();
};
}// namespace godot
