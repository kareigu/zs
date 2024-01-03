#pragma once

#include <cstdint>

#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/capsule_shape3d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/ray_cast3d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

using InButtons_t = uint8_t;
namespace InButtons {
  constexpr InButtons_t Jump = 1;
  constexpr InButtons_t Duck = 2;
  constexpr InButtons_t Sneak = 4;
  constexpr InButtons_t Attack1 = 8;
  constexpr InButtons_t Attack2 = 16;
}// namespace InButtons

using StateFlags_t = uint8_t;
namespace StateFlags {
  constexpr StateFlags_t Ducked = 1;
  constexpr StateFlags_t Sneaking = 2;
  constexpr StateFlags_t OnLadder = 4;
}// namespace StateFlags

class Player : public CharacterBody3D {
  GDCLASS(Player, CharacterBody3D)
public:
  static constexpr real_t PLAYER_HEIGHT = 2.0f;
  static constexpr real_t PLAYER_EYE_HEIGHT = PLAYER_HEIGHT - 0.1f;
  static constexpr real_t PLAYER_DUCK_HEIGHT = PLAYER_HEIGHT - 0.75f;
  static constexpr real_t PLAYER_DUCK_EYE_HEIGHT = PLAYER_DUCK_HEIGHT - 0.1f;

  static constexpr real_t DUCK_JUMP_EXTRA_HEIGHT = (PLAYER_HEIGHT - PLAYER_EYE_HEIGHT) * 2.5;
  static constexpr real_t SNEAKING_SPEED_MULTIPLIER = 0.68;
  static constexpr real_t DUCK_SPEED_MULTIPLIER = 0.6;
  static constexpr real_t DUCK_TIME = 0.4;

  struct MoveVars {
    real_t max_speed;
    real_t acceleration;
    real_t deceleration;
  };


  Ref<PackedScene> get_character_model() const;
  void set_character_model(const Ref<PackedScene> character_model_path);

  PackedFloat32Array get_move_vars() const;
  void set_move_vars(PackedFloat32Array move_vars);

  Control* get_ui() const;
  void set_ui(Control* ui);

  void _ready() override;
  void _process(double delta) override;
  void _physics_process(double delta) override;

  void _input(const Ref<InputEvent>& event) override;

  Player();
  ~Player();

protected:
  static void _bind_methods();

private:
  void load_character_model();
  void toggle_menu();
  void update_cursor();

  InButtons_t m_buttons = 0;
  InButtons_t m_old_buttons = 0;
  StateFlags_t m_state = 0;
  bool m_menu_open = false;

  void update_timers(double delta);
  void duck(double delta);
  void unduck(double delta);
  bool m_ducking = false;
  real_t m_duck_time = 0;

  bool m_jump_queued = false;

  void air_control(double delta, godot::Vector3 target_dir, real_t target_speed);
  void air_move(double delta);
  void ground_move(double delta);
  void friction(double delta, real_t magnitude);
  void accelerate(double delta, godot::Vector3 wish_dir, real_t wish_speed, real_t accel);
  void air_accelerate(double delta, godot::Vector3 wish_dir, real_t wish_speed, real_t accel);
  void handle_surf();
  void handle_ladder();
  void apply_gravity(double delta);

  Vector3 m_ladder_normal = Vector3();

  real_t m_friction = 6;
  real_t m_gravity;
  real_t m_jump_force = 7;
  real_t m_air_control = 0.3f;
  MoveVars m_ground_vars = MoveVars{5.5, 14, 10};
  MoveVars m_air_vars = MoveVars{3.5, 2, 2};
  MoveVars m_strafe_vars = MoveVars{0.5, 50, 50};

  Vector2 m_move_dir;
  Vector3 m_direction_norm = Vector3();
  Vector3 m_player_velocity = Vector3();

  Vector2 m_mouse_vel;
  real_t m_x_sensitivity = 0.5f;
  real_t m_y_sensitivity = 0.5f;
  real_t m_min_x_rot = Math_PI / -2.0f;
  real_t m_max_x_rot = Math_PI / 2.0f;
  bool m_clamp_rotation = true;


  CollisionShape3D* m_collision_shapes[2]{nullptr, nullptr};
  uint8_t m_cur_collision_shape = 0;

  Camera3D* m_camera = nullptr;
  RayCast3D* m_top_ray = nullptr;
  RayCast3D* m_bottom_ray = nullptr;

  Node* m_character_model_node = nullptr;
  Ref<PackedScene> m_character_model = nullptr;

  Control* m_ui = nullptr;
  Label* m_fps_debug_label = nullptr;
  Label* m_view_angles_debug_label = nullptr;
  Label* m_velocity_debug_label = nullptr;
};
}// namespace godot
