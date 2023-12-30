#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/shape3d.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

struct Trace {
  godot::Vector3 end_pos = godot::Vector3();
  real_t fraction = 0;
  godot::Vector3 normal = godot::Vector3();
  godot::String type = "";
  godot::PackedStringArray groups = godot::PackedStringArray();
  uint32_t num_intersects = 0;
  bool hit = false;

  Trace(godot::PhysicsDirectSpaceState3D* space_state);

  void standard(godot::Vector3 origin, godot::Vector3 dest, const godot::Ref<godot::Shape3D>& shape, godot::RID exclude);
  void count_intersects(godot::Vector3 origin, const godot::Ref<godot::Shape3D>& shape, godot::RID exclude);

private:
  godot::PhysicsDirectSpaceState3D* m_space_state = nullptr;

protected:
  static void _bind_methods();
};
