#version 450
// curve.vert — shared vertex shader for all line geometry

layout(location = 0) in vec4 in_position;

layout(push_constant) uniform PushConstants {
    mat4  mvp;
    int   mode;       // 0 = use per-curve colour, 1 = axes grey, 2 = epsilon ball
    float colour[3];  // RGB for mode 0
} pc;

layout(location = 0) out vec3 frag_color;

void main() {
    gl_Position = pc.mvp * in_position;

    if (pc.mode == 1) {
        // Axes: soft warm white
        frag_color = vec3(0.75, 0.75, 0.72);
    } else if (pc.mode == 2) {
        // Epsilon ball: bright white
        frag_color = vec3(1.0, 1.0, 1.0);
    } else {
        // Curve: use per-entry colour from push constants
        frag_color = vec3(pc.colour[0], pc.colour[1], pc.colour[2]);
    }
}
