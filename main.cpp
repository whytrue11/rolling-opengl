#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <tiny_obj_loader.h>

namespace
{
  struct Vertex
  {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
  };

  class Shader
  {
  public:
    unsigned int ID;

    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath)
    {
      // 1. retrieve the vertex/fragment source code from filePath
      std::string vertexCode;
      std::string fragmentCode;
      std::ifstream vShaderFile;
      std::ifstream fShaderFile;
      // ensure ifstream objects can throw exceptions:
      vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      try
      {
        // open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
      }
      catch (std::ifstream::failure& e)
      {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
      }
      const char* vShaderCode = vertexCode.c_str();
      const char* fShaderCode = fragmentCode.c_str();
      // 2. compile shaders
      unsigned int vertex, fragment;
      // vertex shader
      vertex = glCreateShader(GL_VERTEX_SHADER);
      glShaderSource(vertex, 1, &vShaderCode, NULL);
      glCompileShader(vertex);
      checkCompileErrors(vertex, "VERTEX");
      // fragment Shader
      fragment = glCreateShader(GL_FRAGMENT_SHADER);
      glShaderSource(fragment, 1, &fShaderCode, NULL);
      glCompileShader(fragment);
      checkCompileErrors(fragment, "FRAGMENT");
      // shader Program
      ID = glCreateProgram();
      glAttachShader(ID, vertex);
      glAttachShader(ID, fragment);
      glLinkProgram(ID);
      checkCompileErrors(ID, "PROGRAM");
      // delete the shaders as they're linked into our program now and no longer necessary
      glDeleteShader(vertex);
      glDeleteShader(fragment);
    }

    // activate the shader
    // ------------------------------------------------------------------------
    void use()
    {
      glUseProgram(ID);
    }

    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string& name, bool value) const
    {
      glUniform1i(glGetUniformLocation(ID, name.c_str()), (int) value);
    }

    // ------------------------------------------------------------------------
    void setInt(const std::string& name, int value) const
    {
      glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    // ------------------------------------------------------------------------
    void setFloat(const std::string& name, float value) const
    {
      glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setMat4(const std::string& name, glm::mat4 value) const
    {
      glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &value[0][0]);
    }

  private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, std::string type)
    {
      int success;
      char infoLog[1024];
      if (type != "PROGRAM")
      {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
          glGetShaderInfoLog(shader, 1024, NULL, infoLog);
          std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog
            << "\n -- --------------------------------------------------- -- " << std::endl;
        }
      }
      else
      {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
          glGetProgramInfoLog(shader, 1024, NULL, infoLog);
          std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog
            << "\n -- --------------------------------------------------- -- " << std::endl;
        }
      }
    }
  };

  // Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
  enum Camera_Movement
  {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
  };

// Default camera values
  const float YAW = -90.0f;
  const float PITCH = 0.0f;
  const float SPEED = 4.5f;
  const float SENSITIVITY = 0.1f;
  const float ZOOM = 45.0f;

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
  class Camera
  {
  public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
      float yaw = YAW, float pitch = PITCH) noexcept: Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED),
      MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
      Position = position;
      WorldUp = up;
      Yaw = yaw;
      Pitch = pitch;
      updateCameraVectors();
    }

    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(
      glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
      Position = glm::vec3(posX, posY, posZ);
      WorldUp = glm::vec3(upX, upY, upZ);
      Yaw = yaw;
      Pitch = pitch;
      updateCameraVectors();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
      return glm::lookAt(Position, Position + Front, Up);
    }

    glm::mat4 GetProjView()
    {
      return glm::perspective(glm::radians(Zoom), 1280.f / 720, 0.1f, 1000.f);
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
      float velocity = MovementSpeed * deltaTime;
      if (direction == FORWARD)
      {
        Position += Front * velocity;
      }
      if (direction == BACKWARD)
      {
        Position -= Front * velocity;
      }
      if (direction == LEFT)
      {
        Position -= Right * velocity;
      }
      if (direction == RIGHT)
      {
        Position += Right * velocity;
      }
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
      xoffset *= MouseSensitivity;
      yoffset *= MouseSensitivity;

      Yaw += xoffset;
      Pitch += yoffset;

      // make sure that when pitch is out of bounds, screen doesn't get flipped
      if (constrainPitch)
      {
        if (Pitch > 89.0f)
        {
          Pitch = 89.0f;
        }
        if (Pitch < -89.0f)
        {
          Pitch = -89.0f;
        }
      }

      // update Front, Right and Up Vectors using the updated Euler angles
      updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
      Zoom -= (float) yoffset;
      if (Zoom < 1.0f)
      {
        Zoom = 1.0f;
      }
      if (Zoom > 45.0f)
      {
        Zoom = 45.0f;
      }
    }

  private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
      // calculate the new Front vector
      glm::vec3 front;
      front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
      front.y = sin(glm::radians(Pitch));
      front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
      Front = glm::normalize(front);
      // also re-calculate the Right and Up vector
      Right = glm::normalize(glm::cross(Front,
        WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
      Up = glm::normalize(glm::cross(Right, Front));
    }
  };

  Camera camera;

  void glfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos)
  {
    thread_local double x = 0;
    thread_local double y = 0;

    camera.ProcessMouseMovement(xpos - x, y - ypos);

    x = xpos;
    y = ypos;
  }

  struct
  {
    float deltaTime = 0.016f;
    glm::vec3 origin{0.0f, 0.0f, 0.0f};
    float orbit = 0.f;
    float orbitSpeed = 20.f;
    float radius = 5.f;
    float rotation = 0.0f;
    float rotationSpeed = 80.f;
    float plane = -2.5f;
  } simulation_data{};

  std::vector<Vertex> simulation(std::vector<Vertex> vertices)
  {
    glm::vec3 axis = glm::vec4{1.0f, 0.0f, 0.0f, 0.0f} *
      glm::rotate(glm::radians(simulation_data.orbit), glm::vec3{0.0f, 1.0f, 0.0f});

    float lowestVertexY = INFINITY;
    for (auto& v : vertices)
    {
      v.Position = glm::vec4{v.Position, 0.0f} * glm::rotate(glm::radians(simulation_data.rotation), axis);
      v.Normal = glm::vec4{v.Normal, 0.0f} * glm::rotate(glm::radians(simulation_data.rotation), axis);

      if (v.Position.y < lowestVertexY)
      {
        lowestVertexY = v.Position.y;
      }
    }

    float offsetY = simulation_data.plane - lowestVertexY;

    for (auto& v : vertices)
    {
      v.Position.y += offsetY;
    }

    glm::vec3 position = simulation_data.origin + axis * simulation_data.radius;

    simulation_data.orbit += simulation_data.orbitSpeed * simulation_data.deltaTime;
    simulation_data.rotation += -simulation_data.rotationSpeed * simulation_data.deltaTime;

    for (auto& v : vertices)
    {
      v.Position += position;
    }

    return vertices;
  }

  void loadModel(const char* filename, std::vector<Vertex>& vertices, std::vector<unsigned>& indices)
  {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename);

    auto& meshIndices = shapes.front().mesh.indices;

    for (std::size_t i = 0; i < meshIndices.size(); ++i)
    {
      Vertex& vertex = vertices.emplace_back();

      vertex.Position = glm::vec3{
        attrib.vertices[meshIndices[i].vertex_index * 3 + 0],
        attrib.vertices[meshIndices[i].vertex_index * 3 + 1],
        attrib.vertices[meshIndices[i].vertex_index * 3 + 2]
      };

      vertex.Position = glm::normalize(vertex.Position);

      vertex.Normal = glm::vec3{
        attrib.normals[meshIndices[i].normal_index * 3 + 0],
        attrib.normals[meshIndices[i].normal_index * 3 + 1],
        attrib.normals[meshIndices[i].normal_index * 3 + 2]
      };

      vertex.TexCoords = glm::vec2{
        attrib.texcoords[meshIndices[i].texcoord_index * 2 + 0],
        attrib.texcoords[meshIndices[i].texcoord_index * 2 + 1],
      };

      indices.push_back(i);
    }
  }
}

int main()
{
  glfwInit();

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

  GLFWwindow* window = glfwCreateWindow(1280, 720, "cw-c", nullptr, nullptr);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPosCallback(window, glfwCursorPosCallback);

  glfwMakeContextCurrent(window);

  glewInit();

  std::vector<Vertex> vertices;
  std::vector<unsigned> indices;
  loadModel("assets/icosahedron.obj", vertices, indices);

  std::vector<Vertex> sphereVertices;
  std::vector<unsigned> sphereIndices;
  loadModel("assets/sphere.obj", sphereVertices, sphereIndices);

  for (auto& v : sphereVertices)
  {
    v.Position.y -= 1.5f;
  }

  std::vector<Vertex> planeVertices{
    {glm::vec3{-10, -2.5, -10}, glm::vec3{}, glm::vec2{0, 0}},
    {glm::vec3{-10, -2.5, 10},  glm::vec3{}, glm::vec2{0, 5}},
    {glm::vec3{10, -2.5, 10},   glm::vec3{}, glm::vec2{5, 5}},
    {glm::vec3{10, -2.5, -10},  glm::vec3{}, glm::vec2{5, 0}}
  };
  std::vector<unsigned> planeIndices{0, 1, 3, 3, 2, 1};

  int width, height, channels;

  void* data = stbi_load("assets/texture.jpg", &width, &height, &channels, 0);

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(data);

  data = stbi_load("assets/grass.jpg", &width, &height, &channels, 0);

  GLuint planeTexture;
  glGenTextures(1, &planeTexture);
  glBindTexture(GL_TEXTURE_2D, planeTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(data);

  GLuint vao;
  GLuint vbo;
  GLuint ebo;

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, Position));

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, Normal));

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, TexCoords));

  glBindVertexArray(0);

  glEnable(GL_DEPTH_TEST);

  Shader shader("assets/vertex.glsl", "assets/fragment.glsl");
  Shader planeShader("assets/vertex.glsl", "assets/planeFragment.glsl");

  while (!glfwWindowShouldClose(window))
  {
    glfwSwapBuffers(window);

    if (glfwGetKey(window, GLFW_KEY_W))
    {
      camera.ProcessKeyboard(FORWARD, simulation_data.deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A))
    {
      camera.ProcessKeyboard(LEFT, simulation_data.deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S))
    {
      camera.ProcessKeyboard(BACKWARD, simulation_data.deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D))
    {
      camera.ProcessKeyboard(RIGHT, simulation_data.deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_ESCAPE))
    {
      break;
    }

    glClearColor(0.565, 0.89, 1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    std::vector<Vertex> verticesTransformed = simulation(vertices);

    glBindVertexArray(vao);

    glBindTexture(GL_TEXTURE_2D, texture);

    shader.use();
    shader.setMat4("view", camera.GetViewMatrix());
    shader.setMat4("proj", camera.GetProjView());

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verticesTransformed.size() * sizeof(Vertex)),
      verticesTransformed.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned)), indices.data(),
      GL_STATIC_DRAW);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sphereVertices.size() * sizeof(Vertex)),
      sphereVertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sphereIndices.size() * sizeof(unsigned)), sphereIndices.data(),
      GL_STATIC_DRAW);
    glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

    planeShader.use();
    planeShader.setMat4("view", camera.GetViewMatrix());
    planeShader.setMat4("proj", camera.GetProjView());

    glBindTexture(GL_TEXTURE_2D, planeTexture);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(planeVertices.size() * sizeof(Vertex)),
      planeVertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(planeIndices.size() * sizeof(unsigned)),
      planeIndices.data(),
      GL_STATIC_DRAW);
    glDrawElements(GL_TRIANGLES, planeIndices.size(), GL_UNSIGNED_INT, 0);

    glfwPollEvents();
  }

  glfwTerminate();

  return 0;
}
