#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;

layout(push_constant) uniform PushConstants {
    mat4  mvp;
    int   mode;
    float color[3];
} pc;

layout(location = 0) out vec4 frag_color; // Changed to vec4 to match input requirements

void main() {
    // For now, let's use a raw position to guarantee visibility
    // If this works, change it back to: pc.mvp * vec4(in_position, 1.0);
    gl_Position = vec4(in_position, 1.0); 

    if (pc.mode == 0) {
        frag_color = in_color;
    } else {
        frag_color = vec4(pc.color[0], pc.color[1], pc.color[2], 1.0);
    }
}