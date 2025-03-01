#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Window dimensions
#define WIDTH 800
#define HEIGHT 600

// Shader sources
const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n"
    "layout (location = 2) in vec2 aTexCoord;\n"
    "out vec3 FragPos;\n"
    "out vec3 Normal;\n"
    "out vec2 TexCoord;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "    FragPos = vec3(model * vec4(aPos, 1.0));\n"
    "    Normal = mat3(transpose(inverse(model))) * aNormal;\n"
    "    TexCoord = aTexCoord;\n"
    "    gl_Position = projection * view * vec4(FragPos, 1.0);\n"
    "}\0";

const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 FragPos;\n"
    "in vec3 Normal;\n"
    "in vec2 TexCoord;\n"
    "uniform vec3 lightPos;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec3 lightColor;\n"
    "uniform vec3 objectColor;\n"
    "void main()\n"
    "{\n"
    "    // Ambient lighting\n"
    "    float ambientStrength = 0.2;\n"
    "    vec3 ambient = ambientStrength * lightColor;\n"
    "    // Diffuse lighting\n"
    "    vec3 norm = normalize(Normal);\n"
    "    vec3 lightDir = normalize(lightPos - FragPos);\n"
    "    float diff = max(dot(norm, lightDir), 0.0);\n"
    "    vec3 diffuse = diff * lightColor;\n"
    "    // Specular lighting\n"
    "    float specularStrength = 0.5;\n"
    "    vec3 viewDir = normalize(viewPos - FragPos);\n"
    "    vec3 reflectDir = reflect(-lightDir, norm);\n"
    "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
    "    vec3 specular = specularStrength * spec * lightColor;\n"
    "    // Combine results\n"
    "    vec3 result = (ambient + diffuse + specular) * objectColor;\n"
    "    FragColor = vec4(result, 1.0);\n"
    "}\0";

// Struct for matrices
typedef struct {
    float m[16];
} Matrix4;

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
unsigned int createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource);
Matrix4 perspective(float fovy, float aspect, float near, float far);
Matrix4 lookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ);
Matrix4 identity();
Matrix4 translate(Matrix4 m, float x, float y, float z);
Matrix4 rotate(Matrix4 m, float angle, float x, float y, float z);
void setMat4(unsigned int shader, const char* name, Matrix4 mat);
void setVec3(unsigned int shader, const char* name, float x, float y, float z);

// Vertex data for a cube
float vertices[] = {
    // positions          // normals           // texture coords
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
};

// Cube positions
float cubePositions[][3] = {
    { 0.0f,  0.0f,  0.0f},
    { 2.0f,  5.0f, -15.0f},
    {-1.5f, -2.2f, -2.5f},
    {-3.8f, -2.0f, -12.3f},
    { 2.4f, -0.4f, -3.5f}
};

// Camera variables
float cameraPos[3] = {0.0f, 0.0f, 3.0f};
float cameraFront[3] = {0.0f, 0.0f, -1.0f};
float cameraUp[3] = {0.0f, 1.0f, 0.0f};
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Main function
int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    
    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL 3D Scene for Orange Pi CM4", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    // Create shader program
    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    // VAO, VBO
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Use shader program
    glUseProgram(shaderProgram);

    // Light properties
    setVec3(shaderProgram, "lightColor", 1.0f, 1.0f, 1.0f);
    setVec3(shaderProgram, "objectColor", 1.0f, 0.5f, 0.0f); // Orange color

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        processInput(window);

        // Render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate shader program
        glUseProgram(shaderProgram);

        // Update light position
        float lightX = sin(glfwGetTime()) * 2.0f;
        float lightY = sin(glfwGetTime() / 2.0f) * 1.0f;
        float lightZ = cos(glfwGetTime()) * 2.0f;
        setVec3(shaderProgram, "lightPos", lightX, lightY, lightZ);
        setVec3(shaderProgram, "viewPos", cameraPos[0], cameraPos[1], cameraPos[2]);

        // Create transformations
        Matrix4 view = lookAt(
            cameraPos[0], cameraPos[1], cameraPos[2],
            cameraPos[0] + cameraFront[0], cameraPos[1] + cameraFront[1], cameraPos[2] + cameraFront[2],
            cameraUp[0], cameraUp[1], cameraUp[2]
        );
        Matrix4 projection = perspective(45.0f, (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

        // Set transformation matrices
        setMat4(shaderProgram, "view", view);
        setMat4(shaderProgram, "projection", projection);

        // Render cubes
        glBindVertexArray(VAO);
        for (unsigned int i = 0; i < 5; i++) {
            // Calculate model matrix
            Matrix4 model = identity();
            model = translate(model, cubePositions[i][0], cubePositions[i][1], cubePositions[i][2]);
            float angle = 20.0f * i + glfwGetTime() * 25.0f;
            model = rotate(model, angle, 1.0f, 0.3f, 0.5f);
            setMat4(shaderProgram, "model", model);

            // Draw cube
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

// Process input
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);

    const float cameraSpeed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        cameraPos[0] += cameraSpeed * cameraFront[0];
        cameraPos[1] += cameraSpeed * cameraFront[1];
        cameraPos[2] += cameraSpeed * cameraFront[2];
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        cameraPos[0] -= cameraSpeed * cameraFront[0];
        cameraPos[1] -= cameraSpeed * cameraFront[1];
        cameraPos[2] -= cameraSpeed * cameraFront[2];
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        float tmpX = cameraFront[1] * cameraUp[2] - cameraFront[2] * cameraUp[1];
        float tmpY = cameraFront[2] * cameraUp[0] - cameraFront[0] * cameraUp[2];
        float tmpZ = cameraFront[0] * cameraUp[1] - cameraFront[1] * cameraUp[0];
        float len = sqrt(tmpX * tmpX + tmpY * tmpY + tmpZ * tmpZ);
        tmpX /= len;
        tmpY /= len;
        tmpZ /= len;
        cameraPos[0] -= cameraSpeed * tmpX;
        cameraPos[1] -= cameraSpeed * tmpY;
        cameraPos[2] -= cameraSpeed * tmpZ;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        float tmpX = cameraFront[1] * cameraUp[2] - cameraFront[2] * cameraUp[1];
        float tmpY = cameraFront[2] * cameraUp[0] - cameraFront[0] * cameraUp[2];
        float tmpZ = cameraFront[0] * cameraUp[1] - cameraFront[1] * cameraUp[0];
        float len = sqrt(tmpX * tmpX + tmpY * tmpY + tmpZ * tmpZ);
        tmpX /= len;
        tmpY /= len;
        tmpZ /= len;
        cameraPos[0] += cameraSpeed * tmpX;
        cameraPos[1] += cameraSpeed * tmpY;
        cameraPos[2] += cameraSpeed * tmpZ;
    }
}

// Callback for window resize
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Create shader program
unsigned int createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource) {
    // Vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Check for errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
    }
    
    // Fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // Check for errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
    }
    
    // Shader program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // Check for errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }
    
    // Delete shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

// Matrix functions
Matrix4 identity() {
    Matrix4 mat;
    memset(mat.m, 0, sizeof(float) * 16);
    mat.m[0] = 1.0f;
    mat.m[5] = 1.0f;
    mat.m[10] = 1.0f;
    mat.m[15] = 1.0f;
    return mat;
}

Matrix4 translate(Matrix4 m, float x, float y, float z) {
    Matrix4 result = m;
    result.m[12] = m.m[0] * x + m.m[4] * y + m.m[8] * z + m.m[12];
    result.m[13] = m.m[1] * x + m.m[5] * y + m.m[9] * z + m.m[13];
    result.m[14] = m.m[2] * x + m.m[6] * y + m.m[10] * z + m.m[14];
    result.m[15] = m.m[3] * x + m.m[7] * y + m.m[11] * z + m.m[15];
    return result;
}

Matrix4 rotate(Matrix4 m, float angle, float x, float y, float z) {
    Matrix4 result;
    float radians = angle * M_PI / 180.0f;
    float c = cos(radians);
    float s = sin(radians);
    float norm = sqrt(x * x + y * y + z * z);
    
    if (norm != 1.0f) {
        x /= norm;
        y /= norm;
        z /= norm;
    }
    
    float xx = x * x;
    float xy = x * y;
    float xz = x * z;
    float yy = y * y;
    float yz = y * z;
    float zz = z * z;
    float xs = x * s;
    float ys = y * s;
    float zs = z * s;
    float oneMinusC = 1.0f - c;
    
    Matrix4 rotation;
    rotation.m[0] = xx * oneMinusC + c;
    rotation.m[1] = xy * oneMinusC + zs;
    rotation.m[2] = xz * oneMinusC - ys;
    rotation.m[3] = 0.0f;
    
    rotation.m[4] = xy * oneMinusC - zs;
    rotation.m[5] = yy * oneMinusC + c;
    rotation.m[6] = yz * oneMinusC + xs;
    rotation.m[7] = 0.0f;
    
    rotation.m[8] = xz * oneMinusC + ys;
    rotation.m[9] = yz * oneMinusC - xs;
    rotation.m[10] = zz * oneMinusC + c;
    rotation.m[11] = 0.0f;
    
    rotation.m[12] = 0.0f;
    rotation.m[13] = 0.0f;
    rotation.m[14] = 0.0f;
    rotation.m[15] = 1.0f;
    
    // Multiply m by rotation
    result.m[0] = m.m[0] * rotation.m[0] + m.m[4] * rotation.m[1] + m.m[8] * rotation.m[2];
    result.m[1] = m.m[1] * rotation.m[0] + m.m[5] * rotation.m[1] + m.m[9] * rotation.m[2];
    result.m[2] = m.m[2] * rotation.m[0] + m.m[6] * rotation.m[1] + m.m[10] * rotation.m[2];
    result.m[3] = m.m[3] * rotation.m[0] + m.m[7] * rotation.m[1] + m.m[11] * rotation.m[2];
    
    result.m[4] = m.m[0] * rotation.m[4] + m.m[4] * rotation.m[5] + m.m[8] * rotation.m[6];
    result.m[5] = m.m[1] * rotation.m[4] + m.m[5] * rotation.m[5] + m.m[9] * rotation.m[6];
    result.m[6] = m.m[2] * rotation.m[4] + m.m[6] * rotation.m[5] + m.m[10] * rotation.m[6];
    result.m[7] = m.m[3] * rotation.m[4] + m.m[7] * rotation.m[5] + m.m[11] * rotation.m[6];
    
    result.m[8] = m.m[0] * rotation.m[8] + m.m[4] * rotation.m[9] + m.m[8] * rotation.m[10];
    result.m[9] = m.m[1] * rotation.m[8] + m.m[5] * rotation.m[9] + m.m[9] * rotation.m[10];
    result.m[10] = m.m[2] * rotation.m[8] + m.m[6] * rotation.m[9] + m.m[10] * rotation.m[10];
    result.m[11] = m.m[3] * rotation.m[8] + m.m[7] * rotation.m[9] + m.m[11] * rotation.m[10];
    
    result.m[12] = m.m[12];
    result.m[13] = m.m[13];
    result.m[14] = m.m[14];
    result.m[15] = m.m[15];
    
    return result;
}

Matrix4 perspective(float fovy, float aspect, float near, float far) {
    Matrix4 result;
    memset(result.m, 0, sizeof(float) * 16);
    
    float tan_half_fovy = tan(fovy * M_PI / 360.0f);
    
    result.m[0] = 1.0f / (aspect * tan_half_fovy);
    result.m[5] = 1.0f / tan_half_fovy;
    result.m[10] = -(far + near) / (far - near);
    result.m[11] = -1.0f;
    result.m[14] = -(2.0f * far * near) / (far - near);
    
    return result;
}

Matrix4 lookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ) {
    Matrix4 result;
    
    float f[3];
    f[0] = centerX - eyeX;
    f[1] = centerY - eyeY;
    f[2] = centerZ - eyeZ;
    
    float norm = sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    if (norm > 0.0f) {
        f[0] /= norm;
        f[1] /= norm;
        f[2] /= norm;
    }
    
    float s[3];
    s[0] = f[1] * upZ - f[2] * upY;
    s[1] = f[2] * upX - f[0] * upZ;
    s[2] = f[0] * upY - f[1] * upX;
    
    norm = sqrt(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
    if (norm > 0.0f) {
        s[0] /= norm;
        s[1] /= norm;
        s[2] /= norm;
    }
    
    float u[3];
