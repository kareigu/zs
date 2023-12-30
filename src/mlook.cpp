#include "mlook.h"
#include "common.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/core/math.hpp>

void MLook::_bind_methods() {
  godot::ClassDB::bind_method(godot::D_METHOD("sensitivity"), &MLook::sensitivity);
  godot::ClassDB::bind_method(godot::D_METHOD("set_sensitivity", "sensitivity"),
                              &MLook::set_sensitivity);
  godot::ClassDB::add_property("MLook",
                               godot::PropertyInfo(godot::Variant::FLOAT, "sensitivity"),
                               "set_sensitivity", "sensitivity");
  godot::ClassDB::bind_method(godot::D_METHOD("head_height"), &MLook::head_height);
  godot::ClassDB::bind_method(godot::D_METHOD("set_head_height", "head_height"),
                              &MLook::set_head_height);
  godot::ClassDB::add_property("MLook",
                               godot::PropertyInfo(godot::Variant::FLOAT, "head_height"),
                               "set_head_height", "head_height");
}

void MLook::_ready() {
  m_camera = get_node<godot::Camera3D>("view");
  m_player = cast_to<godot::CharacterBody3D>(get_parent());

  if (SGL(godot::Engine)->is_editor_hint())
    return;
  SGL(godot::Input)->set_mouse_mode(godot::Input::MOUSE_MODE_CAPTURED);
}

void MLook::_input(const godot::Ref<godot::InputEvent>& event) {
  auto mouse_motion = cast_to<godot::InputEventMouseMotion>(*event);
  if (mouse_motion != nullptr) {
    m_mouse_move = mouse_motion->get_relative() * 0.1f;
    m_mouse_rotation_x -= mouse_motion->get_relative().y / 2000 * m_sensitivity;
    m_mouse_rotation_x = godot::Math::clamp(m_mouse_rotation_x, (real_t) Math_PI / -2, (real_t) Math_PI / 2);
    m_player->rotate_y(-mouse_motion->get_relative().x / 2000 * m_sensitivity);
  }
}

void MLook::_physics_process(double delta) {
  if (SGL(godot::Engine)->is_editor_hint())
    return;

  m_camera->set_rotation(godot::Vector3(m_mouse_rotation_x, 0, 0));
}

real_t MLook::sensitivity() const {
  return m_sensitivity;
}
void MLook::set_sensitivity(real_t sensitivity) {
  m_sensitivity = sensitivity;
}
real_t MLook::head_height() const {
  return get_position().y;
}
void MLook::set_head_height(real_t head_height) {
  auto pos = get_position();
  pos.y = head_height;
  set_position(pos);
}
