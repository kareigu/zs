#pragma once
#include "mlook.h"

#include <cstdint>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/node_path.hpp>


using move_speed_t = real_t;
namespace MoveSpeed {
constexpr move_speed_t MAX = 32;
}

using current_hull_t = uint8_t;
namespace CurrentHull {
constexpr current_hull_t Standard = 0;
constexpr current_hull_t Duck = 1;
}// namespace CurrentHull

using p_state_t = uint8_t;
namespace PState {
constexpr p_state_t Grounded = 0;
constexpr p_state_t Falling = 1;
}// namespace PState

struct MoveVars {
  double dt = 0;
  real_t fmove = 0;
  real_t smove = 0;
  godot::Vector3 velocity = godot::Vector3();
  move_speed_t move_speed = MoveSpeed::MAX;
  current_hull_t current_hull = CurrentHull::Standard;
  p_state_t state = PState::Grounded;
  godot::Vector3 ground_normal = godot::Vector3();
  real_t prev_y = 0;
  real_t impact_velocity = 0;
  bool ground_plane = true;
};

class PMove : public godot::Node {
  GDCLASS(PMove, godot::Node);

public:
  const godot::NodePath& head() const;
  void set_head(const godot::NodePath& head);
  const godot::NodePath& hull() const;
  void set_hull(const godot::NodePath& hull);
  const godot::NodePath& duck_hull() const;
  void set_duck_hull(const godot::NodePath& duck_hull);

  void _ready() override;
  void _input(const godot::Ref<godot::InputEvent>& event) override;
  void _process(double delta) override;
  void _physics_process(double delta) override;

private:
  MoveVars m_move_vars;

  void categorise_position();
  void ground_move();
  void air_move();
  void ground_accelerate(godot::Vector3 wish_dir, real_t wish_speed);
  void air_accelerate(godot::Vector3 wish_dir, real_t accel);
  void air_control(godot::Vector3 wish_dir);


  real_t slope_speed(real_t y_normal);

  godot::CharacterBody3D* m_player = nullptr;

  godot::NodePath m_head_path;
  MLook* m_head = nullptr;
  void update_head();

  godot::NodePath m_hull_path;
  godot::CollisionShape3D* m_hull = nullptr;
  godot::NodePath m_duck_hull_path;
  godot::CollisionShape3D* m_duck_hull = nullptr;
  void update_hulls();

protected:
  static void _bind_methods();
};
