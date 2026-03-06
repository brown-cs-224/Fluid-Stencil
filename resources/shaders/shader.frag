#version 330 core
out vec4 fragColor;

// Additional information for lighting
in vec4 color;
in vec4 position_worldSpace;

uniform int cube = 0;
uniform float red = 1.0;
uniform float green = 1.0;
uniform float blue = 1.0;
uniform float alpha = 1.0;

void main() {
    if (cube == 1) {
        fragColor = vec4(1.0, 1.0, 1.0, 1);
        return;
    }


    vec3 c = vec3(color[0], color[1], color[2]);

    //REMOVE LATER
    c = c * 0.7 + vec3(0.3); // 70% original color + 30% white

    fragColor = vec4(c, 1.0);
}
