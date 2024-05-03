#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <chrono>
#include <iostream>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"
#include "sphere.hpp"


class timestamp {
  std::chrono::time_point<std::chrono::system_clock> start;
public:
  timestamp() {}
  void begin() { start = std::chrono::system_clock::now(); }
  double get_elapsed_time() {
    return ((std::chrono::duration<double>)(std::chrono::system_clock::now() - start)).count();
  }
};

class camera {
  enum MODE { STILL, FOCUS, TRACK };
  glm::mat4 m_offset;
  glm::vec3 m_position;
  glm::vec3 m_start;
  glm::vec3 m_focus_point;
  glm::vec3 *m_p_target;
  timestamp m_timestamp;
  const double m_total_time = 0.5;
  camera::MODE m_mode;
  double smooth(double x);

public:
  float zoom = 1.0f;
  float rotation = M_PI / 4;
  camera() {
    const float of = 1.0f;
    m_offset = glm::mat4(1.0f);
    m_offset = glm::translate(m_offset, glm::vec3(of, 0.0f, 0.0f));
    glm::vec3 point(0.0f);
    focus(point);
  }
  void focus(const glm::vec3 &point);
  void track(glm::vec3 *const target);

  void update();

  glm::mat4 get_view_matrix() const {
    glm::mat4 view(1.0f);
    glm::vec3 position = m_position + glm::vec3(0, 5, 10) * zoom;
    view = glm::lookAt(position, m_position, glm::vec3(0.0f, 1.0f, 0.0f));
    return view * m_offset;
  }
};

// world rendering
class environment {
  glm::mat4 proj;
  shader main_shader;
  mesh sphere_mesh;
  GLFWwindow *const window;
  std::vector<object *> objects;

public:
  static camera current_camera;
  environment(GLFWwindow *window)
      : window(window),
        main_shader("vertex_shader.glsl", "fragment_shader.glsl"),
        sphere_mesh(&main_shader) {
    sphere::gen_vertex_data(120, sphere_mesh);
  }
  ~environment() {
    auto clean = [](object *p_object) { delete p_object; };
    std::for_each(objects.begin(), objects.end(), clean);
  };
  void update(float delta);
  void draw();
  sphere *create(std::string &name, unsigned int radius);
  int object_count() const { return objects.size(); }
  object *object_at(unsigned int idx) {
    return (idx < objects.size()) ? objects[idx] : NULL;
  };
};

#endif // !ENVIRONMENT_H
