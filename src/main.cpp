#include <iostream>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Vertex shader para líneas con color por vértice (gradiente RGB)
const char* lineVertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"out vec3 vColor;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"    vColor = aColor; // Pasamos el color al fragment shader\n"
"}\0";

// Fragment shader para líneas (usa el color interpolado)
const char* lineFragmentShaderSource = "#version 330 core\n"
"in vec3 vColor;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"    FragColor = vec4(vColor, 1.0);\n"
"}\n\0";

// Vertex shader para caras texturizadas
const char* texVertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 2) in vec2 aTexCoord;\n"
"out vec2 vTexCoord;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"    vTexCoord = aTexCoord;\n"
"}\0";

// Fragment shader para texturas en las caras
const char* texFragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec2 vTexCoord;\n"
"uniform sampler2D uTexture;\n"
"void main()\n"
"{\n"
"    FragColor = texture(uTexture, vTexCoord);\n"
"}\n\0";

void processInput(GLFWwindow* window, float& camAngle, float& radius, float& camPos, float dt);
GLuint compileShader(GLenum type, const char* src);
GLuint createProgram(const char* vsSrc, const char* fsSrc);
GLuint createCheckerTexture();


int main()
{
    // Inicialización de GLFW y creación de la ventana
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);            
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Proyecto_OpenGL", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);

    // Cargamos punteros de funciones de OpenGL via GLAD
    if (!gladLoadGL())
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Definimos el área de dibujo en píxeles dentro de la ventana
    glViewport(0, 0, 800, 800);

    // Activamos test de profundidad para dibujar objetos 3D correctamente
    glEnable(GL_DEPTH_TEST);

    // Compilación y enlace de los programas de shaders
    GLuint lineProgram = createProgram(lineVertexShaderSource, lineFragmentShaderSource);
    GLuint texProgram  = createProgram(texVertexShaderSource, texFragmentShaderSource);

    // Datos de la pirámide    

    GLfloat vertices[] = {
        // posición           // color RGB         // texCoords
        // Vértice 0
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,    0.0f, 0.0f,
        // Vértice 1
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,    1.0f, 0.0f,
        // Vértice 2
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,    1.0f, 1.0f,
        // Vértice 3
        -0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,    0.0f, 1.0f,
        // Vértice 4 (punta)
         0.0f,  0.5f,  0.0f,  1.0f, 0.0f, 1.0f,    0.5f, 1.0f
    };

    // Índices para las caras (triángulos) de la pirámide
    unsigned int triIndices[] = {
        // Lados
        0, 1, 4,
        1, 2, 4,
        2, 3, 4,
        3, 0, 4,
        // Base (cuadrado como 2 triángulos)
        0, 1, 2,
        2, 3, 0
    };

    // Índices para las aristas (líneas) de la pirámide
    unsigned int lineIndices[] = {
        // Base
        0, 1,
        1, 2,
        2, 3,
        3, 0,
        // Lados
        0, 4,
        1, 4,
        2, 4,
        3, 4
    };

    // VAO, VBO y EBO
    GLuint VAO, VBO, EBO_tri, EBO_line;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO_tri);
    glGenBuffers(1, &EBO_line);

    // Configuramos el VAO (qué buffers y atributos usa)
    glBindVertexArray(VAO);

    // VBO: subimos todos los vértices (pos, color, texcoord)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // EBO para triángulos
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_tri);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triIndices), triIndices, GL_STATIC_DRAW);

    // Atributo 0: posición (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Atributo 1: color (vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Atributo 2: coordenadas de textura (vec2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // EBO para líneas (mismo VAO, cambiamos el buffer al dibujar)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_line);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(lineIndices), lineIndices, GL_STATIC_DRAW);

    // Desvinculamos el VAO (opcional, por seguridad)
    glBindVertexArray(0);

    // Textura para las caras
    GLuint textureID = createCheckerTexture();

    // Uniform locations compartidos
    glUseProgram(lineProgram);
    GLint uModel_line = glGetUniformLocation(lineProgram, "model");
    GLint uView_line  = glGetUniformLocation(lineProgram, "view");
    GLint uProj_line  = glGetUniformLocation(lineProgram, "projection");

    glUseProgram(texProgram);
    GLint uModel_tex = glGetUniformLocation(texProgram, "model");
    GLint uView_tex  = glGetUniformLocation(texProgram, "view");
    GLint uProj_tex  = glGetUniformLocation(texProgram, "projection");

    // Matriz de proyección en perspectiva (45º, relación 1:1)
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

    glUseProgram(lineProgram);
    glUniformMatrix4fv(uProj_line, 1, GL_FALSE, glm::value_ptr(projection));

    glUseProgram(texProgram);
    glUniformMatrix4fv(uProj_tex, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(glGetUniformLocation(texProgram, "uTexture"), 0); // textura en unidad 0

    float camAngle = 0.0f;
    float radius   = 3.0f;
    float camPosF  = 1.0f; 
    float lastTime = (float)glfwGetTime();

    // Bucle principal de render
    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;

        // Entrada de usuario (rotar cámara / zoom)
        processInput(window, camAngle, radius, camPosF, dt);

        // Limpiamos color y profundidad
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Matrices de modelo, vista y proyección
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, now * 0.8f, glm::vec3(0.5f, 1.0f, 0.3f));

        // Cámara orbitando alrededor del origen
        float camX = std::sin(camAngle) * radius;
        float camZ = std::cos(camAngle) * radius;
        glm::vec3 camPos(camX, 1.0f, camZ);
        glm::mat4 view = glm::lookAt(camPos,
                                     glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));

        // Dibujamos caras texturizadas
        glUseProgram(texProgram);
        glUniformMatrix4fv(uModel_tex, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(uView_tex,  1, GL_FALSE, glm::value_ptr(view));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_tri); // índices de triángulos
        glDrawElements(GL_TRIANGLES, sizeof(triIndices) / sizeof(unsigned int), GL_UNSIGNED_INT, 0);

        // Dibujamos líneas de la pirámide (wireframe con gradiente RGB)
        glUseProgram(lineProgram);
        glUniformMatrix4fv(uModel_line, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(uView_line,  1, GL_FALSE, glm::value_ptr(view));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_line); // índices de líneas
        glDrawElements(GL_LINES, sizeof(lineIndices) / sizeof(unsigned int), GL_UNSIGNED_INT, 0);

        // Intercambiamos buffers y procesamos eventos
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpieza de recursos
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO_tri);
    glDeleteBuffers(1, &EBO_line);
    glDeleteProgram(lineProgram);
    glDeleteProgram(texProgram);
    glDeleteTextures(1, &textureID);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window, float& camAngle, float& radius, float& camPos, float dt)
{
    // A / LEFT -> girar cámara a la izquierda
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camAngle -= 1.5f * dt;

    // D / RIGHT -> girar cámara a la derecha
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camAngle += 1.5f * dt;

    // W / UP -> acercar (disminuir radio)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        radius = std::max(0.5f, radius - 1.5f * dt);

    // S / DOWN -> alejar (aumentar radio)
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        radius = std::min(10.0f, radius + 1.5f * dt);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// Función auxiliar para compilar shaders
GLuint compileShader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Error al compilar shader: " << infoLog << std::endl;
    }
    return shader;
}

// Crea un programa a partir de vertex + fragment shader
GLuint createProgram(const char* vsSrc, const char* fsSrc)
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Error al enlazar programa: " << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// Crea una textura sencilla procedural (tipo cuadriculado)
GLuint createCheckerTexture()
{
    const int texWidth = 4;
    const int texHeight = 4;
    unsigned char data[texWidth * texHeight * 3];

    // Generamos un patrón simple de cuadritos
    for (int y = 0; y < texHeight; ++y)
    {
        for (int x = 0; x < texWidth; ++x)
        {
            int index = (y * texWidth + x) * 3;
            bool even = ((x + y) % 2) == 0;
            data[index + 0] = even ? 255 : 50;  // R
            data[index + 1] = even ? 255 : 50;  // G
            data[index + 2] = even ? 255 : 200; // B
        }
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // Parámetros de envoltura (wrap) y filtrado
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Cargamos la textura en memoria de GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}
