// Jeremy Bridges
// CS 330
// 12/1/2019


#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//soil library
#include<SOIL2/SOIL2.h>

using namespace std;

int width, height;
const double PI = 3.14159;
const float toRadians = PI / 180.0f;

//input function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

//declare view matrix
glm::mat4 viewMatrix;

//init FOV
GLfloat fov = 45.0f;

//define cmaera attribs
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraDirection = glm::normalize(cameraPosition - target);
glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));
glm::vec3 cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight));
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

// declare target prototype
glm::vec3 getTarget();

// camera transform prototype
void TransformCamera();

// boolean array for keys and mouse buttons
bool keys[1024], mouseButtons[3];


// boolean to check camera transform
bool isPanning = false, isOrbiting = false;

// radius pitch and yaw
GLfloat radius = 3.f, rawYaw = 0.f, rawPitch = 0.f, degYaw, degPitch;

// variable to normalize camera actions
GLfloat deltaTime = 0.0f, lastFrame = 0.0f;
GLfloat lastX = 400, lastY = 300, xChange, yChange;

bool firstMouseMove = true;  //used to detect first mouse movement

// lightsource position
glm::vec3 lightPosition(0.0f, 0.25f, 0.f);

void initCamera();

// Draw Primitive(s)
void draw()
{
	GLenum mode = GL_TRIANGLES;
	GLsizei indices = 6;
	glDrawElements(mode, indices, GL_UNSIGNED_BYTE, nullptr);
}

// Create and Compile Shaders
static GLuint CompileShader(const string& source, GLuint shaderType)
{
	// Create Shader object
	GLuint shaderID = glCreateShader(shaderType);
	const char* src = source.c_str();

	// Attach source code to Shader object
	glShaderSource(shaderID, 1, &src, nullptr);

	// Compile Shader
	glCompileShader(shaderID);

	// Return ID of Compiled shader
	return shaderID;

}

// Create Program Object
static GLuint CreateShaderProgram(const string& vertexShader, const string& fragmentShader)
{
	// Compile vertex shader
	GLuint vertexShaderComp = CompileShader(vertexShader, GL_VERTEX_SHADER);

	// Compile fragment shader
	GLuint fragmentShaderComp = CompileShader(fragmentShader, GL_FRAGMENT_SHADER);

	// Create program object
	GLuint shaderProgram = glCreateProgram();

	// Attach vertex and fragment shaders to program object
	glAttachShader(shaderProgram, vertexShaderComp);
	glAttachShader(shaderProgram, fragmentShaderComp);

	// Link shaders to create executable
	glLinkProgram(shaderProgram);

	// Delete compiled vertex and fragment shaders
	glDeleteShader(vertexShaderComp);
	glDeleteShader(fragmentShaderComp);

	// Return Shader Program
	return shaderProgram;

}


int main(void)
{
	width = 800; height = 600;

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(width, height, "Main Window", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	//set input callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);


	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK)
		cout << "Error!" << endl;

	GLfloat lampVertices[] = {
		-0.5, -0.5, 0.0,	// index 0
		-0.5, 0.5, 0.0,	// index 1
		0.5, -0.5, 0.0,	// index 2
		0.5, 0.5, 0.0,		// index 3
};
	
	GLfloat vertices[] = {

		// Triangle 1
		-0.5, -0.5, 0.0,	// index 0
		0.6, 0.3, 0.0,		// brown
		0.0, 0.0,			//UV(bl)
		0.0, 0.0, 1.0,		// normal positive z

		0.5, -0.5, 0.0,	// index 1
		0.6, 0.3, 0.0,		// brown
		1.0, 0.0,			// UV(br)
		0.0, 0.0, 1.0,		// normal positive z

		-0.5, 0.5, 0.0,	// index 2	
		0.6, 0.3, 0.0,		// brown
		0.0, 1.0,			// UV(tl)
		0.0, 0.0, 1.0,		// normal positive z

		// Triangle 2	
		0.5, 0.5, 0.0,		// index 3	
		0.6, 0.3, 0.0,		// brown
		1.0, 1.0,			// UV(tr)
		0.0, 0.0, 1.0		// normal positive z
	};

	// Define element indices
	GLubyte indices[] = {
		0, 1, 2,
		1, 2, 3
	};

// Leg(plane) transforms
	glm::vec3 planePositions[] = {
		glm::vec3(0.5f,  0.0f,  0.25f),	// front plane
		glm::vec3(0.45f,  0.0f,  0.2f),	// right plane
		glm::vec3(0.5f,  0.0f,  0.15f),	// back plane
		glm::vec3(0.55f,  0.0f,  0.2f)	// left plane
	};

	glm::float32 planeRotations[] = {
		0.0f, 90.0f, 180.0f, 270.0f
	};
	
	glm::float32 planeRotations2[] = {
		180.0f, 90.0f, 0.0f, 270.0f
	};

// Seat tranforms
	glm::vec3 seatTranslation[] = {
		glm::vec3(0.f, 0.85f, 0.f), // top of seat
		glm::vec3(0.f, 0.75f,  0.f), // bottom of seat
		glm::vec3(0.f, 0.80f, -0.25f), // back of seat
		glm::vec3(0.f, 0.80f, 0.25f), // front of seat
		glm::vec3(-0.625f, 0.80f, 0.0f), // left side of seat
		glm::vec3(0.625f, 0.80f, 0.0f), // right of seat
	};

	glm::float32 seatAngle1[] = {
		270.0f, 90.0f, 180.0f, 0.0f, 270.0f, 90.0f
	};

	glm::vec3 seatRotation1[] = {
		glm::vec3(0.f, 1.f, 0.f), // top of seat
		glm::vec3(0.f, 1.f, 0.f), // bottom of seat
		glm::vec3(0.f, 0.f, 1.f), // back of seat
		glm::vec3(0.f, 0.f, 1.f), // front of seat
		glm::vec3(1.f, 0.f, 0.f), // left side of seat
		glm::vec3(1.f, 0.f, 0.f), // right of seat
	};

	glm::vec3 seatRotation2[] = {
		glm::vec3(1.f, 0.f, 0.f), // top of seat
		glm::vec3(1.f, 0.f, 0.f), // bottom of seat
		glm::vec3(1.f, 0.f, 0.f), // back of seat
		glm::vec3(1.f, 0.f, 0.f), // front of seat
		glm::vec3(0.f, 1.f, 0.f), // left side of seat
		glm::vec3(0.f, 1.f, 0.f), // right of seat
	};

	glm::vec3 seatScale[] = {
		glm::vec3(.5f, 1.25f, 1.f), // top of seat
		glm::vec3(.5f, 1.25f, 1.f), // bottom of seat
		glm::vec3(.1f, 1.25f, 1.f), // back of seat
		glm::vec3(.1f, 1.25f, 1.f), // front of seat
		glm::vec3(.1f, .5f, 1.f), // left side of seat
		glm::vec3(.1f, .5f, 1.f), // right of seat
	};

// Foot transforms
	glm::float32 footRotationsY[] = {
		0.0, 180.0, 90.0, 270.0
	};

	glm::vec3 footTranslation1[] = {
		glm::vec3(0.0f, 0.7f, 0.25f),  //front
		glm::vec3(0.0f, 0.7f, 0.15f),	//back	180
		glm::vec3(0.0f, 0.75f, 0.2f),	//top
		glm::vec3(0.0f, 0.65f,  0.2f),	//bottom
	};

	glm::vec3 footTranslation2[] = {
		glm::vec3(0.0f, -0.3f, 0.25f),  //front
		glm::vec3(0.0f, -0.3f, 0.15f),	//back	180
		glm::vec3(0.0f, -0.25f, 0.2f),	//top
		glm::vec3(0.0f, -0.35f, 0.2f),	//bottom
	};

	// Setup some OpenGL options
	glEnable(GL_DEPTH_TEST);

	// Wireframe mode
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	GLuint legVBO, legEBO, legVAO, seatVBO, seatEBO, seatVAO, footVBO, footEBO, footVAO, lampVBO, lampEBO, lampVAO;

	glGenBuffers(1, &legVBO); // Create VBO
	glGenBuffers(1, &legEBO); // Create EBO

	glGenBuffers(1, &seatVBO); // Create VBO
	glGenBuffers(1, &seatEBO); // Create EBO

	glGenBuffers(1, &footVBO); // Create VBO
	glGenBuffers(1, &footEBO); // Create EBO

	glGenBuffers(1, &lampVBO); // Create VBO
	glGenBuffers(1, &lampEBO); // Create EBO

	glGenVertexArrays(1, &legVAO); // Create VAO
	glGenVertexArrays(1, &seatVAO); // Create VAO
	glGenVertexArrays(1, &footVAO); // Create VAO
	glGenVertexArrays(1, &lampVAO); // Create VAO

	//bind user defined VAO
	glBindVertexArray(legVAO);  
		// VBO and EBO Placed in User-Defined VAO
		glBindBuffer(GL_ARRAY_BUFFER, legVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, legEBO); // Select EBO
	
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // Load indices 
																							 // Specify attribute location and layout to GPU
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11* sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
		glEnableVertexAttribArray(3);
	glBindVertexArray(0); // Unbind VOA or close off (Must call VOA explicitly in loop)

	//bind user defined VAO
	glBindVertexArray(seatVAO);
		// VBO and EBO Placed in User-Defined VAO
		glBindBuffer(GL_ARRAY_BUFFER, seatVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, seatEBO); // Select EBO

		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // Load indices 
																							 // Specify attribute location and layout to GPU
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
		glEnableVertexAttribArray(3);
	glBindVertexArray(0); // Unbind VOA or close off (Must call VOA explicitly in loop)

	//bind user defined VAO
	glBindVertexArray(footVAO);
		// VBO and EBO Placed in User-Defined VAO
		glBindBuffer(GL_ARRAY_BUFFER, footVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, footEBO); // Select EBO
	
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // Load indices 
																							 // Specify attribute location and layout to GPU
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
		glEnableVertexAttribArray(3);
	glBindVertexArray(0); // Unbind VOA or close off (Must call VOA explicitly in loop)

	glBindVertexArray(lampVAO);
		// VBO and EBO Placed in User-Defined VAO
		glBindBuffer(GL_ARRAY_BUFFER, lampVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lampEBO); // Select EBO

		glBufferData(GL_ARRAY_BUFFER, sizeof(lampVertices), lampVertices, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // Load indices 
		// Specify attribute location and layout to GPU
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);
	glBindVertexArray(0); // Unbind VOA or close off (Must call VOA explicitly in loop)

	// load textures
	int stoolTexWidth, stoolTexHeight;
	unsigned char* stoolImage = SOIL_load_image("stool2.jpg", &stoolTexWidth, &stoolTexHeight, 0, SOIL_LOAD_RGB);

	// Generate textures
	GLuint stoolTexture;
	glGenTextures(1, &stoolTexture);
	glBindTexture(GL_TEXTURE_2D, stoolTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, stoolTexWidth, stoolTexHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, stoolImage);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(stoolImage);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Vertex shader source code
	string vertexShaderSource =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vPosition;"
		"layout(location = 1) in vec3 aColor;"
		"layout(location = 2) in vec2 texCoord;"
		"layout(location = 3) in vec3 normal;"
		"out vec3 oColor;"
		"out vec2 oTexCoord;"
		"out vec3 oNormal;"
		"out vec3 fragPos;"
		"uniform mat4 model;"
		"uniform mat4 view;"
		"uniform mat4 projection;"
		"void main()\n"
		"{\n"
		"gl_Position = projection * view * model * vec4(vPosition.x, vPosition.y, vPosition.z, 1.0);"
		"oColor = aColor;"
		"oTexCoord = vec2(texCoord.x, 1.0f - texCoord.y);"
		"oNormal = mat3(transpose(inverse(model))) * normal;"
		"fragPos = vec3(model * vec4(vPosition, 1.0));"
		"}\n";

	// Fragment shader source code
	string fragmentShaderSource =
		"#version 330 core\n"
		"in vec3 oColor;"
		"in vec2 oTexCoord;"
		"in vec3 oNormal;"
		"in vec3 fragPos;"
		"out vec4 fragColor;"
		"uniform sampler2D myTexture;"
		"uniform vec3 objectColor;"
		"uniform vec3 lightColor;"
		"uniform vec3 lightPos;"
		"uniform vec3 viewPos;"
		"void main()\n"
		"{\n"
		"// ambient \n "
		"float ambientStrength = 0.4f;"
		"vec3 ambient = ambientStrength * lightColor;"

		"// diffuse \n "
		"vec3 norm = normalize(oNormal);"
		"vec3 lightDir = normalize(lightPos - fragPos);"
		"float diff = max(dot(norm, lightDir), 0.0);"
		"vec3 diffuse = diff * lightColor;"

		"// specularity \n "
		"float specularStrength = 5.5;"
		"vec3 viewDir = normalize(viewPos - fragPos);"
		"vec3 reflectDir = reflect(-lightDir, norm);"
		"float spec = pow(max(dot(viewDir, reflectDir), 0.0), 256);"
		"vec3 specular = specularStrength * spec * lightColor;"

		"vec3 result = (ambient + diffuse + specular) * objectColor;"
		"fragColor = texture(myTexture, oTexCoord) * vec4(result, 1.0f);"
		"}\n";

	// lamp vertex shader source code
	string lampVertexShaderSource =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vPosition;"
		"uniform mat4 model;"
		"uniform mat4 view;"
		"uniform mat4 projection;"
		"void main()\n"
		"{\n"
		"gl_Position = projection * view * model * vec4(vPosition.x, vPosition.y, vPosition.z, 1.0);"
		"}\n";

	// lamp Fragment shader source code
	string lampFragmentShaderSource =
		"#version 330 core\n"
		"out vec4 fragColor;"
		"void main()\n"
		"{\n"
		"// color of light object \n"
		"fragColor = vec4(1.f);"
		"}\n";

	// Creating Shader Program
	GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
	GLuint lampShaderProgram = CreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		// set delta Time
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		
		// Resize window and graphics simultaneously
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use Shader Program exe and select VAO before drawing 
		glUseProgram(shaderProgram); // Call Shader per-frame when updating attributes


		// Declare transformations (can be initialized outside loop)		
		glm::mat4 projectionMatrix;

		viewMatrix = glm::lookAt(cameraPosition, getTarget(), worldUp);
		projectionMatrix = glm::perspective(fov, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);

		// Get matrix's uniform location and set matrix
		GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
		GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
		GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

		// Get light and object color and light position location
		GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
		GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
		GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
		GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");

		// Assign Colors
		glUniform3f(objectColorLoc, 0.6f, 0.33f, 0.26f);
		glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);

		// set light position
		glUniform3f(lightPosLoc, lightPosition.x, lightPosition.y, lightPosition.z);

		// specify view position
		glUniform3f(viewPosLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		// bind stool texture
		glBindTexture(GL_TEXTURE_2D, stoolTexture);

		//DRAW front left leg
		glBindVertexArray(legVAO); // User-defined VAO must be called before draw. 
			for (GLuint i = 0; i < 4; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, planePositions[i] * glm::vec3(-1.0, 1.0, 1.0));
				modelMatrix = glm::rotate(modelMatrix, planeRotations[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, 1.5f, 1.f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				// Draw primitive(s)
				draw();
			}

			// DRAW front right leg
			for (GLuint i = 0; i < 4; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, planePositions[i] * glm::vec3(1.0, 1.0, 1.0));
				modelMatrix = glm::rotate(modelMatrix, planeRotations[i] * toRadians, glm::vec3(0.0f, -1.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, 1.5f, 1.f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				// Draw primitive(s)
				draw();
			}
			// DRAW back left leg
			for (GLuint i = 0; i < 4; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, planePositions[i] * glm::vec3(-1.0, 1.0, -1.0));
				modelMatrix = glm::rotate(modelMatrix, planeRotations2[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, 1.5f, 1.f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				// Draw primitive(s)
				draw();
			}
			// DRAW back right leg
			for (GLuint i = 0; i < 4; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, planePositions[i] * glm::vec3(1.0, 1.0, -1.0));
				modelMatrix = glm::rotate(modelMatrix, planeRotations2[i] * toRadians, glm::vec3(0.0f, -1.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, 1.5f, 1.f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				// Draw primitive(s)
				draw();
			}
		glBindVertexArray(0); //Incase different VAO wii be used after

		// Select and transform seat objects
		glBindVertexArray(seatVAO);
			for (GLuint i = 0; i < 6; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, seatTranslation[i]);
				modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, seatRotation1[i]);
				modelMatrix = glm::rotate(modelMatrix, seatAngle1[i] * toRadians, seatRotation2[i]);
				modelMatrix = glm::scale(modelMatrix, seatScale[i]);
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				draw();
			}
		glBindVertexArray(0); //Incase different VAO will be used after

		//select and transform foot rests
			glBindVertexArray(footVAO);
			// top beneath seat
			for (GLuint i = 0; i < 4; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, footTranslation1[i]);
				modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(0.0f, 0.0f, 1.f));
				modelMatrix = glm::rotate(modelMatrix, footRotationsY[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, .9f, 1.f));
				
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				draw();
			}
			
			// front footer
			for (GLuint i = 0; i < 4; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, footTranslation2[i]* glm::vec3(1.0, 1.0, 1.0));
				modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(0.0f, 0.0f, 1.f));
				modelMatrix = glm::rotate(modelMatrix, footRotationsY[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, .9f, 1.f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				draw();
			}

			// back beneath seat
			for (GLuint i = 0; i < 4; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, footTranslation1[i] * glm::vec3(1.0, 1.0, -1.0));
				modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(0.0f, 0.0f, 1.f));
//				modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(0.0f, 1.0f, 0.f));
				modelMatrix = glm::rotate(modelMatrix, footRotationsY[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, .9f, 1.f));
				// shadow not proper in this transform *******new rotation vec??
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				draw();
			}
/*
			// back footer
			for (GLuint i = 0; i < 4; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, footTranslation[i] * glm::vec3(1.0, -1.0, -1.0));
				modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(0.0f, 0.0f, 1.f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.75f, 0.72f, 1.f));
				if (i >= 2) {
					modelMatrix = glm::rotate(modelMatrix, 90.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.f));
				}
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				draw();
			}

			// left side beneath seat
			for (GLuint i = 0; i < 4; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, footTranslation2[i] * glm::vec3(1.0, 1.0, 1.0));
				modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(0.0f, 0.0f, 1.f));
				modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(1.0f, 0.0f, 0.f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.75f, 0.24f, 1.f));
				if (i >= 2) {
					modelMatrix = glm::rotate(modelMatrix, 90.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.f));
				}
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				draw();
			}

			// right side beneath seat
			for (GLuint i = 0; i < 4; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, footTranslation2[i] * glm::vec3(-1.0, 1.0, 1.0));
				modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(0.0f, 0.0f, 1.f));
				modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(1.0f, 0.0f, 0.f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.75f, 0.24f, 1.f));
				if (i >= 2) {
					modelMatrix = glm::rotate(modelMatrix, 90.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.f));
				}
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				draw();
			}
			
			// left side footer
			for (GLuint i = 0; i < 4; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, (footTranslation2[i] + glm::vec3(0.0, 0.1, 0.0)) * glm::vec3(-1.0, -1.0, 1.0));
				modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(0.0f, 0.0f, 1.f));
				modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(1.0f, 0.0f, 0.f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.75f, 0.24f, 1.f));
				if (i >= 2) {
					modelMatrix = glm::rotate(modelMatrix, 90.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.f));
				}
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				draw();
			}

			// right side footer
			for (GLuint i = 0; i < 4; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, (footTranslation2[i] + glm::vec3(0.0, 0.1, 0.0)) * glm::vec3(1.0, -1.0, 1.0));
				modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(0.0f, 0.0f, 1.f));
				modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(1.0f, 0.0f, 0.f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.75f, 0.24f, 1.f));
				if (i >= 2) {
					modelMatrix = glm::rotate(modelMatrix, 90.0f * toRadians, glm::vec3(0.0f, 1.0f, 0.f));
				}
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				draw();
			}

		glBindVertexArray(0); //Incase different VAO will be used after
		glUseProgram(0); // Incase different shader will be used after

*/
		glUseProgram(lampShaderProgram);
			// Get matrix's uniform location and set matrix
			GLint lampModelLoc = glGetUniformLocation(lampShaderProgram, "model");
			GLint lampViewLoc = glGetUniformLocation(lampShaderProgram, "view");
			GLint lampProjLoc = glGetUniformLocation(lampShaderProgram, "projection");

			glUniformMatrix4fv(lampViewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
			glUniformMatrix4fv(lampProjLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
			glBindVertexArray(lampVAO); // User-defined VAO must be called before draw. 
		
		
			for (GLuint i = 0; i < 4; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, planePositions[i] / glm::vec3(8., 8., 8.) + lightPosition);
				modelMatrix = glm::rotate(modelMatrix, planeRotations[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(.125f, .125f, .125f));
				glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

				// Draw primitive(s)
				draw();
			}
		
			glBindVertexArray(0); //Incase different VAO wii be used after
		glUseProgram(0); // Incase different shader will be used after

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		// poll camera transforms
		TransformCamera();
	}

	//Clear GPU resources
	glDeleteVertexArrays(1, &legVAO);
	glDeleteBuffers(1, &legVBO);
	glDeleteBuffers(1, &legEBO);


	glfwTerminate();
	return 0;
}

// define input callback functions
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	//display ASCII keycode
//	cout << "ASCII: " << key << endl;

	if (action == GLFW_PRESS)
		keys[key] = true;
	else if (action == GLFW_RELEASE)
		keys[key] = false;

}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {

	/*
	//dispaly scroll offset
	if (yoffset > 0)
		cout << "scroll up: ";
	if (yoffset < 0)
		cout << "scroll down: ";
	cout << yoffset << endl;
	*/

	// clamp FOV
	if (fov >= 1.f && fov <= 45.f)
		fov -= yoffset * 0.01f;

	//default the FOV
	if (fov < 1.f)
		fov = 1.f;
	if (fov > 45.f)
		fov = 45.f;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

	// checks for mouse button pressed
	if (action == GLFW_PRESS)
		mouseButtons[button] = true;
	else if (action == GLFW_RELEASE)
		mouseButtons[button] = false;

}
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {

	//display mouse x y
	//cout << "Mouse X: " << xpos << endl;
	//cout << "Mouse Y: " << ypos << endl;


	if (firstMouseMove) {
		lastX = xpos;
		lastY = ypos;
		firstMouseMove = false;
	}

	//mouse cursor offset
	xChange = xpos - lastX;
	yChange = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	// Pan camera
	if (isPanning) {

		if (cameraPosition.z < 0.f)
			cameraFront.z = 1.f;
		else
			cameraFront.z = -1.f;


		GLfloat cameraSpeed = xChange * deltaTime;
		cameraPosition += cameraSpeed * cameraRight;

		cameraSpeed = yChange * deltaTime;
		cameraPosition += cameraSpeed * cameraUp;
	}

	// Orbit camera
	if (isOrbiting) {

		rawYaw += xChange;
		rawPitch += yChange;

		// convert yaw/pitch to degrees
		degYaw = glm::radians(rawYaw);
		//degPitch = glm::radians(rawPitch);
		degPitch = glm::clamp(glm::radians(rawPitch), -glm::pi<float>() / 2.f + .1f, glm::pi<float>() / 2.f - .1f);

		// Azimuth / Altitude formula
		cameraPosition.x = target.x + radius * cosf(degPitch) * sinf(degYaw);
		cameraPosition.y = target.y + radius * sinf(degPitch);
		cameraPosition.z = target.z + radius * cosf(degPitch) * cosf(degYaw);
	}

}

glm::vec3 getTarget() {

	// define get target func
	if (isPanning)
		target = cameraPosition + cameraFront;
	return target;
}

// define transform camera func
void TransformCamera() {

	// pan camera
	if (keys[GLFW_KEY_LEFT_ALT] && mouseButtons[GLFW_MOUSE_BUTTON_MIDDLE])
		isPanning = true;
	else
		isPanning = false;

	// oribit camera
	if (keys[GLFW_KEY_LEFT_ALT] && mouseButtons[GLFW_MOUSE_BUTTON_LEFT])
		isOrbiting = true;
	else
		isOrbiting = false;

	//reset cam
	if (keys[GLFW_KEY_F])
		initCamera();
}

void initCamera() {

	cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f);
	target = glm::vec3(0.0f, 0.0f, 0.0f);
	cameraDirection = glm::normalize(cameraPosition - target);
	worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));
	cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight));
	cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
}

