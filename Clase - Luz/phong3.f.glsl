#version 450

in vec4 position;
in vec3 new_normal;

uniform mat4 m,v,p;
uniform mat4 v_inv;

uniform vec4 mat_ambient;
uniform vec4 mat_diffuse;
uniform vec4 mat_specular;
uniform float mat_s;

out vec4 FragColor;

struct LightSource{
    vec4 position;
    vec4 diffuse;
    vec4 specular;
    float constantAttenuation, linearAttenuation, quadraticAttenuation;
};

LightSource light0 = LightSource(
     vec4(0.0, 1.0, 2.0, 1.0),
     vec4(1.0, 1.0, 1.0, 1.0),
     vec4(1.0, 1.0, 1.0, 1.0),
     0.0, 1.0, 0.0
);

LightSource light1 = LightSource(
    vec4(2.0, 0.0, 0.0, 1.0),
    vec4(1.0, 1.0, 1.0, 1.0),
    vec4(1.0, 1.0, 1.0, 1.0),
    0.0, 1.0, 0.0
);

const int numLights = 2;
LightSource lights[numLights];

vec4 scene_ambient = vec4(0.2, 0.2, 0.2, 1.0);

struct Material{
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float s;
};

Material mymaterial = Material(
    vec4(1.0, 1.0, 1.0, 1.0),
    vec4(1.0, 0.0, 1.0, 1.0),
    vec4(1.0, 1.0, 1.0, 1.0),
    100
);

void main(void){
    lights[0] = light0;
    lights[1] = light1;

    vec3 N = new_normal;
    vec3 totalLight = vec3(scene_ambient) * vec3(mat_ambient);
    vec3 V = normalize(vec3(v_inv*vec4(0.0,0.0,0.0,1.0) -
                       position));

    for(int i = 0; i < numLights; i++){
        vec3 L = vec3(lights[i].position - position);
        float distance = length(L);

        L = normalize(L);
        float attenuation = 1.0/(lights[i].constantAttenuation +
                             distance*lights[i].linearAttenuation+
                             distance*distance*lights[i].quadraticAttenuation);

        vec3 diffuse_reflection = attenuation*vec3(lights[i].diffuse)*
                                vec3(mat_diffuse)*
                                max(0.0, dot(N,L));

        vec3 specular_reflection;
        if(dot(N,L) < 0.0){
            specular_reflection = vec3(0.0, 0.0, 0.0);
        }else{
            specular_reflection = attenuation * vec3(lights[i].specular)*
                        vec3(mat_specular)*
                        pow(max(0.0, dot(reflect(-L, N),V)), mat_s);
        }

        totalLight = totalLight + diffuse_reflection + specular_reflection;
    }

    FragColor = vec4(totalLight, 1.0);
}
