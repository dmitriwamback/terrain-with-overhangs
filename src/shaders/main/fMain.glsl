//
//  fMain.glsl
//  Marching Cube Terrain
//
//  Created by Dmitri Wamback on 2025-06-19.
//
#version 410 core

in prop {
    vec3 normal;
    vec3 fragp;
} fs_in;

out vec4 fragc;
vec3 lightPosition = vec3(-100000.0, 100000.0, -100000.0);

void main() {
    vec3 up = vec3(0.0, 1.0, 0.0);
    float slope = dot(normalize(fs_in.normal), up);
    slope = clamp(slope, 0.0, 1.0);

    vec3 grass = vec3(0.13, 0.4, 0.1);
    vec3 dirt  = vec3(0.35, 0.25, 0.15);

    vec3 color = mix(dirt, grass, slope);

    vec3 ambient = color * 0.4;

    vec3 lightDirection = normalize(lightPosition - fs_in.fragp);
    float diff = max(dot(fs_in.normal, lightDirection), 0.4);
    vec3 diffuse = diff * vec3(1.0);

    fragc = vec4(color * (ambient + diffuse), 1.0);
}
