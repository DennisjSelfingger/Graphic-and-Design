///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"


#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	//added this to make it work
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	CreateGLTexture("C:/Users/dself/Downloads/CS330Content/CS330Content/Utilities/textures/rusticwood.jpg", "wood_texture");
	//CreateGLTexture("C:/Users/dself/Downloads/CS330Content/CS330Content/Utilities/textures/tankGlass.jpg", "tank_glass");
	CreateGLTexture("C:/Users/dself/Downloads/CS330Content/CS330Content/Utilities/textures/goodWater.png", "water_texture");
	CreateGLTexture("C:/Users/dself/Downloads/CS330Content/CS330Content/Utilities/textures/knife_handle.jpg", "lip_texture");
	CreateGLTexture("C:/Users/dself/Downloads/CS330Content/CS330Content/Utilities/textures/hardwoodFloor.png", "floor_texture");
	CreateGLTexture("C:/Users/dself/Downloads/CS330Content/CS330Content/Utilities/textures/carpet.png", "carpet_texture");
	CreateGLTexture("C:/Users/dself/Downloads/CS330Content/CS330Content/Utilities/textures/stainless_end.jpg", "handle_texture");


	
	BindGLTextures();
}

void SceneManager::DefineObjectMaterials()
{
	// Glass material for the aquarium - brighter
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.5f);
	glassMaterial.ambientStrength = 0.5f;  // Increased from 0.1
	glassMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 32.0f;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	// Wood material - brighter
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.4f, 0.2f, 0.0f);
	woodMaterial.ambientStrength = 0.6f;  // Increased from 0.2
	woodMaterial.diffuseColor = glm::vec3(0.8f, 0.4f, 0.2f);
	woodMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	woodMaterial.shininess = 8.0f;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	// Metal material - brighter
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
	metalMaterial.ambientStrength = 0.5f;  // Increased from 0.2
	metalMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	metalMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	metalMaterial.shininess = 64.0f;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	// Carpet material - brighter
	OBJECT_MATERIAL carpetMaterial;
	carpetMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	carpetMaterial.ambientStrength = 0.7f;  // Increased from 0.3
	carpetMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	carpetMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	carpetMaterial.shininess = 4.0f;
	carpetMaterial.tag = "carpet";
	m_objectMaterials.push_back(carpetMaterial);
}

void SceneManager::SetupSceneLights()
{
	
		// Main overhead light - much brighter
		m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(0.0f, 5.0f, 2.0f));
		m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(1.0f, 1.0f, 1.0f));
		m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(1.0f, 1.0f, 1.0f));
		m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(1.0f, 1.0f, 1.0f));
		m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 16.0f);
		m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 3.0f);

		// Front light - adding strong frontal illumination
		m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(0.0f, 2.0f, 5.0f));
		m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.5f, 0.5f, 0.5f));
		m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(1.0f, 1.0f, 1.0f));
		m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(1.0f, 1.0f, 1.0f));
		m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 16.0f);
		m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 2.0f);

		// Add a third fill light from the opposite side see if this acually works
		m_pShaderManager->setVec3Value("lightSources[2].position", glm::vec3(0.0f, 3.0f, -8.0f));
		m_pShaderManager->setVec3Value("lightSources[2].ambientColor", glm::vec3(0.3f, 0.3f, 0.3f));
		m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", glm::vec3(0.7f, 0.7f, 0.7f));
		m_pShaderManager->setVec3Value("lightSources[2].specularColor", glm::vec3(0.7f, 0.7f, 0.7f));
		m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 16.0f);
		m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 1.0f);

		// Make sure lighting is enabled
		m_pShaderManager->setBoolValue("bUseLighting", true);
	
}

void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();

	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();  // Add this line
	m_basicMeshes->LoadPlaneMesh();  // Add plane for the floor 
	//texture for glass, then going to try and do water background
	// Adding debug code because it was not loading
	
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Add this near the start of RenderScene()
	m_pShaderManager->setVec3Value("viewPosition", camera.Position.x, camera.Position.y, camera.Position.z);
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 10.0f;// I rotated this for a better perspective so it align more with picture
	float ZrotationDegrees = 0.0f;

	// Aquarium tank (upper box)
	// using texture instead of color
	//adding below two lines to try to enable it to blend
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//  outline for tank
	 SetShaderTexture("lip_texture"); 
	 SetShaderMaterial("metal");
	 SetTextureUVScale(0.5f, 0.5f);
	//SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);  // Pure black color
	glm::vec3 outlineScale = glm::vec3(5.1f, 2.1f, 1.1f);  // Slightly larger than tank
	glm::vec3 outlinePosition = glm::vec3(0.0f, 2.0f, -0.1f);  // Same position as tank but slightly behind
	SetTransformations(outlineScale, XrotationDegrees, YrotationDegrees, ZrotationDegrees, outlinePosition);
	m_basicMeshes->DrawBoxMesh();

	SetShaderTexture("water_texture");
	SetShaderMaterial("glass");
	// Use texture instead of color
	SetTextureUVScale(3.0f, 2.0f);
	//SetShaderColor(0.7f, 0.9f, 1.0f, 0.3f);  // Light blue color si I can seperate the top from bottom
	glm::vec3 tankScale = glm::vec3(5.0f, 2.0f, 1.5f);// making it wider than taller to match pic, not sure i need this comment
	glm::vec3 tankPosition = glm::vec3(0.0f, 2.0f, 0.0f);
	SetTransformations(tankScale, XrotationDegrees, YrotationDegrees, ZrotationDegrees, tankPosition);
	m_basicMeshes->DrawBoxMesh();
	//added below to blend
	glDisable(GL_BLEND);

	// Oval light above tank representing fish hanging on wall..stuffed bass it is
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SetShaderTexture("wood_texture");
	SetShaderMaterial("wood");// Use texture instead of color
	SetTextureUVScale(3.0f, 2.0f);
	//SetShaderColor(1.0f, 1.0f, 0.8f, 1.0f);  // Warm light color
	glm::vec3 ovalScale = glm::vec3(1.0f, 0.3f, 0.8f);  // Stretched horizontally, compressed vertically just representing position and space
	glm::vec3 ovalPosition = glm::vec3(0.0f, 4.0f, 0.0f);  // Positioned above the tank
	SetTransformations(ovalScale, XrotationDegrees, YrotationDegrees, ZrotationDegrees, ovalPosition);
	m_basicMeshes->DrawSphereMesh();

	// Wooden stand (lower box)
	SetShaderTexture("wood_texture");
	SetShaderMaterial("wood");
	SetTextureUVScale(0.5f, 0.5f);
	//SetShaderColor(0.0f, 0.0f, 0.5f, 1.0f);  // Dark blue color to seperate the bottom
	glm::vec3 standScale = glm::vec3(4.8f, 2.0f, 1.5f);
	glm::vec3 standPosition = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(standScale, XrotationDegrees, YrotationDegrees, ZrotationDegrees, standPosition);
	m_basicMeshes->DrawBoxMesh();

	// Door in the middle of the stand
	SetShaderTexture("lip_texture");  // different texture so the door can be seen
	SetShaderMaterial("wood");
	SetTextureUVScale(0.25f, 0.25f);   // Smaller UV scale for more detailed wood grain on the door
	glm::vec3 doorScale = glm::vec3(1.6f, 1.0f, 0.1f);  // Make it thinner than the stand but proportional
	glm::vec3 doorPosition = glm::vec3(0.0f, 0.0f, 0.75f);  // Position it  in front of the stand
	SetTransformations(doorScale, XrotationDegrees, YrotationDegrees, 90.0f, doorPosition);
	m_basicMeshes->DrawBoxMesh();

	// Door handle (small sphere on right side of door)
	SetShaderTexture("handle_texture");  // Using wood texture for the handle
	SetShaderMaterial("metal");
	SetTextureUVScale(0.1f, 0.1f);
	glm::vec3 handleScale = glm::vec3(0.1f, 0.1f, 0.1f);  // Small sphere
	glm::vec3 handlePosition = glm::vec3(-0.3f, 0.0f, 0.9f);  // Positioned right side of door, slightly more forward
	SetTransformations(handleScale, XrotationDegrees, YrotationDegrees, ZrotationDegrees, handlePosition);
	m_basicMeshes->DrawSphereMesh();  // Using sphere mesh for round handle

	// Bottom lip/base
	//// Medium blue creates transition between stand and floor
	SetShaderTexture("lip_texture");
	SetShaderMaterial("metal");
	SetTextureUVScale(0.5f, 0.5f);
	//SetShaderColor(0.2f, 0.2f, 0.8f, 1.0f);  // Different shade of blue
	glm::vec3 lipScale = glm::vec3(5.2f, 0.2f, 1.7f);  // a little wider than finished will
	// be but the third shape i added is just the base and will be adjusted....just added and extra step
	glm::vec3 lipPosition = glm::vec3(0.0f, -1.0f, 0.0f);  // lip below the stand
	SetTransformations(lipScale, XrotationDegrees, YrotationDegrees, ZrotationDegrees, lipPosition);
	m_basicMeshes->DrawBoxMesh();

	// Floor plane
	SetShaderTexture("carpet_texture");
	SetShaderMaterial("carpet");
	SetTextureUVScale(4.0f, 4.0f);
	//SetShaderColor(0.4f, 0.4f, 0.4f, 1.0f);  // Grey color for floor
	glm::vec3 planeScale = glm::vec3(15.0f, 1.0f, 15.0f);  // Make it large enough for the scene
	glm::vec3 planePosition = glm::vec3(0.0f, -1.2f, 0.0f);  // Slightly below the bottom lip
	SetTransformations(planeScale, XrotationDegrees, YrotationDegrees, ZrotationDegrees, planePosition);
	m_basicMeshes->DrawPlaneMesh();
}