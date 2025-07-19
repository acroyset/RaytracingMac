#version 330 core
layout(location = 0) in vec2 aPos;

out vec2 fragCoord; // Pass to fragment shader

void main() {
    const vec2 verts[3] = vec2[3](
    vec2(-1.0, -1.0),
    vec2(3.0, -1.0),
    vec2(-1.0, 3.0)
    );

    vec2 pos = verts[gl_VertexID];
    fragCoord = (pos + 1.0) / 2.0; // Normalize to [0, 1] range
    gl_Position = vec4(pos, 0.0, 1.0); // Required output
}
