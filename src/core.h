//
//  core.h
//  Marching Cube Terrain
//
//  Created by Dmitri Wamback on 2025-06-19.
//

#include <GL/glew.h>
#include <glfw3.h>

GLFWwindow* window;

float seed;
float deltaTime = 0;

int terrainSize = 20;

#include <fstream>
#include <sstream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include "util/noise.h"
#include "object/camera.h"

#include "marchingCubeTable.h"
#include "object/shader.h"
#include "object/vertex.h"
#include "object/terrain.h"

void initialize() {
    glfwInit();
    
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    
    window = glfwCreateWindow(1200, 800, "Raymarching", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    
    glewExperimental = GL_TRUE;
    glewInit();
    glEnable(GL_DEPTH_TEST);
    
    srand(static_cast<unsigned int>(std::time(nullptr)));
    seed = (float)(rand() % 10000) * 10.23322f;
    
    std::vector<Terrain> terrain = std::vector<Terrain>();
    for (int x = -terrainSize/2; x < terrainSize/2; x++) {
        for (int z = -terrainSize/2; z < terrainSize/2; z++) {
            Terrain t = Terrain::CreateTerrain(x, z);
            terrain.push_back(t);
        }
    }
    
    Camera::Initialize();
    glfwSetCursorPosCallback(window, cursor_position_callback);
    
    Shader shader = Shader::Create("/Users/dmitriwamback/Documents/Projects/Marching Cube Terrain/Marching Cube Terrain/src/shaders/main");
    
    double previousTime = glfwGetTime();
    int frameCount = 0;
    
    while (!glfwWindowShouldClose(window)) {
        
        glm::vec4 movement = glm::vec4(0.0f);
                
        movement.z = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ?  0.05f : 0;
        movement.w = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ? -0.05f : 0;
        movement.x = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ?  0.05f : 0;
        movement.y = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ? -0.05f : 0;
        
        if (glfwGetKey(window, GLFW_KEY_E)) {
            terrain = {};
            srand(static_cast<unsigned int>(std::time(nullptr)));
            seed = (float)(rand() % 10000) * 10.23322f;
            for (int x = -terrainSize/2; x < terrainSize/2; x++) {
                for (int z = -terrainSize/2; z < terrainSize/2; z++) {
                    Terrain t = Terrain::CreateTerrain(x, z);
                    terrain.push_back(t);
                }
            }
        }
        
        camera.Update(movement);
        std::cout << camera.position.x << " " << camera.position.y << " " << camera.position.z << '\n';
        
        glClearColor(0.6, 0.7, 0.9, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        shader.Use();
        
        shader.SetMatrix4("projection", camera.projection);
        shader.SetMatrix4("lookAt", camera.lookAt);
        for (Terrain t : terrain) {
            t.Render(shader);
        }
        
        double currentTime = glfwGetTime();
        double previousDeltaTime = glfwGetTime();
        frameCount++;
        
        if (currentTime - previousTime >= 1.0) {

            glfwSetWindowTitle(window, ("Raymarching FPS: " + std::to_string(frameCount)).c_str());

            frameCount = 0;
            previousTime = currentTime;
            
            double currentDeltatime = glfwGetTime();
            deltaTime = (currentDeltatime - previousDeltaTime);
            previousDeltaTime = currentDeltatime;
            std::cout << deltaTime << '\n';
        }
                
        glfwPollEvents();
        glfwSwapBuffers(window);
    }
}
