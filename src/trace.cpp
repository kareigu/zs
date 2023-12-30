#include "trace.h"
#include "godot_cpp/core/error_macros.hpp"
#include <godot_cpp/classes/physics_shape_query_parameters3d.hpp>
#include <godot_cpp/classes/world3d.hpp>

void Trace::_bind_methods() {
}

Trace::Trace(godot::PhysicsDirectSpaceState3D* space_state) : m_space_state(space_state) {}

void Trace::standard(godot::Vector3 origin, godot::Vector3 dest, const godot::Ref<godot::Shape3D>& shape, godot::RID exclude) {
  auto query_params = godot::Ref(memnew(godot::PhysicsShapeQueryParameters3D));
  query_params->set_shape(shape);
  auto query_transform = query_params->get_transform();
  query_transform.origin = origin;
  query_params->set_transform(query_transform);
  query_params->set_motion(dest - origin);
  query_params->set_collide_with_bodies(true);
  query_params->set_exclude({exclude});

  hit = false;

  if (m_space_state == nullptr)
    ERR_PRINT("Couldn't get space_state");
  auto results = m_space_state->cast_motion(query_params);

  if (results.is_empty()) {
    WARN_PRINT("Did not get any hit results for cast_motion");
    fraction = 1;
    end_pos = dest;
    return;
  }

  fraction = results[0];
  end_pos = origin + (dest - origin).normalized() * (origin.distance_to(dest) * fraction);
  hit = true;

  if (query_params == nullptr)
    ERR_PRINT("query_params is null");
  query_transform = query_params->get_transform();
  query_transform.origin = end_pos;
  query_params->set_transform(query_transform);


  auto normal_results = m_space_state->get_rest_info(query_params);
  if (normal_results.is_empty()) {
    normal = godot::Vector3(0, 1, 0);
    return;
  }

  normal = normal_results.get("normal", normal);
}

void Trace::count_intersects(godot::Vector3 origin, const godot::Ref<godot::Shape3D>& shape, godot::RID exclude) {
  auto query_params = godot::Ref(memnew(godot::PhysicsShapeQueryParameters3D));
  query_params->set_shape(shape);
  auto query_transform = query_params->get_transform();
  query_transform.origin = origin;
  query_params->set_transform(query_transform);
  query_params->set_collide_with_bodies(true);
  query_params->set_exclude({exclude});

  if (m_space_state == nullptr)
    ERR_PRINT("Couldn't get space_state");
  auto results = m_space_state->intersect_shape(query_params);
  num_intersects = results.size();
}
