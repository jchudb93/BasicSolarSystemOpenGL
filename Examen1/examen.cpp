#include <iostream>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <iomanip>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader_utils.h"

#include <fstream>


GLuint vbo_surface;
GLuint vbo_color;
GLint uniform_mvp;

GLuint program;

GLint attribute_coord3d;
GLint attribute_color;

GLfloat* surface_vertices;
GLfloat* surface_color;


int numPoints;
int screen_width = 800, screen_height = 800;

int nSteps = 100;


//vector de la superficie
std::vector<GLfloat> vsuperficie;

//factorial precalculado
int factorial[4] = {1, 1, 2, 6};


float binomial(int n, int i){
    return 1.0 * (factorial[n] / (factorial[i] * factorial[n-i]));
}


//genero el polinomio de bernstein
float berstein(int n, int i, float t){
    return binomial(n, i) * powf(t, i) * powf(1-t, n-i);
}

//superficie de bezier
void superficie(int ind, GLfloat ptsCtrl[][4][4][3]){
    float delta = 1.0/nSteps;
    float u = 0.0;
    float v;
    
    for(int i = 0; i <= (nSteps + 1); i++){
        v = 0.0;
        for(int j = 0; j <= (nSteps + 1); j++){
            float x = 0.0;
            float y = 0.0;
            float z = 0.0;
            for(int k = 0; k < 4; k++){
                float bk = berstein(3, k, u);
                for(int l = 0; l < 4; l++){     
                    float bl = berstein(3, l, v);
                    x += bk * bl * ptsCtrl[ind][k][l][0];
                    y += bk * bl * ptsCtrl[ind][k][l][1];
                    z += bk * bl * ptsCtrl[ind][k][l][2];
                }
            }
            
            vsuperficie.push_back(x);
            vsuperficie.push_back(y);
            vsuperficie.push_back(z);
            
            v += delta;
        }
        u += delta;
    }
}

/*
void leerPtosControl(std::ifstream control, GLfloat** aux,int npuntosCtrl){
    for (int i=0; i<npuntosCtrl; i++){
        for (int j = 0; j < 3; j++){
            control >> aux[i][j];
        }   
    }   
}
*/

bool init_resources(){

    /*EXAMEN: Asignar memoria y valores a surface_vertices y surface_color*/


    int npuntosCtrl;

    int numParches;

    std::ifstream control;//archivo de control


    control.open("control.txt"); //abro el archivo
    
    control >> npuntosCtrl;//269
    control >> numParches;//28


    std::cout<<"Puntos de control "<<npuntosCtrl<<std::endl;
    std::cout<<"Parches "<<numParches<<std::endl;

    //puntos de control
    GLfloat aux[npuntosCtrl][3];


    int auxParches[numParches][16];

    
    //leo los puntos de control
    //leerPtosControl(control,aux,npuntosCtrl); ERROR
    
    for (int i=0; i<npuntosCtrl; i++){
        for (int j = 0; j < 3; j++){
            control >> aux[i][j];
        }   
    }   

    //leo los parches 
    for (int i=0; i<numParches; i++){
        for(int j=0; j<16; j++){
            control >> auxParches[i][j];
        }   
    }
    

    //arreglo para ubicar los puntos de control
    GLfloat ptsCtrl[numParches][4][4][3];  


    //ubico los puntos de control segun su posicion en la matriz 4x4
    int m;
    for (int i = 0; i<numParches; i++){
        m = 0;
        for (int j=0; j<4; j++){
            for (int k=0; k<4; k++){
                for (int l =0; l<3; l++){
                    ptsCtrl[i][j][k][l] = aux[auxParches[i][m]][l];
                }               
                m++;
            }   
        }   
    }

    //agrego a la superficie los puntos de control
    for (int i=0; i<numParches; i++){ 
        superficie(i,ptsCtrl);   
    }   
    
    numPoints = vsuperficie.size() / 3;
    surface_vertices = new GLfloat[vsuperficie.size()];
    surface_color = new GLfloat[vsuperficie.size()];
    
    for(int i = 0; i < vsuperficie.size(); i++){
        surface_vertices[i] = vsuperficie[i];
    }

    //genero el color de la superficie 
    
    for(int i = 0; i < vsuperficie.size()/3; i++){
        surface_color[3 * i] = 0.0;
        surface_color[3 * i + 1] = 1.0;
        surface_color[3 * i + 2] = 0.0;
    }
    

    glGenBuffers(1, &vbo_surface);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_surface);
    glBufferData(GL_ARRAY_BUFFER, /*EXAMEN: Tamaño de buffer*/vsuperficie.size()*sizeof(GLfloat), surface_vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &vbo_color);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
    glBufferData(GL_ARRAY_BUFFER, /*EXAMEN: Tamaño de buffer*/vsuperficie.size()*sizeof(GLfloat), surface_color, GL_STATIC_DRAW);

    GLint link_ok = GL_FALSE;
    GLuint vs, fs;
    if((vs = create_shader("basic3.v.glsl", GL_VERTEX_SHADER))==0) return false;
    if((fs = create_shader("basic3.f.glsl", GL_FRAGMENT_SHADER))==0) return false;

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
    if(!link_ok){
        std::cout << "Problemas con el Shader" << std::endl;
        return false;
    }

    attribute_coord3d = glGetAttribLocation(program, "coord3d");
    if(attribute_coord3d == -1){
        std::cout << "No se puede asociar el atributo coord3d" << std::endl;
        return false;
    }

    attribute_color = glGetAttribLocation(program, "color");
    if(attribute_color == -1){
        std::cout << "No se puede asociar el atributo color" << std::endl;
        return false;
    }

    uniform_mvp = glGetUniformLocation(program, "mvp");
    if(uniform_mvp == -1){
        std::cout << "No se puede asociar el uniform mvp" << std::endl;
        return false;
    }



    return true;
}

void onDisplay(){
    //Creamos matrices de modelo, vista y proyeccion
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -1.5));
    glm::mat4 view  = glm::lookAt(glm::vec3(4.0f, 4.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 projection = glm::perspective(45.0f, 1.0f*screen_width/screen_height, 0.1f, 10.0f);
    glm::mat4 mvp = projection * view * model;

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glUseProgram(program);
    //Enviamos la matriz que debe ser usada para cada vertice
    glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

    glEnableVertexAttribArray(attribute_coord3d);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_surface);

    glVertexAttribPointer(
        attribute_coord3d,
        3,
        GL_FLOAT,
        GL_FALSE,
        0, 0
    );

    glEnableVertexAttribArray(attribute_color);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_color);

    glVertexAttribPointer(
        attribute_color,
        3,
        GL_FLOAT,
        GL_FALSE,
        0, 0
    );

    glBindBuffer(GL_ARRAY_BUFFER, vbo_surface);

    glDrawArrays(GL_POINTS, 0, /*EXAMEN: Numero de puntos*/ numPoints);

    glDisableVertexAttribArray(attribute_coord3d);
    glDisableVertexAttribArray(attribute_color);
    glutSwapBuffers();
}

void onReshape(int w, int h){
    screen_width = w;
    screen_height = h;

    glViewport(0,0,screen_width, screen_height);
}

void free_resources(){
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo_surface);
    glDeleteBuffers(1, &vbo_color);


    /*EXAMEN - OPCIONAL: elimine la memoria que se utilizó en los buffers. Use delete o free en los arreglos
                surface_vertices y surface_color
    */             
    delete[] surface_vertices;
    delete[] surface_color;
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
        glutMainLoop();
    }

    free_resources();
    exit(EXIT_SUCCESS);
}
