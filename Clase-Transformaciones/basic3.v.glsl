#version 450

in vec3 coord3d;
uniform mat4 mvp;
uniform mat4 model;

out vec3 f_color;

void main(void){
    gl_Position = mvp * model * vec4(coord3d, 1);
    f_color = 	vec3((coord3d.x+1)/2, (coord3d.y+1)/2, (coord3d.z+1)/2);
}
