#version 450

layout(location = 0) in vec3 vert_world_norm_out;
layout(location = 1) in vec2 vert_uv_out;
layout(location = 2) in vec3 vert_world_pos_out;
layout(location = 3) in vec3 camera_pos_out;

layout(binding = 0) uniform sampler2D trup_tex;
layout(binding = 1) uniform sampler2D smile_tex;

layout(location = 0) out vec4 frag_color;

void main()
{
    vec3 light_pos_out = vec3(1.f, 0.f, 0.f);


    vec3 normal = normalize(vert_world_norm_out);

    int pixel_x = int(gl_FragCoord.x);

    //Ambient
    vec4 trup_color = pixel_x % 2 == 0 ? texture(trup_tex, vert_uv_out) : texture(smile_tex, vert_uv_out);
    float ambient_intensity = 0.1f;
    vec4 ambient = ambient_intensity * trup_color;

    //Diffuse
    vec3 dir_to_light = normalize(light_pos_out - vert_world_pos_out);
    float cosLN = dot(dir_to_light, normal);
    float lambert = clamp(cosLN, 0.f, 1.f);
    vec4 diffuse = lambert * trup_color;

    //Specular
    vec3 dir_to_eye = normalize(camera_pos_out - vert_world_pos_out);

    vec3 dir_light_to_point = dir_to_light * -1.f;
    vec3 light_refl = reflect(dir_light_to_point, normal);

    float cosER = dot(dir_to_eye, light_refl);
    float specular_value = clamp(cosER, 0.f, 1.f);
    vec4 specular_tint = vec4(1.f, 1.f, 1.f, 1.f);
    vec4 specular = specular_tint * pow(specular_value, 50.f);


    frag_color = ambient + diffuse + specular;
}
