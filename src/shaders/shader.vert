#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) in vec3 positionVertex;
layout(location = 1) in vec3 colorVertex;
layout(location = 0) out vec3 colorFrag;

void main() {
    gl_Position = vec4(positionVertex, 1.0);
    colorFrag = colorVertex;
}
