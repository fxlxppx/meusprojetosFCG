#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <algorithm>

const GLuint WIDTH = 800, HEIGHT = 600;

struct Rect {
    glm::vec2 pos;   // centro
    glm::vec2 size;

    bool intersects(const Rect& other) {
        return std::abs(pos.x - other.pos.x) < (size.x + other.size.x) / 2.0f &&
               std::abs(pos.y - other.pos.y) < (size.y + other.size.y) / 2.0f;
    }
};

// Globals
bool jump = false;
float velocityY = 0.0f;
bool isGameOver = false;
std::vector<Rect> obstacles;
float obstacleTimer = 0.0f;
const float gravity = -9.8f;
const float jumpForce = 5.0f;

Rect player = { glm::vec2(-0.8f, -0.5f), glm::vec2(0.05f, 0.1f) };

GLuint shaderID, colorLoc, vao;

void key_callback(GLFWwindow* window, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && !isGameOver)
        jump = true;
}

GLuint setupShader() {
    const char* vs = R"(
        #version 400
        layout (location = 0) in vec3 position;
        uniform vec2 offset;
        uniform vec2 scale;
        void main() {
            gl_Position = vec4((position.xy * scale) + offset, position.z, 1.0);
        }
    )";

    const char* fs = R"(
        #version 400
        uniform vec4 inputColor;
        out vec4 color;
        void main() {
            color = inputColor;
        }
    )";

    GLuint vsID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsID, 1, &vs, NULL);
    glCompileShader(vsID);

    GLuint fsID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fsID, 1, &fs, NULL);
    glCompileShader(fsID);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vsID);
    glAttachShader(prog, fsID);
    glLinkProgram(prog);

    glDeleteShader(vsID);
    glDeleteShader(fsID);
    return prog;
}

GLuint setupGeometry() {
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,

         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f
    };

    GLuint vbo, vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    return vao;
}

void drawRect(Rect rect, glm::vec4 color) {
    glUniform4fv(colorLoc, 1, &color[0]);
    glUniform2fv(glGetUniformLocation(shaderID, "offset"), 1, &rect.pos[0]);
    glUniform2fv(glGetUniformLocation(shaderID, "scale"), 1, &rect.size[0]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

int main() {
    srand((unsigned)time(0));
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Endless Runner", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    shaderID = setupShader();
    colorLoc = glGetUniformLocation(shaderID, "inputColor");
    vao = setupGeometry();
    glUseProgram(shaderID);

    float lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        glfwPollEvents();

        // Física do personagem
        if (!isGameOver) {
            if (jump) {
                velocityY = jumpForce;
                jump = false;
            }
            velocityY += gravity * deltaTime;
            player.pos.y += velocityY * deltaTime;

            if (player.pos.y < -0.5f) {
                player.pos.y = -0.5f;
                velocityY = 0.0f;
            }

            // Obstáculos
            obstacleTimer += deltaTime;
            if (obstacleTimer >= 1.5f) {
                obstacleTimer = 0.0f;
                Rect obs;
                obs.size = glm::vec2(0.05f, 0.1f);
                obs.pos = glm::vec2(1.2f, -0.5f);
                obstacles.push_back(obs);
            }

            for (auto& obs : obstacles) {
                obs.pos.x -= 1.0f * deltaTime;
            }

            // Remove obstáculos fora da tela
            obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
                                           [](Rect& o) { return o.pos.x < -1.2f; }),
                            obstacles.end());

            // Checa colisão
            for (const auto& obs : obstacles) {
                if (player.intersects(obs)) {
                    isGameOver = true;
                    break;
                }
            }
        }

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(vao);

        if (isGameOver)
            drawRect(player, glm::vec4(1, 1, 0, 1)); // amarelo se morreu
        else
            drawRect(player, glm::vec4(0, 0, 1, 1)); // azul

        for (const auto& obs : obstacles) {
            drawRect(obs, glm::vec4(1, 0, 0, 1));
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}