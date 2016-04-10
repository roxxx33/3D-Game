#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <cstdlib>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/glad.h>
#include <FTGL/ftgl.h>
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>

using namespace std;

struct VAO {
	GLuint VertexArrayID;
	GLuint VertexBuffer;
	GLuint ColorBuffer;
	GLuint TextureBuffer;
	GLuint TextureID;

	GLenum PrimitiveMode; // GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_LINE_STRIP_ADJACENCY, GL_LINES_ADJACENCY, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES, GL_TRIANGLE_STRIP_ADJACENCY and GL_TRIANGLES_ADJACENCY
	GLenum FillMode; // GL_FILL, GL_LINE
	int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID; // For use with normal shader
	GLuint TexMatrixID; // For use with texture shader
} Matrices;

struct FTGLFont {
	FTFont* font;
	GLuint fontMatrixID;
	GLuint fontColorID;
} GL3Font;

GLuint programID, fontProgramID, textureProgramID;
float DEG2RAD(float i)
{
	return (i*3.14/180.0);
}

int arr1[10],arr2[10],arr3[10],arr4[10],win = 0,global_flag = 1,arr5[10];
void set()
{	
	for(int i = 0;i < 10; i++)
		arr5[i] = 1;
}

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
	cout << "Compiling shader : " <<  vertex_file_path << endl;
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage( max(InfoLogLength, int(1)) );
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	cout << VertexShaderErrorMessage.data() << endl;

	// Compile Fragment Shader
	cout << "Compiling shader : " << fragment_file_path << endl;
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage( max(InfoLogLength, int(1)) );
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	cout << FragmentShaderErrorMessage.data() << endl;

	// Link the program
	cout << "Linking program" << endl;
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	cout << ProgramErrorMessage.data() << endl;

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
	cout << "Error: " << description << endl;
}

void quit(GLFWwindow *window)
{
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

glm::vec3 getRGBfromHue (int hue)
{
	float intp;
	float fracp = modff(hue/60.0, &intp);
	float x = 1.0 - abs((float)((int)intp%2)+fracp-1.0);

	if (hue < 60)
		return glm::vec3(1,x,0);
	else if (hue < 120)
		return glm::vec3(x,1,0);
	else if (hue < 180)
		return glm::vec3(0,1,x);
	else if (hue < 240)
		return glm::vec3(0,x,1);
	else if (hue < 300)
		return glm::vec3(x,0,1);
	else
		return glm::vec3(1,0,x);
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

struct VAO* create3DTexturedObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* texture_buffer_data, GLuint textureID, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;
	vao->TextureID = textureID;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->TextureBuffer));  // VBO - textures

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

	glBindBuffer (GL_ARRAY_BUFFER, vao->TextureBuffer); // Bind the VBO textures
	glBufferData (GL_ARRAY_BUFFER, 2*numVertices*sizeof(GLfloat), texture_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
						  2,                  // attribute 2. Textures
						  2,                  // size (s,t)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	return vao;
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

void draw3DTexturedObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Bind Textures using texture units
	glBindTexture(GL_TEXTURE_2D, vao->TextureID);

	// Enable Vertex Attribute 2 - Texture
	glEnableVertexAttribArray(2);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->TextureBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle

	// Unbind Textures to be safe
	glBindTexture(GL_TEXTURE_2D, 0);
}

/* Create an OpenGL Texture from an image */
GLuint createTexture (const char* filename)
{
	GLuint TextureID;
	// Generate Texture Buffer
	glGenTextures(1, &TextureID);
	// All upcoming GL_TEXTURE_2D operations now have effect on our texture buffer
	glBindTexture(GL_TEXTURE_2D, TextureID);
	// Set our texture parameters
	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Set texture filtering (interpolation)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Load image and create OpenGL texture
	int twidth, theight;
	unsigned char* image = SOIL_load_image(filename, &twidth, &theight, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, twidth, theight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D); // Generate MipMaps to use
	SOIL_free_image_data(image); // Free the data read from file after creating opengl texture
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess it up

	return TextureID;
}


/**************************
 * Customizable functions *
 **************************/

class Cuboid {
	VAO *cuboid;
	float v[24];
	public:
	void create_cuboid(GLuint textureID)
	{
		static const GLfloat vertex_buffer_data [] = {

			v[0],v[1],v[2],
			v[3],v[4],v[5],
			v[6],v[7],v[8],
			v[9],v[10],v[11],
			v[0],v[1],v[2],
			v[12],v[13],v[14],
			v[15],v[16],v[17],
			v[0],v[1],v[2],
			v[18],v[19],v[20],
			v[9],v[10],v[11],
			v[18],v[19],v[20],
			v[0],v[1],v[2],
			v[0],v[1],v[2],
			v[6],v[7],v[8],
			v[12],v[13],v[14],
			v[15],v[16],v[17],
			v[3],v[4],v[5],
			v[0],v[1],v[2],
			v[6],v[7],v[8],
			v[3],v[4],v[5],
			v[15],v[16],v[17],
			v[21],v[22],v[23],
			v[18],v[19],v[20],
			v[9],v[10],v[11],
			v[18],v[19],v[20],
			v[21],v[22],v[23],
			v[15],v[16],v[17],
			v[21],v[22],v[23],
			v[9],v[10],v[11],
			v[12],v[13],v[14],
			v[21],v[22],v[23],
			v[12],v[13],v[14],
			v[6],v[7],v[8],
			v[21],v[22],v[23],
			v[6],v[7],v[8],
			v[15],v[16],v[17]
		};
		static const GLfloat color_buffer_data [] = {
			0.583f,  0.771f,  0.014f,
			0.609f,  0.115f,  0.436f,
			0.327f,  0.483f,  0.844f,
			0.822f,  0.569f,  0.201f,
			0.435f,  0.602f,  0.223f,
			0.310f,  0.747f,  0.185f,
			0.597f,  0.770f,  0.761f,
			0.559f,  0.436f,  0.730f,
			0.359f,  0.583f,  0.152f,
			0.483f,  0.596f,  0.789f,
			0.559f,  0.861f,  0.639f,
			0.195f,  0.548f,  0.859f,
			0.014f,  0.184f,  0.576f,
			0.771f,  0.328f,  0.970f,
			0.406f,  0.615f,  0.116f,
			0.676f,  0.977f,  0.133f,
			0.971f,  0.572f,  0.833f,
			0.140f,  0.616f,  0.489f,
			0.997f,  0.513f,  0.064f,
			0.945f,  0.719f,  0.592f,
			0.543f,  0.021f,  0.978f,
			0.279f,  0.317f,  0.505f,
			0.167f,  0.620f,  0.077f,
			0.347f,  0.857f,  0.137f,
			0.055f,  0.953f,  0.042f,
			0.714f,  0.505f,  0.345f,
			0.783f,  0.290f,  0.734f,
			0.722f,  0.645f,  0.174f,
			0.302f,  0.455f,  0.848f,
			0.225f,  0.587f,  0.040f,
			0.517f,  0.713f,  0.338f,
			0.053f,  0.959f,  0.120f,
			0.393f,  0.621f,  0.362f,
			0.673f,  0.211f,  0.457f,
			0.820f,  0.883f,  0.371f,
			0.982f,  0.099f,  0.879f
		};

	static const GLfloat texture_buffer_data [] = {
		0,1, // TexCoord 1 - bot left
		1,1, // TexCoord 2 - bot right
		1,0, // TexCoord 3 - top right

		1,0, // TexCoord 3 - top right
		0,0, // TexCoord 4 - top left
		0,1,  // TexCoord 1 - bot left
		
		0,1, // TexCoord 1 - bot left
		1,1, // TexCoord 2 - bot right
		1,0, // TexCoord 3 - top right

		1,0, // TexCoord 3 - top right
		0,0, // TexCoord 4 - top left
		0,1,  // TexCoord 1 - bot left
	
		0,1, // TexCoord 1 - bot left
		1,1, // TexCoord 2 - bot right
		1,0, // TexCoord 3 - top right

		1,0, // TexCoord 3 - top right
		0,0, // TexCoord 4 - top left
		0,1,  // TexCoord 1 - bot left

		0,1, // TexCoord 1 - bot left
		1,1, // TexCoord 2 - bot right
		1,0, // TexCoord 3 - top right

		1,0, // TexCoord 3 - top right
		0,0, // TexCoord 4 - top left
		0,1,  // TexCoord 1 - bot left
		
		0,1, // TexCoord 1 - bot left
		1,1, // TexCoord 2 - bot right
		1,0, // TexCoord 3 - top right

		1,0, // TexCoord 3 - top right
		0,0, // TexCoord 4 - top left
		0,1,  // TexCoord 1 - bot left
		
		0,1, // TexCoord 1 - bot left
		
		1,1, // TexCoord 2 - bot right
		1,0, // TexCoord 3 - top right

		1,0, // TexCoord 3 - top right
		0,0, // TexCoord 4 - top left
		0,1  // TexCoord 1 - bot left
		
	};

	// create3DTexturedObject creates and returns a handle to a VAO that can be used later
	cuboid = create3DTexturedObject(GL_TRIANGLES, 36, vertex_buffer_data, texture_buffer_data, textureID, GL_FILL);
//		cuboid = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
	}
	VAO* get_cuboid()
	{
		return cuboid;
	}
	void set_cuboid_vertices(float v[24])
	{
		for(int i = 0;i < 24;i++)
			this -> v[i] = v[i];
	}
}c1;


class Obstacle_sphere {
	float radius;
	VAO* sphere;
	public:
	VAO* createSphere(GLfloat radius,GLfloat c[3])
	{
		GLfloat* vertex_buffer_data = new GLfloat [3*360];
		GLfloat* color_buffer_data = new GLfloat [3*360];

		for(int i = 0; i < 360;i++)
		{
			vertex_buffer_data [3*i] = (radius * cos(DEG2RAD(i)));
			vertex_buffer_data [3*i + 1] = (radius * sin(DEG2RAD(i)));
			vertex_buffer_data [3*i + 2] = 0;

			color_buffer_data [3*i] = (radius * cos(DEG2RAD(i)));
			color_buffer_data [3*i + 1] = (radius * sin(DEG2RAD(i)));
			color_buffer_data [3*i + 2] = c[2];


		}

		sphere = create3DObject(GL_TRIANGLE_FAN, 360, vertex_buffer_data, color_buffer_data, GL_FILL);
	}

	VAO* get_sphere()
	{
		return sphere;
	}

	void set_radius(float radius)
	{
		this -> radius = radius;
	}

	float get_radius()
	{
		return radius;
	}

}obstacle_sphere;

float jb = 2.5,jt = 0;

class Player {
	//	vertices[8];
	VAO *play;
	float v[24],x,y,z,a;
	int score,lives;
	public:
	void create_player(/*GLuint textureID1*/)
	{
		static const GLfloat vertex_buffer_data [] = {
			v[0],v[1],v[2],
			v[3],v[4],v[5],
			v[6],v[7],v[8],
			v[9],v[10],v[11],
			v[0],v[1],v[2],
			v[12],v[13],v[14],
			v[15],v[16],v[17],
			v[0],v[1],v[2],
			v[18],v[19],v[20],
			v[9],v[10],v[11],
			v[18],v[19],v[20],
			v[0],v[1],v[2],
			v[0],v[1],v[2],
			v[6],v[7],v[8],
			v[12],v[13],v[14],
			v[15],v[16],v[17],
			v[3],v[4],v[5],
			v[0],v[1],v[2],
			v[6],v[7],v[8],
			v[3],v[4],v[5],
			v[15],v[16],v[17],
			v[21],v[22],v[23],
			v[18],v[19],v[20],
			v[9],v[10],v[11],
			v[18],v[19],v[20],
			v[21],v[22],v[23],
			v[15],v[16],v[17],
			v[21],v[22],v[23],
			v[9],v[10],v[11],
			v[12],v[13],v[14],
			v[21],v[22],v[23],
			v[12],v[13],v[14],
			v[6],v[7],v[8],
			v[21],v[22],v[23],
			v[6],v[7],v[8],
			v[15],v[16],v[17]
		};
		static const GLfloat color_buffer_data [] = {

			0.359f,  0.583f,  0.152f,
			0.483f,  0.596f,  0.789f,
			0.359f,  0.161f,  0.639f,
			0.195f,  0.317f,  0.159f,
			0.014f,  0.184f,  0.176f,
			0.271f,  0.328f,  0.170f,
			0.406f,  0.215f,  0.116f,
			0.276f,  0.377f,  0.133f,
			0.171f,  0.572f,  0.833f,
			0.183f,  0.771f,  0.014f,
			0.209f,  0.115f,  0.436f,
			0.327f,  0.483f,  0.244f,
			0.122f,  0.569f,  0.301f,
			0.335f,  0.212f,  0.223f,
			0.310f,  0.747f,  0.185f,
			0.197f,  0.170f,  0.331f,
			0.159f,  0.436f,  0.230f,
			0.140f,  0.216f,  0.489f,
			0.197f,  0.513f,  0.064f,
			0.245f,  0.719f,  0.592f,
			0.543f,  0.021f,  0.978f,
			0.279f,  0.317f,  0.505f,
			0.167f,  0.120f,  0.077f,
			0.347f,  0.357f,  0.137f,
			0.055f,  0.325f,  0.042f,
			0.414f,  0.505f,  0.345f,
			0.283f,  0.290f,  0.734f,
			0.322f,  0.645f,  0.174f,
			0.302f,  0.455f,  0.848f,
			0.122f,  0.587f,  0.120f,
			0.517f,  0.213f,  0.338f,
			0.053f,  0.359f,  0.120f,
			0.393f,  0.221f,  0.362f,
			0.173f,  0.211f,  0.457f,
			0.120f,  0.183f,  0.371f,
			0.182f,  0.099f,  0.179f
		};
/*
	static const GLfloat texture_buffer_data [] = {
		0,1, // TexCoord 1 - bot left
		1,1, // TexCoord 2 - bot right
		1,0, // TexCoord 3 - top right

		1,0, // TexCoord 3 - top right
		0,0, // TexCoord 4 - top left
		0,1  // TexCoord 1 - bot left
	};
*/
	// create3DTexturedObject creates and returns a handle to a VAO that can be used later
//	play = create3DTexturedObject(GL_TRIANGLES, 36, vertex_buffer_data, texture_buffer_data, textureID1, GL_FILL);
		play = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
	}
	VAO* get_player()
	{
		return play;
	}
	void set_player_vertices(float v[24])
	{
		for(int i = 0;i < 24;i ++)
			this -> v[i] = v[i];
	}
	float get_a()
	{
		return a;
	}
	void set_a(float a)
	{
		this -> a = a;
	}
	float get_x()
	{
		return x;
	}
	float get_y()
	{
		return y;
	}
	float get_z()
	{
		return z;
	}
	int get_score()
	{
		return score;
	}
	int get_lives()
	{
		return lives;
	}
	void set_x(float x)
	{
		this -> x = x;
	}
	void set_y(float y)
	{
		this -> y = y;
	}
	void set_z(float z)
	{
		this -> z = z;
	}
	void move_left()
	{
		x -= a;
	}
	void move_right()
	{
		x += a;
	}
	void move_forward()
	{
		y += a;
	}
	void move_backward()
	{
		y -= a;
	}
	int check_void(int posx,int posy)
	{
		if(posy == arr1[posx] /*&& arr4[x] == 0*/)
		{
			score -= 10;
			cout<<"score = "<<score<<endl;
			return 1;
		}
		return 0;
	}
	void set_lives(int lives)
	{
		this -> lives = lives;
	}
	void set_score(int score)
	{
		this -> score = score;
	}
	int check_collision(int posx,int posy,char c)
	{
		if(posy == arr2[posx] && arr5[posx])
		{
			if((x - posx < 0.5 + obstacle_sphere.get_radius()||c == 'l')||(posx - x < 0.5 + obstacle_sphere.get_radius()||c == 'r')||
			(posy - y < 0.5 + obstacle_sphere.get_radius()||c == 'f')||(y - posy < 0.5 + obstacle_sphere.get_radius()||c == 'b')||c == 'd')	
			{	
				if(arr3[posx] == 1)
				{
					if(jb - 1 < 0.5 + obstacle_sphere.get_radius())
					{
						score -= 2;
						cout<<"score = "<<score<<endl;
						arr5[posx] = 0;
						return 1;
					}
				}
				else
				{
					score -= 5;
					cout<<"score = "<<score<<endl;
					arr5[posx] = 0;
					return 1;
				}
			}
		}
		return 0;
	}
	void jump(char c,double t)
	{
/*		if( c == 'l')
			x -= 2;
		else if (c == 'r')
			x += 2;
		else if (c == 'f')
			y += 2;
		else if (c == 'b')
			y -= 2;*/
		set_a(0.07);
		if(c == 'l')
			move_left();
		else if(c == 'r')
			move_right();
		else if(c == 'f')
			move_forward();
		else if(c == 'b')
			move_backward();
		z = 3*t - (4.9)*t*t;


	}
}player;

int posx,posy,flag = 0,player_view = 0,followcam_view = 0,flag_h = 0,flag_h_move = 0,flag_s = 0,flag_f = 0, flag_up = 0, flag_down = 0, flag_right = 0, flag_left = 0,jump_x,jump_y;
double init_time;
//clockwise = 0;
float e1,e2,e3,t1,t2,t3,u1,u2,u3;
void adjust_view(float e1,float e2, float e3,float t1,float t2,float t3,float u1,float u2,float u3)
{
	::e1 = e1;
	::e2 = e2;
	::e3 = e3;
	::t1 = t1;
	::t2 = t2;
	::t3 = t3;
	::u1 = u1;
	::u2 = u2;
	::u3 = u3;
}

VAO* rectangle;

void createRectangle (GLuint textureID)
{
	// GL3 accepts only Triangles. Quads are not supported
	static const GLfloat vertex_buffer_data [] = {
		-100,-100,-5, // vertex 1
		100,-100,-5, // vertex 2
		100, 100,-5, // vertex 3

		100, 100,-5, // vertex 3
		-100, 100,-5, // vertex 4
		-100,-100,-5  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		1,0,0, // color 1
		0,0,1, // color 2
		0,1,0, // color 3

		0,1,0, // color 3
		0.3,0.3,0.3, // color 4
		1,0,0  // color 1
	};

	// Texture coordinates start with (0,0) at top left of the image to (1,1) at bot right
	static const GLfloat texture_buffer_data [] = {
		0,1, // TexCoord 1 - bot left
		1,1, // TexCoord 2 - bot right
		1,0, // TexCoord 3 - top right

		1,0, // TexCoord 3 - top right
		0,0, // TexCoord 4 - top left
		0,1  // TexCoord 1 - bot left
	};

	// create3DTexturedObject creates and returns a handle to a VAO that can be used later
	rectangle = create3DTexturedObject(GL_TRIANGLES, 6, vertex_buffer_data, texture_buffer_data, textureID, GL_FILL);
}

float camera_rotation_angle;
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Function is called first on GLFW_PRESS.
	if (action == GLFW_RELEASE && global_flag) {
		switch (key) {
			case GLFW_KEY_B:
				break;
			case GLFW_KEY_F:
				flag_f = 0;
				break;
			case GLFW_KEY_S:
				flag_s = 0;
				break;
			default:
				break;
		}
	}
	else if (action == GLFW_PRESS && global_flag) {
		int change,check,i;
		switch (key) {
			case GLFW_KEY_ESCAPE:
				quit(window);
				break;
			case GLFW_KEY_H:
				player_view = 0;
				followcam_view = 0;
				flag_h = 1;
				camera_rotation_angle = 90;
				adjust_view(15*cos(camera_rotation_angle*M_PI/180.0f),15*sin(camera_rotation_angle*M_PI/180.0f),10,5,5,0,0,0,1);
				break;
			case GLFW_KEY_U:
				flag_h = 0;
				player_view = 0;
				followcam_view = 0;
				adjust_view(5,5,8,5,5,0,0,1,0);
				break;
			case GLFW_KEY_T:
				flag_h = 0;
				player_view = 0;
				followcam_view = 0;
				adjust_view(11,-3,11,0,0,0,0,0,1);
				break;
			case GLFW_KEY_P:
				flag_h = 0;
				player_view = 1;
				followcam_view = 0;
				break;
			case GLFW_KEY_C:
				flag_h = 0;
				followcam_view = 1;
				player_view = 0;
				break;
			case GLFW_KEY_F:
				if(!flag && !flag_left && !flag_right && !flag_up && !flag_down)
				flag_f = 1;
				break;
			case GLFW_KEY_S:
				if(!flag && !flag_left && !flag_right && !flag_up && !flag_down)
				flag_s = 1;
				break;
			case GLFW_KEY_SPACE:
				if(!flag && !flag_left && !flag_right && !flag_up && !flag_down)
				flag = 1;
				break;
			case GLFW_KEY_LEFT:
				if(!flag_left && !flag_right && !flag_up && !flag_down && !(flag && (flag_left||flag_right||flag_up||flag_down)))				{
				if(flag)
				{
					if(player.get_x()<=1)
						flag = 0;
					else
					{
						init_time = glfwGetTime();
						flag_left = 1;
						jump_x = (int)player.get_x() - 2;
					}
				}
				else if(player.get_x() > 0)
				{
					posx = (int)player.get_x() - 1;
					flag_left = 1;
				}
				}
				break;
			case GLFW_KEY_RIGHT:
				if(!flag_left && !flag_right && !flag_up && !flag_down && !(flag && (flag_left||flag_right||flag_up||flag_down)))				{
				if(flag)
				{
					if(player.get_x()>=8)
						flag = 0;
					else
					{
						init_time = glfwGetTime();
						flag_right = 1;
						jump_x = (int)player.get_x() + 2;
					}
				}
				else if(player.get_x() < 9)
				{
					posx = (int)player.get_x() + 1;
					flag_right = 1;
				}
				}
				break;
			case GLFW_KEY_UP:
				if(!flag_left && !flag_right && !flag_up && !flag_down && !(flag && (flag_left||flag_right||flag_up||flag_down)))				{
				if(flag)
				{
					if(player.get_y() >= 8)
						flag = 0;
					else
					{
						init_time = glfwGetTime();
						flag_up = 1;
						jump_y = (int)player.get_y() + 2;
					}
				}
				else if(player.get_y() < 9)
				{
					posy = (int)player.get_y() + 1;
					flag_up = 1;
				}
				}
				break;
			case GLFW_KEY_DOWN:
				if(!flag_left && !flag_right && !flag_up && !flag_down && !(flag && (flag_left||flag_right||flag_up||flag_down)))				{
				if(flag)
				{
					if(player.get_y() <= 1)
						flag = 0;
					else
					{
						init_time = glfwGetTime();
						flag_down = 1;
						jump_y = (int)player.get_y() - 2;
					}
				}
				else if(player.get_y() > 0)
				{
					posy = (int)player.get_y() - 1;
					flag_down = 1;
				}
				}
				break;
			default:
				break;
		}
	}
	else if(!global_flag)
	{
		switch(key){
			case GLFW_KEY_ESCAPE:
				quit(window);
				break;
			default:
				break;
		}
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
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
	switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT:
			//            if (action == GLFW_RELEASE)
			//                triangle_rot_dir *= -1;
/*			if (action == GLFW_PRESS)
				flag_h = 1;
			else if (action == GLFW_RELEASE)
			{
				flag_h = 0;
				clockwise = 0;
			}*/
			if(action == GLFW_PRESS)
			{
				flag_h_move = 1;
//				camera_rotation_angle = 90;
			}
			else
				flag_h_move = 0;
			break;

		case GLFW_MOUSE_BUTTON_RIGHT:
			//            if (action == GLFW_RELEASE) {
			//                rectangle_rot_dir *= -1;
			//            }
			break;
		default:
			break;
	}
}


void mouse_scroll(GLFWwindow* window, double x,double y)
{
//	cout<<"m going in\n";
	if(flag_h && global_flag)
	{
		float a1,a2;
		a2 = (float)y/4.0;
		if(a2 > 0)
		{
			if(e1 >= 0)
				e1 -= 0.1;
			else
				e1 += 0.1;
			if(e2 >= 0)
				e2 -= 0.1;
			else
				e2 += 0.1;
			if(e3 >= 0)
				e3 -= 0.1;
			else
				e3 += 0.1;
		}
		else if(a2 < 0)
		{
			if(e1 >= 0)
				e1 += 0.1;
			else
				e1 -= 0.1;
			if(e2 >= 0)
				e2 += 0.1;
			else
				e2 -= 0.1;
			if(e3 >= 0)
				e3 += 0.1;
			else
				e3 -= 0.1;
		}
	}
}

void mouse_move(GLFWwindow* window, double x,double y)
{
//	float a = e1,b = e2;
	if(flag_h && flag_h_move && global_flag)
	{
		float a1 = (float)x/4.0;
//		cout<<"m in\n";
		if(a1 < 0)
			camera_rotation_angle -= 1;
		else if (a1 > 0)
			camera_rotation_angle += 1;
		e1 = 15*cos(camera_rotation_angle*M_PI/180.0f);
		e2 = 15*sin(camera_rotation_angle*M_PI/180.0f);
	}

}

/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
	int fbwidth=width, fbheight=height;
	/* With Retina display on Mac OS X, GLFW's FramebufferSize
	 is different from WindowSize */
	glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	 glLoadIdentity ();
	 gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
	// Perspective projection for 3D views
	 Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.2f, 500.0f);

	// Ortho projection for 2D views
	//Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}

float a = -1,b = -1;
int flag_level = 1;
//double init_time,cur_time;
void randomise(int x,int y)
{
	srand((int)time(0));
	for(int i = 0;i < 10; i++)
	{
		int max,min,ch = 1;
		if( i == 0 )
		{
			max = 10;
			min = 1;
		}
		else if(i == 9)
		{
			max = 9;
			min = 0;
		}
		else
		{
			max = 10;
			min = 0;
		}
		if(flag_level)
		{
			arr1[i] = rand() % (max - min) + min;
			arr4[i] = rand() % 2;
		}
		while(ch)
		{
			arr2[i] = rand() % (max - min) + min;
			arr3[i] = rand() % 2;
			if((arr2[i] == y && i == x)||(arr2[i] == y + 1 && i == x)||(arr2[i] == y - 1 && i == x)||(arr2[i] == y && i == x - 1)||(arr2[i] == y && i == x + 1)||(arr2[i] == y + 1 && i == x + 1)||(arr2[i] == y + 1 && i == x - 1)||(arr2[i] == y - 1 && i == x + 1)||(arr2[i] == y - 1 && i == x + 1));
			else
				ch = 0;

		}
	}
}


/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw ()
{
	// clear the color and depth in the frame buffer
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// use the loaded shader program
	// Don't change unless you know what you are doing
	if(player_view)
		adjust_view(player.get_x(),player.get_y(),player.get_z()+3,player.get_x(),player.get_y()+3,player.get_z()+2,0,0,1);
	else if(followcam_view)
		adjust_view(player.get_x(),player.get_y()-1,player.get_z()+3,player.get_x(),player.get_y()+3,player.get_z(),0,0,1);
	glUseProgram (programID);
	glm::vec3 eye(e1,e2,e3);
	glm::vec3 target(t1,t2,t3);
	glm::vec3 up(u1,u2,u3);

	Matrices.view = glm::lookAt( eye, target, up );
	glm::mat4 VP = Matrices.projection * Matrices.view;


	glm::mat4 MVP;	// MVP = Projection * View * Model
	if(global_flag){
		int change,check;
		if(flag_f && player.get_a() < 1)
			player.set_a(player.get_a()+0.001);
		if(flag_s && player.get_a() > 0.01)
			player.set_a(player.get_a()-0.001);
		if(flag)
		{
			double t = glfwGetTime() - init_time;
			if(flag_left == 1)
			{
//				cout<<"left"<<endl;
				if(player.get_x() - jump_x > 2*player.get_a()*t)
				{
					player.jump('l',t);
				
					change = player.check_void(jump_x,(int)player.get_y());
					check = player.check_collision(jump_x,(int)player.get_y(),'l');
					if(change || check)
					{
						player.set_lives(player.get_lives()-1);
						player.set_x(0);
						player.set_y(0);
						player.set_z(0);
						cout<<"Lives Remaining = "<<player.get_lives()<<endl;
						flag = 0;
						flag_left = 0;
					}
				}
				else
				{
					player.set_x(jump_x);
					player.set_z(0);
					flag = 0;
					flag_left = 0;
				}

			}
			else if(flag_right == 1)
			{
//				cout<<"right"<<endl;
				if(jump_x - player.get_x() > 2*player.get_a()*t)
				{
				player.jump('r',t);
				check = player.check_collision(jump_x,(int)player.get_y(),'r');
				change = player.check_void(jump_x,(int)player.get_y());
				if(change || check)
				{
					player.set_lives(player.get_lives()-1);
					player.set_x(0);
					player.set_y(0);
					player.set_z(0);
					cout<<"Lives Remaining = "<<player.get_lives()<<endl;
					flag = 0;
					flag_right = 0;
				}
				}
				else
				{
					player.set_x(jump_x);
					player.set_z(0);
					flag = 0;
					flag_right = 0;
				}
			}
			else if(flag_up == 1)
			{
//				cout<<"up"<<endl;
				if(jump_y - player.get_y() > 2*player.get_a()*t)
				{
					player.jump('f',t);
			
				change = player.check_void((int)player.get_x(),jump_y);
				check = player.check_collision((int)player.get_x(),jump_y,'f');
				if(change || check)
				{
					player.set_lives(player.get_lives()-1);
					player.set_x(0);
					player.set_y(0);
					player.set_z(0);
				        cout<<"Lives Remaining = "<<player.get_lives()<<endl;
					flag = 0;
					flag_up = 0;
				}
				}
				else
				{
					player.set_y(jump_y);
					player.set_z(0);
					flag = 0;
					flag_up = 0;
				}
			}
			else if(flag_down == 1)
			{
//				cout<<"down"<<endl;
				if(player.get_y() - jump_y > 2*player.get_a()*t)
				{
				player.jump('b',t);

				change = player.check_void((int)player.get_x(),jump_y);
				check = player.check_collision((int)player.get_x(),jump_y,'b');
				if(change || check)
				{
					player.set_lives(player.get_lives()-1);
					player.set_x(0);
					player.set_y(0);
					player.set_z(0);
					cout<<"Lives Remaining = "<<player.get_lives()<<endl;
					flag = 0;
					flag_down = 0;
				}
				}
				else
				{
					player.set_y(jump_y);
					player.set_z(0);
					flag = 0;
					flag_down = 0;
				}

			}
		}
		else
		{
			
				if(flag_left == 1)
				{
//						cout<<"l\n";
				if(player.get_x() - posx > player.get_a())
				{
					player.move_left();
			//	cout<<"boo\n";
//					cout<<"a = "<<player.get_a()<<endl;
				change = player.check_void(posx,(int)player.get_y());
				check = player.check_collision(posx,(int)player.get_y(),'l');
				if(change || check)
				{
					player.set_lives(player.get_lives()-1);
					player.set_x(0);
					player.set_y(0);
					player.set_z(0);
					cout<<"Lives Remaining = "<<player.get_lives()<<endl;
					flag_left = 0;
				}
				}
				else
				{
					player.set_x(posx);
					flag_left = 0;
				}
				}

				else if(flag_right == 1)
					{
//						cout<<"r\n";
//						cout<<"a = "<<player.get_a()<<endl;
					if(posx - player.get_x() > player.get_a())
					{
					player.move_right();
//					cout<<"a = "<<player.get_a()<<endl;
				check = player.check_collision(posx,(int)player.get_y(),'r');
				change = player.check_void(posx,(int)player.get_y());
				if(change || check)
				{
					player.set_lives(player.get_lives()-1);
					player.set_x(0);
					player.set_y(0);
					player.set_z(0);
					cout<<"Lives Remaining = "<<player.get_lives()<<endl;
					flag_right = 0;
				}
				}
				else
				{
					player.set_x(posx);
					flag_right = 0;	
				}
				}
					else if(flag_up == 1)
					{
//						cout<<"u\n";
					if(posy - player.get_y() > player.get_a())
					{
					player.move_forward();
//					cout<<"a = "<<player.get_a()<<endl;
				change = player.check_void((int)player.get_x(),posy);
				check = player.check_collision((int)player.get_x(),posy,'f');
				if(change || check)
				{
					player.set_lives(player.get_lives()-1);
					player.set_x(0);
					player.set_y(0);
					player.set_z(0);
				        cout<<"Lives Remaining = "<<player.get_lives()<<endl;
					flag_up = 0;
				}
					}
					else
					{
						player.set_y(posy);
						flag_up = 0;
					}
				}

					else if(flag_down == 1)
					{
//						cout<<"d\n";
					if(player.get_y() - posy > player.get_a())
					{
					player.move_backward();
//					cout<<"a = "<<player.get_a()<<endl;
				change = player.check_void((int)player.get_x(),posy);
				check = player.check_collision((int)player.get_x(),posy,'b');
				if(change || check)
				{
					player.set_lives(player.get_lives()-1);
					player.set_x(0);
					player.set_y(0);
					player.set_z(0);
					cout<<"Lives Remaining = "<<player.get_lives()<<endl;
					flag_down = 0;
				}
				}
				else
				{
//					cout<<"hehe\n";
					player.set_y(posy);
					flag_down = 0;
				}
				}
		}
if(player.check_collision((int)player.get_x(),(int)player.get_y(),'d'))
	{
		player.set_lives(player.get_lives()-1);
		player.set_x(0);
		player.set_y(0);
		player.set_z(0);
                cout<<"Lives Remaining = \n"<<player.get_lives();
	}

	jb = jb + a*0.01;
	jt = jt + b*0.01;
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translate = glm::translate (glm::vec3(player.get_x(),player.get_y(),player.get_z()));
	Matrices.model = translate;
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID,1,GL_FALSE,&MVP[0][0]);
	draw3DObject(player.get_player());
	if(jb <= 1 || jb >= 2.5)
		a = -a;
	if(jt <= -1 || jt >= 1)
		b = -b;
	// Render with texture shaders now
	glUseProgram(textureProgramID);

	// Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
	// glPopMatrix ();
	Matrices.model = glm::mat4(1.0f);

	glm::mat4 translateRectangle = glm::translate (glm::vec3(2, 0, 0));        // glTranslatef
//	glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= translateRectangle;
	MVP = VP * Matrices.model;

	// Copy MVP to texture shaders
	glUniformMatrix4fv(Matrices.TexMatrixID, 1, GL_FALSE, &MVP[0][0]);

	// Set the texture sampler to access Texture0 memory
	glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DTexturedObject(rectangle);
for(int i = 0 ; i < 10 ; i++ )
	{
		for(int j = 0 ; j < 10 ; j++)
		{
			if(j != arr1[i] || j == arr1[i] && arr4[i] == 1)
			{
				// 			Matrices.model = glm::mat4(1.0f);
				glm::mat4 translateCube;
				if(j == arr1[i])
					translateCube = glm::translate (glm::vec3(i,j, jt));
				else
					translateCube = glm::translate (glm::vec3(i,j,0));
				// 			glm::mat4 rotateCube = glm::rotate((float)(cube_rotation*M_PI/180.0f), glm::vec3(1,1,1));
				Matrices.model = translateCube;
				MVP = VP * Matrices.model;
				glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				//  c1.createCube();
				//  VAO* CUBE = c1.getCube();
				//	  		draw3DObject(CUBE[i+j]);
				draw3DTexturedObject(c1.get_cuboid());
			}
		}
	}

	for(int k = 0;k<10;k++)
	{
		for(int i =0;i<=360;i++)
		{
			Matrices.model = glm::mat4(1.0f);
			glm::mat4 translateSphere;
			if(arr3[k] == 0)
				translateSphere = glm::translate (glm::vec3(k,arr2[k],1));
			else
				translateSphere = glm::translate (glm::vec3(k,arr2[k],jb));
			glm::mat4 rotateSphere = glm::rotate((float)(i*M_PI/180.0f), glm::vec3(0,1,0));
			glm::mat4 sphereTransform = (translateSphere*rotateSphere);
			Matrices.model *= sphereTransform;
			MVP = VP * Matrices.model;
			glUniformMatrix4fv(Matrices.MatrixID,1,GL_FALSE,&MVP[0][0]);
			if(arr5[k])
			draw3DObject(obstacle_sphere.get_sphere());
		}
	}
/*
	if(player.check_collision())
	{
		player.set_lives(player.get_lives()-1);
		player.set_x(0);
		player.set_y(0);
		player.set_z(0);
                cout<<"Lives Remaining = \n"<<player.get_lives();
	}

	jb = jb + a*0.01;
	jt = jt + b*0.01;
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translate = glm::translate (glm::vec3(player.get_x(),player.get_y(),player.get_z()));
	Matrices.model = translate;
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID,1,GL_FALSE,&MVP[0][0]);
	draw3DObject(player.get_player());
	if(jb <= 1 || jb >= 2.5)
		a = -a;
	if(jt <= -1 || jt >= 1)
		b = -b;*/
	//  if(flag == 1)
	// 	 camera_rotation_angle -= 1;

	// Render font on screen
	static int fontScale = 0;
	float fontScaleValue = 0.75 + 0.25*sinf(fontScale*M_PI/180.0f);
	glm::vec3 fontColor = getRGBfromHue (fontScale);



	// Use font Shaders for next part of code
	glUseProgram(fontProgramID);
	Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	// Transform the text
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translateText = glm::translate(glm::vec3(-6,4,0));
//	glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
	Matrices.model *= (translateText /* scaleText*/);
	MVP = Matrices.projection * Matrices.view * Matrices.model;
	// send font's MVP and font color to fond shaders
	glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

	// Render font
	GL3Font.font->Render("SCORE = ");
//	GL3Font.font->Render(player.get_score());
	char is[80]; /*= itoa(player.get_score());*/
	sprintf(is,"%d",player.get_score());
	Matrices.model = glm::mat4(1.0f);
	translateText = glm::translate(glm::vec3(-3.5,4,0));
//	glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
	Matrices.model *= (translateText /* scaleText*/);
	MVP = Matrices.projection * Matrices.view * Matrices.model;
	// send font's MVP and font color to fond shaders
	glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);
	GL3Font.font->Render(is);

	sprintf(is,"%d",player.get_score());
	Matrices.model = glm::mat4(1.0f);
	translateText = glm::translate(glm::vec3(3,4,0));
//	glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
	Matrices.model *= (translateText /* scaleText*/);
	MVP = Matrices.projection * Matrices.view * Matrices.model;
	// send font's MVP and font color to fond shaders
	glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);
	GL3Font.font->Render("LEVEL = ");
	
	sprintf(is,"%d",flag_level);
	Matrices.model = glm::mat4(1.0f);
	translateText = glm::translate(glm::vec3(5.5,4,0));
//	glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
	Matrices.model *= (translateText /* scaleText*/);
	MVP = Matrices.projection * Matrices.view * Matrices.model;
	// send font's MVP and font color to fond shaders
	glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);
	GL3Font.font->Render(is);

	Matrices.model = glm::mat4(1.0f);
	translateText = glm::translate(glm::vec3(-6,3,0));
//	glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
	Matrices.model *= (translateText /* scaleText*/);
	MVP = Matrices.projection * Matrices.view * Matrices.model;
	// send font's MVP and font color to fond shaders
	glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);
	GL3Font.font->Render("LIVES = ");

	sprintf(is,"%d",player.get_lives());
	Matrices.model = glm::mat4(1.0f);
	translateText = glm::translate(glm::vec3(-3.5,3,0));
//	glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
	Matrices.model *= (translateText /* scaleText*/);
	MVP = Matrices.projection * Matrices.view * Matrices.model;
	// send font's MVP and font color to fond shaders
	glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);
	GL3Font.font->Render(is);
	}
	else{
	char is[80]; 
	sprintf(is,"%d",player.get_score());
	static int fontScale = 0;
	float fontScaleValue = 0.75 + 0.25*sinf(fontScale*M_PI/180.0f);
	glm::vec3 fontColor = getRGBfromHue (fontScale);

	glUseProgram(fontProgramID);
	Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translateText = glm::translate(glm::vec3(-2,2,0));
	glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
	Matrices.model *= (translateText *scaleText);
	MVP = Matrices.projection * Matrices.view * Matrices.model;
	// send font's MVP and font color to fond shaders
	glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);
	if(win)
		GL3Font.font->Render("YOU WIN!");

	else 
		GL3Font.font->Render("YOU LOSE!");
//	GL3Font.font->Render(player.get_lives());
//	GL3Font.font->Render(player.get_lives());

	Matrices.model = glm::mat4(1.0f);
	translateText = glm::translate(glm::vec3(-5,0,0));
//	glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
	Matrices.model *= (translateText /* scaleText*/);
	MVP = Matrices.projection * Matrices.view * Matrices.model;
	// send font's MVP and font color to fond shaders
	glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);
	GL3Font.font->Render("SCORE = ");

	Matrices.model = glm::mat4(1.0f);
	translateText = glm::translate(glm::vec3(-2.5,0,0));
//	glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
	Matrices.model *= (translateText /* scaleText*/);
	MVP = Matrices.projection * Matrices.view * Matrices.model;
	// send font's MVP and font color to fond shaders
	glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);
	GL3Font.font->Render(is);
	
	//camera_rotation_angle++; // Simulating camera rotation
	fontScale = (fontScale + 1) % 360;}
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
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
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	glfwSwapInterval( 1 );

	/* --- register callbacks with GLFW --- */

	/* Register function to handle window resizes */
	/* With Retina display on Mac OS X GLFW's FramebufferSize
	 is different from WindowSize */
	glfwSetFramebufferSizeCallback(window, reshapeWindow);
	glfwSetWindowSizeCallback(window, reshapeWindow);

	/* Register function to handle window close */
	glfwSetWindowCloseCallback(window, quit);

	/* Register function to handle keyboard input */
	glfwSetKeyCallback(window, keyboard);      // general keyboard input
	glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

	/* Register function to handle mouse click */
	glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
	glfwSetScrollCallback(window,mouse_scroll);
	glfwSetCursorPosCallback(window, mouse_move);
	return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
	// Load Textures
	// Enable Texture0 as current texture memory
	glActiveTexture(GL_TEXTURE0);
	// load an image file directly as a new OpenGL texture
	// GLuint texID = SOIL_load_OGL_texture ("beach.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS); // Buggy for OpenGL3
	GLuint textureID = createTexture("crate.jpg");
	GLuint textureID1 = createTexture("im.jpg");
	// check for an error during the load process
	if(textureID == 0 )
		cout << "SOIL loading error: '" << SOIL_last_result() << "'" << endl;

	// Create and compile our GLSL program from the texture shaders
	textureProgramID = LoadShaders( "TextureRender.vert", "TextureRender.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.TexMatrixID = glGetUniformLocation(textureProgramID, "MVP");


	/* Objects should be created before any other gl function and shaders */
	// Create the models
// Generate the VAO, VBOs, vertices data & copy into the array buffer
	createRectangle (textureID1);
float v[24] = {-0.5f,-0.5f,-10.0f,-0.5f,-0.5f,0.5f,-0.5f,0.5f,0.5f,0.5f,0.5f,-10.0f,-0.5f,0.5f,-10.0f,0.5f,-0.5f,0.5f,0.5f,-0.5f,-10.0f,0.5f,0.5f,0.5f};
	c1.set_cuboid_vertices(v);
	c1.create_cuboid(textureID);
	GLfloat c[3] = {1,0,1};
	obstacle_sphere.set_radius(0.3);
	obstacle_sphere.createSphere(obstacle_sphere.get_radius(),c);
	// Create and compile our GLSL program from the shaders
	for(int i = 0;i < 24; i++)
	{
		float ver[24] = {-0.5f,-0.5f,0.5f,-0.5f,-0.5f,1.5f,-0.5f,0.5f,1.5f,0.5f,0.5f,0.5f,-0.5f,0.5f,0.5f,0.5f,-0.5f,1.5f,0.5f,-0.5f,0.5f,0.5f,0.5f,1.5f};
		v[i] = ver[i];
	}
/*	GLuint textureID1 = createTexture("image2.jpg");
	if(textureID1 == 0)
		cout<<"SOIL loading error: '" << SOIL_last_result() << "'" << endl;

	textureProgramID = LoadShaders( "TextureRender.vert", "TextureRender.frag" );
	Matrices.TexMatrixID = glGetUniformLocation(textureProgramID, "MVP");*/

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL3.vert", "Sample_GL3.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	player.set_player_vertices(v);
	player.create_player();

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL3.vert", "Sample_GL3.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


	reshapeWindow (window, width, height);

	// Background color of the scene
	glClearColor (1.0f, 0.9f, 1.0f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LESS);
//	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Initialise FTGL stuff
	const char* fontfile = "arial.ttf";
	GL3Font.font = new FTExtrudeFont(fontfile); // 3D extrude style rendering

	if(GL3Font.font->Error())
	{
		cout << "Error: Could not load font `" << fontfile << "'" << endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Create and compile our GLSL program from the font shaders
	fontProgramID = LoadShaders( "fontrender.vert", "fontrender.frag" );
	GLint fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform;
	fontVertexCoordAttrib = glGetAttribLocation(fontProgramID, "vertexPosition");
	fontVertexNormalAttrib = glGetAttribLocation(fontProgramID, "vertexNormal");
	fontVertexOffsetUniform = glGetUniformLocation(fontProgramID, "pen");
	GL3Font.fontMatrixID = glGetUniformLocation(fontProgramID, "MVP");
	GL3Font.fontColorID = glGetUniformLocation(fontProgramID, "fontColor");

	GL3Font.font->ShaderLocations(fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform);
	GL3Font.font->FaceSize(1);
	GL3Font.font->Depth(0);
	GL3Font.font->Outset(0, 0);
	GL3Font.font->CharMap(ft_encoding_unicode);

	cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

}

int main (int argc, char** argv)
{
int width = 1000;
	int height = 1000;

	GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

	double last_update_time = glfwGetTime(), current_time;
/*
	srand((int)time(0));
	for(int i = 0;i < 10; i++)
	{
		int max,min;
		if( i == 0 )
		{
			max = 10;
			min = 1;
		}
		else if(i == 9)
		{
			max = 9;
			min = 0;
		}
		else
		{
			max = 10;
			min = 0;
		}
		arr1[i] = rand() % (max - min) + min;
		arr2[i] = rand() % (max - min) + min;
		arr3[i] = rand() % 2;
		arr4[i] = rand() % 2;
	}
	*/
	randomise(0,0);
	set();
	player.set_lives(10);
	player.set_score(100);
	player.set_x(0);
	player.set_y(0);
	player.set_z(0);
	player.set_a(0.01);
	/* Draw in loop */
	int lives = player.get_lives();
	cout<<"Lives Remaining = "<<lives<<endl;
	cout<<"Score = 100"<<endl;
	adjust_view(5,5,8,5,5,0,0,1,0);
	while (!glfwWindowShouldClose(window)) {
		// OpenGL Draw commands
		draw();

		// Swap Frame Buffer in double buffering
		glfwSwapBuffers(window);

		// Poll for Keyboard and mouse events
		glfwPollEvents();
		if(lives == 0 || (win && flag_level == 3)) 
			global_flag = 0;
		else if(win)
		{
			win = 0;
			randomise(0,0);
			set();
			player.set_x(0);
			player.set_y(0);
			player.set_z(0);
			player.set_a(0.01);
			flag_level ++;
		}

		// Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
		current_time = glfwGetTime(); // Time in seconds
		if ((current_time - last_update_time) >= 10) { // atleast 0.5s elapsed since last frame

			randomise(player.get_x(),player.get_y());

			last_update_time = current_time;
		}
		lives = player.get_lives();
		if(player.get_x() == 9 && player.get_y() == 9)
			win = 1;
	}
	if(win == 0)
		cout<<"YOU LOSE!"<<endl;
	else
		cout<<"YOU WIN!"<<endl;
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
