#version 450

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 Mvp;
    mat4 Model;
} PC;

layout(location = 0) out vec3 FragNormal;
layout(location = 1) out vec2 FragTexCoord;
layout(location = 2) out vec3 vert_world_pos_out;
layout(location = 3) out vec3 camera_pos_out;

void main()
{
    gl_Position = PC.Mvp * vec4(InPosition, 1.0);
    FragNormal = mat3(transpose(inverse(PC.Model))) * InNormal;
    FragTexCoord = InTexCoord;
    vert_world_pos_out = (PC.Model * vec4(InPosition, 1.0)).xyz;
    camera_pos_out = vec3(0.f, 0.f, -1.f);
}

