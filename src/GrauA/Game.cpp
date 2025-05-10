#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

using namespace std;
using namespace glm;

const GLuint WIDTH = 800, HEIGHT = 600;

struct Sprite {
    GLuint VAO;
    GLuint texID;
    vec2 pos;
    vec2 size;
    float angle = 0.0f;
    int nFrames = 1;
    int nAnimations = 1;
    int iFrame = 0;
    int iAnimation = 0;
    float ds = 1.0f, dt = 1.0f;
};

struct Rect {
    vec2 pos;
    vec2 size;

    bool intersects(const Rect& other) {
        return abs(pos.x - other.pos.x) < (size.x + other.size.x) / 2.0f &&
               abs(pos.y - other.pos.y) < (size.y + other.size.y) / 2.0f;
    }
};

const char* vertexShaderSource = R"(
#version 400
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoord;

uniform mat4 projection;
uniform mat4 model;
out vec2 tex_coord;

void main() {
    tex_coord = texCoord;
    gl_Position = projection * model * vec4(position, 0.0, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 400
in vec2 tex_coord;
out vec4 color;
uniform sampler2D tex_buff;
uniform vec2 offset_tex;

void main() {
    color = texture(tex_buff, tex_coord + offset_tex);
}
)";

GLuint compileShaders();
GLuint setupSpriteVAO(float& ds, float& dt, int nFrames, int nAnimations);
int loadTexture(string filePath);
void drawSprite(GLuint shaderID, const Sprite& spr);

void key_callback(GLFWwindow* window, int key, int, int action, int);
bool keys[1024];

GLuint compileShaders() {
    GLint success;
    GLchar infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Erro ao compilar Vertex Shader:\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Erro ao compilar Fragment Shader:\n" << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Erro ao linkar Shader Program:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void drawSprite(GLuint shaderID, const Sprite& spr) {
    glBindVertexArray(spr.VAO);
    glBindTexture(GL_TEXTURE_2D, spr.texID);

    mat4 model = mat4(1.0f);
    model = translate(model, vec3(spr.pos, 0.0f));
    model = rotate(model, radians(spr.angle), vec3(0.0f, 0.0f, 1.0f));
    model = scale(model, vec3(spr.size, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}

GLuint setupSpriteVAO(float& ds, float& dt, int nFrames, int nAnimations) {
    ds = 1.0f / nFrames;
    dt = 1.0f / nAnimations;

    float vertices[] = {     
        -0.5f,  0.5f, 0.0f, dt,
        -0.5f, -0.5f, 0.0f, 0.0f,
         0.5f,  0.5f, ds,   dt,
        -0.5f, -0.5f, 0.0f, 0.0f,
         0.5f, -0.5f, ds,   0.0f,
         0.5f,  0.5f, ds,   dt
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return VAO;
}

int loadTexture(string filePath)
{
	GLuint texID;

	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int width, height, nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}


Sprite background, player;
vector<Sprite> enemies;
float gravity = -9.8f;
float velocityY = 0.0f;
bool isOnGround = true;
bool jump = false;
bool isGameOver = false;
float lastFrameTime = 0.0f;
float frameInterval = 1.0f / 12.0f;
float obstacleTimer = 0.0f;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && !isGameOver)
        jump = true;
}

int main() {
    srand((unsigned)time(0));
    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 8);
    stbi_set_flip_vertically_on_load(true);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Endless Runner", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    GLuint shaderID = compileShaders();
    glUseProgram(shaderID);

    mat4 projection = ortho(-1.0f, 1.0f, -0.75f, 0.75f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));
    glUniform1i(glGetUniformLocation(shaderID, "tex_buff"), 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    background.texID = loadTexture("../assets/sprites/Background.png");
    background.size = vec2(2.0f, 1.5f);
    background.VAO = setupSpriteVAO(background.ds, background.dt, 1, 1);

    player.texID = loadTexture("../assets/sprites/sprite_dino.png");
    player.size = vec2(0.1f, 0.2f);
    player.pos = vec2(-0.8f, -0.5f);
    player.nFrames = 8;
    player.nAnimations = 1;
    player.VAO = setupSpriteVAO(player.ds, player.dt, player.nFrames, player.nAnimations);

    Sprite baseEnemy;
    baseEnemy.texID = loadTexture("../assets/sprites/slimer-idle.png");
    baseEnemy.size = vec2(0.1f, 0.2f);
    baseEnemy.nFrames = 8;
    baseEnemy.nAnimations = 1;
    baseEnemy.VAO = setupSpriteVAO(baseEnemy.ds, baseEnemy.dt, baseEnemy.nFrames, baseEnemy.nAnimations);

    float lastTime = glfwGetTime();
    float nextObstacleTime = 1.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        glfwPollEvents();

        if (!isGameOver) {
            if (jump && isOnGround) {
                velocityY = 3.0f;
                isOnGround = false;
                jump = false;
            }

            velocityY += gravity * deltaTime;
            player.pos.y += velocityY * deltaTime;

            if (player.pos.y < -0.5f) {
                player.pos.y = -0.5f;
                velocityY = 0.0f;
                isOnGround = true;
            }

            obstacleTimer += deltaTime;
            if (obstacleTimer >= nextObstacleTime) {
                obstacleTimer = 0.0f;
                nextObstacleTime = 1.0f + static_cast<float>(rand()) / RAND_MAX * 1.5f;
                Sprite newEnemy = baseEnemy;
                newEnemy.pos = vec2(1.2f, -0.5f);
                enemies.push_back(newEnemy);
            }

            for (auto& e : enemies) {
                e.pos.x -= 1.0f * deltaTime;
            }

            enemies.erase(remove_if(enemies.begin(), enemies.end(), [](Sprite& e) {
                return e.pos.x < -1.2f;
            }), enemies.end());

            for (const auto& e : enemies) {
                Rect r1 = {player.pos, player.size};
                Rect r2 = {e.pos, e.size};
                if (r1.intersects(r2)) {
                    isGameOver = true;
                }
            }

            float now = glfwGetTime();
            if (now - lastFrameTime >= frameInterval) {
                player.iFrame = (player.iFrame + 1) % player.nFrames;
                for (auto& e : enemies)
                    e.iFrame = (e.iFrame + 1) % e.nFrames;
                lastFrameTime = now;
            }
        }

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniform2f(glGetUniformLocation(shaderID, "offset_tex"), 0.0f, 0.0f);
        drawSprite(shaderID, background);

        glUniform2f(glGetUniformLocation(shaderID, "offset_tex"), player.iFrame * player.ds, player.iAnimation * player.dt);
        drawSprite(shaderID, player);

        for (const auto& e : enemies) {
            glUniform2f(glGetUniformLocation(shaderID, "offset_tex"), e.iFrame * e.ds, e.iAnimation * e.dt);
            drawSprite(shaderID, e);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}