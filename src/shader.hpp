#ifndef SHADER_H
#define SHADER_H

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include <fstream>
#include <iostream>

static std::string read_file(std::string path) {
  std::string data = "test";
  std::ifstream file;
  file.open(path, std::ios::binary);
  if (file.is_open()) {
    data.assign((std::istreambuf_iterator<char>(file)),
                (std::istreambuf_iterator<char>()));
  }
  file.close();
  return data;
}

static const unsigned int load_shader(std::string source_path,
                               const unsigned int shader_object) {
  const char *shader_source = read_file(source_path).c_str();
  glShaderSource(shader_object, 1, (const GLchar *const *)&shader_source, NULL);
  glCompileShader(shader_object);

  /* check for compilation errors  */
  int success;
  char info_log[512];
  glGetShaderiv(shader_object, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader_object, 512, NULL, info_log);
    std::cerr << "[ERROR] SHADER COMPILATION FAILED\n" << info_log;
  }
  return shader_object;
}

class shader {
  const unsigned int m_program;

public:
  shader(std::string vertex_path, std::string fragment_path) : m_program(glCreateProgram()) {
    /* create and link vertex and fragment shaders */

    const unsigned int vertex_shader = load_shader(vertex_path, glCreateShader(GL_VERTEX_SHADER));
    const unsigned int fragment_shader = load_shader(fragment_path, glCreateShader(GL_FRAGMENT_SHADER));

    glAttachShader(m_program, vertex_shader);
    glAttachShader(m_program, fragment_shader);
    glLinkProgram(m_program);
    /* handle linking errors */
    int success;
    char info_log[512];
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(m_program, 512, NULL, info_log);
      std::cerr << "[ERROR] LINKING SHADER PROGRAM FAILED\n" << info_log;
    }
    /* delete used shader objects */
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
  }
  ~shader() {
    glDeleteProgram(m_program);
  }

  void bind();
  void unbind();
  unsigned int get_uniform_location(std::string name);
};

#endif // !SHADER_H
