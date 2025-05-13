#include<iostream>

//include glad before GLFW to avoid header conflict or define "#define GLFW_INCLUDE_NONE"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <map>
#include <algorithm>

// #include <ft2build.h>
// #include FT_FREETYPE_H



#include "./utils/camera.h"
#include "./utils/shader.h"
#include "./utils/object.h"
#include "./utils/particles.h"
#include "./utils/text.h"


const int width = 500;
const int height = 500;

GLuint compileShader(std::string shaderCode, GLenum shaderType);
GLuint compileProgram(GLuint vertexShader, GLuint fragmentShader);
void processInput(GLFWwindow* window);
void loadCubemapFace(const char * file, const GLenum& targetCube);



#ifndef NDEBUG
void APIENTRY glDebugOutput(GLenum source,
	GLenum type,
	unsigned int id,
	GLenum severity,
	GLsizei length,
	const char* message,
	const void* userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}
#endif

Camera camera(glm::vec3(0.0, 0.0, 0.2));

// Plan récepteur  :  n·x + d = 0  → plan (0,1,0) à y = 0
const glm::vec3 planeN(0.0f, 1.0f, 0.0f);
const float     planeD = 0.0f;

// -----------------------------------------------------------------------------
// Calcule la matrice de projection d’ombre (Eq. 7.4 du PDF)
glm::mat4 shadowMatrix3(const glm::vec3& L,const glm::vec3& n,float d)
{
float nDotL = glm::dot(n, L) + d;        // n·l + d
glm::mat4 M(0.0f);
M[0][0] = nDotL - L.x * n.x;  M[1][0] = -L.x * n.y;      M[2][0] = -L.x * n.z;      M[3][0] = -L.x * d;
M[0][1] = -L.y * n.x;         M[1][1] = nDotL - L.y*n.y; M[2][1] = -L.y * n.z;      M[3][1] = -L.y * d;
M[0][2] = -L.z * n.x;         M[1][2] = -L.z * n.y;      M[2][2] = nDotL - L.z*n.z; M[3][2] = -L.z * d;
M[0][3] = -n.x;               M[1][3] = -n.y;            M[2][3] = -n.z;            M[3][3] =  nDotL;

return M;
}
// -----------------------------------------------------------------------------


int main(int argc, char* argv[])
{


	

	//Boilerplate
	//Create the OpenGL context 
	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialise GLFW \n");
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
	//create a debug context to help with Debugging
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif


	//Create the window
	GLFWwindow* window = glfwCreateWindow(width, height, "Escape the GNOMGNUM", nullptr, nullptr);
	if (window == NULL)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window\n");
	}

	glfwMakeContextCurrent(window);
	//load openGL function
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		throw std::runtime_error("Failed to initialize GLAD");
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);

#ifndef NDEBUG
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
#endif

	/*================ CREATE SHADERS & OBJECTS ================*/

	char shaderSimLightV[128] = PATH_TO_SHADERS "/vertSrc.txt";
	char shaderSimLightF[128] = PATH_TO_SHADERS "/fragSrc.txt";

	char shaderSunV[128] = PATH_TO_SHADERS "/sunVert.txt";
	char shaderSunF[128] = PATH_TO_SHADERS "/sunFrag.txt";

	char shaderSphV[128] = PATH_TO_SHADERS "/sphVert.txt";
	char shaderSphF[128] = PATH_TO_SHADERS "/sphFrag.txt";
	
	Shader shader(shaderSimLightV, shaderSimLightF);
	Shader shaderG(shaderSimLightV, shaderSimLightF);
	Shader shaderGnom(shaderSimLightV, shaderSimLightF);
	Shader shader2(shaderSunV, shaderSunF);
	Shader shaderball(PATH_TO_SHADERS "/ballInst.vert", PATH_TO_SHADERS "/ballInst.frag");
	// Shader shader3(shaderSphV, shaderSphF);
	Shader shadertext(PATH_TO_SHADERS "/glyph.vert", PATH_TO_SHADERS "/glyph.frag");

	Shader cubeMapShader = Shader(PATH_TO_SHADERS "/cubmap.vert", PATH_TO_SHADERS "/cubmap.frag");
	Shader shader3 = Shader(PATH_TO_SHADERS "/water.vert", PATH_TO_SHADERS "/water.frag");
	Shader shadowShader(PATH_TO_SHADERS "/shadow.vert", PATH_TO_SHADERS "/shadow.frag");

	char path[] = PATH_TO_OBJECTS "/moai5shadow.obj";
	Object moai(path);
	moai.makeObject(shader);
	moai.centerModel();
	moai.createText("./../textures/uii.png");

	char pathshadow[] = PATH_TO_OBJECTS "/moai5shadow.obj";
	Object moai2(path);
	moai2.makeObject(shadowShader, false);

	char pathWater[] = PATH_TO_OBJECTS "/water2.obj";
	Object water(pathWater);
	water.makeObject(shader3);

	
	char pathFire[] = PATH_TO_OBJECTS "/campfire.obj";
	Object firecamp(pathFire);
	firecamp.makeObject(shader);
	firecamp.createText("./../textures/Firecamp.png");

	char pathGnom[] = PATH_TO_OBJECTS "/gnomon.obj";
	Object gnom(pathGnom);
	gnom.makeObject(shader);
	gnom.createText("./../textures/RuskinParkSundial01_Model_5_u1_v1_diffuse.jpeg");

	Object gnomonshadow(PATH_TO_OBJECTS "/gnomon.obj");
	gnomonshadow.makeObject(shadowShader, false);

	char path2[] = PATH_TO_OBJECTS "/plage2.obj";
	Object beach(path2);
	beach.makeObject(shader);
	beach.createText("./../textures/plage.png");	

	char pathCub[] = PATH_TO_OBJECTS "/sphere_smooth.obj"; 
	Object sun(pathCub);
	sun.makeObject(shader2, true);
	sun.createText( "./../textures/2k_sun.jpg");

	Object ball1(pathCub);
	ball1.makeObject(shaderball, false);
	ball1.generateCircleInstances(8, 50.0f, 60.0f, 60.0f);

	//SKYBOX
	char pathCube[] = PATH_TO_OBJECTS "/cube.obj";
	Object cubeMap(pathCube);
	cubeMap.makeObject(cubeMapShader);

	glm::vec3 materialColour = glm::vec3(0.5f,0.6,0.8);


	/*================ FPS ================*/

	double prev = 0;
	int deltaFrame = 0;
	//fps function
	auto fps = [&](double now) {
		double deltaTime = now - prev;
		deltaFrame++;
		if (deltaTime > 0.5) {
			prev = now;
			const double fpsCount = (double)deltaFrame / deltaTime;
			deltaFrame = 0;
			std::cout << "\r FPS: " << fpsCount;
		}
	};

	/*================ MODEL MATRIX ================*/

	glm::mat4 modelmoai = glm::mat4(1.0);
	modelmoai = glm::translate(modelmoai, glm::vec3(0.0, 0.0, 0.0));
	// modelmoai = glm::scale(modelmoai, glm::vec3(0.5, 0.5, 0.5));
	glm::mat4 inverseModel = glm::transpose( glm::inverse(modelmoai));

	glm::mat4 modelmoaishadow = glm::mat4(1.0);
	modelmoaishadow = glm::translate(modelmoaishadow, glm::vec3(0.0, 0.0, 0.0));

	glm::mat4 modelfirecamp = glm::mat4(1.0);
	modelfirecamp = glm::translate(modelfirecamp, glm::vec3(0.0, 0.0, 0.0));
	glm::mat4 inverseModelFirecamp = glm::transpose( glm::inverse(modelfirecamp));

	glm::mat4 modelwater = glm::mat4(1.0);
	modelwater = glm::translate(modelwater, glm::vec3(0.0, 0.0, 0.0));
	glm::mat4 inverseModelWater = glm::transpose( glm::inverse(modelwater));

	float minY = std::numeric_limits<float>::max();
	for (const Vertex& vertex : water.vertices) {
		glm::vec4 worldPos = modelwater * glm::vec4(vertex.Position, 1.0f);
		if (worldPos.y < minY) {
			minY = worldPos.y;
		}
	}
	std::cout << "[INFO] Hauteur minimale de l'objet 'water' : y = " << minY << std::endl;


	glm::mat4 modelgnom = glm::mat4(1.0);
	modelgnom = glm::translate(modelgnom, glm::vec3(0.0, 0.0, 0.0));
	// // modelgnom = glm::scale(modelgnom, glm::vec3(0.5, 0.5, 0.5));
	// glm::mat4 inverseModelGnom = glm::transpose( glm::inverse(modelgnom));
	glm::mat4 modelgnom2 = glm::mat4(1.0);
	modelgnom2 = glm::translate(modelgnom2, glm::vec3(0.0, 0.0, 0.0));
	glm::mat4 inverseModelGnom2 = glm::transpose( glm::inverse(modelgnom2));

	glm::mat4 sphere = glm::mat4(1.0);
	sphere = glm::translate(sphere, glm::vec3(-2.0, 0.0, 0.0));
	sphere = glm::scale(sphere, glm::vec3(0.5, 0.5, 0.5));
	glm::mat4 inverseModelSphere = glm::transpose( glm::inverse(sphere));

	glm::mat4 secondModel = glm::translate(glm::mat4(1.0f),glm::vec3(0.0, 0.0, 0.0));
	glm::mat4 inverseModel2 = glm::transpose( glm::inverse(secondModel));

	glm::mat4 modelSun = glm::translate(modelSun, glm::vec3(0.0, 0.0, 0.0));
	modelSun = glm::scale(modelSun, glm::vec3(0.5, 0.5, 0.5));
	glm::mat4 inverseModelSun= glm::transpose(glm::inverse(modelSun));

	glm::mat4 modelball = glm::translate(modelball, glm::vec3(0.0, 0.0, 0.0));
	// modelball = glm::scale(modelball, glm::vec3(0.1, 0.1, 0.1));
	glm::mat4 inverseModelBall= glm::transpose(glm::inverse(modelball));

	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 perspective = camera.GetProjectionMatrix();

	glm::vec3 light_pos = glm::vec3(1.0, 2.0, 1.5);
	glm::vec3 light_direction = glm::vec3(-0.2f, -1.0f, -0.3f);


	glfwSwapInterval(1);
	//Rendering

	/*================ SET SPECIFICS SHADERS ================*/
	
	float ambient = 0.1;
	float diffuse = 1.0;
	float specular = 0.8;

	shader.use();
	shader.setFloat("shininess", 256.0f);
	shader.setFloat("light.ambient_strength", ambient);
	shader.setFloat("light.diffuse_strength", diffuse);
	shader.setFloat("light.specular_strength", specular);
	shader.setFloat("light.constant", 1.0);
	shader.setFloat("light.linear", 0.14);
	shader.setFloat("light.quadratic", 0.07);
	shader.setFloat("sunIntensity", 1.0f);

	shader3.use();
	shader3.setFloat("shininess", 256.0f);
	shader3.setVector3f("materialColour", materialColour);
	shader3.setFloat("light.ambient_strength", ambient);
	shader3.setFloat("light.diffuse_strength", diffuse);
	shader3.setFloat("light.specular_strength", specular);
	shader3.setFloat("light.constant", 1.0);
	shader3.setFloat("light.linear", 0.14);
	shader3.setFloat("light.quadratic", 0.07);
	shader3.setFloat("sunIntensity", 1.0f);

	

	

	GLuint cubeMapTexture;
	glGenTextures(1, &cubeMapTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);

	// texture parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_set_flip_vertically_on_load(false);

	std::string pathToCubeMap = PATH_TO_TEXTURE "/cubemaps/Daylight/";

	std::map<std::string, GLenum> facesToLoad = { 
		{pathToCubeMap + "posx.jpg",GL_TEXTURE_CUBE_MAP_POSITIVE_X},
		{pathToCubeMap + "posy.jpg",GL_TEXTURE_CUBE_MAP_POSITIVE_Y},
		{pathToCubeMap + "posz.jpg",GL_TEXTURE_CUBE_MAP_POSITIVE_Z},
		{pathToCubeMap + "negx.jpg",GL_TEXTURE_CUBE_MAP_NEGATIVE_X},
		{pathToCubeMap + "negy.jpg",GL_TEXTURE_CUBE_MAP_NEGATIVE_Y},
		{pathToCubeMap + "negz.jpg",GL_TEXTURE_CUBE_MAP_NEGATIVE_Z},
	};
	//load the six faces
	for (std::pair<std::string, GLenum> pair : facesToLoad) {
		loadCubemapFace(pair.first.c_str(), pair.second);
	}

	char fileVert[128] = PATH_TO_SHADERS"/part.vert";
	char fileFrag[128] = PATH_TO_SHADERS"/part.frag";
	Shader shaderParticules(fileVert, fileFrag);
	ParticleSystem particleSystem(camera, shaderParticules);
	particleSystem.initParticleSystem();


	float yTranslation = 0.0f;
	float yref = 0.0f;
	float dirball = 1.0f;

	float amplitude = 2.0f;
	float speed = 1.5f; // vitesse de montée/descente
	float pauseDuration = 1.0f; // en secondes

	enum { UP, PAUSE_TOP, DOWN, PAUSE_BOTTOM } state = UP;
	float yball = -amplitude;
	double lastTime = glfwGetTime();
	double pauseStart = 0.0;

	// --- TEXTE -----------------------------------------------------------
	GLuint text_VBO, text_VAO;   // VAO/VBO pour le texte
	LoadFontCharacters("/Library/Fonts/Arial Unicode.ttf", 48);   // 1. remplit `Characters`
	SetupTextVAO(text_VAO, text_VBO);                              // 2. crée le VAO/VBO
	// --------------------------------------------------------------------

	/*================ RENDERING LOOP ================*/

	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		view = camera.GetViewMatrix();
		glfwPollEvents();
		double now = glfwGetTime();
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// float damptime = -0.04;
		float damptime = -1.0f;

		float distSun = -50.0f;
		// auto delta = glm::vec3(distSun * std::cos(-damptime * now), -damptime*10,distSun * std::sin(-damptime *  now));
		auto delta = glm::vec3(distSun * std::cos(damptime * now), distSun * std::sin(damptime *  now), distSun);
		float intensityMap = fmin(fmax(std::sin(-damptime * now), 0.0f) + 0.1f, 1.0f);


		//Use the shader Class to send the uniform
		shader.use();

		shader.setMatrix4("M", modelmoai);
		shader.setMatrix4("itM", inverseModel);
		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);
		shader.setVector3f("u_view_pos", camera.Position);

		//std::cout << delta.z <<std::endl;
		// shader.setVector3f("light.light_pos", delta);
		shader.setVector3f("light.light_pos", delta);
		shader.setVector3f("u_light_direction", light_direction);
		shader.setFloat("now", 0.1 * (now));

		shader.setInteger("ourTexture", 0);		
		moai.drawText();
		glDepthFunc(GL_LEQUAL);
		moai.draw();

		shader3.use();
		shader3.setMatrix4("M", modelwater);
		shader3.setMatrix4("itM", inverseModelWater);
		shader3.setMatrix4("V", view);
		shader3.setMatrix4("P", perspective);
		shader3.setVector3f("u_view_pos", camera.Position);
		shader3.setVector3f("light.light_pos", delta);
		shader3.setVector3f("u_light_direction", light_direction);
		shader3.setFloat("now", 0.1 * (now));
		shader3.setFloat("intensity", intensityMap);
		// shader3.setInteger("ourTexture", 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP,cubeMapTexture);
		cubeMapShader.setInteger("cubemapTexture", 1);		
		// water.drawText();
		water.draw();

		shader.use();
		shader.setMatrix4("M", modelfirecamp);
		shader.setMatrix4("itM", inverseModelFirecamp);
		firecamp.drawText();
		firecamp.draw();

		shader.use();
		glm::mat4 inverseModelGnom = glm::transpose( glm::inverse(modelgnom));
		shader.setMatrix4("M", modelgnom);
		shader.setMatrix4("itM", inverseModelGnom);
		gnom.drawText();
		gnom.draw();

		shader.use();
		shader.setMatrix4("M", secondModel);
		shader.setMatrix4("itM", inverseModel2);
		beach.drawText();
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		beach.draw();

		// printf("delta.y: %f\n", delta.y);
		if(delta.y>=0.0)
		{	glm::mat4 S  = shadowMatrix3(delta, planeN, 0.1);
			shadowShader.use();
			shadowShader.setMatrix4("MVP", perspective*view* S * modelmoai);
			shadowShader.setVector3f("color", glm::vec3(0.0, 0.0, 0.0));
			glDisable(GL_DEPTH_TEST);
			glStencilFunc(GL_EQUAL, 1, 0xFF);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			moai2.draw();
			glStencilFunc(GL_ALWAYS, 0, 0xFF);
			glEnable(GL_DEPTH_TEST);

			shadowShader.use();
			shadowShader.setMatrix4("MVP", perspective*view* S * inverseModelGnom2);
			shadowShader.setVector3f("color", glm::vec3(0.0, 0.0, 0.0));
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			gnomonshadow.draw();
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
		}

		shader2.use();
		modelSun = glm::mat4(1.0f);
		modelSun = glm::translate(modelSun, delta);
		shader2.setMatrix4("M", modelSun);
		shader2.setMatrix4("V", view);
		shader2.setMatrix4("P", perspective);
		shader2.setInteger("ourTexture", 0);	
		shader2.setFloat("intensity", 1.0f + 10*intensityMap);
		glDepthFunc(GL_LEQUAL);
		sun.drawText();
		sun.draw();

		shaderball.use();
		modelball = glm::mat4(1.0f);
		

		double deltaTime = now - lastTime;
		lastTime = now;

		switch(state) {
			case UP:
				yball += speed * deltaTime;
				if (yball >= amplitude) {
					yball = amplitude;
					state = PAUSE_TOP;
					pauseStart = now;
				}
				break;

			case PAUSE_TOP:
				if (now - pauseStart >= pauseDuration) {
					state = DOWN;
				}
				break;

			case DOWN:
				yball -= speed * deltaTime;
				if (yball <= -amplitude) {
					yball = -amplitude;
					state = PAUSE_BOTTOM;
					pauseStart = now;
				}
				break;

			case PAUSE_BOTTOM:
				if (now - pauseStart >= pauseDuration) {
					state = UP;
				}
				break;
		}
		modelball = glm::translate(modelball, glm::vec3(0.0, yball, -60.0));
		modelball = glm::scale(modelball, glm::vec3(0.5, 0.5, 0.5));
		// shaderball.setMatrix4("M", glm::mat4(1.0f));
		shaderball.setMatrix4("V", view);
		shaderball.setMatrix4("P", perspective);
		// shaderball.setInteger("ourTexture", 0);	
		// shaderball.setFloat("intensity", 1.0f + 10*intensityMap);
		// glDepthFunc(GL_LEQUAL);
		// ball1.drawText();
		// ball1.draw();
		shaderball.setVector3f("u_view_pos", camera.Position);

		std::vector<glm::vec3> colors(ball1.instanceCount);
		for (unsigned i = 0; i < colors.size(); ++i) {
			// Exemple 1 : dégradé arc-en-ciel
			float t = i / float(colors.size()-1);
			colors[i] = glm::vec3( sin(t*M_PI), t, 1.0-t );
			// Exemple 2 : rouge/bleu alternés
			// colors[i] = (i % 2 == 0) ? glm::vec3(1,0,0) : glm::vec3(0,0,1);
		}
		ball1.setupInstanceColors(colors);
		ball1.drawInstanced();

	
		cubeMapShader.use();
		cubeMapShader.setMatrix4("V", view);
		cubeMapShader.setMatrix4("P", perspective);
		// float intensityMap = max(distSun * std::cos(damptime * now), 0.0f);
		cubeMapShader.setFloat("intensity", intensityMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP,cubeMapTexture);
		cubeMap.draw();
		glDepthFunc(GL_LESS);

		particleSystem.drawParticles(view, perspective, camera.GetCameraRight(), camera.GetCameraUp());
		
		if(delta.y>=0.0)
			RenderText(shadertext, "This is sample text", 0.25f, 0.25f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f), text_VAO, text_VBO);
		else if (delta.y<=0.0)
			RenderText(shadertext, "(C) LearnOpenGL.com", 540.0f, 570.0f, 0.5f, glm::vec3(0.3, 0.7f, 0.9f), text_VAO, text_VBO);

		fps(now);
		glfwSwapBuffers(window);
	}

	/*================ CLEAN RESSOURCE ================*/

	particleSystem.cleanUp();
	glfwDestroyWindow(window);
	glfwTerminate();


	return 0;
}

void loadCubemapFace(const char * path, const GLenum& targetFace)
{
	int imWidth, imHeight, imNrChannels;
	unsigned char* data = stbi_load(path, &imWidth, &imHeight, &imNrChannels, 0);
	if (data)
	{

		glTexImage2D(targetFace, 0, GL_RGB, imWidth, imHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		//glGenerateMipmap(targetFace);
	}
	else {
		std::cout << "Failed to Load texture" << std::endl;
		const char* reason = stbi_failure_reason();
		std::cout << (reason == NULL ? "Probably not implemented by the student" : reason) << std::endl;
	}
	stbi_image_free(data);
}


void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(LEFT, 0.1);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(RIGHT, 0.1);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(FORWARD, 0.1);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(BACKWARD, 0.1);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(UP, 0.1);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(DOWN, 0.1);

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(1, 0.0,1);
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(-1, 0.0,1);

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(0.0, 1.0, 1);
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(0.0, -1.0, 1);


}

