#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
	GLuint VertexArrayID;
	GLuint VertexBuffer;
	GLuint ColorBuffer;

	GLenum PrimitiveMode;
	GLenum FillMode;
	int NumVertices;
};
typedef struct VAO VAO;
struct COLOR{
	float r,g,b;
};
typedef struct COLOR COLOR;
struct Sprite{
	glm::vec3 pos;
	glm::vec3 back;
	glm::vec3 scale;
	int speed;
	int dr;
	glm::vec3 ori;
	glm::vec3 theta;
	glm::vec3 limit;
	COLOR color;
	bool active;
	int status;
	bool choose;
	bool hover;
	VAO* object;
	VAO* line;
	string name;
};
typedef struct Sprite Sprite;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

int do_rot, floor_rel;;
GLuint programID;
double last_update_time, current_time;
float rectangle_rotation = 0;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	//    printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	//    fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	//    printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	//    fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	//    fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	//    fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

void initGLEW(void){
	glewExperimental = GL_TRUE;
	if(glewInit()!=GLEW_OK){
		fprintf(stderr,"Glew failed to initialize : %s\n", glewGetErrorString(glewInit()));
	}
	if(!GLEW_VERSION_3_3)
		fprintf(stderr, "3.3 version not available\n");
}



/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
			0,                  // attribute 0. Vertices
			3,                  // size (x,y,z)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
			1,                  // attribute 1. Color
			3,                  // size (r,g,b)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
	GLfloat* color_buffer_data = new GLfloat [3*numVertices];
	for (int i=0; i<numVertices; i++) {
		color_buffer_data [3*i] = red;
		color_buffer_data [3*i + 1] = green;
		color_buffer_data [3*i + 2] = blue;
	}

	return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Enable Vertex Attribute 1 - Color
	glEnableVertexAttribArray(1);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/
Sprite floor_grey;
Sprite floor_orange;
Sprite rectangle_line;
Sprite cube[2];
int dom;
int rec;
int mode;
int toppling;
bool merged;
float rectangle_rot_dir = 1;
bool rectangle_rot_status = true;
int dim=10;
int boardMatrix[10][10];
/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Function is called first on GLFW_PRESS.

	if (action == GLFW_RELEASE) {
		switch (key) {
			case GLFW_KEY_C:
				rectangle_rot_status = !rectangle_rot_status;
				break;
			case GLFW_KEY_P:
				break;
			case GLFW_KEY_X:
				// do something ..
				break;
			default:
				break;
		}
	}
	else if (action == GLFW_PRESS) {
		switch (key) {
			case GLFW_KEY_ESCAPE:
				quit(window);
				break;
			default:
				break;
		}
	}
}
int NorthDOM(){
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y != cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 1

		if(cube[0].pos.y > cube[1].pos.y)
			return 0; // fore most is dom
		else return 1;
	}
	if(cube[0].pos.x != cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 2
		if(cube[0].pos.x > cube[1].pos.x)
			return 0; // both are dom
		else return 1;
	}
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z != cube[1].pos.z){
		//case 3
		if(cube[0].pos.z < cube[1].pos.z)
			return 0; // lower is dom
		else return 1;
	}
}
int NorthMode(){
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y != cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 1
		return 1;
	}
	if(cube[0].pos.x != cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 2
		return 2;
	}
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z != cube[1].pos.z){
		//case 3
		return 3;
	}
}
int SouthDOM(){
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y != cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 1

		if(cube[0].pos.y < cube[1].pos.y)
			return 0; // back most is dom
		else return 1;
	}
	if(cube[0].pos.x != cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 2
		if(cube[0].pos.x > cube[1].pos.x)
			return 0; // both are dom
		else return 1;
	}
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z != cube[1].pos.z){
		//case 3
		if(cube[0].pos.z < cube[1].pos.z)
			return 0; // lower is dom
		else return 1;
	}
}
int SouthMode(){
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y != cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 1
		return 3;
	}
	if(cube[0].pos.x != cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 2
		return 2;
	}
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z != cube[1].pos.z){
		//case 3
		return 1;
	}
}
int EastDOM(){
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y != cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 1

		if(cube[0].pos.y > cube[1].pos.y)
			return 0; 
		else return 1;//both are dom
	}
	if(cube[0].pos.x != cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 2
		if(cube[0].pos.x > cube[1].pos.x)
			return 0; // fore x is dom
		else return 1;
	}
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z != cube[1].pos.z){
		//case 3
		if(cube[0].pos.z < cube[1].pos.z)
			return 0; // lower is dom
		else return 1;
	}
}
int EastMode(){
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y != cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 1
		return 2;
	}
	if(cube[0].pos.x != cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 2
		return 1;
	}
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z != cube[1].pos.z){
		//case 3
		return 3;
	}
}
int WestDOM(){
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y != cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 1

		if(cube[0].pos.y > cube[1].pos.y)
			return 0; 
		else return 1;//both are dom
	}
	if(cube[0].pos.x != cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 2
		if(cube[0].pos.x < cube[1].pos.x)
			return 0; // less x is dom
		else return 1;
	}
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z != cube[1].pos.z){
		//case 3
		if(cube[0].pos.z < cube[1].pos.z)
			return 0; // lower is dom
		else return 1;
	}
}
int WestMode(){
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y != cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 1
		return 2;
	}
	if(cube[0].pos.x != cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z == cube[1].pos.z){
		//case 2
		return 3;
	}
	if(cube[0].pos.x == cube[1].pos.x && cube[0].pos.y == cube[1].pos.y && cube[0].pos.z != cube[1].pos.z){
		//case 3
		return 1;
	}
}
void CubeToppleNorth(){
	cube[dom].pos = glm::vec3(  cube[dom].back.x,
			cube[dom].back.y + cube[dom].scale.y + cube[dom].scale.z*sin(cube[dom].theta.y*M_PI/180.0f)*sqrt(2),
			floor_grey.scale.z                 + cube[dom].scale.z*cos(cube[dom].theta.y*M_PI/180.0f)*sqrt(2)
			);
	float dist;
	if(mode == 1)
	{
		cube[rec].pos = glm::vec3(  cube[dom].back.x,
				cube[dom].pos.y - cos(45+cube[dom].theta.y*M_PI/180.0f),
				cube[dom].pos.z + sin(45+cube[dom].theta.y*M_PI/180.0f)
				);
	}
	else if (mode ==2){
		cube[rec].pos = glm::vec3(  cube[rec].back.x,
				cube[dom].pos.y ,
				cube[dom].pos.z 
				);
	}
	else if (mode ==3){
		cube[rec].pos = glm::vec3(  cube[dom].back.x,
				cube[dom].pos.y + sin(45+cube[dom].theta.y*M_PI/180.0f),
				cube[dom].pos.z + cos(45+cube[dom].theta.y*M_PI/180.0f)
				);
	}


	cube[dom].theta.y += cube[dom].dr;
	cube[rec].theta.y += cube[rec].dr;

	if(cube[dom].theta.y >= cube[dom].limit.y){
		if(mode==1){
			cube[rec].pos = glm::vec3(cube[rec].back.x,cube[rec].back.y+2,cube[rec].back.z+1);	
		}
		else if(mode==2){
			cube[rec].pos = glm::vec3(cube[rec].back.x,cube[rec].back.y+1,cube[rec].back.z);	
		}
		else if(mode==3){
			cube[rec].pos = glm::vec3(cube[rec].back.x,cube[rec].back.y+2,cube[rec].back.z-1);	
		}
		cube[dom].pos = glm::vec3(cube[dom].back.x,cube[dom].back.y+1,cube[dom].back.z);
		cube[dom].theta.y=cube[rec].limit.y;
		cube[rec].theta.y=cube[rec].limit.y;
		toppling=0;
	}
}
void CubeToppleSouth(){

	cube[dom].pos = glm::vec3(  cube[dom].back.x,
			cube[dom].back.y - cube[dom].scale.y + cube[dom].scale.z*sin(cube[dom].theta.y*M_PI/180.0f)*sqrt(2),
			floor_grey.scale.z                 + cube[dom].scale.z*cos(cube[dom].theta.y*M_PI/180.0f)*sqrt(2)
			);
	if(mode == 1)
	{
		cube[rec].pos = glm::vec3(  cube[dom].back.x,
				cube[dom].pos.y - cos(45+cube[dom].theta.y*M_PI/180.0f),
				cube[dom].pos.z + sin(45+cube[dom].theta.y*M_PI/180.0f)
				);
	}
	else if (mode ==2){
		cube[rec].pos = glm::vec3(  cube[rec].back.x,
				cube[dom].pos.y,
				cube[dom].pos.z
				);
	}
	else if (mode ==3){
		cube[rec].pos = glm::vec3(  cube[dom].back.x,
				cube[dom].pos.y + sin(45+cube[dom].theta.y*M_PI/180.0f),
				cube[dom].pos.z + cos(45+cube[dom].theta.y*M_PI/180.0f)
				);
	}
	cube[dom].theta.y += cube[dom].dr;
	cube[rec].theta.y += cube[rec].dr;

	if(cube[dom].theta.y <= cube[dom].limit.y){
		if(mode==1){
			cube[rec].pos = glm::vec3(cube[rec].back.x,cube[rec].back.y-2,cube[rec].back.z-1);	
		}
		else if(mode==2){
			cube[rec].pos = glm::vec3(cube[rec].back.x,cube[rec].back.y-1,cube[rec].back.z);	
		}
		else if(mode==3){
			cube[rec].pos = glm::vec3(cube[rec].back.x,cube[rec].back.y-2,cube[rec].back.z+1);	
		}
		cube[dom].pos = glm::vec3(cube[dom].back.x,cube[dom].back.y-1,cube[dom].back.z);	
		toppling=0;
		cube[dom].theta.y=-45;
		cube[rec].theta.y=-45;
	}
}
void CubeToppleWest(){

	cube[dom].pos = glm::vec3(  cube[dom].back.x - cube[dom].scale.y + cube[dom].scale.z*sin(cube[dom].theta.x*M_PI/180.0f)*sqrt(2),
			cube[dom].back.y, 
			floor_grey.scale.z                 + cube[dom].scale.z*cos(cube[dom].theta.x*M_PI/180.0f)*sqrt(2)
			);
	if(mode == 1)
	{
		cube[rec].pos = glm::vec3(  cube[dom].pos.x - cos(45+cube[dom].theta.x*M_PI/180.0f),
				cube[dom].pos.y,
				cube[dom].pos.z + sin(45+cube[dom].theta.x*M_PI/180.0f)
				);
	}
	else if (mode ==2){
		cube[rec].pos = glm::vec3(  cube[dom].pos.x,
				cube[rec].pos.y,
				cube[dom].pos.z
				);
	}
	else if (mode ==3){
		cube[rec].pos = glm::vec3(  cube[dom].pos.x + sin(45+cube[dom].theta.x*M_PI/180.0f),
				cube[dom].pos.y,
				cube[dom].pos.z + cos(45+cube[dom].theta.x*M_PI/180.0f)
				);
	}
	cube[dom].theta.x += cube[dom].dr;
	cube[rec].theta.x += cube[rec].dr;
	if(cube[dom].theta.x <= cube[dom].limit.x){
		if(mode==1){
			cube[rec].pos = glm::vec3(cube[rec].back.x-2,cube[rec].back.y,cube[rec].back.z-1);	
		}
		else if(mode==2){
			cube[rec].pos = glm::vec3(cube[rec].back.x-1,cube[rec].back.y,cube[rec].back.z);	
		}
		else if(mode==3){
			cube[rec].pos = glm::vec3(cube[rec].back.x-2,cube[rec].back.y,cube[rec].back.z+1);	
		}
		cube[dom].pos = glm::vec3(cube[dom].back.x-1,cube[dom].back.y,cube[dom].back.z);	
		cube[dom].theta.x=-45;
		cube[rec].theta.x=-45;
		toppling=0;
	}
}
void CubeToppleEast(){
	cube[dom].pos = glm::vec3(  cube[dom].back.x + cube[dom].scale.y + cube[dom].scale.z*sin(cube[dom].theta.x*M_PI/180.0f)*sqrt(2),
			cube[dom].back.y, 
			floor_grey.scale.z                 + cube[dom].scale.z*cos(cube[dom].theta.x*M_PI/180.0f)*sqrt(2)
			);
	if(mode == 1)
	{
		cube[rec].pos = glm::vec3(  cube[dom].pos.x - cos(45+cube[dom].theta.x*M_PI/180.0f),
				cube[dom].back.y,
				cube[dom].pos.z + sin(45+cube[dom].theta.x*M_PI/180.0f)
				);
	}
	else if (mode ==2){
		cube[rec].pos = glm::vec3(  cube[dom].pos.x ,
				cube[rec].pos.y,
				cube[dom].pos.z 
				);
	}
	else if (mode ==3){
		cube[rec].pos = glm::vec3(  cube[dom].pos.x + sin(45+cube[dom].theta.x*M_PI/180.0f),
				cube[dom].back.y,
				cube[dom].pos.z + cos(45+cube[dom].theta.x*M_PI/180.0f)
				);
	}
	cube[dom].theta.x += cube[dom].dr;
	cube[rec].theta.x += cube[rec].dr;
	if(cube[dom].theta.x >= cube[dom].limit.x){
		if(mode==1){
			cube[rec].pos = glm::vec3(cube[rec].back.x+2,cube[rec].back.y,cube[rec].back.z+1);	
		}
		else if(mode==2){
			cube[rec].pos = glm::vec3(cube[rec].back.x+1,cube[rec].back.y,cube[rec].back.z);	
		}
		else if(mode==3){
			cube[rec].pos = glm::vec3(cube[rec].back.x+2,cube[rec].back.y,cube[rec].back.z-1);	
		}
		cube[dom].pos = glm::vec3(cube[dom].back.x+1,cube[dom].back.y,cube[dom].back.z);	
		cube[dom].theta.x=-cube[dom].limit.x;
		cube[rec].theta.x=-cube[rec].limit.x;
		toppling=0;
	}
}
void CubeActivateTopple(int dir){
	if(toppling !=0)
		return;

	else if(dir==1){
		dom = NorthDOM();
		rec = 1 - dom;
		mode = NorthMode();
		toppling = dir;

		cube[dom].back = cube[dom].pos;
		cube[rec].back = cube[rec].pos;

		cube[dom].dr = cube[dom].speed;
		cube[rec].dr = cube[rec].speed;

		cube[dom].theta.y = cube[dom].ori.y = -45;
		cube[dom].limit.y  = 90 + cube[dom].theta.y ;
		cube[rec].limit.y  = 90 + cube[rec].theta.y ;
	}
	else if(dir==2){
		dom = SouthDOM();
		rec = 1 - dom;
		mode = SouthMode();
		toppling = dir;
		cube[dom].back = cube[dom].pos;
		cube[rec].back = cube[rec].pos;

		cube[dom].dr = -cube[dom].speed;
		cube[rec].dr = -cube[rec].speed;
		cube[dom].theta.y = cube[dom].ori.y = 45;	
		cube[dom].limit.y  = -90 + cube[dom].theta.y ;
		cube[rec].limit.y  = -90 + cube[rec].theta.y ;
	}
	else if(dir==3){
		dom = WestDOM();
		rec = 1 - dom;
		mode = WestMode();
		toppling = dir;
		cube[dom].back = cube[dom].pos;
		cube[rec].back = cube[rec].pos;
		cube[dom].dr = -cube[dom].speed;
		cube[rec].dr = -cube[rec].speed;
		cube[dom].theta.x = cube[dom].ori.x = 45;	
		cube[dom].limit.x  = -90 + cube[dom].theta.x ;
		cube[rec].limit.x  = -90 + cube[rec].theta.x ;
	}
	else if(dir==4){

		dom = EastDOM();
		rec = 1 - dom;
		mode = EastMode();
		toppling=dir;

		cube[dom].back = cube[dom].pos;
		cube[rec].back = cube[rec].pos;

		cube[dom].dr = cube[dom].speed;
		cube[rec].dr = cube[rec].speed;

		cube[dom].theta.x = cube[dom].ori.x = -45;
		cube[dom].limit.x  = 90 + cube[dom].theta.x ;
		cube[rec].limit.x  = 90 + cube[rec].theta.x ;
	}
}
/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
			quit(window);
			break;
		case 'a':
			CubeActivateTopple(3);
			break;
		case 'd':
			CubeActivateTopple(4);
			break;
		case 'w':
			CubeActivateTopple(1);
			break;
		case 's':
			CubeActivateTopple(2);
			break;
		case 'r':
			break;
		case 'f':
			break;
		case 'e':
			break;
		case 'j':
			break;
		case 'l':
			break;
		case 'i':
			break;
		case 'k':
			break;
		case 'y':
			break;
		case 'h':
			break;
		case 'g':
			break;
		case ' ':
			break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
	switch (button) {
		case GLFW_MOUSE_BUTTON_RIGHT:
			if (action == GLFW_RELEASE) {
				rectangle_rot_dir *= -1;
			}
			break;
		default:
			break;
	}
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
	int fbwidth=width, fbheight=height;
	glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = M_PI/2;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// Store the projection matrix in a variable for future use
	// Perspective projection for 3D views
	Matrices.projection = glm::perspective(fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

	// Ortho projection for 2D views
	//Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}

VAO *rectangle, *cam, *floor_vao, *rectangle_grey, *rectangle_orange, *rectangle_grey_line;

// Creates the rectangle object used in this sample code

void createRectangle ()
{
	floor_grey.pos = floor_orange.pos = glm::vec3(0,0,0);
	floor_grey.scale = floor_orange.scale = glm::vec3(0.5,0.5,0.1);

	cube[0].scale = cube[0].scale = glm::vec3(0.5,0.5,0.5);
	cube[0].pos = cube[0].pos = glm::vec3(0,0,floor_grey.scale.z+cube[0].scale.z);
	cube[0].speed =	 10;

	cube[1].scale = cube[1].scale = glm::vec3(0.5,0.5,0.5);
	cube[1].pos = cube[1].pos = glm::vec3(1,0,floor_grey.scale.z+cube[1].scale.z);
	cube[1].speed =	 10;

	// GL3 accepts only Triangles. Quads are not supported
	static const GLfloat vertex_buffer_data[] = {
		-1.0f,-1.0f,-1.0f, // triangle 1 : begin
		-1.0f,-1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f, // triangle 1 : end
		1.0f, 1.0f,-1.0f, // triangle 2 : begin
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f, // triangle 2 : end
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f
	};
	float grey=(float)112/255;
	float line=(float)50/255;
	float b=(float)112/255;
	COLOR orange;
	orange.r=1;
	orange.g=162.0f/255.0f;
	orange.b=0;
	static const GLfloat color_buffer_data_grey[] = {
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey,
		grey,  grey,  grey
	};
	static const GLfloat color_buffer_data_orange[] = {
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b,
		orange.r,  orange.g,  orange.b
	};
	cube[1].color.r = cube[0].color.r=1;
	cube[1].color.g = cube[0].color.g=162.0f/255.0f;
	cube[1].color.b = cube[0].color.b=200.0f/255.0f;
	static const GLfloat color_buffer_data_cube1[] = {
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b,
		cube[0].color.r,  cube[0].color.g,  cube[0].color.b
	};
	static const GLfloat color_buffer_data_line[] = {
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line,
		line,  line,  line
	};


	// create3DObject creates and returns a handle to a VAO that can be used later
	floor_grey.object = create3DObject(GL_TRIANGLES, 13*3, vertex_buffer_data, color_buffer_data_grey, GL_FILL);

	floor_grey.line = floor_orange.line = cube[1].line =cube[0].line = create3DObject(GL_TRIANGLES, 13*3, vertex_buffer_data, color_buffer_data_line, GL_LINE);

	floor_orange.object = create3DObject(GL_TRIANGLES, 13*3, vertex_buffer_data, color_buffer_data_orange, GL_FILL);

	cube[0].object = create3DObject(GL_TRIANGLES, 13*3, vertex_buffer_data, color_buffer_data_cube1, GL_FILL);

	cube[1].object = create3DObject(GL_TRIANGLES, 13*3, vertex_buffer_data, color_buffer_data_cube1, GL_FILL);


}
void createCam ()
{
	// GL3 accepts only Triangles. Quads are not supported
	static const GLfloat vertex_buffer_data [] = {
		-0.1, 0, 0,
		0.1, 0, 0, 
		0, 0.1, 0,
	};

	static const GLfloat color_buffer_data [] = {
		1, 1, 1,
		1, 1, 1,
		1, 1, 1,
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	cam = create3DObject(GL_TRIANGLES, 1*3, vertex_buffer_data, color_buffer_data, GL_LINE);
}
void createFloor ()
{
	// GL3 accepts only Triangles. Quads are not supported
	static const GLfloat vertex_buffer_data [] = {
		-2, -1, 2,
		2, -1, 2, 
		-2, -1, -2,
		-2, -1, -2,
		2, -1, 2, 
		2, -1, -2,
	};

	static const GLfloat color_buffer_data [] = {
		0.65, 0.165, 0.165,
		0.65, 0.165, 0.165,
		0.65, 0.165, 0.165,
		0.65, 0.165, 0.165,
		0.65, 0.165, 0.165,
		0.65, 0.165, 0.165,
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	floor_vao = create3DObject(GL_TRIANGLES, 2*3, vertex_buffer_data, color_buffer_data, GL_FILL);
}



float camera_rotation_angle = 90;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw (GLFWwindow* window, float x, float y, float w, float h, int doM, int doV, int doP)
{
	int fbwidth, fbheight;
	glfwGetFramebufferSize(window, &fbwidth, &fbheight);
	glViewport((int)(x*fbwidth), (int)(y*fbheight), (int)(w*fbwidth), (int)(h*fbheight));


	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram(programID);

	// Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (0, 0, 0);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (0, 1, 0);

	// Compute Camera matrix (view)
	if(doV)
		Matrices.view = glm::lookAt(eye, target, up); // Fixed camera for 2D (ortho) in XY plane
	else
		Matrices.view = glm::mat4(1.0f);

	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	glm::mat4 VP;
	if (doP)
		VP = Matrices.projection * Matrices.view;
	else
		VP = Matrices.view;
	glm::mat4 MVP;	// MVP = Projection * View * Model

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	for(int i=0;i<dim;i++){
		for(int j=0;j<dim;j++){


			// Load identity to model matrix
			Matrices.model = glm::mat4(1.0f);

			glm::mat4 translateRectangle = glm::translate (glm::vec3(floor_grey.pos.x+i, floor_grey.pos.y+j, floor_grey.pos.z));        // glTranslatef
			glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));
			glm::mat4 myScalingMatrix = glm::scale(glm::mat4(1.0f),floor_grey.scale);
			Matrices.model *= (translateRectangle * rotateRectangle * myScalingMatrix);
			MVP = VP * Matrices.model;
			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

			// draw3DObject draws the VAO given to it using current MVP matrix
			if(boardMatrix[i][j]==1)
				draw3DObject(floor_grey.object);
			else
				draw3DObject(floor_orange.object);

			draw3DObject(floor_grey.	line);
		}
	}

	// Load identity to model matrix
	Matrices.model = glm::mat4(1.0f);

	glm::mat4 translateCam = glm::translate(eye);
	glm::mat4 rotateCam = glm::rotate((float)((90 - camera_rotation_angle)*M_PI/180.0f), glm::vec3(0,1,0));
	Matrices.model *= (translateCam * rotateCam);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(cam);

	Matrices.model = glm::mat4(1.0f);

	glm::mat4 translateCube1 = glm::translate (cube[0].pos);        // glTranslatef
	glm::mat4 rotateCube1X = glm::rotate((float)(-(cube[0].theta.x+45)*M_PI/180.0f), glm::vec3(0,-1,0));
	glm::mat4 rotateCube1Y = glm::rotate((float)(-(cube[0].theta.y+45)*M_PI/180.0f), glm::vec3(1,0,0));
	glm::mat4 myScalingMatrix2 = glm::scale(glm::mat4(1.0f),cube[0].scale);
	Matrices.model *= (translateCube1 * rotateCube1X * rotateCube1Y * myScalingMatrix2);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix

	draw3DObject(cube[0].object);
	draw3DObject(cube[0].line);


	Matrices.model = glm::mat4(1.0f);

	glm::mat4 translateCube2 = glm::translate (cube[1].pos);        // glTranslatef
	glm::mat4 rotateCube2X = glm::rotate((float)(-(cube[1].theta.x+45)*M_PI/180.0f), glm::vec3(0,-1,0));
	glm::mat4 rotateCube2Y = glm::rotate((float)(-(cube[1].theta.y+45)*M_PI/180.0f), glm::vec3(1,0,0));
	glm::mat4 myScalingMatrix = glm::scale(glm::mat4(1.0f),cube[1].scale);
	Matrices.model *= (translateCube2 * rotateCube2X * rotateCube2Y * myScalingMatrix);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix

	draw3DObject(cube[1].object);
	draw3DObject(cube[1].line);



}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height){
	GLFWwindow* window; // window desciptor/handle

	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

	if (!window) {
		exit(EXIT_FAILURE);
		glfwTerminate();
	}

	glfwMakeContextCurrent(window);
	//    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	glfwSwapInterval( 1 );
	glfwSetFramebufferSizeCallback(window, reshapeWindow);
	glfwSetWindowSizeCallback(window, reshapeWindow);
	glfwSetWindowCloseCallback(window, quit);
	glfwSetKeyCallback(window, keyboard);      // general keyboard input
	glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling
	glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

	return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
	/* Objects should be created before any other gl function and shaders */
	// Create the models
	createRectangle ();
	createCam();
	createFloor();

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


	reshapeWindow (window, width, height);

	// Background color of the scene
	glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

	// cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	// cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	// cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	// cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}
void gameEngine(){
	if(toppling!=0){
		if(toppling == 1){
			CubeToppleNorth();
		}
		if(toppling == 2){
			CubeToppleSouth();
		}
		if(toppling == 3){
			CubeToppleWest();
		}
		if(toppling == 4){
			CubeToppleEast();
		}
	}
}
int main (int argc, char** argv)
{
	merged=true;
	int width = 600;
	int height = 600;

	do_rot = 0;
	floor_rel = 1;

	GLFWwindow* window = initGLFW(width, height);
	initGLEW();
	initGL (window, width, height);


	last_update_time = glfwGetTime();

	for(int i=0;i<dim;i++){
		for(int j=0;j<dim;j++){
			if(i!=j)
				boardMatrix[i][j]=1;
			else
				boardMatrix[i][j]=2;
		}
	}

	/* Draw in loop */
	cube[0].theta.x=cube[0].ori.x=-45;
	cube[0].theta.y=cube[0].ori.y=-45;
	cube[1].theta.x=cube[1].ori.x=-45;
	cube[1].theta.y=cube[1].ori.y=-45;
	while (!glfwWindowShouldClose(window)) {

		// clear the color and depth in the frame buffer
		gameEngine();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// OpenGL Draw commands
		current_time = glfwGetTime();
		if(do_rot)
			camera_rotation_angle += 90*(current_time - last_update_time); // Simulating camera rotation
		if(camera_rotation_angle > 720)
			camera_rotation_angle -= 720;
		last_update_time = current_time;
		draw(window, 0, 0, 1, 1, 0, 1, 1);
		// draw(window, 0.5, 0, 0.5, 0.5, 0, 1, 1);
		// draw(window, 0, 0.5, 0.5, 0.5, 1, 0, 1);
		// draw(window, 0.5, 0.5, 0.5, 0.5, 0, 0, 1);

		// Swap Frame Buffer in double buffering
		glfwSwapBuffers(window);

		// Poll for Keyboard and mouse events
		glfwPollEvents();
	}

	glfwTerminate();
	//    exit(EXIT_SUCCESS);
}
