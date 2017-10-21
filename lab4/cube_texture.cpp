#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <cstring>

#include <GL/glew.h>
#include <vector>
#include <GL/freeglut.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader_utils.h"

#include "res_texture.c"

#define FOURCC_DXT1 0x31545844 
#define FOURCC_DXT3 0x33545844 
#define FOURCC_DXT5 0x35545844 




int screen_width=800, screen_height=600;
GLuint vbo_cube_vertices;
GLuint ibo_cube_elements;
GLuint program;

GLint attribute_coord3d;
GLint uniform_mvp;

GLuint vbo_cube_texcoords;
GLint attribute_texcoord;

GLuint texture_id;
GLint uniform_texture;


GLuint cargarTextura(const char * imagepath){

  unsigned char header[124];

  FILE *fp; 
 
  
  fp = fopen(imagepath, "rb"); 
  if (fp == NULL){
    printf("%s no se pudo abrir el archivo\n", imagepath); getchar(); 
    return 0;
  }
   
  
  char filecode[4]; 
  fread(filecode, 1, 4, fp); 
  if (strncmp(filecode, "DDS ", 4) != 0) { 
    fclose(fp); 
    return 0; 
  }
  
  
  fread(&header, 124, 1, fp); 

  unsigned int height      = *(unsigned int*)&(header[8 ]);
  unsigned int width       = *(unsigned int*)&(header[12]);
  unsigned int linearSize  = *(unsigned int*)&(header[16]);
  unsigned int mipMapCount = *(unsigned int*)&(header[24]);
  unsigned int fourCC      = *(unsigned int*)&(header[80]);

 
  unsigned char * buffer;
  unsigned int bufsize;
  
  bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize; 
  buffer = (unsigned char*)malloc(bufsize * sizeof(unsigned char)); 
  fread(buffer, 1, bufsize, fp); 
  
  fclose(fp);

  unsigned int components  = (fourCC == FOURCC_DXT1) ? 3 : 4; 
  unsigned int format;
  switch(fourCC) 
  { 
  case FOURCC_DXT1: 
    format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; 
    break; 
  case FOURCC_DXT3: 
    format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; 
    break; 
  case FOURCC_DXT5: 
    format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; 
    break; 
  default: 
    free(buffer); 
    return 0; 
  }

  
  GLuint textureID;
  glGenTextures(1, &textureID);


  glBindTexture(GL_TEXTURE_2D, textureID);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1); 
  
  unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16; 
  unsigned int offset = 0;

  
  for (unsigned int level = 0; level < mipMapCount && (width || height); ++level) 
  { 
    unsigned int size = ((width+3)/4)*((height+3)/4)*blockSize; 
    glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height,  
      0, size, buffer + offset); 
   
    offset += size; 
    width  /= 2; 
    height /= 2; 

    
    if(width < 1) width = 1;
    if(height < 1) height = 1;

  } 

  free(buffer); 

  return textureID;


}

bool cargarOBJ(const char * path,   
  std::vector<glm::vec3> & vertices,
  std::vector<glm::vec2> & uvs,
  std::vector<glm::vec3> & normales){
  printf("cargando archivo %s\n", path);

  std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
  std::vector<glm::vec3> temp_vertices; 
  std::vector<glm::vec2> temp_uvs;
  std::vector<glm::vec3> temp_normals;

  FILE * file = fopen(path,"r");
  if(file == NULL){
    printf("no se pudo cargar el archivo\n");
    getchar();
    return false;
  }

  for(;;){
    char linea[128];

    int res = fscanf(file,"%s",linea);
    if(res == EOF) break;

    if(strcmp(linea,"v") ==0){
      glm::vec3 vertex;
      fscanf(file,"%f %f %f \n", &vertex.x, &vertex.y, &vertex.z);
      temp_vertices.push_back(vertex);
    }else if(strcmp(linea,"vt")==0){
      glm::vec2 uv;
      fscanf(file,"%f %f\n",&uv.x,&uv.y);
      uv.y= -uv.y;
      temp_uvs.push_back(uv);
    }else if (strcmp(linea,"vn")==0){
      glm::vec3 normal;
      fscanf(file,"%f %f %f\n",&normal.x,&normal.y,&normal.z);
      temp_normals.push_back(normal);
    }else if(strcmp(linea,"f")==0){
      unsigned int vertexIndex[3],uvIndex[3],normalIndex[3];
      int matches = fscanf(file, "%d %d %d %d %d %d %d %d %d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2] );
      if(matches != 9){
        printf("%s\n", "no se pudo leer el archivo");
        fclose(file);
        return false;
      }
      vertexIndices.push_back(vertexIndex[0]);
      vertexIndices.push_back(vertexIndex[1]);
      vertexIndices.push_back(vertexIndex[2]);
      uvIndices.push_back(uvIndex[0]);
      uvIndices.push_back(uvIndex[1]);
      uvIndices.push_back(uvIndex[2]);
      normalIndices.push_back(normalIndex[0]);
      normalIndices.push_back(normalIndex[1]);
      normalIndices.push_back(normalIndex[2]);
    }else {
      char bufferaux[10000];
      fgets(bufferaux,1000,file);
    }


  }

    for( unsigned int i=0; i<vertexIndices.size(); i++ ){

    
    unsigned int vertexIndex = vertexIndices[i];
    unsigned int uvIndex = uvIndices[i];
    unsigned int normalIndex = normalIndices[i];
    
    
    glm::vec3 vertex = temp_vertices[ vertexIndex-1 ];
    glm::vec2 uv = temp_uvs[ uvIndex-1 ];
    glm::vec3 normal = temp_normals[ normalIndex-1 ];
    
    
    vertices.push_back(vertex);
    uvs.push_back(uv);
    normales .push_back(normal);
  
  }
  fclose(file);
  return true;

}


int init_resources()
{

  GLuint textura = cargarTextura("House_3_AO.dds");

  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> uvs;
  std::vector<glm::vec3> normales;
  bool res;
  res = cargarOBJ("House_3_AO.obj",vertices,uvs,normales);

  GLfloat cube_vertices[] = {
    // front
    -1.0, -1.0,  1.0,
     1.0, -1.0,  1.0,
     1.0,  1.0,  1.0,
    -1.0,  1.0,  1.0,
    // top
    -1.0,  1.0,  1.0,
     1.0,  1.0,  1.0,
     1.0,  1.0, -1.0,
    -1.0,  1.0, -1.0,
    // back
     1.0, -1.0, -1.0,
    -1.0, -1.0, -1.0,
    -1.0,  1.0, -1.0,
     1.0,  1.0, -1.0,
    // bottom
    -1.0, -1.0, -1.0,
     1.0, -1.0, -1.0,
     1.0, -1.0,  1.0,
    -1.0, -1.0,  1.0,
    // left
    -1.0, -1.0, -1.0,
    -1.0, -1.0,  1.0,
    -1.0,  1.0,  1.0,
    -1.0,  1.0, -1.0,
    // right
     1.0, -1.0,  1.0,
     1.0, -1.0, -1.0,
     1.0,  1.0, -1.0,
     1.0,  1.0,  1.0,
  };

  GLuint vertexbuffer;

  glGenBuffers(1, &vertexbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

  GLfloat cube_texcoords[48];

  cube_texcoords[0] = 0.0;  cube_texcoords[1] = 0.0;
  cube_texcoords[2] = 1.0;  cube_texcoords[3] = 0.0;
  cube_texcoords[4] = 1.0;  cube_texcoords[5] = 1.0;
  cube_texcoords[6] = 0.0;  cube_texcoords[7] = 1.0;

  for(int i = 8; i < 48; i++){
    cube_texcoords[i] = cube_texcoords[i%8];
  }

  GLuint uvbuffer;
  glGenBuffers(1, &uvbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
  glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);


  GLushort cube_elements[] = {
    // front
    0,  1,  2,
    2,  3,  0,
    // top
    4,  5,  6,
    6,  7,  4,
    // back
    8,  9, 10,
    10, 11,  8,
    // bottom
    12, 13, 14,
    14, 15, 12,
    // left
    16, 17, 18,
    18, 19, 16,
    // right
    20, 21, 22,
    22, 23, 20,
  };

  glGenBuffers(1, &ibo_cube_elements);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_elements);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_elements), cube_elements, GL_STATIC_DRAW);

  glGenTextures(1, &texture_id); 
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D,
               0,//niveles de resolucion
               GL_RGB,//formato del pixel
               res_texture.width, //ancho de la imagen
               res_texture.height, //alto de la imagen
               0,//desplazamiento de la textura
               GL_RGB,//formato de la info
               GL_UNSIGNED_BYTE,//tipo de dato de la textura
               res_texture.pixel_data); //formato de los pixeles


  GLint link_ok = GL_FALSE;

  GLuint vs, fs;
  if ((vs = create_shader("cube.v.glsl", GL_VERTEX_SHADER))   == 0) return 0;
  if ((fs = create_shader("cube.f.glsl", GL_FRAGMENT_SHADER)) == 0) return 0;

  program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
  if (!link_ok) {
    fprintf(stderr, "glLinkProgram:");
    print_log(program);
    return 0;
  }

  const char* attribute_name;
  attribute_name = "coord3d";
  attribute_coord3d = glGetAttribLocation(program, attribute_name);
  if (attribute_coord3d == -1) {
    fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
    return 0;
  }

  attribute_texcoord = glGetAttribLocation(program,
                                           "texcoord");
  if(attribute_texcoord == -1){
    fprintf(stderr, "Could not bind attribute texcoord\n");
    return 0;
  }

  const char* uniform_name;
  uniform_name = "mvp";
  uniform_mvp = glGetUniformLocation(program, uniform_name);
  if (uniform_mvp == -1) {
    fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
    return 0;
  }

  uniform_texture = glGetUniformLocation(program, "mytexture");
  if (uniform_texture == -1) {

    fprintf(stderr, "Could not bind uniform mytexture\n");
    return 0;
  }

  return 1;
}

void onIdle() {
  float angle = glutGet(GLUT_ELAPSED_TIME) / 1000.0 * glm::radians(15.0);  // base 15° per second
  glm::mat4 anim = \
    glm::rotate(glm::mat4(1.0f), angle*3.0f, glm::vec3(1, 0, 0)) *  // X axis
    glm::rotate(glm::mat4(1.0f), angle*2.0f, glm::vec3(0, 1, 0)) *  // Y axis
    glm::rotate(glm::mat4(1.0f), angle*4.0f, glm::vec3(0, 0, 1));   // Z axis

  glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0, -4.0));
  glm::mat4 view = glm::lookAt(glm::vec3(0.0, 2.0, 0.0), glm::vec3(0.0, 0.0, -4.0), glm::vec3(0.0, 1.0, 0.0));
  glm::mat4 projection = glm::perspective(45.0f, 1.0f*screen_width/screen_height, 0.1f, 10.0f);

  glm::mat4 mvp = projection * view * model * anim;
  glUseProgram(program);
  glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
  glutPostRedisplay();
}

void onDisplay()
{
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glUseProgram(program);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glUniform1i(uniform_texture, 0);

  glEnableVertexAttribArray(attribute_coord3d);
  // Describe our vertices array to OpenGL (it can't guess its format automatically)
  glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_vertices);
  glVertexAttribPointer(
    attribute_coord3d, // attribute
    3,                 // number of elements per vertex, here (x,y,z)
    GL_FLOAT,          // the type of each element
    GL_FALSE,          // take our values as-is
    0,                 // no extra data between each position
    0                  // offset of first element
  );

  glEnableVertexAttribArray(attribute_texcoord);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_texcoords);

  glVertexAttribPointer(
    attribute_texcoord,
    2,
    GL_FLOAT,
    GL_FALSE,
    0,0
    );

  /* Push each element in buffer_vertices to the vertex shader */
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_elements);
  int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
  glDrawElements(GL_TRIANGLES, size/sizeof(GLushort), GL_UNSIGNED_SHORT, 0);

  glDisableVertexAttribArray(attribute_coord3d);
  glDisableVertexAttribArray(attribute_texcoord);
  glutSwapBuffers();
}

void onReshape(int width, int height) {
  screen_width = width;
  screen_height = height;
  glViewport(0, 0, screen_width, screen_height);
}

void free_resources()
{
  glDeleteProgram(program);
  glDeleteBuffers(1, &vbo_cube_vertices);
  glDeleteBuffers(1, &ibo_cube_elements);
  glDeleteBuffers(1, &vbo_cube_texcoords);
  glDeleteTextures(1, &texture_id);
}


int main(int argc, char* argv[]) {
  glutInit(&argc, argv);
  glutInitContextVersion(2,0);
  glutInitDisplayMode(GLUT_RGBA|GLUT_ALPHA|GLUT_DOUBLE|GLUT_DEPTH);
  glutInitWindowSize(screen_width, screen_height);
  glutCreateWindow("My Textured Cube");

  GLenum glew_status = glewInit();
  if (glew_status != GLEW_OK) {
    fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
    return 1;
  }

  if (!GLEW_VERSION_2_0) {
    fprintf(stderr, "Error: your graphic card does not support OpenGL 2.0\n");
    return 1;
  }

  if (init_resources()) {
    glutDisplayFunc(onDisplay);
    glutReshapeFunc(onReshape);
    glutIdleFunc(onIdle);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glutMainLoop();
  }

  free_resources();
  return 0;
}
