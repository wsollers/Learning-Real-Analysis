#version 450

layout(location = 0) in vec4 frag_color; // Must match vertex output exactly
layout(location = 0) out vec4 out_color; // Final screen color must be location 0

void main() {
    out_color = frag_color;
}