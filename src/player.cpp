#include "player.h"
#include "trace.h"

#include <godot_cpp/classes/cylinder_shape3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

void Player::_bind_methods() {
  ClassDB::bind_method(D_METHOD("get_character_model"),
                       &Player::get_character_model);
  ClassDB::bind_method(
          D_METHOD("set_character_model", "character_model"),
          &Player::set_character_model);
  ClassDB::add_property("Player",
                        PropertyInfo(Variant::OBJECT, "character_model",
                                     PROPERTY_HINT_RESOURCE_TYPE,
                                     "PackedScene"),
                        "set_character_model", "get_character_model");
  ClassDB::bind_method(D_METHOD("get_move_vars"),
                       &Player::get_move_vars);
  ClassDB::bind_method(
          D_METHOD("set_move_vars", "ground_vars"),
          &Player::set_move_vars);
  ClassDB::add_property("Player",
                        PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "move_vars"),
                        "set_move_vars", "get_move_vars");
  ClassDB::bind_method(D_METHOD("get_ui"),
                       &Player::get_ui);
  ClassDB::bind_method(
          D_METHOD("set_ui", "ui"),
          &Player::set_ui);
  ClassDB::add_property("Player",
                        PropertyInfo(Variant::OBJECT, "ui",
                                     PROPERTY_HINT_NODE_TYPE,
                                     "Control"),
                        "set_ui", "get_ui");
}

void Player::_ready() {
  load_character_model();

  if (Engine::get_singleton()->is_editor_hint())
    return;
  set_process_input(true);
  update_cursor();
}
void Player::_input(const Ref<InputEvent>& event) {
  Engine* engine = Engine::get_singleton();
  if (engine->is_editor_hint())
    return;

  if (m_menu_open)
    return;

  if (event.is_null())
    return;

  auto mouse_event = cast_to<InputEventMouseMotion>(*event);
  if (mouse_event == nullptr)
    return;

  m_mouse_vel = mouse_event->get_relative() / -1000;
}

void Player::_process(double delta) {
  Engine* engine = Engine::get_singleton();
  if (engine->is_editor_hint())
    return;

  Input* input = Input::get_singleton();

  real_t y_rot = m_mouse_vel.x * m_x_sensitivity;
  real_t x_rot = m_mouse_vel.y * m_y_sensitivity;

  rotate_object_local(Vector3(0, 1, 0), y_rot);
  m_camera->rotate_object_local(Vector3(1, 0, 0), x_rot);
  if (m_clamp_rotation) {
    auto camera_rot = m_camera->get_rotation();
    camera_rot.x = Math::clamp(camera_rot.x, m_min_x_rot, m_max_x_rot);
    m_camera->set_rotation(camera_rot);
  }
  orthonormalize();
  m_camera->orthonormalize();
  m_mouse_vel = Vector2();

  if (input->is_action_just_pressed("ui_cancel"))
    toggle_menu();

  m_move_dir = input->get_vector("left", "right", "forward", "back").normalized();

  if (input->is_action_just_pressed("jump") || input->is_action_just_released("jump_wheel"))
    m_buttons |= InButtons::Jump;
  if (input->is_action_just_released("jump"))
    m_buttons &= ~InButtons::Jump;

  if (input->is_action_pressed("duck"))
    m_buttons |= InButtons::Duck;
  else
    m_buttons &= ~InButtons::Duck;

  if (input->is_action_pressed("sneak"))
    m_buttons |= InButtons::Sneak;
  else
    m_buttons &= ~InButtons::Sneak;

  if (m_cur_collision_shape == 0) {
    m_collision_shapes[0]->set_disabled(false);
    m_collision_shapes[1]->set_disabled(true);
  } else {
    m_collision_shapes[0]->set_disabled(true);
    m_collision_shapes[1]->set_disabled(false);
  }

  if (m_view_angles_debug_label)
    m_view_angles_debug_label->set_text(String("view_angles: ") + Vector2(m_camera->get_rotation().x, get_rotation().y));

  if (m_view_angles_debug_label)
    m_velocity_debug_label->set_text(String("velocity: ") + String::num_real(get_velocity().length()));

  if (m_fps_debug_label)
    m_fps_debug_label->set_text(String("FPS: ") + String::num_real(engine->get_frames_per_second()));
}

void Player::_physics_process(double delta) {
  if (Engine::get_singleton()->is_editor_hint())
    return;

  Input* input = Input::get_singleton();


  if (!(m_buttons & InButtons::Jump) || !is_on_floor())
    m_jump_queued = false;
  if (m_buttons & InButtons::Jump && !m_jump_queued && is_on_floor()) {
    m_jump_queued = true;
    m_buttons &= ~InButtons::Jump;
  } else
    m_buttons &= ~InButtons::Jump;

  if (m_buttons & InButtons::Sneak)
    m_state |= StateFlags::Sneaking;
  if (!(m_buttons & InButtons::Sneak))
    m_state &= ~StateFlags::Sneaking;

  duck(delta);

  if (is_on_floor()) {
    ground_move(delta);
  } else
    air_move(delta);

  set_velocity(m_player_velocity);
  move_and_slide();
  m_player_velocity = get_velocity();

  handle_surf();

  m_duck_time -= delta;
}

void Player::air_move(double delta) {
  auto wish_dir = godot::Vector3(m_move_dir.x, 0, m_move_dir.y);
  wish_dir = get_transform().basis.xform(wish_dir);

  real_t wish_speed = wish_dir.length();
  wish_speed *= m_air_vars.max_speed;

  wish_dir.normalize();
  m_direction_norm = wish_dir;

  real_t wishspeed_ctrl = wish_speed;
  real_t accel = m_player_velocity.dot(wish_dir) < 0
                       ? m_air_vars.deceleration
                       : m_air_vars.acceleration;

  if (m_move_dir.y == 0 && m_move_dir.x != 0) {
    if (wish_speed > m_strafe_vars.max_speed)
      wish_speed = m_strafe_vars.max_speed;
    accel = m_strafe_vars.acceleration;
  }

  air_accelerate(delta, wish_dir, wish_speed, accel);
  if (m_air_control > 0)
    air_control(delta, wish_dir, wishspeed_ctrl);

  m_player_velocity.y -= m_gravity * delta;
}

void Player::air_control(double delta, godot::Vector3 target_dir, real_t target_speed) {
  if (godot::Math::abs(m_move_dir.y) < 0.001 || godot::Math::abs(target_speed) < 0.001)
    return;

  real_t y_speed = m_player_velocity.y;
  m_player_velocity.y = 0;
  real_t speed = m_player_velocity.length();
  m_player_velocity.normalize();

  real_t dot = m_player_velocity.dot(target_dir);
  real_t k = 32;
  k *= m_air_control * dot * dot * delta;

  if (dot > 0) {
    for (int i = 0; i < 3; i++)
      m_player_velocity[i] *= speed + target_dir[i] * k;

    m_player_velocity.normalize();
    m_direction_norm = m_player_velocity;
  }

  m_player_velocity.x *= speed;
  m_player_velocity.y = y_speed;
  m_player_velocity.z *= speed;
}

void Player::ground_move(double delta) {
  if (!m_jump_queued)
    friction(delta, 1.0f);
  else
    friction(delta, 0);


  auto wish_dir = godot::Vector3(m_move_dir.x, 0, m_move_dir.y);
  wish_dir = get_transform().basis.xform(wish_dir);
  wish_dir.normalize();
  m_direction_norm = wish_dir;


  real_t wish_speed = wish_dir.length();
  wish_speed *= m_ground_vars.max_speed;

  accelerate(delta, wish_dir, wish_speed, m_ground_vars.acceleration);

  m_player_velocity.y = 0;
  if (m_state & StateFlags::Sneaking) {
    real_t vel = m_player_velocity.length();
    real_t max_speed = m_ground_vars.max_speed * SNEAKING_SPEED_MULTIPLIER;
    if (vel > max_speed) {
      m_player_velocity.normalize();
      m_player_velocity.x = m_player_velocity.x * max_speed;
      m_player_velocity.z = m_player_velocity.z * max_speed;
    }
  }
  m_player_velocity.y = -m_gravity * delta;

  if (m_jump_queued) {
    m_player_velocity.y = m_jump_force;
    m_jump_queued = false;
    if (m_buttons & InButtons::Jump)
      m_buttons &= ~InButtons::Jump;
  }
}

void Player::friction(double delta, real_t magnitude) {
  auto vec = m_player_velocity;
  vec.y = 0;
  real_t speed = vec.length();
  if (speed < 0.01) {
    m_player_velocity.x = 0;
    m_player_velocity.z = 0;
    return;
  }

  real_t friction = m_friction;


  if (is_on_floor()) {
    auto space_state = get_world_3d()->get_direct_space_state();
    Trace trace(space_state);
    auto current_shape = m_collision_shapes[m_cur_collision_shape];
    auto ray_start = current_shape->get_global_position() + vec * delta;
    auto ray_end = ray_start - Vector3(0, 0.1, 0);
    trace.standard(ray_start, ray_end, current_shape->get_shape(), get_rid());
    trace.count_intersects(ray_end, current_shape->get_shape(), get_rid());
    if (trace.num_intersects == 0 && trace.fraction == 1) {
      WARN_PRINT("Adding friction, going over edge | normal : " + trace.normal);
      friction *= 10;
    }
    /* auto query = PhysicsRayQueryParameters3D().create(ray_start, ray_end, std::numeric_limits<uint32_t>::max(), {this->get_rid()}); */
    /**/
    /* auto result = space_state->intersect_ray(query); */
    /**/
    /* if (result.is_empty()) { */
    /*   friction *= 1.4; */
    /* } */
  }

  real_t drop = 0;

  if (is_on_floor()) {
    real_t control = speed < m_ground_vars.deceleration ? m_ground_vars.deceleration : speed;
    drop = control * friction * delta * magnitude;
  }

  real_t new_speed = speed - drop;
  if (new_speed < 0) new_speed = 0;
  new_speed /= speed;

  for (int i = 0; i < 3; i++)
    m_player_velocity[i] *= new_speed;
}

void Player::accelerate(double delta, godot::Vector3 wish_dir, real_t wish_speed, real_t accel) {
  real_t current_speed = m_player_velocity.dot(wish_dir);
  real_t add_speed = wish_speed - current_speed;

  if (add_speed <= 0) return;

  real_t accel_speed = accel * delta * wish_speed;
  if (accel_speed > add_speed)
    accel_speed = add_speed;

  for (int i = 0; i < 3; i++)
    m_player_velocity[i] += accel_speed * wish_dir[i];
}

void Player::air_accelerate(double delta, godot::Vector3 wish_dir, real_t wish_speed, real_t accel) {
  real_t cap_speed = wish_speed;
  if (cap_speed > 10)
    cap_speed = 10;

  real_t current_speed = m_player_velocity.dot(wish_dir);
  real_t add_speed = cap_speed - current_speed;

  if (add_speed <= 0) return;

  real_t accel_speed = accel * delta * wish_speed;
  if (accel_speed > add_speed)
    accel_speed = add_speed;

  for (int i = 0; i < 3; i++)
    m_player_velocity[i] += accel_speed * wish_dir[i];
}

void Player::handle_surf() {
  if (!is_on_wall_only())
    return;
  auto last_collision = get_last_slide_collision();
  if (last_collision.is_null())
    return;

  if (last_collision->get_normal().y <= 0.1)
    return;

  if (m_direction_norm.length() <= 0)
    return;

  auto view_dir = get_transform().basis[2];
  view_dir.y += m_camera->get_rotation().x;
  auto move = last_collision->get_normal().cross(view_dir).normalized();
  m_player_velocity = m_player_velocity.slide(move);
}

void Player::duck(double delta) {
  if (!(m_buttons & InButtons::Duck) && !m_ducking && !(m_state & StateFlags::Ducked))
    return;

  if (m_player_velocity.y <= 0) {
    auto hor_vel = m_player_velocity;
    hor_vel.y = 0;
    real_t vel = hor_vel.length();
    real_t multiplier = m_state & StateFlags::Sneaking
                              ? DUCK_SPEED_MULTIPLIER * SNEAKING_SPEED_MULTIPLIER
                              : DUCK_SPEED_MULTIPLIER;
    real_t max_speed = m_ground_vars.max_speed * multiplier;
    if (vel > max_speed) {
      hor_vel.normalize();
      m_player_velocity.x = hor_vel.x * max_speed;
      m_player_velocity.z = hor_vel.z * max_speed;
    }
  }

  if (!(m_buttons & InButtons::Duck)) {
    unduck(delta);
    return;
  }

  if (m_buttons & InButtons::Duck && !(m_state & StateFlags::Ducked)) {
    m_duck_time = 1;
    m_ducking = true;
  }

  if (!m_ducking)
    return;

  if ((m_duck_time <= (1.0 - DUCK_TIME)) || !is_on_floor()) {
    m_cur_collision_shape = 1;
    m_state |= StateFlags::Ducked;
    m_ducking = false;

    translate_object_local(Vector3(0, DUCK_JUMP_EXTRA_HEIGHT, 0));
    m_camera->set_position(Vector3(0, PLAYER_DUCK_EYE_HEIGHT, 0));
  } else {
    m_cur_collision_shape = 1;
    auto camera_height = Math::lerp(m_camera->get_position().y, PLAYER_DUCK_EYE_HEIGHT, (real_t) delta * 5);
    m_camera->set_position(Vector3(0, camera_height, 0));
  }
}

void Player::unduck(double delta) {
  auto space_state = get_world_3d()->get_direct_space_state();
  Trace trace(space_state);
  auto standing_shape = m_collision_shapes[0];
  trace.count_intersects(
          standing_shape->get_global_position(),
          standing_shape->get_shape(),
          get_rid());
  if (trace.num_intersects > 1)
    return;

  m_cur_collision_shape = 0;
  m_state &= ~StateFlags::Ducked;
  m_ducking = false;
  m_duck_time = 0;
  translate_object_local(Vector3(0, -DUCK_JUMP_EXTRA_HEIGHT, 0));
  m_camera->set_position(Vector3(0, PLAYER_EYE_HEIGHT, 0));
}

Player::Player() {
  m_collision_shapes[0] = memnew(CollisionShape3D);
  auto stand_shape = memnew(CylinderShape3D);
  m_collision_shapes[0]->set_shape(stand_shape);
  stand_shape->set_radius(0.4);
  stand_shape->set_height(PLAYER_HEIGHT);
  m_collision_shapes[0]->set_position(Vector3(0, PLAYER_HEIGHT / 2, 0));
  add_child(m_collision_shapes[0]);

  m_collision_shapes[1] = memnew(CollisionShape3D);
  auto duck_shape = memnew(CylinderShape3D);
  m_collision_shapes[1]->set_shape(duck_shape);
  duck_shape->set_radius(0.4);
  duck_shape->set_height(PLAYER_DUCK_HEIGHT);
  m_collision_shapes[1]->set_position(Vector3(0, PLAYER_DUCK_HEIGHT / 2, 0));
  add_child(m_collision_shapes[1]);

  m_camera = memnew(Camera3D);
  m_camera->set_current(true);
  m_camera->set_position(Vector3(0, PLAYER_EYE_HEIGHT, 0));
  add_child(m_camera);

  m_top_ray = memnew(RayCast3D);
  m_top_ray->set_position(Vector3(0, PLAYER_EYE_HEIGHT, 0));
  m_top_ray->set_target_position(Vector3(0, PLAYER_HEIGHT - PLAYER_EYE_HEIGHT, 0));
  add_child(m_top_ray);

  m_bottom_ray = memnew(RayCast3D);
  m_bottom_ray->set_target_position(Vector3(0, -PLAYER_HEIGHT, 0));
  m_bottom_ray->set_position(Vector3(0, PLAYER_EYE_HEIGHT, 0));
  add_child(m_bottom_ray);

  m_mouse_vel = Vector2();

  m_gravity = static_cast<double>(ProjectSettings::get_singleton()->get("physics/3d/default_gravity"));
}

void Player::load_character_model() {
  if (m_character_model_node != nullptr) {
    remove_child(m_character_model_node);
    memdelete(m_character_model_node);
  }

  if (m_character_model.is_null()) {
    ERR_PRINT("character_model is null");
    return;
  }

  auto character_model = m_character_model->instantiate();
  if (character_model == nullptr) {
    ERR_PRINT("Couldn't instantiate character_model");
    return;
  }
  character_model->set_name("chracter_model");
  add_child(character_model);
  m_character_model_node = character_model;
}

void Player::toggle_menu() {
  m_menu_open = !m_menu_open;
  update_cursor();
}

void Player::update_cursor() {
  auto input = Input::get_singleton();
  if (m_menu_open) {
    input->set_mouse_mode(Input::MouseMode::MOUSE_MODE_VISIBLE);
  } else {
    input->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
  }
}

Player::~Player() {}

Ref<PackedScene> Player::get_character_model() const {
  return m_character_model;
}
void Player::set_character_model(const Ref<PackedScene> character_model) {
  m_character_model = character_model;
  load_character_model();
}

PackedFloat32Array Player::get_move_vars() const {
  auto array = PackedFloat32Array();
  array.append(m_ground_vars.max_speed);
  array.append(m_ground_vars.acceleration);
  array.append(m_ground_vars.deceleration);
  array.append(m_air_vars.max_speed);
  array.append(m_air_vars.acceleration);
  array.append(m_air_vars.deceleration);
  array.append(m_strafe_vars.max_speed);
  array.append(m_strafe_vars.acceleration);
  array.append(m_strafe_vars.deceleration);
  array.resize(9);
  return array;
}
void Player::set_move_vars(PackedFloat32Array ground_vars) {
  if (ground_vars.size() < 9) {
    WARN_PRINT("Invalid MoveVars provided: less than 3 values");
    return;
  }

  m_ground_vars.max_speed = ground_vars[0];
  m_ground_vars.acceleration = ground_vars[1];
  m_ground_vars.deceleration = ground_vars[2];
  m_air_vars.max_speed = ground_vars[3];
  m_air_vars.acceleration = ground_vars[4];
  m_air_vars.deceleration = ground_vars[5];
  m_strafe_vars.max_speed = ground_vars[6];
  m_strafe_vars.acceleration = ground_vars[7];
  m_strafe_vars.deceleration = ground_vars[8];
}

Control* Player::get_ui() const {
  return m_ui;
}

void Player::set_ui(Control* ui) {
  if (m_ui == ui)
    return;

  m_ui = ui;
  if (m_ui == nullptr)
    return;
  m_view_angles_debug_label = m_ui->get_node<Label>("aspect_ratio_container/v_box_container/h_box_container2/debug_info_container/view_angles_label");
  m_fps_debug_label = m_ui->get_node<Label>("aspect_ratio_container/v_box_container/h_box_container2/debug_info_container/fps_label");
  m_velocity_debug_label = m_ui->get_node<Label>("aspect_ratio_container/v_box_container/h_box_container2/debug_info_container/velocity_label");
}
