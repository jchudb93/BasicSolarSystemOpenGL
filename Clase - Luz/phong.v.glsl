attribute vec3 coord3d;
attribute vec3 normal;

varying vec4 position; //Posicion del objeto transformado
varying vec3 new_normal; //Normal transformada

uniform mat4 m,v,p;
uniform mat3 m_3x3_inv_transp;

void main(void){
    position = m * vec4(coord3d, 1.0);
    new_normal = normalize(m_3x3_inv_transp * normal);
    mat4 mvp = p*v*m;
    gl_Position = mvp * vec4(coord3d, 1.0);
}
