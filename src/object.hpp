#ifndef OBJECT_H 
#define OBJECT_H

#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <random>
#include "gui.hpp"
#include "shader.hpp"
#include "simulation.hpp"
#include "utils.h"
#define _USE_MATH_DEFINES
#include <cmath>


// linear interpolate between two vectors
inline glm::vec3 lerp3f(glm::vec3 x, glm::vec3 y, float t) {
  return x * (1.f - t) + y * t;
}

inline float lerp1f(float x, float y, float t) {
  return x * (1.f - t) + y * t;
}

// modify lerp value for a more organic motion
inline double smooth(double x) { 
  return tanh(sqrt(x) * 6 - M_PI) / 2 + 0.503; 
}

// store vertices to draw with opengl 
class mesh {
  shader *m_shader;
  unsigned int m_VAO;
  unsigned int m_VBO;
  unsigned int m_EBO;
protected:
  unsigned int m_elements;

public:
  mesh(shader *s) : m_shader(s) {
    // opengl initialise buffers
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    glGenVertexArrays(1, &m_VAO);
  }
  ~mesh() {
    // opengl delete vertex data 
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
    glDeleteVertexArrays(1, &m_VAO);
  }
  void set_elements(unsigned int elements);
  void set_shader(shader *s);
  shader *get_shader() const { return m_shader; }
  void bind();
  void unbind();
  void write_begin();
  void write_end();
  virtual void draw();
};

class line_mesh : public mesh {
public:
  line_mesh(shader* s) : mesh(s) {}
  void draw() override;
};

// inherit GUI functionality
class object : public GUIitem {
  // move_to data
  glm::vec3 m_start;
  glm::vec3 m_end;
  timestamp m_timestamp;
  const double m_total_time = 0.2;
  enum MODE { STILL, ACTIVE };
  object::MODE m_mode;
  // colour mod

protected:
  mesh *m_mesh;
  float m_scale;
  glm::fvec3 m_colour;

  // pure function to pass custom object transform matrix to the draw call
  virtual glm::mat4 model_matrix() const = 0;
public:
  glm::vec3 position;

  // initialise defaults, random colour
  object(std::string &name, mesh *mesh, float scale, glm::vec3 col = glm::vec3((float)std::rand()/RAND_MAX, (float)std::rand()/RAND_MAX, (float)std::rand()/RAND_MAX))
      : GUIitem(name), m_mesh(mesh), m_scale(scale), m_mode(MODE::STILL), position(0.0f), m_colour(col) {}

  object(const char * name, mesh *mesh, float scale, glm::vec3 col = glm::vec3((float)std::rand()/RAND_MAX, (float)std::rand()/RAND_MAX, (float)std::rand()/RAND_MAX))
      : GUIitem(name), m_mesh(mesh), m_scale(scale), m_mode(MODE::STILL), position(0.0f), m_colour(col) {}

  // swap to another shader
  void set_shader(shader *shader) const { m_mesh->set_shader(shader); };
  // draw object normally
  virtual void draw(glm::mat4 &vp_matrix) const; 
  // draw object as a single colour with custom scalar
  virtual void draw(glm::mat4 &vp_matrix, float scale) const;
  // frame logic step
  virtual void update(float delta);
  void move_to(glm::vec3 location) {
    // store time
    m_timestamp.begin();
    // store start and end points for linear interpolation
    m_start = position;
    m_end = location;
    // switch to active state
    m_mode = MODE::ACTIVE;
  };
  // pure function, passes information to the environment used to build a simulation from tree data
  virtual int get_type_code() const = 0;
};

// root node for tree
class root : public object {
protected:
  glm::mat4 model_matrix() const override { return glm::mat4(1.0f); };
public:
  root() : object("root", NULL, 0.0f) {}
  void draw(glm::mat4 &vp_matrix) const override {};
  void draw(glm::mat4 &vp_matrix, float scale) const override {};
  int get_type_code() const override { return -1; };
};

// world class, inherits object, stands at the top of the node tree
class world : public object {
  struct {
    particle* pa1;
    particle* pa2;
    plane* pl;
    spring* sp;
  } simulation_objects;
  glm::mat4 model_matrix() const override;
  simulation* current_simulation;

public:
  static line_mesh* world_mesh;
  // simulation data
  float time_scale;
  float distance;
  float friction;
  float gravity;
  float restitution;
  world(std::string& name, float scale)
      : object(name, world_mesh, scale), 
        time_scale(1.0f),
        distance(1.0f),
        friction(0.0f),
        gravity(9.8f),
        restitution(0.5f)
  { simulation_objects = {NULL, NULL, NULL};
    current_simulation = NULL; }
  ~world() { delete current_simulation; }

  void start_simulation() { DEBUG_TEXT("world initiating simulation");
    current_simulation->start(); };
  void end_simulation() {
    current_simulation->end();
  }
  void child_added(object* child);
  void child_removed(object* child);
  bool create_simulation();

  // override draw functions to disable drawing
  bool can_simulate() const { return current_simulation != NULL; } 

  // called by children to reset the simulation state when editing variables
  static void reset_simulation(GUIitem* world) { 
    if (static_cast<class world*>(world)->current_simulation) { 
      static_cast<class world*>(world)->current_simulation->reset(); 
    }
  }

  static void gen_vertex_data(line_mesh &mesh);
  void show() const override;
  int get_type_code() const override { return 0; };
  void update(float delta) override;
};

class plane : public object {
  glm::mat4 model_matrix() const override;
public:
  // custom orientation and length
  float rotation;
  float length;
  static mesh* plane_mesh;
  plane(std::string& name, float scale)
      : object(name, plane_mesh, scale, glm::vec3(0.133, 0.11, 0.208)), rotation(3*M_PI/8), length(3.0f) {}
  static void gen_vertex_data(mesh &mesh);
  void show() const override;
  int get_type_code() const override { return 1; };
};

class particle : public object {
  glm::mat4 model_matrix() const override;
public:
  // editable values
  float force;
  float mass;
  float u_velocity;
  static mesh* particle_mesh;
  particle(std::string &name, float scale)
      : object(name, particle_mesh, scale, glm::vec3(0.0f, 0.0f, 1.0f)), mass(1.0f), force(0.0f), u_velocity(0.0f) {}
  static void gen_vertex_data(unsigned int nodes, mesh &mesh);
  // get radius size
  float get_radius() const { return m_scale; };

  void show() const override;
  int get_type_code() const override { return 3; };
};

class point : public object {
  glm::mat4 model_matrix() const override;
public:
  point(std::string& name, float scale)
      : object(name, particle::particle_mesh, scale) {}
  void show() const override;
  int get_type_code() const override { return 2; };
};

class spring : public object {
  glm::mat4 model_matrix() const override;
public:
  // editable values
  float length;
  float extension;
  float elasticity;
  float rotation;
  // globals
  static float coil_width;
  static int coils;
  static mesh* spring_mesh;
  static mesh* spring_mesh_highlight;
  spring(std::string& name, float scale)
      : object(name, spring_mesh, scale, glm::vec3(0.667f, 0.663f, 0.678f)),
     rotation(0.0f),
    extension(0.5f), 
    length(1.0f),
    elasticity(9.8f)
  {}
  static void gen_vertex_data(const int coils, const int nodes, const float coil_width, const float thickness, mesh &mesh);
  void show() const override;
  void draw(glm::mat4 &vp_matrix, float scale) const override;
  int get_type_code() const override { return 4; };
  float get_scale() const { return m_scale; }
};


#endif // !OBJECT_H
