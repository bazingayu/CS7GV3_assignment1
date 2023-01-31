//Some Windows Headers (For Time, IO, etc.)
#include <Windows.h>
#include <mmsystem.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "maths_funcs.h"
//#include "teapot.h" // car mesh
#include "vector"
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

#define MESH_NAME "D:/Projects/realtime_rendering/teapot1.obj"
int teapot_vertex_count;

#pragma warning(disable : 4996)
// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;

GLuint shaderProgramID;
GLuint blinnid ;
GLuint phongid ;
GLuint goochid ;
GLuint minnaertid ;
mat4 global_translate_up = identity_mat4();
mat4 global_translate_down = identity_mat4();
mat4 view_global = look_at(	vec3(0.0, 0.0, 1.1),
                               vec3(0.0, 0.0, 0.0),
                               vec3(0.0, 1.0, 0.0));
unsigned int teapot_vao = 0;
int width = 1024;
int height = 768;
mat4 persp_global = identity_mat4();
char keyFunction;
float rotate_x = 0.0f;
float delta = 2.0f;
float scalef = 0.8f;


GLuint loc1;
GLuint loc2;
GLfloat rotatez = 0.0f;

typedef struct ModelData
{
    size_t mPointCount = 0;
    std::vector<vec3> mVertices;
    std::vector<vec3> mNormals;
    std::vector<vec2> mTextureCoords;
} ModelData;

ModelData teapotModel;

ModelData load_mesh(const char* file_name) {
    ModelData modelData;

    /* Use assimp to read the model file, forcing it to be read as    */
    /* triangles. The second flag (aiProcess_PreTransformVertices) is */
    /* relevant if there are multiple meshes in the model file that   */
    /* are offset from the origin. This is pre-transform them so      */
    /* they're in the right position.                                 */
    const aiScene* scene = aiImportFile(
            file_name,
            aiProcess_Triangulate | aiProcess_PreTransformVertices
    );

    if (!scene) {
        fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
        return modelData;
    }

    printf("  %i materials\n", scene->mNumMaterials);
    printf("  %i meshes\n", scene->mNumMeshes);
    printf("  %i textures\n", scene->mNumTextures);

    for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
        const aiMesh* mesh = scene->mMeshes[m_i];
        printf("    %i vertices in mesh\n", mesh->mNumVertices);
        modelData.mPointCount += mesh->mNumVertices;
        for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
            if (mesh->HasPositions()) {
                const aiVector3D* vp = &(mesh->mVertices[v_i]);
                modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
            }
            if (mesh->HasNormals()) {
                const aiVector3D* vn = &(mesh->mNormals[v_i]);
                modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
            }
            if (mesh->HasTextureCoords(0)) {
                const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
                modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));
            }
            if (mesh->HasTangentsAndBitangents()) {
                /* You can extract tangents and bitangents here              */
                /* Note that you might need to make Assimp generate this     */
                /* data for you. Take a look at the flags that aiImportFile  */
                /* can take.                                                 */
            }
        }
    }

    aiReleaseImport(scene);
    return modelData;
}

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

// Create a NULL-terminated string by reading the provided file
char* readShaderSource(const char* shaderFile) {
    FILE* fp = fopen(shaderFile, "rb"); //!->Why does binary flag "RB" work and not "R"... wierd msvc thing?

    if (fp == NULL) { return NULL; }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);
    char* buf = new char[size + 1];
    fread(buf, 1, size, fp);
    buf[size] = '\0';

    fclose(fp);

    return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
    // create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
    const char* pShaderSource = readShaderSource(pShaderText);

    // Bind the source code to the shader, this happens before compilation
    glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
    // compile the shader and check for errors
    glCompileShader(ShaderObj);
    GLint success;
    // check for shader related errors using glGetShaderiv
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }
    // Attach the compiled shader object to the program object
    glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders_gooch()
{
    //Start the process of setting up our shaders by creating a program ID
    //Note: we will link all the shaders together into this ID
    shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }


    // Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgramID, "D:/Projects/realtime_rendering/simpleVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(shaderProgramID, "D:/Projects/realtime_rendering/phongFragmentShader1.txt", GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };
    // After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgramID);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
    if (Success == 0) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
    }

    // program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgramID);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
    // Finally, use the linked shader program
    // Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
    glUseProgram(shaderProgramID);
    return shaderProgramID;
}


GLuint CompileShaders_minnaert()
{
    //Start the process of setting up our shaders by creating a program ID
    //Note: we will link all the shaders together into this ID
    shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }


    // Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgramID, "D:/Projects/realtime_rendering/simpleVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(shaderProgramID, "D:/Projects/realtime_rendering/phongFragmentShader2.txt", GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };
    // After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgramID);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
    if (Success == 0) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
    }

    // program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgramID);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
    // Finally, use the linked shader program
    // Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
    glUseProgram(shaderProgramID);
    return shaderProgramID;
}

GLuint CompileShaders_phong()
{
    //Start the process of setting up our shaders by creating a program ID
    //Note: we will link all the shaders together into this ID
    shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }


    // Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgramID, "D:/Projects/realtime_rendering/simpleVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(shaderProgramID, "D:/Projects/realtime_rendering/phongFragmentShader.txt", GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };
    // After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgramID);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
    if (Success == 0) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
    }

    // program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgramID);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
    // Finally, use the linked shader program
    // Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
    glUseProgram(shaderProgramID);
    return shaderProgramID;
}

GLuint CompileShaders()
{
    //Start the process of setting up our shaders by creating a program ID
    //Note: we will link all the shaders together into this ID
    shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }


    // Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgramID, "D:/Projects/realtime_rendering/simpleVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(shaderProgramID, "D:/projects/realtime_rendering/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };
    // After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgramID);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
    if (Success == 0) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
    }

    // program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgramID);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
    // Finally, use the linked shader program
    // Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
    glUseProgram(shaderProgramID);
    return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS

void generateObjectBufferTeapot() {

    teapotModel = load_mesh(MESH_NAME);
    GLuint vp_vbo = 0;

    loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
    loc2 = glGetAttribLocation(shaderProgramID, "vertex_normals");

    glGenBuffers(1, &vp_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
    glBufferData(GL_ARRAY_BUFFER, teapotModel.mPointCount * sizeof(vec3), &teapotModel.mVertices[0], GL_STATIC_DRAW);

    GLuint vn_vbo = 0;
    glGenBuffers(1, &vn_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
    glBufferData(GL_ARRAY_BUFFER, teapotModel.mPointCount * sizeof(vec3), &teapotModel.mNormals[0], GL_STATIC_DRAW);

    teapot_vertex_count = teapotModel.mPointCount * sizeof(vec3);
    glGenVertexArrays(1, &teapot_vao);
    glBindVertexArray(teapot_vao);


    glEnableVertexAttribArray(loc1);
    glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
    glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(loc2);
    glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
    glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

}


#pragma endregion VBO_FUNCTIONS

mat4 ortho(float left, float right, float bottom, float top, float nearr, float farr)
{
    return transpose(mat4(2.0f / (right - left), 0.0f, 0.0f, 0.0f,
                          0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
                          0.0f, 0.0f, 2.0f / (nearr - farr), 0.0f,
                          (left + right) / (left - right), (bottom + top) / (bottom - top), (nearr + farr) / (nearr - farr), 1.0f));
}

void display() {
    mat4 view, persp_proj, model;
    // tell GL to only draw onto a pixel if the shape is closer to the viewer
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glMatrixMode(GL_PROJECTION);
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glUseProgram(phongid);

    glViewport(0, 0, width, height);
    view = view_global;
    persp_proj = perspective(50.0f, (float)width / (float)height, 0.1f, 50.0f);

    //Declare your uniform variables that will be used in your shader
    int matrix_location1 = glGetUniformLocation(phongid, "model");
    int view_mat_location1 = glGetUniformLocation(phongid, "view");
    int proj_mat_location1 = glGetUniformLocation(phongid, "proj");

    //root node
    mat4 model1 = identity_mat4();
    model1 = translate(model1, vec3(0.0f, -0.1f, 0.0f));
    model1 = scale(model1, vec3(scalef, scalef, scalef));
    model1 = rotate_y_deg(model1, rotate_x);

    // update uniforms & draw
    glUniformMatrix4fv(proj_mat_location1, 1, GL_FALSE, persp_proj.m);
    glUniformMatrix4fv(view_mat_location1, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, model1.m);
    glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);



    //*********************************************************
    glUseProgram(goochid);

    //Declare your uniform variables that will be used in your shader
    int matrix_location = glGetUniformLocation(goochid, "model");
    int view_mat_location = glGetUniformLocation(goochid, "view");
    int proj_mat_location = glGetUniformLocation(goochid, "proj");

    model = identity_mat4();
    model = translate(model, vec3(0.0f, 0.3f, 0.0f));
    model = scale(model, vec3(scalef, scalef, scalef));
    model = rotate_y_deg(model, rotate_x);

    // update uniforms & draw
    glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
    glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(matrix_location, 1, GL_FALSE, model.m);
    glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

    //*******************************************************************************************//








    glUseProgram(minnaertid);

    //Declare your uniform variables that will be used in your shader
    int matrix_location2 = glGetUniformLocation(minnaertid, "model");
    int view_mat_location2 = glGetUniformLocation(minnaertid, "view");
    int proj_mat_location2 = glGetUniformLocation(minnaertid, "proj");

    //root node
    mat4 model2 = identity_mat4();
    model2 = translate(model2, vec3(0.0f, -0.5f, -0.0f));
    model2 = scale(model2, vec3(scalef, scalef, scalef));
    model2 = rotate_y_deg(model2, rotate_x);

    // update uniforms & draw
    glUniformMatrix4fv(proj_mat_location2, 1, GL_FALSE, persp_proj.m);
    glUniformMatrix4fv(view_mat_location2, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(matrix_location2, 1, GL_FALSE, model2.m);
    glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);


    glutSwapBuffers();
}



void updateScene() {

    // Placeholder code, if you want to work with framerate
    // Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
//    static DWORD  last_time = 0;
//    DWORD  curr_time = timeGetTime();
//    float  delta = (curr_time - last_time) * 0.001f;
//    if (delta > 0.03f)
//        delta = 0.03f;
//    last_time = curr_time;
//
//    rotatez += 0.03f;
    rotate_x += delta;
    // Draw the next frame
    glutPostRedisplay();
}


void init()
{

    glClearColor(0.0, 0.0, 0.0, 0.0);
    // Set up the shaders
    blinnid = CompileShaders();
    phongid = CompileShaders_phong();
    goochid = CompileShaders_gooch();
    minnaertid = CompileShaders_minnaert();

    // load teapot mesh into a vertex buffer array
    generateObjectBufferTeapot();
}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {

    if (key == 'r' || key == 'R') {
        global_translate_up = rotate_y_deg(global_translate_up,2.0);
        printf("rotate_y\n");
        glutPostRedisplay();
    }
    if (key == 's' || key == 'S') {
        global_translate_down = scale(global_translate_down, vec3(0.90, 0.90, 0.90));
        printf("scaledown\n");
        glutPostRedisplay();
    }
    if (key == 'w' || key == 'W') {
        global_translate_down = scale(global_translate_down, vec3(1.1, 1.1, 1.1));
        printf("scaleup\n");
        glutPostRedisplay();
    }

    if (key == 'd' || key == 'D') {
        global_translate_up = rotate_x_deg(global_translate_up, 2.0);
        printf("scaleup\n");
        glutPostRedisplay();
    }

    if (key == 'a' || key == 'A') {
        global_translate_up = rotate_z_deg(global_translate_up, 2.0);
        printf("scaleup\n");
        glutPostRedisplay();
    }
    if (key == 'g' || key == 'G') {

        printf("Gooch Shader Selected\n");
        glDeleteProgram(shaderProgramID);
        GLuint shaderProgramID = CompileShaders_gooch();
        glutPostRedisplay();
    }
    if (key == 'm' || key == 'M') {

        printf("Minnaert Shader Selected\n");
        glDeleteProgram(shaderProgramID);
        GLuint shaderProgramID = CompileShaders_minnaert();
        glutPostRedisplay();
    }
    if (key == 'p' || key == 'P') {

        printf("Phong Shader Selected\n");
        glDeleteProgram(shaderProgramID);
        GLuint shaderProgramID = CompileShaders_phong();
        glutPostRedisplay();
    }
    if (key == 'b' || key == 'B') {

        printf("Blinn Phong Shader Selected\n");
        glDeleteProgram(shaderProgramID);
        GLuint shaderProgramID = CompileShaders();
        glutPostRedisplay();
    }
    if (key == 'z' || key == 'Z') {

        printf("Auto Rotate On");
        global_translate_up = rotate_y_deg(global_translate_up, rotatez);
        glutPostRedisplay();
    }
    if (key == 'q' || key == 'Q') {

        static bool wire = true;
        wire = !wire;
        glPolygonMode(GL_FRONT_AND_BACK, (wire ? GL_LINE : GL_FILL));
        glutPostRedisplay();
    }
}

// Method to handle special keys function
void keySpecial(int keyspecial, int x, int y) {

    switch (keyspecial)
    {
        case GLUT_KEY_UP:
            view_global = translate(view_global, vec3(0.0, 0.0, 0.009));
            printf("cameramoveup\n");
            glutPostRedisplay();
            break;

        case GLUT_KEY_DOWN:
            view_global = translate(view_global, vec3(0.0, 0.00, -0.009));
            printf("cameramovedown\n");
            glutPostRedisplay();
            break;
        case GLUT_KEY_RIGHT:
            view_global = translate(view_global, vec3(0.009, 0.00, 0.0));
            printf("cameramoveright\n");
            glutPostRedisplay();
            break;
        case GLUT_KEY_LEFT:
            view_global = translate(view_global, vec3(-0.009, 0.0, 0.0));
            printf("cameramoveleft\n");
            glutPostRedisplay();
            break;
    }
}



int main(int argc, char** argv) {

    // Set up the window
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(width, height);
    glutCreateWindow("Different Shader Example");

    // Tell glut where the display function is
    glutDisplayFunc(display);
    glutIdleFunc(updateScene);


    // A call to glewInit() must be done after glut is initialized!
    GLenum res = glewInit();
    // Check for any errors
    if (res != GLEW_OK) {
        fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
        return 1;
    }
    // Set up your objects and shaders
    init();
    printf("Press T for TOON Shader\n");
    printf("Press B for Blinn Phong Shader\n");
    printf("Press Q to toggle wireframe\n");
    glutKeyboardFunc(keypress);
    glutSpecialFunc(keySpecial);
    // Begin infinite event loop
    glutMainLoop();
    return 0;
}