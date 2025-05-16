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
#include <numeric> // Required for std::iota
#include <random>  // Required for std::mt19937
#include <vector>
#include <array>
#include <string>
#include <memory>   // for std::unique_ptr


#include "./utils/camera.h"
#include "./utils/shader.h"
#include "./utils/object.h"
#include "./utils/particles.h"
#include "./utils/text.h"
#include "./utils/picking.h"


const int width = 500;
const int height = 500;

GLuint compileShader(std::string shaderCode, GLenum shaderType);
GLuint compileProgram(GLuint vertexShader, GLuint fragmentShader);
void processInput(GLFWwindow* window);
void loadCubemapFace(const char * file, const GLenum& targetCube);
static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

float speedTime = 1.0f; // Vitesse rendu
float damptime = -0.05f;

inline bool  showHitboxes  = false;      //Hitboxes : afficher les sphères de picking
inline bool  godMode = false;      // GodMod : G = view libre

// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//  Spawns coloured spheres each night and computes their colour histogram
// ---------------------------------------------------------------------------
struct SphereSpawner
{
    bool active       = false;
    bool resultsReady = false;
    std::array<int,3> histogram {0,0,0};   // blanc, bleu, mauve

    void start(Object& balls)
    {
        active        = true;
        resultsReady  = false;
        histogram     = {0,0,0};

        std::vector<glm::vec3> colours(balls.instanceCount);
        for (std::size_t i = 0; i < colours.size(); ++i)
        {
            int col = rand() % 3;
            if      (col == 0) colours[i] = glm::vec3(1.0f);             // blanc
            else if (col == 1) colours[i] = glm::vec3(0.0f,1.0f,1.0f);   // bleu
            else               colours[i] = glm::vec3(0.5f,0.0f,0.5f);   // mauve
            histogram[col]++;
        }
        balls.setupInstanceColors(colours);   // applique visuellement les couleurs
    }

    void update(bool isNight)
    {
        if (!active) return;
        if (!isNight) { active = false; resultsReady = true; }
    }

    std::vector<int> bottleOrder() const
    {
        // trie les indices 0‑1‑2 par fréquence décroissante
        std::vector<int> idx = {0,1,2};
        std::sort(idx.begin(), idx.end(),
                  [&](int a,int b){ return histogram[a] > histogram[b]; });

        std::vector<int> order;
        for (int i : idx) {
            switch (i) {
                case 0: order.push_back(5); break;   //blanc → bottleC 
                case 1: order.push_back(3); break;   //bleu  → bottleB 
                case 2: order.push_back(4); break;   //mauve → bottleA 
            }
        }
        return order;
    }
};

// Instance globale
inline SphereSpawner gSpawner;

// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//  Gameplay : machine à états minimaliste pour la quête
// ---------------------------------------------------------------------------
enum class QuestStage {
    WakeGuardian,
    ActivateGnomon,
    ActivateRelics,
    ReturnGuardian,
    Finale
};

// ---------------------------------------------------------------------------
//  Gameplay – Quest manager (version stable)
// ---------------------------------------------------------------------------
struct QuestManager
{
    // --- état ---
    QuestStage stage = QuestStage::WakeGuardian;
    std::vector<int> requiredOrder;   // ordre attendu (3,4,5)
    std::vector<int> playerOrder;     // clics joueur
    bool seqOk = false;

    // --- UI ---
    std::string hudText  = "Reactivate the old island guardian";
    std::string moaiText = "";

    const std::string& objective()    const { return hudText;  }
    const std::string& guardianLine() const { return moaiText; }

    // --- transition ---
    void onInteract(int objectID, bool isNight)
    {
        switch (stage)
        {
        /* 0. Réveiller --------------------------------------------------- */
        case QuestStage::WakeGuardian:
            if (objectID == 1) {                // Moaï
                moaiText = "I've been sleeping for centuries... This island hides an unresolved quest...";
                hudText  = "Activate the gnomon as soon as it gets dark";
                stage    = QuestStage::ActivateGnomon;
            }
            break;

        /* 1. Activer le gnomon de nuit ----------------------------------- */
        case QuestStage::ActivateGnomon:
            if (isNight && objectID == 2) {     // Gnomon
                moaiText = "There is this story with three relics.. and misterious shadow...";
                hudText  = "Find and collect the relics in the right order (use the gnomon)";
                requiredOrder = gSpawner.bottleOrder();   // calcul déjà fait
                playerOrder.clear();
                stage    = QuestStage::ActivateRelics;
            }
            break;

        /* 2. Activer les bouteilles -------------------------------------- */
        case QuestStage::ActivateRelics:
            if (objectID >= 3 && objectID <= 5) {
                playerOrder.push_back(objectID);
                std::size_t idx = playerOrder.size() - 1;

                if (objectID != requiredOrder[idx]) {      // mauvais clic
                    playerOrder.clear();
                  hudText = "[Quest] Wrong order, here we go again !";
				  moaiText = "I was sure you were such a loser..";
				  
                }
                else if (playerOrder.size() == requiredOrder.size()) {
                    seqOk  = true;
                    hudText = "Go back to the guard";
					moaiText = "I had no doubts about you";
                    stage   = QuestStage::ReturnGuardian;
                }
            }
            break;

        /* 3. Retour au gardien => FIN ------------------------------------------- */
        case QuestStage::ReturnGuardian:
            if (objectID == 1) {
                moaiText = "Thank you, traveller. Your adventure ends here.";
                hudText  = "Contemplate the sunrise and sunset...";
                stage    = QuestStage::Finale;
            }
            break;

        default:
            break;
        }
    }
};

// Instances globales
inline QuestManager gQuest;
inline int gCycleDayNight = 0;



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

Camera camera(glm::vec3(0.0, 0.5, 2.0));

// Plan récepteur  :  n·x + d = 0  → plan (0,1,0) à y = 0
glm::vec3 planeN(0.0f, 1.0f, 0.0f);
float planeD = 0.0f;

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


// Picking buffer for object selection
PickingTexture gPickingTexture;
// ------------------------------------------------------------
// Selection state (for temporary highlight on picked objects)
int   gSelectedObjectID    = 0;
double gSelectionTimestamp = -10.0;               // time of last pick
const glm::vec3 gHighlightColour(1.0f, 0.2f, 0.2f); // flashy red-orange
// ------------------------------------------------------------


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
	GLFWwindow* window = glfwCreateWindow(width, height, "Special Island", nullptr, nullptr);
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

	// Framebuffer for picking
	if (!gPickingTexture.Init(width, height)) {
		std::cerr << "Failed to init picking FBO\n";
		return -1;
	}
	glfwSetMouseButtonCallback(window, MouseButtonCallback);

	/*================ CREATE SHADERS & OBJECTS ================*/

	char shaderSimLightV[128] = PATH_TO_SHADERS "/src.vert";
	char shaderSimLightF[128] = PATH_TO_SHADERS "/src.frag";

	char shaderSunV[128]      = PATH_TO_SHADERS "/sun.vert";
	char shaderSunF[128]      = PATH_TO_SHADERS "/sun.frag";

	Shader shader( PATH_TO_SHADERS "/src.vert", PATH_TO_SHADERS "/src.frag");
	Shader shader2(PATH_TO_SHADERS "/sun.vert", PATH_TO_SHADERS "/sun.frag");
	Shader shaderball(PATH_TO_SHADERS "/ballInst.vert", PATH_TO_SHADERS "/ballInst.frag");
	Shader shadertext(PATH_TO_SHADERS "/glyph.vert", PATH_TO_SHADERS "/glyph.frag");

	Shader cubeMapShader = Shader(PATH_TO_SHADERS "/cubmap.vert", PATH_TO_SHADERS "/cubmap.frag");
	Shader shader3 = Shader(PATH_TO_SHADERS "/water.vert", PATH_TO_SHADERS "/water.frag");
	Shader shadowShader(PATH_TO_SHADERS "/shadow.vert", PATH_TO_SHADERS "/shadow.frag");

	Shader pickShader(PATH_TO_SHADERS "/pick.vert", PATH_TO_SHADERS "/pick.frag"); 

	/*================ OBJECTS ================*/

	char path[] = PATH_TO_OBJECTS "/moai5shadow.obj";
	Object moai(path);
	moai.makeObject(shader);
	moai.centerModel();
	moai.createText("./../textures/uii.png");
	// Hitbox invisible : même mesh « sphere_smooth », sans texture
	Object hitSphereMoai(PATH_TO_OBJECTS "/sphere_smooth.obj");
	hitSphereMoai.makeObject(shader, true); 
	Object moai2(path);
	moai2.makeObject(shadowShader, false);

	char pathWater[] = PATH_TO_OBJECTS "/water2.obj";
	Object water(pathWater);
	water.makeObject(shader3);

	
	char pathFire[] = PATH_TO_OBJECTS "/campfire.obj";
	Object firecamp(pathFire);
	firecamp.makeObject(shader);
	firecamp.createText("./../textures/Firecamp.png");

	char pathGnom[] = PATH_TO_OBJECTS "/gnom.obj";
	Object gnom(pathGnom);
	gnom.makeObject(shader);
	gnom.createText("./../textures/RuskinParkSundial01_Model_5_u1_v1_diffuse.jpeg");
	gnom.centerModel();
	Object hitSphereGnom(PATH_TO_OBJECTS "/sphere_smooth.obj");
	hitSphereGnom.makeObject(shader, true); 

	Object gnomonshadow(PATH_TO_OBJECTS "/gnomshad.obj");
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
	ball1.generateCircleInstances(20, 50.0f, 80.0f, 40.0f);

	Object bottleA(PATH_TO_OBJECTS "/bottleA.obj");
	bottleA.makeObject(shader, true);
	bottleA.createText("./../textures/Firecamp.png");
	bottleA.centerModel();
	Object hitSphereBottleA(PATH_TO_OBJECTS "/sphere_smooth.obj");
	hitSphereBottleA.makeObject(shader, true); 

	Object bottleB(PATH_TO_OBJECTS "/bottleB.obj");
	bottleB.makeObject(shader, true);
	bottleB.createText("./../textures/Firecamp.png");
	bottleB.centerModel();
	Object hitSphereBottleB(PATH_TO_OBJECTS "/sphere_smooth.obj");
	hitSphereBottleB.makeObject(shader, true); 

	Object bottleC(PATH_TO_OBJECTS "/bottleC.obj");
	bottleC.makeObject(shader, true);
	bottleC.createText("./../textures/Firecamp.png");
	bottleC.centerModel();
	Object hitSphereBottleC(PATH_TO_OBJECTS "/sphere_smooth.obj");
	hitSphereBottleC.makeObject(shader, true); 

	char textpalm[] = PATH_TO_TEXTURE "/palmtree.jpg";
	Object tree1(PATH_TO_OBJECTS "/tree.obj");
	tree1.makeObject(shader);
	tree1.createText(textpalm);
	Object treeshad1(PATH_TO_OBJECTS "/tree.obj");
	treeshad1.makeObject(shadowShader, false);
	Object tree2(PATH_TO_OBJECTS "/tree2.obj");
	tree2.makeObject(shader);
	tree2.createText(textpalm);
	Object treeshad2(PATH_TO_OBJECTS "/tree2.obj");
	treeshad2.makeObject(shadowShader, false);
	Object tree3(PATH_TO_OBJECTS "/tree3.obj");
	tree3.makeObject(shader);
	tree3.createText(textpalm);
	Object treeshad3(PATH_TO_OBJECTS "/tree3.obj");
	treeshad3.makeObject(shadowShader, false);
	Object tree4(PATH_TO_OBJECTS "/tree4.obj");
	tree4.makeObject(shader);
	tree4.createText(textpalm);
	Object treeshad4(PATH_TO_OBJECTS "/tree4.obj");
	treeshad4.makeObject(shadowShader, false);
	Object tree5(PATH_TO_OBJECTS "/tree5.obj");
	tree5.makeObject(shader);
	tree5.createText(textpalm);
	Object treeshad5(PATH_TO_OBJECTS "/tree5.obj");
	treeshad5.makeObject(shadowShader, false);

	char textleaf[] = PATH_TO_TEXTURE "/palmleaf.jpg";
	Object leaf1(PATH_TO_OBJECTS "/leaf.obj");
	leaf1.makeObject(shader);
	leaf1.createText(textleaf);
	Object leafshad1(PATH_TO_OBJECTS "/leaf.obj");
	leafshad1.makeObject(shadowShader, false);
	Object leaf2(PATH_TO_OBJECTS "/leaf2.obj");
	leaf2.makeObject(shader);
	leaf2.createText(textleaf);
	Object leafshad2(PATH_TO_OBJECTS "/leaf2.obj");
	leafshad2.makeObject(shadowShader, false);
	Object leaf3(PATH_TO_OBJECTS "/leaf3.obj");
	leaf3.makeObject(shader);
	leaf3.createText(textleaf);
	Object leafshad3(PATH_TO_OBJECTS "/leaf3.obj");
	leafshad3.makeObject(shadowShader, false);
	Object leaf4(PATH_TO_OBJECTS "/leaf4.obj");
	leaf4.makeObject(shader);
	leaf4.createText(textleaf);
	Object leafshad4(PATH_TO_OBJECTS "/leaf4.obj");
	leafshad4.makeObject(shadowShader, false);
	Object leaf5(PATH_TO_OBJECTS "/leaf5.obj");
	leaf5.makeObject(shader);
	leaf5.createText(textleaf);
	Object leafshad5(PATH_TO_OBJECTS "/leaf5.obj");
	leafshad5.makeObject(shadowShader, false);


	//SKYBOX
	char pathCube[] = PATH_TO_OBJECTS "/cube.obj";
	Object cubeMap(pathCube);
	cubeMap.makeObject(cubeMapShader);

	glm::vec3 materialColour = glm::vec3(0.5f,0.6,0.8);

	/*=========  Level of water (Y) ============*/
	// float minY = std::numeric_limits<float>::max();
	// for (const Vertex& vertex : water.vertices) {
	// 	glm::vec4 worldPos = modelwater * glm::vec4(vertex.Position, 1.0f);
	// 	if (worldPos.y < minY) {
	// 		minY = worldPos.y;
	// 	}
	// }
	// std::cout << "[INFO] Hauteur minimale de l'objet 'water' : y = " << minY << std::endl;

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
	// modelmoai = glm::translate(modelmoai, glm::vec3(0.0, 0.0, 0.0));
	glm::mat4 inverseModel = glm::transpose( glm::inverse(modelmoai));

	glm::mat4 modelmoaishadow = glm::mat4(1.0);
	modelmoaishadow = glm::translate(modelmoaishadow, glm::vec3(0.0, 0.0, 0.0));

	glm::mat4 modelfirecamp = glm::mat4(1.0);
	glm::mat4 inverseModelFirecamp = glm::transpose( glm::inverse(modelfirecamp));

	glm::mat4 modelwater = glm::mat4(1.0);
	glm::mat4 inverseModelWater = glm::transpose( glm::inverse(modelwater));

	glm::mat4 modelgnom = glm::mat4(1.0);
	glm::mat4 modelgnom2 = glm::mat4(1.0);
	glm::mat4 inverseModelGnom2 = glm::transpose( glm::inverse(modelgnom2));

	glm::mat4 sphere = glm::mat4(1.0);
	sphere = glm::translate(sphere, glm::vec3(-2.0, 0.0, 0.0));
	sphere = glm::scale(sphere, glm::vec3(0.5, 0.5, 0.5));
	glm::mat4 inverseModelSphere = glm::transpose( glm::inverse(sphere));

	glm::mat4 secondModel = glm::translate(glm::mat4(1.0f),glm::vec3(0.0, 0.0, 0.0));
	glm::mat4 inverseModel2 = glm::transpose( glm::inverse(secondModel));

	glm::mat4 modelSun = glm::translate(modelSun, glm::vec3(0.0, 0.0, 0.0));

	glm::mat4 modelball = glm::translate(modelball, glm::vec3(0.0, 0.0, 0.0));
	glm::mat4 inverseModelBall= glm::transpose(glm::inverse(modelball));

	glm::mat4 modelBottleA = glm::mat4(1.0f);
	glm::mat4 inverseModelBottleA= glm::transpose(glm::inverse(modelBottleA));

	glm::mat4 modelBottleB = glm::mat4(1.0f);
	glm::mat4 inverseModelBottleB= glm::transpose(glm::inverse(modelBottleB));

	glm::mat4 modelBottleC = glm::mat4(1.0f);
	glm::mat4 inverseModelBottleC= glm::transpose(glm::inverse(modelBottleC));

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

	// glm::vec3 materialColour = glm::vec3(1.0f);

	shader.use();
	shader.setFloat("shininess", 256.0f);
	shader.setFloat("light.ambient_strength", ambient);
	shader.setFloat("light.diffuse_strength", diffuse);
	shader.setFloat("light.specular_strength", specular);
	shader.setFloat("light.constant", 1.0);
	shader.setFloat("light.linear", 0.14);
	shader.setFloat("light.quadratic", 0.07);
	shader.setFloat("sunIntensity", 1.0f);

	shader.setVector3f("materialColor", glm::vec3(1.0f));   // par défaut


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



	float yball = -1.0f;           // Position initiale : sous la mer (jour)
	float translationSpeed = 10.0f; // m/s pour rejoindre la position cible
	double lastTime = glfwGetTime();

	// --- TEXTE -----------------------------------------------------------
	GLuint text_VBO, text_VAO;   // VAO/VBO pour le texte
	LoadFontCharacters("/Library/Fonts/Arial Unicode.ttf", 48);   // 1. remplit `Characters`
	SetupTextVAO(text_VAO, text_VBO);                              // 2. crée le VAO/VBO
	// --------------------------------------------------------------------

	/*================ RENDERING LOOP ================*/
	glm::mat4 modelHitMoai = glm::scale(modelmoai, glm::vec3(1.2f));   
		modelHitMoai = glm::translate(modelHitMoai, glm::vec3(-0.000288667, 0.749567, -4.76613));
	glm::mat4 modelHitGnom = modelgnom;   
			modelHitGnom = glm::translate(modelHitGnom, glm::vec3(-21.8273, -0.490323, -19.5177));

	glm::mat4 modelHitBottleA = glm::mat4(1.0);   
	modelHitBottleA = glm::translate(modelHitBottleA, glm::vec3(-30.4561,-1.26976, 27.0012));

	glm::mat4 modelHitBottleB = glm::mat4(1.0);   
	modelHitBottleB = glm::translate(modelHitBottleB, glm::vec3(19.7625, -0.789905, 26.9709));

	glm::mat4 modelHitBottleC = glm::mat4(1.0);   
	modelHitBottleC = glm::translate(modelHitBottleC, glm::vec3(-5.53361, -0.258626, 11.1182));
	/*===================== GAMEPLAY ========================*/

	std::vector<glm::vec3> colors(ball1.instanceCount);
	glm::vec3 mauve(0.5f, 0.0f, 0.5f); //mauve
	glm::vec3 bleu(0.0f, 1.0f, 1.0f);  //bleu clair
	// glm::vec3 cyan(0.0f, 1.0f, 1.0f); // cyan
	glm::vec3 blanc(1.0f, 1.0f, 1.0f); //blanc
	std::vector<int> indices(ball1.instanceCount);
	std::iota(indices.begin(), indices.end(), 0); // Remplit avec 0, 1, ..., instanceCount-1
	auto seed = static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count());
	std::mt19937 g(seed);
	std::shuffle(indices.begin(), indices.end(), g);
	/*================ TEXTE GAMEPLAY ===================*/
	char text1[128] = "Go to the beach at midnight... Come back to the campfire to see the sun rise";

	// int cycle_daynight = 0;
	gCycleDayNight += 1;
	std::cout << "Cycle day-night count: " << gCycleDayNight << std::endl;
	int numtalkmoai = 0;

	/*================ RENDERING LOOP ================*/

	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		view = camera.GetViewMatrix();
		glfwPollEvents();
		double now = glfwGetTime();

		// =========================================================
		// 1st pass : render IDs into the off‑screen picking buffer
		// =========================================================
		gPickingTexture.EnableWriting();

		/* ---- correctifs ---- */
		glDisable(GL_MULTISAMPLE);         //pas de moyenne d’échantillons
		glDisable(GL_BLEND);               //pas de transparence
		glDisable(GL_CULL_FACE);           //faces avant / arrière toutes conservées
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);   //ID « vide » = 0,0,0
		/* -------------------- */

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		pickShader.use();
		pickShader.setMatrix4("V", view);
		pickShader.setMatrix4("P", perspective);

		// ---- moai ----
		pickShader.setFloat("ObjectID", 1.0f);
		pickShader.setMatrix4("M", modelHitMoai);  
		hitSphereMoai.draw(true);

		// ---- gnom ----
		pickShader.setFloat("ObjectID", 2.0f);
		pickShader.setMatrix4("M", modelHitGnom);  
		hitSphereGnom.draw(true);

		// ---- bottleA ----
		pickShader.setFloat("ObjectID", 3.0f);
		pickShader.setMatrix4("M", modelHitBottleA);  
		hitSphereBottleA.draw(true);

		// ---- bottleB ----
		pickShader.setFloat("ObjectID", 4.0f);
		pickShader.setMatrix4("M", modelHitBottleB);  
		hitSphereBottleB.draw(true);
		
		// ---- bottleC ----
		pickShader.setFloat("ObjectID", 5.0f);
		pickShader.setMatrix4("M", modelHitBottleC);  
		hitSphereBottleC.draw(true);

		gPickingTexture.DisableWriting();
		// =========================================================
		// End picking pass – back to normal forward rendering
		// =========================================================

		// ----- handle temporary highlight after picking -----
		double elapsed         = now - gSelectionTimestamp;
		bool   highlightActive = elapsed < 2.0;   // 2-second window

	
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		float distSun = -500.0f;
		// auto delta = glm::vec3(distSun * std::cos(-damptime * now), -damptime*10,distSun * std::sin(-damptime *  now));
		// float damptime = -0.01;
		// float damptime = -0.6f;
		// float damptime = -1.0f;
		now *= speedTime;
		auto delta = glm::vec3(distSun * std::cos(damptime * now),distSun * std::sin(damptime *  now), distSun);
		auto delta2 = glm::vec3(distSun/10 * std::cos(damptime * now), distSun/10 * std::sin(damptime *  now), distSun/10);

		static float lastAngle = 0.0f;
		float currentAngle = std::fmod(-damptime * now, 2.0f * M_PI); // Calculate the current angle in radians

		if (currentAngle < lastAngle) { 
			// cycle_daynight += 1;
			gCycleDayNight += 1;
			// std::cout << "Cycle day-night count: " << gCycleDayNight << std::endl;
			std::cout << "\rCycle day-night: " << gCycleDayNight << std::flush;
			// std::cout << "Cycle day-night count: " << cycle_daynight << std::endl;
		}
		gQuest.onInteract(0, gCycleDayNight);   // informe la quête du cycle

		lastAngle = currentAngle;
		float intensityMap = fmin(fmax(std::sin(-damptime * now), 0.0f) + 0.1f, 1.0f);

		/* ----------  Hit-spheres (debug) ---------- */
        if (showHitboxes) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);   // fil de fer
            shader.use();
            shader.setVector3f("materialColor", glm::vec3(0.0f, 1.0f, 0.0f));  // vert pétant

            shader.setMatrix4("M", modelHitMoai);     hitSphereMoai.draw(true);
            shader.setMatrix4("M", modelHitGnom);     hitSphereGnom.draw(true);
            shader.setMatrix4("M", modelHitBottleA);  hitSphereBottleA.draw(true);
            shader.setMatrix4("M", modelHitBottleB);  hitSphereBottleB.draw(true);
            shader.setMatrix4("M", modelHitBottleC);  hitSphereBottleC.draw(true);

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }


		//Use the shader Class to send the uniform
		shader.use();
		shader.setMatrix4("M", modelmoai);
		shader.setMatrix4("itM", inverseModel);
		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);
		shader.setVector3f("u_view_pos", camera.Position);

		//std::cout << delta.z <<std::endl;
		// shader.setVector3f("light.light_pos", delta);
		shader.setVector3f("light.light_pos", delta2);
		shader.setVector3f("u_light_direction", light_direction);
		shader.setFloat("now", 0.1 * (now));
		shader.setInteger("ourTexture", 0);	
		shader.setVector3f("materialColor",
			(highlightActive && gSelectedObjectID == 1)
				? gHighlightColour
				: glm::vec3(1.0f));	
		moai.drawText();
		glDepthFunc(GL_LEQUAL);
		moai.draw();
		shader.setVector3f("materialColor", glm::vec3(1.0f)); 

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
		shader.setVector3f("materialColor",
			(highlightActive && gSelectedObjectID == 2)
				? gHighlightColour
				: glm::vec3(1.0f));	
		gnom.draw();
		shader.setVector3f("materialColor", glm::vec3(1.0f));  

		shader.use();
		shader.setMatrix4("M", glm::mat4(1.0));
		shader.setMatrix4("itM", glm::transpose( glm::inverse(glm::mat4(1.0))));
		tree1.drawText();
		shader.setVector3f("materialColor",glm::vec3(1.0f));	
		tree1.draw();
		tree2.draw();
		tree3.draw();
		tree4.draw();
		tree5.draw();
		leaf1.drawText();
		leaf1.draw();
		leaf2.draw();
		leaf3.draw();
		leaf4.draw();
		leaf5.draw();
		
		shader.use();
		shader.setMatrix4("M", modelBottleA);
		shader.setMatrix4("itM", inverseModelBottleA);
		// bottleA.drawText();
		shader.setVector3f("materialColor",(highlightActive && gSelectedObjectID == 3) ? gHighlightColour: glm::vec3(1.0f));
		bottleA.drawText();
		bottleA.draw();
		shader.setVector3f("materialColor", glm::vec3(1.0f));  

		shader.use();
		shader.setMatrix4("M", modelBottleB);
		shader.setMatrix4("itM", inverseModelBottleB);
		shader.setVector3f("materialColor",(highlightActive && gSelectedObjectID == 4) ? gHighlightColour: glm::vec3(1.0f));
		bottleB.drawText();
		bottleB.draw();
		shader.setVector3f("materialColor", glm::vec3(1.0f));  

		shader.use();
		shader.setMatrix4("M", modelBottleC);
		shader.setMatrix4("itM", inverseModelBottleC);
		shader.setVector3f("materialColor",(highlightActive && gSelectedObjectID == 5) ? gHighlightColour: glm::vec3(1.0f));
		bottleC.drawText();
		bottleC.draw();
		shader.setVector3f("materialColor", glm::vec3(1.0f));  

		shader.use();
		shader.setMatrix4("M", secondModel);
		shader.setMatrix4("itM", inverseModel2);
		beach.drawText();
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		beach.draw();
		
		
		shader3.use();
		shader3.setMatrix4("M", modelwater);
		shader3.setMatrix4("itM", inverseModelWater);
		shader3.setMatrix4("V", view);
		shader3.setMatrix4("P", perspective);
		shader3.setVector3f("u_view_pos", camera.Position);
		shader3.setVector3f("light.light_pos", delta2);
		shader3.setVector3f("u_light_direction", light_direction);
		shader3.setFloat("now", 0.1 * (now));
		shader3.setFloat("intensity", intensityMap);
		// shader3.setInteger("ourTexture", 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP,cubeMapTexture);
		cubeMapShader.setInteger("cubemapTexture", 1);		
		// water.drawText();
		water.draw();

		// printf("delta.y: %f\n", delta.y);
		if(delta.y>=0.0 && ((std::cos(damptime * now)>= -0.9)))
		{	
			planeD = 0.3f;
			glm::mat4 S  = shadowMatrix3(delta, planeN, planeD);
			shadowShader.use();
			shadowShader.setMatrix4("MVP", perspective*view* S * modelmoai);
			shadowShader.setVector3f("color", glm::vec3(0.0, 0.0, 0.0));
			glDisable(GL_DEPTH_TEST);
			glStencilFunc(GL_EQUAL, 1, 0xFF);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			moai2.draw();
			treeshad1.draw();
			treeshad2.draw();
			treeshad3.draw();
			treeshad4.draw();
			treeshad5.draw();
			leafshad1.draw();
			leafshad2.draw();
			leafshad3.draw();
			leafshad4.draw();
			leafshad5.draw();
			glStencilFunc(GL_ALWAYS, 0, 0xFF);
			glEnable(GL_DEPTH_TEST);

			planeD = 0.5f;
			S  = shadowMatrix3(delta, planeN,  planeD);
			shadowShader.use();
			shadowShader.setMatrix4("MVP", perspective*view* S * inverseModelGnom2);
			if((std::cos(damptime * now) <= 0.985 ) && (std::cos(damptime * now )>= 0.97))
			{
				shadowShader.setVector3f("color", blanc);
			}
			else if(std::cos(damptime * now) <= 0.79  && (std::cos(damptime * now )>= 0.76))
			{
				shadowShader.setVector3f("color", mauve);
			}
			else if((std::cos(damptime * now) <= -0.18 ) && (std::cos(damptime * now )>= -0.23))
			{
				shadowShader.setVector3f("color", bleu);
			}
			else
			{
				shadowShader.setVector3f("color", glm::vec3(0.0, 0.0, 0.0));
			}
			// shadowShader.setVector3f("color", glm::vec3(0.0, 0.0, 0.0));
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			gnomonshadow.draw();
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
		}

		shader2.use();
		modelSun = glm::mat4(1.0f);
		modelSun = glm::translate(modelSun, delta2);
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

		// --- Animation jour / nuit du ballon ----------------------------------
		float targetY = (delta.y >= 0.0f) ? -4.0f : 4.0f;   // jour : -1, nuit : +1
		if (yball < targetY)
			yball = std::fmin(yball + translationSpeed * float(deltaTime), targetY);
		else if (yball > targetY)
			yball = std::fmax(yball - translationSpeed * float(deltaTime), targetY);
		// ----------------------------------------------------------------------
		modelball = glm::translate(modelball, glm::vec3(0.0, yball, 0.0));
		modelball = glm::scale(modelball, glm::vec3(0.5, 0.5, 0.5));
		shaderball.setMatrix4("M",  modelball);
		shaderball.setMatrix4("itM", glm::transpose(glm::inverse(modelball)));
		shaderball.setMatrix4("V", view);
		shaderball.setMatrix4("P", perspective);
		shaderball.setVector3f("u_view_pos", camera.Position);

		// Attribuer les couleurs
		int count = 0;
		for (int i = 0; i < 11; ++i) {
			colors[indices[count++]] = mauve;
		}
		for (int i = 0; i < 5; ++i) {
			colors[indices[count++]] = bleu;
		}
		for (int i = 0; i < 4; ++i) {
			colors[indices[count++]] = blanc;
		}

		ball1.setupInstanceColors(colors);
		ball1.drawInstanced();

	
		cubeMapShader.use();
		cubeMapShader.setMatrix4("V", view);
		cubeMapShader.setMatrix4("P", perspective);
		cubeMapShader.setFloat("intensity", intensityMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP,cubeMapTexture);
		cubeMap.draw();
		glDepthFunc(GL_LESS);

		particleSystem.drawParticles(view, perspective, camera.GetCameraRight(), camera.GetCameraUp());
		bool textActive = elapsed < 10.0;

		fps(now);

		shadertext.use();
		RenderText(shadertext,
				gQuest.objective(),
				275.0f, 580.0f,   // position (px)
				0.3f,            // échelle
				glm::vec3(1.0f), text_VAO, text_VBO); // couleur blanche
		RenderText(shadertext,
				gQuest.guardianLine(),
				12.0f, 550.0f,   // position (px)
				0.4f,            // échelle
				glm::vec3(1.0f,0.0f, 0.0f ), text_VAO, text_VBO); // couleur blanche
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

	
	/* ====== Debug toggles ====== */
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) 
		speedTime += 0.1f;

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) 
		speedTime -= 0.1f;

    static bool hPressed = false;
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        if (!hPressed) {
            showHitboxes = !showHitboxes;
        }
        hPressed = true;
    } else {
        hPressed = false;
    }

	// 3. God-mode (G)
	static bool gPressed = false;
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
		if (!gPressed) {
			godMode = !godMode;
		}
		gPressed = true;
	} else {
		gPressed = false;
	}

	// --- Clamping joueur ---
    if (!godMode) {
        camera.Position.x = glm::clamp(camera.Position.x, -30.0f, 30.0f);
        camera.Position.z = glm::clamp(camera.Position.z, -30.0f, 30.0f);
        camera.Position.y = glm::clamp(camera.Position.y,   1.0f,  2.0f);
    }


}
// Mouse button callback for picking
static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        int winW, winH;
        glfwGetWindowSize(window, &winW, &winH);

        // OpenGL’s origin is at the lower‑left
        PickingTexture::PixelInfo info = gPickingTexture.ReadPixel(static_cast<unsigned int>(xpos),
                                                                  static_cast<unsigned int>(winH - ypos - 1));

        std::cout << "\nPicked Object ID: " << info.ObjectID << std::endl;

		gSelectedObjectID    = static_cast<int>(info.ObjectID);
		bool nowIsNight = std::sin(-damptime * speedTime * glfwGetTime()) < 0.0f;
		gQuest.onInteract(gSelectedObjectID, nowIsNight);
        gSelectionTimestamp  = glfwGetTime();
    }
}
