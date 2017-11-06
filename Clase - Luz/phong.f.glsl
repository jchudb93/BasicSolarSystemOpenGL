varying vec4 position;
varying vec3 new_normal;

uniform mat4 m, v, p;
uniform mat4 v_inv;

struct LightSource{
    vec4 position;
    vec4 diffuse;
    vec4 specular;
    float constantAttenuation, linearAttenuation, quadraticAttenuation;
};

const int numLights = 2;
LightSource lights[numLights];

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

vec4 scene_ambient = vec4(0.2, 0.2, 0.2, 1.0);

struct Material{
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shineness;
};

Material mymaterial = Material(
    vec4(1.0, 1.0, 1.0, 1.0),
    vec4(1.0, 1.0, 0.0, 1.0),
    vec4(1.0, 1.0, 1.0, 1.0),
    100.0
);

void main(void){
    lights[0] = light0;
    lights[1] = light1;

    vec3 V = normalize(vec3(v_inv * vec4(0.0, 0.0, 0.0, 1.0)- position));
    vec3 totalLighting = vec3(scene_ambient) * vec3(mymaterial.ambient);

    for(int index = 0; index < numLights; index++){
        vec3 L = vec3(lights[index].position - position);
        float distance = length(L);
        L = normalize(L);
        float attenuation = 1.0/(lights[index].constantAttenuation +
                                 lights[index].linearAttenuation * distance +
                                 lights[index].quadraticAttenuation * distance *distance);

        vec3 diffuseReflection = attenuation*vec3(lights[index].diffuse)*vec3(mymaterial.diffuse)*
                                max(0.0, dot(new_normal, L));
        vec3 specularReflection;

        if(dot(new_normal, L)<0.0){
            specularReflection = vec3(0.0,0.0,0.0);
        }else{
            specularReflection = attenuation * vec3(lights[index].specular)*vec3(mymaterial.specular)*
                                pow(max(0.0, dot(reflect(-L, new_normal), V)), mymaterial.shineness);
        }

        totalLighting = totalLighting + diffuseReflection + specularReflection;

    }

    gl_FragColor = vec4(totalLighting, 1.0);
}
