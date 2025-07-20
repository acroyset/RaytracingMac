#include <chrono>
#include <ctime>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

bool apple = false;

GLFWwindow* window = nullptr;
GLuint shaderProgram = 0;
GLuint displayShader = 0;
GLuint vao = 0;

GLuint pingpongFBO[2];
GLuint pingpongTex[2];


void createPingPongBuffers(int width, int height) {
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongTex);

    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, pingpongTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongTex[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "Ping-pong FBO " << i << " not complete!\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
std::string loadShaderSource(const char* path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
GLuint compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compilation failed:\n" << log << std::endl;
    }
    return shader;
}
GLuint createShaderProgram(const char* vertPath, const char* fragPath) {
    GLuint vert = compileShader(GL_VERTEX_SHADER, loadShaderSource(vertPath));
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, loadShaderSource(fragPath));
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
bool setup() {
    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #endif

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    window = glfwCreateWindow(mode->width, mode->height, "Modular OpenGL Shader Window", monitor, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }

    shaderProgram = createShaderProgram("shaders/fullscreen.vert", "shaders/fullscreen.frag");
    displayShader = createShaderProgram("shaders/fullscreen.vert", "shaders/display.frag");

    glfwSetInputMode(window , GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    return true;
}
bool shouldClose() {
    return glfwWindowShouldClose(window);
}
void shutdown() {
    glDeleteProgram(shaderProgram);
    glDeleteVertexArrays(1, &vao);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void setBasisVectors(const glm::vec3& forward, glm::vec3& up, glm::vec3& right) {
    constexpr glm::vec3 world_up(0, 1, 0);
    right = glm::normalize(glm::cross(forward, world_up));
    up = glm::normalize(glm::cross(right, forward));
}
bool updateCamera(glm::vec3& cameraPos, glm::vec3& camForward, glm::vec3& camUp, glm::vec3& camRight, int width, int height, float speed, float sensitivity, float dt) {
    float centerX = apple ? float(width) / 4 : float(width) / 2;
    float centerY = apple ? float(height) / 4 : float(height) / 2;
    double xpos, ypos;
    bool moved = false;
    glfwGetCursorPos(window, &xpos, &ypos);
    auto delta = glm::vec2(xpos - centerX, -(ypos - centerY));
    if (delta.x*delta.x + delta.y*delta.y > 0) {
        delta *= 2.0f/float(height) * sensitivity;
        camForward += delta.x * camRight + delta.y * camUp;
        camForward = glm::normalize(camForward);
        moved = true;
        setBasisVectors(camForward, camUp, camRight);
        glfwSetCursorPos(window, centerX, centerY);
    }

    glm::vec3 change = glm::vec3(0, 0, 0);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        change += camForward;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        change -= camForward;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        change -= camRight;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        change += camRight;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        change += camUp;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        change -= camUp;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
       glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
        speed *= 2;
       }
    if (pow(change.x, 2) + pow(change.y, 2) + pow(change.z, 2) > 0) {
        change = glm::normalize(change);
        cameraPos += change*speed*dt;
        moved = true;
    }
    return moved;
}

float randomValue(unsigned int& state){
    state = state * 747796405u + 2891336453u;
    unsigned int result = ((state >> ((state >> 28) + 4u)) ^ state) * 277803737u;
    result = (result >> 22) ^ result;
    return float(result) * (1/4294967295.0f);
}

class Timer {
    std::chrono::system_clock::time_point start;

public:
    explicit Timer(const bool paused = false) {
        start = std::chrono::high_resolution_clock::now();
    }

    int reset() {
        const std::chrono::system_clock::time_point end = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        start = end;
        return duration.count();
    }
};

int main() {
    if (!setup()) return -1;

    std::vector spheres = {
        glm::vec3(0, -200, 50),
        glm::vec3(700, -350, 150),
        glm::vec3(100, -400, -400),
        glm::vec3(300, -425, -100),
        glm::vec3(750, -450, -150),
        glm::vec3(-350, -375, -350),
        glm::vec3(450, -440, -300),
    };
    std::vector<float> radii = {
        300,
        150,
        100,
        75,
        50,
        125,
        60
    };
    std::vector colors = {
        glm::vec3(0.1,1,1.5),
        glm::vec3(0.9, 0.2, 0.2),
        glm::vec3(0.2, 0.9, 0.2),
        glm::vec3(0.2, 0.2, 0.9),
        glm::vec3(2, 1, 0.2),
        glm::vec3(0.9, 0.9, 0.9),
        glm::vec3(0.9, 0.9, 0.9),
    };
    std::vector<float> smoothness = {
        0,
        0,
        0,
        0,
        0,
        0,
        1
    };
    std::vector<float> emission = {
        1,
        0,
        0,
        0,
        1,
        0,
        0
    };

    glm::vec3 cameraPos = glm::vec3(0, 0, -500);
    glm::vec3 camTarget = glm::vec3(0, -200, 0);
    glm::vec3 camForward = glm::normalize(camTarget-cameraPos);
    glm::vec3 camUp, camRight;
    setBasisVectors(camForward, camUp, camRight);

    int size = int(spheres.size());
    int frameCount = 0;
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);
    createPingPongBuffers(width, height);
    GLuint resLoc = glGetUniformLocation(shaderProgram, "resolution");
    int ping = 0; int pong = 1;

    Timer deltaTimer;
    while (!shouldClose()) {
        const auto dt = deltaTimer.reset()/1000000.0f;
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[ping]);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);

        bool moved = updateCamera(cameraPos, camForward, camUp, camRight, width, height, 500, 2, dt);


        glUniform3fv(glGetUniformLocation(shaderProgram, "spheres[0]"), size, glm::value_ptr(spheres[0]));
        glUniform1fv(glGetUniformLocation(shaderProgram, "radii[0]"), size, &radii[0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, "colors[0]"), size, glm::value_ptr(colors[0]));
        glUniform1fv(glGetUniformLocation(shaderProgram, "smoothness[0]"), size, &smoothness[0]);
        glUniform1fv(glGetUniformLocation(shaderProgram, "emission[0]"), size, &emission[0]);
        glUniform1i(glGetUniformLocation(shaderProgram, "size"), size);
        glUniform3f(glGetUniformLocation(shaderProgram, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "camForward"), camForward.x, camForward.y, camForward.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "camUp"), camUp.x, camUp.y, camUp.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "camRight"), camRight.x, camRight.y, camRight.z);
        glUniform2f(resLoc, static_cast<float>(width), static_cast<float>(height));
        glUniform1i(glGetUniformLocation(shaderProgram, "frameCount"), frameCount);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pingpongTex[pong]);
        glUniform1i(glGetUniformLocation(shaderProgram, "uPrevFrame"), 0);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(displayShader); // just draws the texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pingpongTex[ping]);
        glUniform1i(glGetUniformLocation(displayShader, "screenTex"), 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Swap ping-pong buffers
        std::swap(ping, pong);
        frameCount++;

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (moved) {
            frameCount = 0;

            // Clear ping-pong buffers to black:
            for (unsigned int i : pingpongFBO) {
                glBindFramebuffer(GL_FRAMEBUFFER, i);
                glViewport(0, 0, width, height);
                glClearColor(0, 0, 0, 1);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }
    shutdown();
    return 0;
}


