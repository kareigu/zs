#include "pmove.h"
#include "common.h"
#include "trace.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/basis.hpp>


void PMove::_bind_methods() {
  godot::ClassDB::bind_method(godot::D_METHOD("head"), &PMove::head);
  godot::ClassDB::bind_method(godot::D_METHOD("set_head", "head"),
                              &PMove::set_head);
  godot::ClassDB::add_property("PMove",
                               godot::PropertyInfo(
                                       godot::Variant::NODE_PATH,
                                       "head",
                                       godot::PROPERTY_HINT_NODE_PATH_VALID_TYPES,
                                       "MLook"),
                               "set_head", "head");
  godot::ClassDB::bind_method(godot::D_METHOD("hull"), &PMove::hull);
  godot::ClassDB::bind_method(godot::D_METHOD("set_hull", "hull"),
                              &PMove::set_hull);
  godot::ClassDB::add_property("PMove",
                               godot::PropertyInfo(
                                       godot::Variant::NODE_PATH,
                                       "hull",
                                       godot::PROPERTY_HINT_NODE_PATH_VALID_TYPES,
                                       "CollisionShape3D"),
                               "set_hull", "hull");
  godot::ClassDB::bind_method(godot::D_METHOD("duck_hull"), &PMove::duck_hull);
  godot::ClassDB::bind_method(godot::D_METHOD("set_duck_hull", "duck_hull"),
                              &PMove::set_duck_hull);
  godot::ClassDB::add_property("PMove",
                               godot::PropertyInfo(
                                       godot::Variant::NODE_PATH,
                                       "duck_hull",
                                       godot::PROPERTY_HINT_NODE_PATH_VALID_TYPES,
                                       "CollisionShape3D"),
                               "set_duck_hull", "duck_hull");
}


void PMove::_ready() {
  auto parent_node = get_parent();
  m_player = cast_to<godot::CharacterBody3D>(parent_node);

  update_head();
  update_hulls();
}

void PMove::_input(const godot::Ref<godot::InputEvent>& event) {
  if (SGL(godot::Engine)->is_editor_hint())
    return;
}

void PMove::_process(double delta) {
  if (SGL(godot::Engine)->is_editor_hint())
    return;

  auto input = SGL(godot::Input);

  auto move_dir = input->get_vector(
          "left",
          "right",
          "forward",
          "back");
  m_move_vars.fmove = move_dir.x;
  m_move_vars.smove = move_dir.y;
}


void PMove::_physics_process(double delta) {
  if (SGL(godot::Engine)->is_editor_hint())
    return;

  if (m_head == nullptr) {
    ERR_PRINT("m_head is null");
    return;
  }

  if (m_player == nullptr) {
    ERR_PRINT("m_player is null");
    return;
  }

  m_move_vars.dt = delta;
  categorise_position();
  switch (m_move_vars.state) {
    case PState::Grounded:
      ground_move();
      break;
    case PState::Falling:
      air_move();
      break;
    default:
      ERR_PRINT("Unimplemented state");
      break;
  }
}

void PMove::categorise_position() {
  auto player_transform = m_player->get_global_transform();
  auto down = player_transform.origin + godot::Vector3(0, -0.1, 0);
  auto space_state = m_player->get_world_3d()->get_direct_space_state();
  Trace trace(space_state);
  trace.standard(player_transform.origin, down, m_hull->get_shape(), m_player->get_rid());

  m_move_vars.ground_plane = false;
  if (trace.fraction == 1) {
    m_move_vars.state = PState::Falling;
    m_move_vars.ground_normal = godot::Vector3(0, 1, 0);
    return;
  }

  m_move_vars.ground_plane = true;
  m_move_vars.ground_normal = trace.normal;

  if (m_move_vars.ground_normal.y < 0.7) {
    m_move_vars.state = PState::Falling;
    return;
  }

  if (m_move_vars.state == PState::Falling)
    ERR_PRINT("Fall damage");

  player_transform.origin = trace.end_pos;
  m_player->set_transform(player_transform);
  m_move_vars.prev_y = player_transform.origin.y;
  m_move_vars.impact_velocity = 0;
  m_move_vars.state = PState::Grounded;
}

void PMove::ground_move() {
  auto player_transform = m_player->get_global_transform();
  auto wish_dir = player_transform.basis.xform(godot::Vector3(m_move_vars.fmove, 0, m_move_vars.smove)).normalized();
  wish_dir = wish_dir.slide(m_move_vars.ground_normal);

  ground_accelerate(wish_dir, slope_speed(m_move_vars.ground_normal.y));

  uint8_t ccd_max = 5;
  for (uint8_t _i = 0; _i < ccd_max; _i++) {
    auto ccd_step = m_move_vars.velocity / ccd_max;
    auto collision = m_player->move_and_collide(ccd_step * m_move_vars.dt);
    if (collision.is_null())
      return;

    auto normal = collision->get_normal();
    if (normal.y < 0.7) {
      auto stepped = false;
      if (!stepped && m_move_vars.velocity.dot(normal) < 0)
        m_move_vars.velocity = m_move_vars.velocity.slide(normal);
    } else
      m_move_vars.velocity = m_move_vars.velocity.slide(normal);
  }
}

void PMove::air_move() {
  auto player_transform = m_player->get_global_transform();
  auto wish_dir = player_transform.basis.xform(godot::Vector3(m_move_vars.fmove, 0, m_move_vars.smove)).normalized();
  wish_dir = wish_dir.slide(m_move_vars.ground_normal);

  air_accelerate(wish_dir, m_move_vars.velocity.dot(wish_dir) < 0 ? 10 : 0.25);

  if (!m_move_vars.ground_plane)
    air_control(wish_dir);

  m_move_vars.velocity.y -= 80 * m_move_vars.dt;

  if (player_transform.origin.y >= m_move_vars.prev_y)
    m_move_vars.prev_y = player_transform.origin.y;

  m_move_vars.impact_velocity = godot::Math::abs(godot::Math::round(m_move_vars.velocity.y));

  uint8_t ccd_max = 5;
  for (uint8_t i = 0; i < ccd_max; i++) {
    auto ccd_step = m_move_vars.velocity / ccd_max;
    auto collision = m_player->move_and_collide(ccd_step * m_move_vars.dt);
    if (collision.is_null())
      continue;
    auto normal = collision->get_normal();
    if (m_move_vars.velocity.dot(normal) < 0)
      m_move_vars.velocity = m_move_vars.velocity.slide(normal);
  }
}

void PMove::ground_accelerate(godot::Vector3 wish_dir, real_t wish_speed) {
  real_t friction = 6;

  if (wish_dir != godot::Vector3())
    m_move_vars.velocity = m_move_vars.velocity.lerp(wish_dir * wish_speed, 10 * m_move_vars.dt);
  else
    m_move_vars.velocity = m_move_vars.velocity.lerp(godot::Vector3(), friction * m_move_vars.dt);
}

void PMove::air_accelerate(godot::Vector3 wish_dir, real_t accel) {
  real_t wish_speed = slope_speed(m_move_vars.ground_normal.y);
  real_t current_speed = m_move_vars.velocity.dot(wish_dir);
  real_t add_speed = wish_speed - current_speed;
  if (add_speed <= 0)
    return;

  real_t accel_speed = accel * m_move_vars.dt * wish_speed;
  if (accel_speed > add_speed)
    accel_speed = add_speed;

  for (int i = 0; i < 3; i++)
    m_move_vars.velocity[i] += accel_speed * wish_dir[i];
}

void PMove::air_control(godot::Vector3 wish_dir) {
  if (m_move_vars.fmove == 0)
    return;

  real_t original_y = m_move_vars.velocity.y;
  m_move_vars.velocity.y = 0;
  real_t speed = m_move_vars.velocity.length();
  m_move_vars.velocity.normalize();

  real_t dot = m_move_vars.velocity.dot(wish_dir);
  if (dot <= 0) {
    m_move_vars.velocity.x *= speed;
    m_move_vars.velocity.y *= original_y;
    m_move_vars.velocity.z *= speed;
    return;
  }
  real_t k = 32 * 0.9 * dot * dot * m_move_vars.dt;
  for (int i = 0; i < 3; i++)
    m_move_vars.velocity[i] *= speed + wish_dir[i] * k;
  m_move_vars.velocity.normalize();
}

real_t PMove::slope_speed(real_t y_normal) {
  if (y_normal <= 0.97) {
    auto multiplier = m_move_vars.velocity.y > 0 ? y_normal : 2 - y_normal;
    return godot::Math::clamp(m_move_vars.move_speed * multiplier, 5.0f, m_move_vars.move_speed * 1.2f);
  }
  return m_move_vars.move_speed;
}

const godot::NodePath& PMove::head() const {
  return m_head_path;
}

void PMove::update_head() {
  if (m_head_path.is_empty()) {
    m_head = nullptr;
    return;
  }
  bool is_editor = SGL(godot::Engine)->is_editor_hint();

  auto node = get_node_or_null(m_head_path);
  if (node == nullptr && is_editor)
    ERR_PRINT("Error getting head at path " + m_head_path);
  m_head = cast_to<MLook>(node);
  if (m_head == nullptr && is_editor)
    ERR_PRINT("Failed casting head to MLook");
}

void PMove::set_head(const godot::NodePath& head) {
  if (m_head_path == head)
    return;

  m_head_path = head;
  update_head();
}

const godot::NodePath& PMove::hull() const {
  return m_hull_path;
}
const godot::NodePath& PMove::duck_hull() const {
  return m_duck_hull_path;
}

void PMove::update_hulls() {
  bool is_editor = SGL(godot::Engine)->is_editor_hint();
  {
    auto node = get_node_or_null(m_hull_path);
    if (node == nullptr && is_editor)
      ERR_PRINT("Error getting hull at path " + m_hull_path);
    m_hull = cast_to<godot::CollisionShape3D>(node);
    if (m_hull == nullptr && is_editor)
      ERR_PRINT("Failed casting hull to CollisionShape3D");
  }
  {
    auto node = get_node_or_null(m_duck_hull_path);
    if (node == nullptr && is_editor)
      ERR_PRINT("Error getting duck_hull at path " + m_duck_hull_path);
    m_duck_hull = cast_to<godot::CollisionShape3D>(node);
    if (m_duck_hull == nullptr && is_editor)
      ERR_PRINT("Failed casting duck_hull to CollisionShape3D");
  }
}
void PMove::set_hull(const godot::NodePath& hull) {
  if (m_hull_path == hull)
    return;
  m_hull_path = hull;
  update_hulls();
}
void PMove::set_duck_hull(const godot::NodePath& duck_hull) {
  if (m_duck_hull_path == duck_hull)
    return;
  m_duck_hull_path = duck_hull;
  update_hulls();
}
