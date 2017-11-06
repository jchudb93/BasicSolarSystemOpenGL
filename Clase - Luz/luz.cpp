
#include <iostream>
#include <cstdlib>
#include <vector>
#include <cstdio>
#include <GL/glew.h>
#include <GL/freeglut.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader_utils.h"

GLuint program;
GLint attribute_coord;
GLint attribute_normal;
GLint uniform_m;
GLint uniform_v;
GLint uniform_p;
GLint uniform_m_3x3_inv_transp, uniform_v_inv;

GLint uniform_mat_ambient;
GLint uniform_mat_light_position;
GLint uniform_mat_diffuse;
GLint uniform_mat_specular;
GLint uniform_mat_s;
GLfloat uniform_float_alfa;
//Variables para el movimiento
GLfloat alfa=0.0, beta=0.0, theta=0.0, phi=0.0;

GLfloat t = 1.0f*glutGet(GLUT_ELAPSED_TIME);


int screen_width = 800, screen_height = 800;

typedef struct Vertex{
    float x, y, z;
    float nx, ny, nz;
    int numTA;

}Vertex;

typedef struct Triangle{
    unsigned int indices[3];
    float nx, ny, nz;
}Triangle;

typedef struct Mesh{
    //Informacion de estructura
    int numVertices;
    int numTriangles;
    Vertex* vertices;
    Triangle* triangles;

    //Información para transformación inicial
    Vertex center;
    float scale;

    //Matriz de transformación
    glm::mat4 model_transform;

    //Buffers para graficado
    GLfloat* object_vertices;
    GLfloat* object_normal;
    GLushort* object_indexes;

    //Id's para buffers
    GLuint vbo_object;
    GLuint vbo_normal;
    GLuint ibo_object;

    //Info de material
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    glm::vec4 light_position;
    GLfloat* float_alfa;
    float s;

}Mesh;

typedef struct Scene{
    int numMeshes;
    Mesh* meshes[5];
}Scene;


Scene scene;
//Mesh* mainMesh;
int numEdges;

Mesh* leerOFF(const char* filename){
    FILE* fid = fopen(filename, "rt");

    //Leer formato
    char buffer[1024];
    fscanf(fid, "%s", buffer);

    if(strcmp(buffer, "OFF")!=0){
        printf("Error de formato\n");
        exit(EXIT_FAILURE);
    }

    int nverts, ntriang, nedges;
    fscanf(fid, "%d %d %d", &nverts, &ntriang, &nedges);
    printf("%d, %d, %d\n", nverts, ntriang, nedges);

    Mesh* mesh = new Mesh;
    mesh->numVertices = nverts;
    mesh->numTriangles = ntriang;

    mesh->vertices = new Vertex[nverts];
    mesh->triangles = new Triangle[ntriang];
    mesh->center.x = 0.0;
    mesh->center.y = 0.0;
    mesh->center.z = 0.0;

    int i;
    for(i = 0; i < nverts; i++){
        fscanf(fid, "%f %f %f", &mesh->vertices[i].x,
               &mesh->vertices[i].y,
               &mesh->vertices[i].z);

        mesh->center.x += mesh->vertices[i].x;
        mesh->center.y += mesh->vertices[i].y;
        mesh->center.z += mesh->vertices[i].z;
        mesh->vertices[i].nx = 0.0;
        mesh->vertices[i].ny = 0.0;
        mesh->vertices[i].nz = 0.0;
        mesh->vertices[i].numTA = 0;
    }

    for(i = 0; i < ntriang; i++){
        int nv;
        fscanf(fid, "%d %d %d %d", &nv, &mesh->triangles[i].indices[0],
                                        &mesh->triangles[i].indices[1],
                                        &mesh->triangles[i].indices[2]);
    }

    fclose(fid);
    mesh->center.x /= nverts;
    mesh->center.y /= nverts;
    mesh->center.z /= nverts;

    float maxx = -1.0e-10, maxy= -1.0e-10, maxz= -1.0e-10;
    float minx = 1.0e10, miny= 1.0e10, minz= 1.0e10;

    for(int i = 0; i < mesh->numVertices; i++){
        if(mesh->vertices[i].x < minx)
            minx = mesh->vertices[i].x;
        if(mesh->vertices[i].x > maxx)
            maxx = mesh->vertices[i].x;
        if(mesh->vertices[i].y < miny)
            miny = mesh->vertices[i].y;
        if(mesh->vertices[i].y > maxy)
            maxy = mesh->vertices[i].y;
        if(mesh->vertices[i].z < minz)
            minz = mesh->vertices[i].z;
        if(mesh->vertices[i].z > maxz)
            maxz = mesh->vertices[i].z;
    }

    unsigned int p0, p1, p2;
    //Computar normales de los triángulos
    for(int i = 0; i < mesh->numTriangles; i++){
        p0 = mesh->triangles[i].indices[0];
        p1 = mesh->triangles[i].indices[1];
        p2 = mesh->triangles[i].indices[2];

        glm::vec3 A = glm::vec3(mesh->vertices[p1].x - mesh->vertices[p0].x,
                                mesh->vertices[p1].y - mesh->vertices[p0].y,
                                mesh->vertices[p1].z - mesh->vertices[p0].z);
        glm::vec3 B = glm::vec3(mesh->vertices[p2].x - mesh->vertices[p0].x,
                                mesh->vertices[p2].y - mesh->vertices[p0].y,
                                mesh->vertices[p2].z - mesh->vertices[p0].z);

        glm::vec3 C = glm::normalize(glm::cross(A, B));
        mesh->vertices[p0].nx += C.x;
        mesh->vertices[p0].ny += C.y;
        mesh->vertices[p0].nz += C.z;   mesh->vertices[p0].numTA++;

        mesh->vertices[p1].nx += C.x;
        mesh->vertices[p1].ny += C.y;
        mesh->vertices[p1].nz += C.z;   mesh->vertices[p1].numTA++;

        mesh->vertices[p2].nx += C.x;
        mesh->vertices[p2].ny += C.y;
        mesh->vertices[p2].nz += C.z;   mesh->vertices[p2].numTA++;
    }

    for(int i = 0; i < mesh->numVertices; i++){
        mesh->vertices[i].nx /= mesh->vertices[i].numTA;
        mesh->vertices[i].ny /= mesh->vertices[i].numTA;
        mesh->vertices[i].nz /= mesh->vertices[i].numTA;

        glm::vec3 v = glm::vec3(mesh->vertices[i].nx,
                                mesh->vertices[i].ny,
                                mesh->vertices[i].nz);
        v = normalize(v);
        mesh->vertices[i].nx = v.x;
        mesh->vertices[i].ny = v.y;
        mesh->vertices[i].nz = v.z;
    }

    float diag = sqrt((maxx-minx)*(maxx-minx) +
                      (maxy-miny)*(maxy-miny)+
                      (maxz-minz)*(maxz-minz));
    mesh->scale = 2.0/diag;

    mesh->model_transform = glm::mat4(1.0f);
    return mesh;
}

void init_buffers(Mesh* mesh){


    mesh->object_vertices = new GLfloat[mesh->numVertices * 3];
    mesh->object_normal = new GLfloat[mesh->numVertices * 3];
    mesh->object_indexes = new GLushort[mesh->numTriangles * 3];

    int i;

    for(i = 0; i < mesh->numVertices; i++){
        mesh->object_vertices[3 * i] = mesh->vertices[i].x;
        mesh->object_vertices[3 * i + 1] = mesh->vertices[i].y;
        mesh->object_vertices[3 * i + 2] = mesh->vertices[i].z;

        mesh->object_normal[3 * i] = mesh->vertices[i].nx;
        mesh->object_normal[3 * i + 1] = mesh->vertices[i].ny;
        mesh->object_normal[3 * i + 2] = mesh->vertices[i].nz;
    }

    for(i = 0; i < mesh->numTriangles; i++){
        mesh->object_indexes[3 * i] = mesh->triangles[i].indices[0];
        mesh->object_indexes[3 * i + 1] = mesh->triangles[i].indices[1];
        mesh->object_indexes[3 * i + 2] = mesh->triangles[i].indices[2];
    }

    glGenBuffers(1, &mesh->vbo_object);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_object);
    glBufferData(GL_ARRAY_BUFFER, mesh->numVertices * 3 * sizeof(GLfloat),
                 mesh->object_vertices, GL_STATIC_DRAW);


    glGenBuffers(1, &mesh->vbo_normal);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_normal);
    glBufferData(GL_ARRAY_BUFFER, mesh->numVertices * 3 * sizeof(GLfloat),
                 mesh->object_normal, GL_STATIC_DRAW);

    glGenBuffers(1, &mesh->ibo_object);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo_object);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->numTriangles * 3 * sizeof(GLushort),
                 mesh->object_indexes, GL_STATIC_DRAW);
}



void animacion(){

    GLfloat t1 = 1.0f * glutGet(GLUT_ELAPSED_TIME);
    GLfloat delta = t1-t;

    t = t1;

    scene.meshes[0]->float_alfa=alfa;


}

bool init_resources(){

    scene.meshes[0] = leerOFF("NR34.off");
    scene.meshes[1] = leerOFF("NR0.off");
    scene.numMeshes = 2;

    scene.meshes[0]->model_transform = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)) *
                                        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f),
                                                    glm::vec3(1.0f, 0.0f, 0.0f));

    scene.meshes[1]->model_transform = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
                                        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f),
                                                    glm::vec3(1.0f, 0.0f, 0.0f));

    scene.meshes[0]->ambient = glm::vec4(0.2,0.2,0.2,1.0);
    scene.meshes[0]->diffuse = glm::vec4(1.0, 0.0, 0.0, 1.0);
    scene.meshes[0]->specular = glm::vec4(1.0,1.0,1.0,1.0);
    scene.meshes[0]->s = 100.0;

    scene.meshes[1]->ambient = glm::vec4(0.2,0.2,0.2,1.0);
    scene.meshes[1]->diffuse = glm::vec4(0.0, 1.0, 0.0, 1.0);
    scene.meshes[1]->specular = glm::vec4(1.0,1.0,1.0,1.0);
    scene.meshes[1]->s = 100.0;

    init_buffers(scene.meshes[0]);
    init_buffers(scene.meshes[1]);

    GLint link_ok = GL_FALSE;
    GLuint vs, fs;
    if((vs = create_shader("phong3.v.glsl", GL_VERTEX_SHADER))==0) return false;
    if((fs = create_shader("phong3.f.glsl", GL_FRAGMENT_SHADER))==0) return false;

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
    if(!link_ok){
        std::cout << "Problemas con el Shader" << std::endl;
        return false;
    }

    attribute_coord = glGetAttribLocation(program, "coord3d");
    if(attribute_coord == -1){
        std::cout << "No se puede asociar el atributo coord" << std::endl;
        return false;
    }

    attribute_normal = glGetAttribLocation(program, "normal");
    if(attribute_normal == -1){
            std::cout << "No se puede asociar el atributo normal" << std::endl;
            return false;
    }

    uniform_m = glGetUniformLocation(program, "m");
    if(uniform_m == -1){
        std::cout << "No se puede asociar el uniform m" << std::endl;
        return false;
    }

    uniform_v = glGetUniformLocation(program, "v");
    if(uniform_v == -1){
        std::cout << "No se puede asociar el uniform v" << std::endl;
        return false;
    }
    uniform_p = glGetUniformLocation(program, "p");
    if(uniform_p == -1){
        std::cout << "No se puede asociar el uniform p" << std::endl;
        return false;
    }

    uniform_m_3x3_inv_transp = glGetUniformLocation(program, "m_3x3_inv_transp");
    if(uniform_m_3x3_inv_transp == -1){
        std::cout << "No se puede asociar el uniform m_3x3_inv_transp" << std::endl;
        return false;
    }

    uniform_v_inv = glGetUniformLocation(program, "v_inv");
    if(uniform_v_inv == -1){
        std::cout << "No se puede asociar el uniform v_inv" << std::endl;
        return false;
    }

    uniform_mat_ambient = glGetUniformLocation(program, "mat_ambient");
    if(uniform_mat_ambient == -1){
        std::cout << "No se puede asociar el uniform mat_ambient" << std::endl;
        return false;
    }

    uniform_mat_diffuse = glGetUniformLocation(program, "mat_diffuse");
    if(uniform_mat_diffuse == -1){
        std::cout << "No se puede asociar el uniform mat_diffuse" << std::endl;
        return false;
    }

    uniform_mat_specular = glGetUniformLocation(program, "mat_specular");
    if(uniform_mat_specular == -1){
        std::cout << "No se puede asociar el uniform mat_specular" << std::endl;
        return false;
    }

    uniform_mat_s = glGetUniformLocation(program, "mat_s");
    if(uniform_mat_s == -1){
        std::cout << "No se puede asociar el uniform mat_s" << std::endl;
        return false;
    }
    return true;
}

void graficarObjeto(Mesh* mesh){
        //Creamos matrices de modelo, vista y proyeccion
    glm::mat4 model =   mesh->model_transform *
                        glm::scale(glm::mat4(1.0f), glm::vec3(mesh->scale,
                                                              mesh->scale,
                                                              mesh->scale)) *
                        glm::translate(glm::mat4(1.0f), glm::vec3(-mesh->center.x,
                                                                  -mesh->center.y,
                                                                  -mesh->center.z));

    glm::mat4 view  = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
                                  glm::vec3(0.0f, 0.0f, 0.0f),
                                  glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 projection = glm::perspective(45.0f, 1.0f*screen_width/screen_height,
                                            0.1f, 100.0f);

    glm::mat3 m_3x3_inv_transp = glm::transpose(glm::inverse(glm::mat3(mesh->model_transform)));
    glm::mat4 v_inv = glm::inverse(view);

    glUseProgram(program);

    //Enviamos la matriz que debe ser usada para cada vertice
    glUniformMatrix4fv(uniform_p, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(uniform_v, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(uniform_m,1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(uniform_v_inv, 1, GL_FALSE, glm::value_ptr(v_inv));
    glUniformMatrix3fv(uniform_m_3x3_inv_transp, 1, GL_FALSE, glm::value_ptr(m_3x3_inv_transp));

    glUniform4fv(uniform_mat_ambient, 1,glm::value_ptr(mesh->ambient));
    glUniform4fv(uniform_mat_diffuse, 1, glm::value_ptr(mesh->diffuse));
    glUniform4fv(uniform_mat_specular,1,  glm::value_ptr(mesh->specular));
    glUniform4fv(uniform_mat_light_position,1,glm::value_ptr(mesh->model_transform));
    glUniform4fv(uniform_mat_float_alfa,1,glm::value_ptr(mesh->float_alfa));
    glUniform1f(uniform_mat_s, mesh->s);

    glEnableVertexAttribArray(attribute_coord);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_object);

    glVertexAttribPointer(
        attribute_coord,
        3,
        GL_FLOAT,
        GL_FALSE,
        0, 0
    );

    glEnableVertexAttribArray(attribute_normal);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_normal);

    glVertexAttribPointer(
        attribute_normal,
        3,
        GL_FLOAT,
        GL_FALSE,
        0,0
    );


    //Dibujar las primitivas
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo_object);
    int size;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

    //Dibujar los triánglos
    glDrawElements(GL_TRIANGLES, size/sizeof(GLushort), GL_UNSIGNED_SHORT, 0);

    glDisableVertexAttribArray(attribute_coord);
    glDisableVertexAttribArray(attribute_normal);
}

void onDisplay(){

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for(int i = 0; i < scene.numMeshes; i++)
        graficarObjeto(scene.meshes[i]);

    glutSwapBuffers();
}

void onReshape(int w, int h){
    screen_width = w;
    screen_height = h;

    glViewport(0,0,screen_width, screen_height);
}

void free_resources(){
    glDeleteProgram(program);

    for(int i = 0; i < scene.numMeshes; i++){
        glDeleteBuffers(1, &scene.meshes[i]->vbo_object);
        glDeleteBuffers(1, &scene.meshes[i]->ibo_object);
        glDeleteBuffers(1, &scene.meshes[i]->vbo_normal);
        delete[] scene.meshes[i]->object_vertices;
        delete[] scene.meshes[i]->object_indexes;
        delete[] scene.meshes[i]->object_normal;
        delete[] scene.meshes[i]->vertices;
        delete[] scene.meshes[i]->triangles;
        delete scene.meshes[i];
    }
}

int main(int argc, char* argv[]){
    glutInit(&argc, argv);
    glutInitContextVersion(2,0);
    glutInitDisplayMode(GLUT_RGBA | GLUT_ALPHA | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(screen_width, screen_height);
    glutCreateWindow("OpenGL");

    GLenum glew_status = glewInit();
    if(glew_status != GLEW_OK){
        std::cout << "Error inicializando GLEW" << std::endl;
        exit(EXIT_FAILURE);
    }

    if(!GLEW_VERSION_2_0){
        std::cout << "Tu tarjeta grafica no soporta OpenGL 2.0" << std::endl;
        exit(EXIT_FAILURE);
    }

    if(init_resources()){
        glutDisplayFunc(onDisplay);
        glutReshapeFunc(onReshape);
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glutIdleFunc(animacion);
        glutMainLoop();
    }

    free_resources();
    exit(EXIT_SUCCESS);
}
