#version 450

in vec3 coord3d;
in vec3 normal;

uniform mat4 m, v, p;
uniform mat3 m_3x3_inv_transp;

out vec4 position;
out vec3 new_normal;

void main(void){
    position = m * vec4(coord3d, 1.0);
    new_normal = normalize(m_3x3_inv_transp * normal);

    mat4 mvp = p * v * m;
    gl_Position = mvp * vec4(coord3d, 1.0);
}
