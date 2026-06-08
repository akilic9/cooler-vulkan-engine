#version 460

layout(location = 0) out vec3 fragColour;

const vec2 vertices[3] = vec2[](vec2(0.0, -0.5),
                                vec2(0.5,  0.5),
                                vec2(-0.5, 0.5));

const vec3 colours[3] = vec3[](vec3(1.0, 0.0, 0.0),
                               vec3(0.0, 1.0, 0.0),
                               vec3(0.0, 0.0, 1.0));

void main()
{
    gl_Position = vec4(vertices[gl_VertexIndex], 0.0, 1.0);
    fragColour = colours[gl_VertexIndex];
}