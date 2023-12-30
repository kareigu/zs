#pragma once


#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node3d.hpp>

class MLook : public godot::Node3D {
  GDCLASS(MLook, godot::Node3D)
public:
  real_t sensitivity() const;
  void set_sensitivity(real_t sensitivity);
  real_t head_height() const;
  void set_head_height(real_t head_height);

  void _ready() override;
  void _input(const godot::Ref<godot::InputEvent>& event) override;
  void _physics_process(double delta) override;

private:
  real_t m_sensitivity = 0.5;
  real_t m_head_height = 0;

  godot::Vector2 m_mouse_move = godot::Vector2();
  real_t m_mouse_rotation_x = 0;

  godot::Camera3D* m_camera = nullptr;
  godot::CharacterBody3D* m_player = nullptr;

protected:
  static void _bind_methods();
};
