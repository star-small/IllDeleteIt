#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <signal.h>
#include <sys/wait.h>

// Настройки окружения рабочего стола
#define WINDOW_TITLE "OpenGL ES Desktop Environment"
#define BACKGROUND_COLOR 0.05f, 0.1f, 0.2f, 1.0f  // Темно-синий фон

// Шейдеры
const char* vertexShaderSource = 
    "attribute vec3 aPos;\n"
    "attribute vec3 aNormal;\n"
    "varying vec3 FragPos;\n"
    "varying vec3 Normal;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "    FragPos = vec3(model * vec4(aPos, 1.0));\n"
    "    Normal = mat3(model) * aNormal;\n"
    "    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "}\0";

const char* fragmentShaderSource = 
    "precision mediump float;\n"
    "varying vec3 FragPos;\n"
    "varying vec3 Normal;\n"
    "uniform vec3 lightPos;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec3 lightColor;\n"
    "uniform vec3 objectColor;\n"
    "void main()\n"
    "{\n"
    "    // Фоновое освещение\n"
    "    float ambientStrength = 0.2;\n"
    "    vec3 ambient = ambientStrength * lightColor;\n"
    "    // Диффузное освещение\n"
    "    vec3 norm = normalize(Normal);\n"
    "    vec3 lightDir = normalize(lightPos - FragPos);\n"
    "    float diff = max(dot(norm, lightDir), 0.0);\n"
    "    vec3 diffuse = diff * lightColor;\n"
    "    // Объединяем результаты\n"
    "    vec3 result = (ambient + diffuse) * objectColor;\n"
    "    gl_FragColor = vec4(result, 1.0);\n"
    "}\0";

// Структура для матриц 4x4
typedef struct {
    float m[16];
} Matrix4;

// X11 и EGL переменные
Display* x_display = NULL;
Window root_window;
EGLDisplay egl_display;
EGLSurface egl_surface;
EGLContext egl_context;
EGLConfig egl_config;
int screen_width, screen_height;
int running = 1;

// Переменные для 3D объектов
unsigned int shaderProgram;
unsigned int VBO;

// Данные вершин для куба
float vertices[] = {
    // позиции          // нормали
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};

// Позиции декоративных 3D объектов
float cubePositions[][3] = {
    { -2.0f,  0.0f, -5.0f},
    {  2.0f,  1.0f, -6.0f},
    { -1.5f, -1.2f, -3.0f},
    {  1.8f, -0.6f, -4.0f},
    {  0.0f,  0.8f, -2.0f}
};

// Камера
float cameraPos[3] = {0.0f, 0.0f, 3.0f};
float cameraFront[3] = {0.0f, 0.0f, -1.0f};
float cameraUp[3] = {0.0f, 1.0f, 0.0f};

// Обработчик сигналов
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        running = 0;
    }
}

// Функции для работы с X11
int init_x11() {
    x_display = XOpenDisplay(NULL);
    if (x_display == NULL) {
        fprintf(stderr, "Не удалось открыть соединение с X сервером\n");
        return 0;
    }

    int screen = DefaultScreen(x_display);
    root_window = RootWindow(x_display, screen);
    screen_width = DisplayWidth(x_display, screen);
    screen_height = DisplayHeight(x_display, screen);

    // Захватываем контроль над корневым окном для управления рабочим столом
    XSelectInput(x_display, root_window, 
                 ButtonPressMask | ButtonReleaseMask | 
                 PointerMotionMask | KeyPressMask | KeyReleaseMask | 
                 ExposureMask | StructureNotifyMask);

    // Устанавливаем курсор
    Cursor cursor = XCreateFontCursor(x_display, XC_left_ptr);
    XDefineCursor(x_display, root_window, cursor);
    XFreeCursor(x_display, cursor);

    return 1;
}

// Функции для работы с EGL
int init_egl() {
    egl_display = eglGetDisplay((EGLNativeDisplayType)x_display);
    if (egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "Не удалось получить EGL дисплей\n");
        return 0;
    }

    if (!eglInitialize(egl_display, NULL, NULL)) {
        fprintf(stderr, "Не удалось инициализировать EGL\n");
        return 0;
    }

    // Выбираем конфигурацию EGL
    const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };
    
    EGLint num_configs;
    if (!eglChooseConfig(egl_display, config_attribs, &egl_config, 1, &num_configs)) {
        fprintf(stderr, "Не удалось выбрать конфигурацию EGL: %x\n", eglGetError());
        return 0;
    }

    // Создаем EGL поверхность для корневого окна
    egl_surface = eglCreateWindowSurface(egl_display, egl_config, 
                                        (EGLNativeWindowType)root_window, NULL);
    if (egl_surface == EGL_NO_SURFACE) {
        fprintf(stderr, "Не удалось создать поверхность EGL: %x\n", eglGetError());
        return 0;
    }

    // Создаем контекст OpenGL ES 2.0
    const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    
    egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
    if (egl_context == EGL_NO_CONTEXT) {
        fprintf(stderr, "Не удалось создать контекст EGL: %x\n", eglGetError());
        return 0;
    }

    // Делаем контекст текущим
    if (!eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context)) {
        fprintf(stderr, "Не удалось сделать контекст EGL текущим: %x\n", eglGetError());
        return 0;
    }

    return 1;
}

// Освобождение ресурсов EGL
void deinit_egl() {
    if (egl_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (egl_context != EGL_NO_CONTEXT) {
            eglDestroyContext(egl_display, egl_context);
        }
        if (egl_surface != EGL_NO_SURFACE) {
            eglDestroySurface(egl_display, egl_surface);
        }
        eglTerminate(egl_display);
    }
}

// Освобождение ресурсов X11
void deinit_x11() {
    if (x_display) {
        XCloseDisplay(x_display);
    }
}

// Создание шейдерной программы
unsigned int create_shader_program(const char* vertex_source, const char* fragment_source) {
    // Вершинный шейдер
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_source, NULL);
    glCompileShader(vertex_shader);
    
    // Проверка ошибок компиляции вершинного шейдера
    int success;
    char info_log[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
        fprintf(stderr, "Ошибка компиляции вершинного шейдера: %s\n", info_log);
    }
    
    // Фрагментный шейдер
    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_source, NULL);
    glCompileShader(fragment_shader);
    
    // Проверка ошибок компиляции фрагментного шейдера
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
        fprintf(stderr, "Ошибка компиляции фрагментного шейдера: %s\n", info_log);
    }
    
    // Связывание шейдеров в программу
    unsigned int shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    
    // Проверка ошибок связывания
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, NULL, info_log);
        fprintf(stderr, "Ошибка связывания шейдерной программы: %s\n", info_log);
    }
    
    // Удаляем шейдеры, они уже связаны с программой
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return shader_program;
}

// Функции для работы с матрицами
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
    
    // Умножаем m на rotation
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
    u[0] = s[1] * f[2] - s[2] * f[1];
    u[1] = s[2] * f[0] - s[0] * f[2];
    u[2] = s[0] * f[1] - s[1] * f[0];
    
    memset(result.m, 0, sizeof(float) * 16);
    result.m[0] = s[0];
    result.m[1] = u[0];
    result.m[2] = -f[0];
    result.m[3] = 0.0f;
    
    result.m[4] = s[1];
    result.m[5] = u[1];
    result.m[6] = -f[1];
    result.m[7] = 0.0f;
    
    result.m[8] = s[2];
    result.m[9] = u[2];
    result.m[10] = -f[2];
    result.m[11] = 0.0f;
    
    result.m[12] = -(s[0] * eyeX + s[1] * eyeY + s[2] * eyeZ);
    result.m[13] = -(u[0] * eyeX + u[1] * eyeY + u[2] * eyeZ);
    result.m[14] = f[0] * eyeX + f[1] * eyeY + f[2] * eyeZ;
    result.m[15] = 1.0f;
    
    return result;
}

// Установка uniform переменных в шейдере
void set_mat4(unsigned int shader, const char* name, Matrix4 mat) {
    GLint loc = glGetUniformLocation(shader, name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, mat.m);
}

void set_vec3(unsigned int shader, const char* name, float x, float y, float z) {
    GLint loc = glGetUniformLocation(shader, name);
    glUniform3f(loc, x, y, z);
}

// Инициализация OpenGL ресурсов
void init_gl() {
    // Создаем шейдерную программу
    shaderProgram = create_shader_program(vertexShaderSource, fragmentShaderSource);

    // Создаем буфер вершин (VBO)
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Получаем местоположение атрибутов
    GLint posAttrib = glGetAttribLocation(shaderProgram, "aPos");
    GLint normalAttrib = glGetAttribLocation(shaderProgram, "aNormal");

    // Настраиваем атрибуты
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(posAttrib);
    
    glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(normalAttrib);

    // Включаем тест глубины
    glEnable(GL_DEPTH_TEST);

    // Используем шейдерную программу
    glUseProgram(shaderProgram);

    // Устанавливаем свойства света
    set_vec3(shaderProgram, "lightColor", 1.0f, 1.0f, 1.0f);
    set_vec3(shaderProgram, "objectColor", 1.0f, 0.5f, 0.0f);  // Оранжевый цвет для кубов
}

// Очистка OpenGL ресурсов
void deinit_gl() {
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
}

// Запуск приложения (вместо терминала)
void launch_terminal() {
    // Создаем дочерний процесс
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        // Запускаем терминал
        execlp("xterm", "xterm", NULL);
        // Если execlp вернулся, значит произошла ошибка
        exit(1);
    }
}

// Обработка событий X11
void process_x11_events() {
    XEvent event;
    
    // Проверяем наличие событий, но не блокируем
    while (XPending(x_display)) {
        XNextEvent(x_display, &event);
        
        switch (event.type) {
            case ButtonPress:
                // Обработка нажатия кнопки мыши
                if (event.xbutton.button == 3) {  // Правая кнопка мыши
                    launch_terminal();
                }
                break;
                
            case KeyPress:
                // Обработка нажатия клавиши
                {
                    KeySym key = XLookupKeysym(&event.xkey, 0);
                    if (key == XK_Escape) {
                        running = 0;  // Выход по нажатию Escape
                    } else if (key ==
