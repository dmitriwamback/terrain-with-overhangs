//
//  terrain.h
//  Marching Cube Terrain
//
//  Created by Dmitri Wamback on 2025-06-19.
//

#ifndef terrain_h
#define terrain_h

#include <thread>

class Terrain {
public:
    std::array<float, 16 * 256 * 16> density;
    std::vector<Vertex> vertices;
    glm::vec3 position, scale, rotation;
    
    static Terrain CreateTerrain(int xOffset, int yOffset);
    void Render(Shader shader);
    void Generate(int xOffset, int yOffset);
    glm::mat4 CreateModelMatrix();
private:
    uint32_t vertexArrayObject, vertexBufferObject, indexBufferObject;
};

Terrain Terrain::CreateTerrain(int xOffset, int yOffset) {
    Terrain terrain = Terrain();
    
    terrain.Generate(xOffset, yOffset);
    return terrain;
}

void Terrain::Render(Shader shader) {
    shader.Use();
        
    glm::mat4 model = CreateModelMatrix();
        
    glBindVertexArray(vertexArrayObject);
    shader.SetMatrix4("model", model);
    
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);
}

inline int index3D(int x, int y, int z) {
    return x * 256 * 16 + y * 16 + z;
}

void Terrain::Generate(int xOffset, int yOffset) {
    const int size = 16;
    const float isolevel = 0.0f;

    const float frequency = 0.025f;
    const float lacunarity = 1.5f;
    const float persistence = 0.6f;
    const float heightScale = 102.0f;
    const float caveFreq = 10.0f;
    
    vertices = {};
    
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;

    std::vector<std::thread> threads;

    int chunkPerThread = size / numThreads;
    int leftover = size % numThreads;

    for (unsigned int t = 0; t < numThreads; ++t) {
        int startX = t * chunkPerThread;
        int endX = startX + chunkPerThread;
        if (t == numThreads - 1) endX += leftover;

        threads.emplace_back([=, this]() {
            for (int x = startX; x < endX; ++x) {
                for (int y = 0; y < 256; ++y) {
                    for (int z = 0; z < size; ++z) {

                        float xi = (float)(x + seed + xOffset*8) * frequency / (float)size;
                        float yi = (float)y * frequency / (float)size;
                        float zi = (float)(z + seed + yOffset*8) * frequency / (float)size;

                        float mountainNoise = noiseLayer(xi, zi, lacunarity, persistence, 10, seed);
                        float baseHeight = pow(mountainNoise, 1.0f) * heightScale;
                        
                        float basePlateau = noiseLayer(xi * 0.2f, zi * 0.2f, 1.2, 0.2, 3, seed);
                        baseHeight += basePlateau * 5.0f + 5;

                        float caveNoise = noiseLayer(xi * caveFreq, yi * caveFreq, lacunarity, persistence, 10, zi * caveFreq);
                        caveNoise = glm::clamp(caveNoise, 0.0f, caveNoise);
                        
                        float terrainSurface = (float)y - baseHeight;
                        float _density = terrainSurface + caveNoise * 10.0f;
                        
                        if (y < 4) _density = -1.0f;

                        density[index3D(x, y, z)] = _density;
                    }
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
        
    glm::vec3 vertexOffsets[8] = {
        {0, 0, 0},
        {1, 0, 0},
        {1, 1, 0},
        {0, 1, 0},
        {0, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
        {0, 1, 1}
    };

    glm::ivec2 edgeVertexMap[12] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };

    for (int x = 0; x < size - 1; x++) {
        for (int y = 0; y < 256 - 1; y++) {
            for (int z = 0; z < size - 1; z++) {
                float cubeValues[8];
                glm::vec3 cubePositions[8];
                
                for (int i = 0; i < 8; ++i) {
                    glm::vec3 pos = glm::vec3(x, y, z) + vertexOffsets[i];
                    int px = static_cast<int>(pos.x);
                    int py = static_cast<int>(pos.y);
                    int pz = static_cast<int>(pos.z);
                    
                    cubePositions[i] = pos;

                    if (px >= 0 && px < size && py >= 0 && py < 256 && pz >= 0 && pz < size) {
                        
                        int index = index3D(px, py, pz);
                        
                        cubeValues[i] = density[index];
                    }
                    else {
                        cubeValues[i] = 1.0f;
                    }
                }

                int cubeIndex = 0;
                for (int i = 0; i < 8; i++)
                    if (cubeValues[i] < isolevel) cubeIndex |= (1 << i);

                if (edgeTable[cubeIndex] == 0) continue;

                glm::vec3 edgeVertices[12];

                for (int i = 0; i < 12; i++) {
                    if (edgeTable[cubeIndex] & (1 << i)) {
                        int v0 = edgeVertexMap[i].x;
                        int v1 = edgeVertexMap[i].y;
                        float val0 = cubeValues[v0];
                        float val1 = cubeValues[v1];
                        glm::vec3 p0 = cubePositions[v0];
                        glm::vec3 p1 = cubePositions[v1];

                        float denom = val1 - val0;
                        float mu = 0.5f;
                        if (fabs(denom) > 1e-5f) {
                            mu = (isolevel - val0) / denom;
                            mu = glm::clamp(mu, 0.0f, 1.0f);
                        }
                        edgeVertices[i] = p0 + mu * (p1 - p0);
                    }
                }

                for (int i = 0; triTable[cubeIndex][i] != -1; i += 3) {
                    glm::vec3 v0 = edgeVertices[triTable[cubeIndex][i]];
                    glm::vec3 v1 = edgeVertices[triTable[cubeIndex][i + 1]];
                    glm::vec3 v2 = edgeVertices[triTable[cubeIndex][i + 2]];

                    glm::vec3 normal = glm::normalize(glm::cross(v2 - v0, v1 - v0));
                    glm::vec3 scale = glm::vec3(2.0f, 1.0f, 2.0f);
                    
                    vertices.push_back({v0 * scale, normal, glm::vec2(0.0f)});
                    vertices.push_back({v1 * scale, normal, glm::vec2(0.0f)});
                    vertices.push_back({v2 * scale, normal, glm::vec2(0.0f)});
                }
            }
        }
    }
    
    scale = glm::vec3(1.0f);
    rotation = glm::vec3(0.0f);
    position = glm::vec3(xOffset * size, -10.0f, yOffset * size);
    
    glGenVertexArrays(1, &vertexArrayObject);
    glBindVertexArray(vertexArrayObject);
    
    glGenBuffers(1, &vertexBufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, vertex));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
}

glm::mat4 Terrain::CreateModelMatrix() {
    
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 translationMatrix = glm::mat4(1.0f);
    translationMatrix = glm::translate(translationMatrix, position);
    
    glm::mat4 scaleMatrix = glm::mat4(1.0f);
    scaleMatrix = glm::scale(scaleMatrix, scale);
    
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0)) *
                               glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0)) *
                               glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1));
    
    model = translationMatrix * rotationMatrix * scaleMatrix;
    
    return model;
}

#endif /* terrain_h */
