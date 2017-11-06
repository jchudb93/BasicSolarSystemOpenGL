attribute vec3 coord3d;
attribute vec3 normal;
uniform mat4 m, p, v;
uniform mat3 m_3x3_inv_transp;
uniform mat4 v_inv;

varying vec4 color;

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

    vec3 N = normalize(m_3x3_inv_transp * normal);
    vec3 totalLight = vec3(scene_ambient) * vec3(mymaterial.ambient);
    vec3 V = normalize(vec3(v_inv*vec4(0.0,0.0,0.0,1.0) -
                       m*vec4(coord3d, 1.0)));

    for(int i = 0; i < numLights; i++){
        vec3 L = vec3(lights[i].position - m*vec4(coord3d, 1.0));
        float distance = length(L);

        L = normalize(L);
        float attenuation = 1.0/(lights[i].constantAttenuation +
                             distance*lights[i].linearAttenuation+
                             distance*distance*lights[i].quadraticAttenuation);

        vec3 diffuse_reflection = attenuation*vec3(lights[i].diffuse)*
                                vec3(mymaterial.diffuse)*
                                max(0.0, dot(N,L));

        vec3 specular_reflection;
        if(dot(N,L) < 0.0){
            specular_reflection = vec3(0.0, 0.0, 0.0);
        }else{
            specular_reflection = attenuation * vec3(lights[i].specular)*
                        vec3(mymaterial.specular)*
                        pow(max(0.0, dot(reflect(-L, N),V)), mymaterial.s);
        }

        totalLight = totalLight + diffuse_reflection + specular_reflection;
    }

    color = vec4(totalLight, 1.0);

    mat4 mvp = p * v * m;
    gl_Position = mvp * vec4(coord3d, 1);
}
