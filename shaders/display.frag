#version 330 core
out vec4 FragColor;
in vec2 fragCoord;
uniform sampler2D screenTex;

void main() {
    FragColor = texture(screenTex, fragCoord);
}
