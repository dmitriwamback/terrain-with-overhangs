//
//  terrain.h
//  Marching Cube Terrain
//
//  Created by Dmitri Wamback on 2025-06-19.
//

#ifndef terrain_h
#define terrain_h

class Terrain {
public:
    std::vector<std::vector<std::vector<float>>> density;
    std::vector<Vertex> vertices;
    glm::vec3 position, scale, rotation;
    
    static Terrain CreateTerrain();
    void Render(Shader shader);
    glm::mat4 CreateModelMatrix();
private:
    uint32_t vertexArrayObject, vertexBufferObject, indexBufferObject;
};

Terrain Terrain::CreateTerrain() {
    Terrain terrain = Terrain();
    
    const int size = 128;
    const float isolevel = 0.0f;
    
    const float frequency = 0.15f;
    const float lacunarity = 1.6f;
    const float persistence = 0.6f;
    const float heightScale = 150.0f;
    const float caveFreq = 7.0f;
    
    srand(static_cast<unsigned int>(std::time(nullptr)));
    float seed = (float)(rand()%1000000)*2.24821838f;

    terrain.density = std::vector<std::vector<std::vector<float>>>(128, std::vector<std::vector<float>>(128, std::vector<float>(128)));

    for (int x = 0; x < size; x++) {
        for (int y = 0; y < 128; y++) {
            for (int z = 0; z < size; z++) {

                float xi = (float)(x + seed) * frequency / (float)size;
                float yi = (float)y * frequency / (float)size;
                float zi = (float)(z + seed) * frequency / (float)size;

                float mountainNoise = noiseLayer(xi, zi, lacunarity, persistence, 20, 6);
                float baseHeight = pow(mountainNoise, 1.0f) * heightScale;

                float caveNoise = noiseLayer(xi * caveFreq, yi * caveFreq, lacunarity, persistence, 20, zi * caveFreq);
                
                float terrainSurface = (float)y - baseHeight;
                float density = terrainSurface + caveNoise * 12.0f;

                terrain.density[x][y][z] = density;
            }
        }
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
        for (int y = 0; y < 128 - 1; y++) {
            for (int z = 0; z < size - 1; z++) {
                float cubeValues[8];
                glm::vec3 cubePositions[8];

                for (int i = 0; i < 8; ++i) {
                    glm::vec3 pos = glm::vec3(x, y, z) + vertexOffsets[i];
                    cubePositions[i] = pos;
                    cubeValues[i] = terrain.density[(int)pos.x][(int)pos.y][(int)pos.z];
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
                    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
                    
                    terrain.vertices.push_back({v0 * scale, normal, glm::vec2(0.0f)});
                    terrain.vertices.push_back({v1 * scale, normal, glm::vec2(0.0f)});
                    terrain.vertices.push_back({v2 * scale, normal, glm::vec2(0.0f)});
                }
            }
        }
    }
    
    terrain.scale = glm::vec3(1.0f);
    terrain.rotation = glm::vec3(0.0f);
    terrain.position = glm::vec3(-size/2, -10.0f, -size/2);
    
    glGenVertexArrays(1, &terrain.vertexArrayObject);
    glBindVertexArray(terrain.vertexArrayObject);
    
    glGenBuffers(1, &terrain.vertexBufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, terrain.vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, terrain.vertices.size() * sizeof(Vertex), terrain.vertices.data(), GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, vertex));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

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
