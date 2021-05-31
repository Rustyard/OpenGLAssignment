#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <sstream>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include "../include/getBMP.h"

#define ID_LIGHT_OFF 0
#define ID_LIGHT_ON 1
#define ID_LIGHT_TURN_RED 2
#define ID_LIGHT_TURN_GREEN 3
#define ID_LIGHT_TURN_BLUE 4
#define ID_LIGHT_TURN_WHITE 5
#define ID_LIGHT_TURN_DIM 6
#define ID_QUIT 7

#define MODEL_NUMBERS 5

#define OBJ_BUNNY 0
#define OBJ_CAT 1
#define OBJ_DOG 2
#define OBJ_DUCK 3
#define OBJ_TIGER 4

using namespace std;

// Function declaration.
void makeMenu();
void enableLighting();
void movement();

// Keymap, for smooth keyboard movement control
map<unsigned char, bool> keyState;

// Globals.
static float PI = 3.1415926;
static int windowWidth = 800, windowHeight = 800;
static unsigned int texture[1]; // Array of texture indices.
static unsigned int textureCube; // Skybox.
static unsigned int textureTiger; // texture for tiger.
static float cameraX = 0.0f, cameraY = 10.0f, cameraZ = 15.0f; // Camera position.
static float lookatX = 0.0f, lookatY = 10.0f, lookatZ = 0.0f; // Camera look at position.
static float upX = 0.0f, upY = 1.0f, upZ = 0.0f; // Camera upward vector.
static float yaw = PI, pitch = 0.0f; // Camera rotation angle.
static bool canMoveCamera = false;
static float sensitivity = 0.001f; // mouse sensitivity
static bool enableLight = true;
static float moveSpeed = 0.1f;
static float fov = 70.0f;
static int controlModel = 0; // showing which model is in control

// global lighting
static float lightAmb[] = { 0.0, 0.0, 0.0, 1.0 };
static float lightDifAndSpec[] = { 1.0, 1.0, 1.0, 1.0 };
static float lightPos[] = { -20, 20, 20, 0.0 }; // fourth value: 0 for spot light, 1 for directional light
static float globAmb[] = { 0.2, 0.2, 0.2, 1.0 };

// model control
static float tBunny[] = {0.0, 0.0, 0.0};
static float tCat[] = {0.0, 0.0, 0.0};
static float tDog[] = {0.0, 0.0, 0.0};
static float tDuck[] = {0.0, 0.0, 0.0};
static float tTiger[] = {0.0, 0.0, 0.0};

static float rBunny[] = {0.0, 0.0, 0.0};
static float rCat[] = {0.0, 0.0, 0.0};
static float rDog[] = {0.0, 0.0, 0.0};
static float rDuck[] = {0.0, 0.0, 0.0};
static float rTiger[] = {0.0, 0.0, 0.0};

// Vectors used in model processing.
/**
 * Vectors of vertices of different objects, in this structure:
 * {x0, y0, z0, x1, y1, z1, ... , xN, yN, zN, ... }, { ... }, {...}
 * ^object1                                          ^object2 ^object3 and so on
 *
 * e.g. {x2,y2,z2} is the coordinate of point 2 (the third point), written in code like this:
 *      verticesOf[thisObj][2*3], verticesOf[thisObj][2*3 + 1], verticesOf[thisObj][2*3 + 2]
 *                          ^                         ^                             ^
 *                               these are treated as the index of certain point
 */
static vector<vector<float>> verticesOf;

/**
 * The texture coordinate of different objects.
 * {x0, y0, x1, y1, ... }, { ... }, { ... }
 *
 * e.g. {x1, y1} is the texture coordinate of vertex 1
 *      textureCoordinateOf[thisObj][1*2], textureCoordinateOf[thisObj][1*2 + 1]
 */
static vector<vector<float>> textureCoordinateOf;

/**
 * Vectors of triangle-only faces of different objects, in this structure:
 * {f0v0, f0v1, f0v2, f1v0, f1v1, f1v2, ... }, { ... }, ...
 * ^object1                                    ^object2 and so on
 *
 * f for face, v for vertex, each face has and only has 3 vertices
 * e.g. f1v1 is the second point of the second face. In code:
 *       int vertexIndex = facesOf[thisObj][1*3 + 1];
 *                                          ^     ^
 *                                          face  vertex
 */
static vector<vector<int>> facesOf;

/**
 * Stores the normal coordinate for each face (face normals) of different objects.
 * Used in flat shading.
 * {x0, y0, z0, x1, y1, z1, ... , xN, yN, zN, ... }, { ... }, ...
 * e.g. {x0,y0,z0} is the normal vector coordinate of face 0.
 */
static vector<vector<float>> faceNormalsOf;

/**
 * Stores the normal coordinate for each vertex (vertex normals) of different objects.
 * Used in smooth shading.
 * {x0, y0, z0, x1, y1, z1, ... , xN, yN, zN, ... }, { ... }, ...
 * e.g. {x0,y0,z0} is the normal vector coordinate of vertex 0.
 */
static vector<vector<float>> vertexNormalsOf;

/**
 * Stores the volume of each face, for different objects.
 * Used for calculating vertex normals.
 * {volume0, volume1, volume2, ... }, { ... }, ...
 * e.g. volume0 is the volume of face 0. In code:
 *      float volume = faceVolumesOf[thisObj][0];
 */
static vector<vector<float>> faceVolumesOf;

/**
 * Bounding box center coordinates for each objects.
 * {x0, y0, z0, x1, y1, z1, ... }
 * e.g. {x0,y0,z0} is the center coordinate for object 0. In code:
 *      centerOf[0*3], centerOf[0*3 + 1], centerOf[0*3 + 2]
 */
static vector<float> centerOf;

/**
 * Diagonal length of bounding box of each objects.
 * {length0, length1, length2, ... }
 */
static vector<float> diagonalLengthOf;
// Vector section end

// Implementation
/**
 * Compute the bounding box for a specified object.
 * @param thisObj Indicating which object is being processed.
 */
void ComputeBoundingBox(int thisObj)
{
    float minX,maxX, minY,maxY, minZ,maxZ;
    // read value from vertices vector
    for (int i = 0; i < verticesOf[thisObj].size(); i += 3) // massive bug fix in this "for"!
    {
        if (i == 0) {
            minX = maxX = verticesOf[thisObj][i];
            minY = maxY = verticesOf[thisObj][i + 1];
            minZ = maxZ = verticesOf[thisObj][i + 2];
        }
        else {
            if (verticesOf[thisObj][i] > maxX) maxX = verticesOf[thisObj][i];
            if (verticesOf[thisObj][i] < minX) minX = verticesOf[thisObj][i];
            if (verticesOf[thisObj][i + 1] > maxY) maxY = verticesOf[thisObj][i + 1];
            if (verticesOf[thisObj][i + 1] < minY) minY = verticesOf[thisObj][i + 1];
            if (verticesOf[thisObj][i + 2] > maxZ) maxZ = verticesOf[thisObj][i + 2];
            if (verticesOf[thisObj][i + 2] < minZ) minZ = verticesOf[thisObj][i + 2];
        }
    }
    centerOf[thisObj * 3]     = (minX + maxX) / 2;
    centerOf[thisObj * 3 + 1] = (minY + maxY) / 2;
    centerOf[thisObj * 3 + 2] = (minZ + maxZ) / 2;

    diagonalLengthOf[thisObj] = (float)sqrt(pow(maxX - minX, 2.0) + pow(maxY - minY, 2.0) + pow(maxZ - minZ, 2.0));

    // center all the vertices
    for (int i = 0; i < verticesOf[thisObj].size(); i += 3) {
        verticesOf[thisObj][i]   -= centerOf[thisObj*3];
        verticesOf[thisObj][i+1] -= centerOf[thisObj*3 + 1];
        verticesOf[thisObj][i+2] -= centerOf[thisObj*3 + 2];
    }
}

/**
 * Compute the face normal of each face.
 * @param thisObj Indicating which object is being processed.
 */
void ComputeFaceNormals(int thisObj)
{
    float firstPoint[3] = { 0.0, 0.0, 0.0 };
    float secondPoint[3] = { 0.0, 0.0, 0.0 };
    float thirdPoint[3] = { 0.0, 0.0, 0.0 };

    float firstVector[3] = { 0.0,0.0,0.0 };
    float secondVector[3] = { 0.0,0.0,0.0 };

    // float tempCenterX, tempCenterY, tempCenterZ;
    float tempNormalX, tempNormalY, tempNormalZ;

    for (int i = 0; i < facesOf[thisObj].size(); i += 3)
    {
        // get the x,y,z of first, second and third point of the face
        firstPoint[0]  = verticesOf[thisObj][facesOf[thisObj][  i  ] * 3];
        firstPoint[1]  = verticesOf[thisObj][facesOf[thisObj][  i  ] * 3 + 1];
        firstPoint[2]  = verticesOf[thisObj][facesOf[thisObj][  i  ] * 3 + 2];
        secondPoint[0] = verticesOf[thisObj][facesOf[thisObj][i + 1] * 3];
        secondPoint[1] = verticesOf[thisObj][facesOf[thisObj][i + 1] * 3 + 1];
        secondPoint[2] = verticesOf[thisObj][facesOf[thisObj][i + 1] * 3 + 2];
        thirdPoint[0]  = verticesOf[thisObj][facesOf[thisObj][i + 2] * 3];
        thirdPoint[1]  = verticesOf[thisObj][facesOf[thisObj][i + 2] * 3 + 1];
        thirdPoint[2]  = verticesOf[thisObj][facesOf[thisObj][i + 2] * 3 + 2];
        // FYI:                             ^This is a vertex index^

        // calculate 2 vectors from three ordered points
        firstVector[0] = secondPoint[0] - firstPoint[0];
        firstVector[1] = secondPoint[1] - firstPoint[1];
        firstVector[2] = secondPoint[2] - firstPoint[2];
        secondVector[0] = thirdPoint[0] - secondPoint[0];
        secondVector[1] = thirdPoint[1] - secondPoint[1];
        secondVector[2] = thirdPoint[2] - secondPoint[2];

        // compute normal
        tempNormalX = firstVector[1] * secondVector[2] - firstVector[2] * secondVector[1];
        tempNormalY = firstVector[2] * secondVector[0] - firstVector[0] * secondVector[2];
        tempNormalZ = firstVector[0] * secondVector[1] - firstVector[1] * secondVector[0];

        double tempNormalLength = sqrt(pow(tempNormalX,2) + pow(tempNormalY,2) + pow(tempNormalZ,2));
        // normalize
        faceNormalsOf[thisObj].push_back((float)(tempNormalX / tempNormalLength));
        faceNormalsOf[thisObj].push_back((float)(tempNormalY / tempNormalLength));
        faceNormalsOf[thisObj].push_back((float)(tempNormalZ / tempNormalLength));
    }
}

/**
 * Compute the vertex normal of each vertex. Vertex normal is calculated with weighted averaging of
 * face normals whose face contains that vertex.
 * @param thisObj Indicating which object is being processed.
 */
void ComputeVertexNormals(int thisObj)
{
    unsigned int vertexCount = verticesOf[thisObj].size() / 3;

    float firstPoint[3] = { 0.0, 0.0, 0.0 };
    float secondPoint[3] = { 0.0, 0.0, 0.0 };
    float thirdPoint[3] = { 0.0, 0.0, 0.0 };

    /**
    * Stores the information of all faces each vertex is in, for current object only.
    * Structure:
    * {v0f0, v0f1, v0f2, ... }, {v1f0, v1f1, ... }, { ... }, ...
    *   ^vertex0                  ^vertex1
    * In code example:
    *      vertexToFace[vertex0][face0]
    */
    vector<vector<int>> vertexToFace(vertexCount);

    for (int i = 0; i < facesOf[thisObj].size(); i += 3) {
        // get the x,y,z of first, second and third point of the face
        firstPoint[0]  = verticesOf[thisObj][facesOf[thisObj][  i  ] * 3];
        firstPoint[1]  = verticesOf[thisObj][facesOf[thisObj][  i  ] * 3 + 1];
        firstPoint[2]  = verticesOf[thisObj][facesOf[thisObj][  i  ] * 3 + 2];
        secondPoint[0] = verticesOf[thisObj][facesOf[thisObj][i + 1] * 3];
        secondPoint[1] = verticesOf[thisObj][facesOf[thisObj][i + 1] * 3 + 1];
        secondPoint[2] = verticesOf[thisObj][facesOf[thisObj][i + 1] * 3 + 2];
        thirdPoint[0]  = verticesOf[thisObj][facesOf[thisObj][i + 2] * 3];
        thirdPoint[1]  = verticesOf[thisObj][facesOf[thisObj][i + 2] * 3 + 1];
        thirdPoint[2]  = verticesOf[thisObj][facesOf[thisObj][i + 2] * 3 + 2];

        // compute face volume for each face
        float temp1P = (secondPoint[1] - firstPoint[1]) * (thirdPoint[2] - firstPoint[2]);
        float temp2P = (secondPoint[2] - firstPoint[2]) * (thirdPoint[0] - firstPoint[0]);
        float temp3P = (secondPoint[0] - firstPoint[0]) * (thirdPoint[1] - firstPoint[1]);
        float temp1N = (secondPoint[1] - firstPoint[1]) * (thirdPoint[0] - firstPoint[0]);
        float temp2N = (secondPoint[0] - firstPoint[0]) * (thirdPoint[2] - firstPoint[2]);
        float temp3N = (secondPoint[2] - firstPoint[2]) * (thirdPoint[1] - firstPoint[1]);
        float tempFaceVolume = (float)0.5 * abs(temp1P + temp2P + temp3P - temp1N - temp2N - temp3N);
        faceVolumesOf[thisObj].push_back(tempFaceVolume);

        // register face information for its vertices
        vertexToFace[facesOf[thisObj][  i  ]].push_back(i / 3);
        vertexToFace[facesOf[thisObj][i + 1]].push_back(i / 3);
        vertexToFace[facesOf[thisObj][i + 2]].push_back(i / 3);
        // FYI:      ^-----this vertex-----^ is in      ^ this face
    }


    for (int i = 0; i < vertexCount; i++) {
        int j;
        float resultNormalX = 0.0;
        float resultNormalY = 0.0;
        float resultNormalZ = 0.0;
        vector<float> faceVolume;
        vector<float> faceNormal;
        float totalVolume = 0.0;

        // get the face volumes for each face which contains vertex i
        // get the normals too
        for (j = 0; j < vertexToFace[i].size(); j++) {
            faceVolume.push_back(faceVolumesOf[thisObj][vertexToFace[i][j]]);
            faceNormal.push_back(faceNormalsOf[thisObj][vertexToFace[i][j] * 3]);
            faceNormal.push_back(faceNormalsOf[thisObj][vertexToFace[i][j] * 3 + 1]);
            faceNormal.push_back(faceNormalsOf[thisObj][vertexToFace[i][j] * 3 + 2]);
        }

        // compute the actual normal
        for (j = 0; j < faceVolume.size(); j++) {
            totalVolume += faceVolume[j];
        }
        for (j = 0; j < faceVolume.size(); j++) {
            resultNormalX += (faceVolume[j] / totalVolume) * faceNormal[j * 3];
            resultNormalY += (faceVolume[j] / totalVolume) * faceNormal[j * 3 + 1];
            resultNormalZ += (faceVolume[j] / totalVolume) * faceNormal[j * 3 + 2];
        }
        auto resultNormalLength = (float)sqrt(pow(resultNormalX, 2) + pow(resultNormalY, 2) + pow(resultNormalZ, 2));
        // normalize
        vertexNormalsOf[thisObj].push_back(resultNormalX / resultNormalLength);
        vertexNormalsOf[thisObj].push_back(resultNormalY / resultNormalLength);
        vertexNormalsOf[thisObj].push_back(resultNormalZ / resultNormalLength);
    }
}

/**
 * Load an OBJ file into verticesOf[thisObj] and facesOf[thisObj].
 * @param fileName The name of OBJ file to load.
 * @param thisObj The object index.
 */
void loadOBJ(const std::string& fileName, int thisObj)
{
    // clear the data before reading into it
    verticesOf[thisObj].clear();
    facesOf[thisObj].clear();
    textureCoordinateOf[thisObj].clear();

    std::string line;
    int count, vertexIndex1, vertexIndex2, vertexIndex3;
    float coordinateValue;
    char currentCharacter, previousCharacter;

    // Open the OBJ file.
    std::ifstream inFile(fileName.c_str(), std::ifstream::in);

    // Read successive lines.
    while (getline(inFile, line))
    {
        // Line has vertex data.
        if (line.substr(0, 2) == "v ")
        {
            // Initialize a string from the character after "v " to the end.
            std::istringstream currentString(line.substr(2));

            // Read x, y and z values. The (optional) w value is not read.
            for (count = 1; count <= 3; count++)
            {
                currentString >> coordinateValue;
                verticesOf[thisObj].push_back(coordinateValue);
            }
        }

        // Line has face data.
        else if (line.substr(0, 2) == "f ")
        {
            // Initialize a string from the character after "f " to the end.
            std::istringstream currentString(line.substr(2));

            // Strategy in the following to detect a vertex index within a face line is based on the
            // fact that vertex indices are exactly those that follow a white space. Texture and
            // normal indices are ignored.
            // Moreover, from the third vertex of a face on output one triangle per vertex, that
            // being the next triangle in a fan triangulation of the face about the first vertex.
            previousCharacter = ' ';
            count = 0;
            while (currentString.get(currentCharacter))
            {
                // Stop processing line at comment.
                if ((previousCharacter == '#') || (currentCharacter == '#')) break;

                // Current character is the start of a vertex index.
                if ((previousCharacter == ' ') && (currentCharacter != ' '))
                {
                    // Move the string cursor back to just before the vertex index.
                    currentString.unget();

                    // Read the first vertex index, decrement it so that the index range is from 0, increment vertex counter.
                    if (count == 0)
                    {
                        currentString >> vertexIndex1;
                        vertexIndex1--;
                        count++;
                    }

                        // Read the second vertex index, decrement it, increment vertex counter.
                    else if (count == 1)
                    {
                        currentString >> vertexIndex2;
                        vertexIndex2--;
                        count++;
                    }

                        // Read the third vertex index, decrement it, increment vertex counter AND output the first triangle.
                    else if (count == 2)
                    {
                        currentString >> vertexIndex3;
                        vertexIndex3--;
                        count++;
                        facesOf[thisObj].push_back(vertexIndex1);
                        facesOf[thisObj].push_back(vertexIndex2);
                        facesOf[thisObj].push_back(vertexIndex3);
                    }

                        // From the fourth vertex and on output the next triangle of the fan.
                    else
                    {
                        vertexIndex2 = vertexIndex3;
                        currentString >> vertexIndex3;
                        vertexIndex3--;
                        facesOf[thisObj].push_back(vertexIndex1);
                        facesOf[thisObj].push_back(vertexIndex2);
                        facesOf[thisObj].push_back(vertexIndex3);
                    }

                    // Begin the process of detecting the next vertex index just after the vertex index just read.
                    currentString.get(previousCharacter);
                }

                    // Current character is not the start of a vertex index. Move ahead one character.
                else previousCharacter = currentCharacter;
            }
        }
        // line has texture coordinate data
        else if (line.substr(0, 3) == "vt ") {
            // Initialize a string from the character after "vt " to the end.
            std::istringstream currentString(line.substr(3));

            // Read x, y and z values. The (optional) w value is not read.
            for (count = 1; count <= 2; count++)
            {
                currentString >> coordinateValue;
                textureCoordinateOf[thisObj].push_back(coordinateValue);
            }
        }
        // Nothing other than vertex and face data and texture coordinate is processed.
    }

    // Close the OBJ file.
    inFile.close();
}

/**
 * Load an OBJ file and call calculation functions to process that OBJ.
 * @param fileName The name of OBJ file to load.
 * @param thisObj The object index.
 */
void loadOBJAndProcess(const std::string& fileName, int thisObj)
{
    loadOBJ(fileName, thisObj);
    ComputeBoundingBox(thisObj);
    ComputeFaceNormals(thisObj);
    ComputeVertexNormals(thisObj);
}

// Load external textures.
void loadTextures()
{
    // load tiger texture.
    imageFile *tigerTexture;
    tigerTexture = getBMP("../models/TigerTexture.bmp");
    glBindTexture(GL_TEXTURE_2D, textureTiger);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tigerTexture->width, tigerTexture->height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, tigerTexture->data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Local storage for bmp image data.
    imageFile *image[1];

    // Load the images.
    image[0] = getBMP("../textures/grass.bmp");

    // Bind grass image to texture object texture[0].
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image[0]->width, image[0]->height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, image[0]->data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // load skybox texture, code from skybox.cpp
    // Local storage for bmp image data.
    imageFile *imageCube[6];

    // Load the six cube map images.
    imageCube[0] = getBMP("../textures/IceRiver/posx.bmp");
    imageCube[1] = getBMP("../textures/IceRiver/negx.bmp");
    imageCube[2] = getBMP("../textures/IceRiver/posy.bmp");
    imageCube[3] = getBMP("../textures/IceRiver/negy.bmp");
    imageCube[4] = getBMP("../textures/IceRiver/posz.bmp");
    imageCube[5] = getBMP("../textures/IceRiver/negz.bmp");

    // Bind the cube map texture and define its 6 component textures.
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureCube);
    for (int face = 0; face < 6; face++)
    {
        int target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
        glTexImage2D(target, 0, GL_RGBA, imageCube[0]->width, imageCube[0]->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageCube[face]->data);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

// Initialization routine.
void setup()
{
    // initialize vectors
    verticesOf.resize(MODEL_NUMBERS);
    facesOf.resize(MODEL_NUMBERS);
    // faceCentersOf.resize(MODEL_NUMBERS);
    faceNormalsOf.resize(MODEL_NUMBERS);
    vertexNormalsOf.resize(MODEL_NUMBERS);
    faceVolumesOf.resize(MODEL_NUMBERS);
    centerOf.resize(MODEL_NUMBERS * 3);
    diagonalLengthOf.resize(MODEL_NUMBERS);
    textureCoordinateOf.resize(MODEL_NUMBERS);

    glClearColor(1.0, 1.0, 1.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_CLAMP);

    // load obj models
    loadOBJAndProcess("../models/Bunny.obj", OBJ_BUNNY);
    loadOBJAndProcess("../models/Cat.obj", OBJ_CAT);
    loadOBJAndProcess("../models/Dog.obj", OBJ_DOG);
    loadOBJAndProcess("../models/Duck.obj", OBJ_DUCK);
    loadOBJAndProcess("../models/Tiger.obj", OBJ_TIGER);

    // Create texture ids.
    glGenTextures(2, texture);
    glGenTextures(1, &textureCube);

    // Load external textures.
    loadTextures();


    // Turn on OpenGL texturing.
    glEnable(GL_TEXTURE_2D);

    // Create menu.
    makeMenu();
}

/**
 * Draw certain model in the scene.
 * @param thisObj The object index.
 * @param isFlatShaded Is render style flat or smooth.
 * @param x Offset for x-axis.
 * @param y Offset for y-axis.
 * @param z Offset for z-axis.
 * @param s Relative scale.
 */
void drawMesh(int thisObj, bool isFlatShaded, const float* translate, float scaleAll, float* angleRotate, float* color)
{
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glPushMatrix();

    glEnable(GL_NORMALIZE); // crucial operation when scaling model: re-normalize all normals

    float s = scaleAll / diagonalLengthOf[thisObj];

    bool hasTexture = false;
    if (!textureCoordinateOf[thisObj].empty()) {
        hasTexture = true;
    }

    glScalef(s, s, s);
    glTranslatef(translate[0]/s, translate[1]/s, translate[2]/s); // move
    // rotate
    glRotatef(angleRotate[0], 1.0, 0.0, 0.0);
    glRotatef(angleRotate[1], 0.0, 1.0, 0.0);
    glRotatef(angleRotate[2], 0.0, 0.0, 1.0);

    // Material property vectors.
    float matAmbAndDif[] = { color[0], color[1], color[2], 1.0 };
    float matSpec[] = { 1.0, 1.0, 1.0, 1.0 };
    float matShine[] = { 50.0 };

    // Material properties.
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, matAmbAndDif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShine);

    if (isFlatShaded) {
        glShadeModel(GL_FLAT);
        glBegin(GL_TRIANGLES);
        for (int i = 0; i < facesOf[thisObj].size(); i += 3) {
            glNormal3f(faceNormalsOf[thisObj][i],faceNormalsOf[thisObj][i+1],faceNormalsOf[thisObj][i+2]);
            if (hasTexture) glTexCoord2f(textureCoordinateOf[thisObj][facesOf[thisObj][i]*2],
                                         textureCoordinateOf[thisObj][facesOf[thisObj][i]*2 + 1]);
            glVertex3f(verticesOf[thisObj][facesOf[thisObj][i]*3],
                       verticesOf[thisObj][facesOf[thisObj][i]*3+1],
                       verticesOf[thisObj][facesOf[thisObj][i]*3+2]);
            if (hasTexture) glTexCoord2f(textureCoordinateOf[thisObj][facesOf[thisObj][i+1]*2],
                                         textureCoordinateOf[thisObj][facesOf[thisObj][i+1]*2 + 1]);
            glVertex3f(verticesOf[thisObj][facesOf[thisObj][i+1]*3],
                       verticesOf[thisObj][facesOf[thisObj][i+1]*3+1],
                       verticesOf[thisObj][facesOf[thisObj][i+1]*3+2]);
            if (hasTexture) glTexCoord2f(textureCoordinateOf[thisObj][facesOf[thisObj][i+2]*2],
                                         textureCoordinateOf[thisObj][facesOf[thisObj][i+2]*2 + 1]);
            glVertex3f(verticesOf[thisObj][facesOf[thisObj][i+2]*3],
                       verticesOf[thisObj][facesOf[thisObj][i+2]*3+1],
                       verticesOf[thisObj][facesOf[thisObj][i+2]*3+2]);
        }
        glEnd();
    }
    else {
        glShadeModel(GL_SMOOTH);
        glBegin(GL_TRIANGLES);
        for (int i = 0; i < facesOf[thisObj].size(); i += 3) {
            glNormal3f(vertexNormalsOf[thisObj][facesOf[thisObj][i]*3],
                       vertexNormalsOf[thisObj][facesOf[thisObj][i]*3+1],
                       vertexNormalsOf[thisObj][facesOf[thisObj][i]*3+2]);
            if (hasTexture) glTexCoord2f(textureCoordinateOf[thisObj][facesOf[thisObj][i]*2],
                                         textureCoordinateOf[thisObj][facesOf[thisObj][i]*2 + 1]);
            glVertex3f(verticesOf[thisObj][facesOf[thisObj][i]*3],
                       verticesOf[thisObj][facesOf[thisObj][i]*3+1],
                       verticesOf[thisObj][facesOf[thisObj][i]*3+2]);
            glNormal3f(vertexNormalsOf[thisObj][facesOf[thisObj][i+1]*3],
                       vertexNormalsOf[thisObj][facesOf[thisObj][i+1]*3+1],
                       vertexNormalsOf[thisObj][facesOf[thisObj][i+1]*3+2]);
            if (hasTexture) glTexCoord2f(textureCoordinateOf[thisObj][facesOf[thisObj][i+1]*2],
                                         textureCoordinateOf[thisObj][facesOf[thisObj][i+1]*2 + 1]);
            glVertex3f(verticesOf[thisObj][facesOf[thisObj][i+1]*3],
                       verticesOf[thisObj][facesOf[thisObj][i+1]*3+1],
                       verticesOf[thisObj][facesOf[thisObj][i+1]*3+2]);
            glNormal3f(vertexNormalsOf[thisObj][facesOf[thisObj][i+2]*3],
                       vertexNormalsOf[thisObj][facesOf[thisObj][i+2]*3+1],
                       vertexNormalsOf[thisObj][facesOf[thisObj][i+2]*3+2]);
            if (hasTexture) glTexCoord2f(textureCoordinateOf[thisObj][facesOf[thisObj][i+2]*2],
                                         textureCoordinateOf[thisObj][facesOf[thisObj][i+2]*2 + 1]);
            glVertex3f(verticesOf[thisObj][facesOf[thisObj][i+2]*3],
                       verticesOf[thisObj][facesOf[thisObj][i+2]*3+1],
                       verticesOf[thisObj][facesOf[thisObj][i+2]*3+2]);
        }
        glEnd();
    }

    glPopMatrix();
}

/**
 * Draw a skybox (actually a plane).
 */
void drawSkybox() {
    glEnable(GL_TEXTURE_CUBE_MAP);

    // rotate the texture with view angle
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glRotatef(-yaw/PI*180 + 180, 0.0, 1.0, 0.0);
    glRotatef(pitch/PI*180, 1.0, 0.0, 0.0);

    // Disable depth buffer.
    glDepthMask(GL_FALSE);

    // Draw a square textured with cubemap after reversing POV rotations.
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(lookatX, lookatY, lookatZ);
    glRotatef(yaw/PI*180 + 180, 0.0, 1.0, 0.0);
    glRotatef(pitch/PI*180, 1.0, 0.0, 0.0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureCube);
    glBegin(GL_POLYGON);
    // support 2:1 widescreen
    glTexCoord3f(-2.0, 1.0, 1.0); glVertex3f(-1000.0, -500.0, -500.0);
    glTexCoord3f(2.0, 1.0, 1.0); glVertex3f(1000.0, -500.0, -500.0);
    glTexCoord3f(2.0, -1.0, 1.0); glVertex3f(1000.0, 500.0, -500.0);
    glTexCoord3f(-2.0, -1.0, 1.0); glVertex3f(-1000.0, 500.0, -500.0);
    glEnd();
    glPopMatrix();

    // Enable depth buffer.
    glDepthMask(GL_TRUE);
    glDisable(GL_TEXTURE_CUBE_MAP);

    // clear texture rotation
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
}

/**
 * Enable OpenGL light0.
 */
void enableLighting()
{
    float lightDark[] = { 0.0, 0.0, 0.0, 0.0 };
    glEnable(GL_LIGHTING);
    if (enableLight) {
        glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDifAndSpec);
        glLightfv(GL_LIGHT0, GL_SPECULAR, lightDifAndSpec);
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    }
    else {
        glLightfv(GL_LIGHT0, GL_AMBIENT, lightDark);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDark);
        glLightfv(GL_LIGHT0, GL_SPECULAR, lightDark);
        glLightfv(GL_LIGHT0, GL_POSITION, lightDark);
    }
    glEnable(GL_LIGHT0);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globAmb); // Global ambient light.
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE); // Enable two-sided lighting.
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE); // Enable local viewpoint.
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR); // Enable separate specular light calculation.
}

// Drawing routine.
void drawScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    drawSkybox();

    glMatrixMode(GL_PROJECTION); // for setting perspective
    glLoadIdentity();

    // Enable light.
    enableLighting();

    gluPerspective(fov, (float)windowWidth/(float)windowHeight, 0.01, 5000);

    gluLookAt(cameraX, cameraY, cameraZ, lookatX, lookatY, lookatZ, upX, upY, upZ);

    glMatrixMode(GL_MODELVIEW); // switch back to modelview for usual matrix operations

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // transforms on object
    float translateBunny[] = {0.0f+tBunny[0], 3.0f+tBunny[1], 0.0f+tBunny[2]};
    float rotateBunny[] = {0.0f+rBunny[0], 0.0f+rBunny[1], 0.0f+rBunny[2]};
    float colorBunny[] = {1.0, 0.0, 1.0};
    drawMesh(OBJ_BUNNY, false, translateBunny, 10, rotateBunny, colorBunny);

    float translateCat[] = {5.0f+tCat[0], 5.0f+tCat[1], 0.0f+tCat[2]};
    float rotateCat[] = {-90.0f+rCat[0], 0.0f+rCat[1], 60.0f+rCat[2]};
    float colorCat[] = {1.0, 0.0, 0.0};
    drawMesh(OBJ_CAT, true, translateCat, 10, rotateCat, colorCat);

    float translateDog[] = {-6.0f+tDog[0], 5.0f+tDog[1], 0.0f+tDog[2]};
    float rotateDog[] = {-90.0f+rDog[0], 0.0f+rDog[1], 30.0f+rDog[2]};
    float colorDog[] = {0.0, 1.0, 0.0};
    drawMesh(OBJ_DOG, true, translateDog, 10, rotateDog, colorDog);

    float translateDuck[] = {0.0f+tDuck[0], 3.0f+tDuck[1], 6.0f+tDuck[2]};
    float rotateDuck[] = {-90.0f+rDuck[0], 0.0f+rDuck[1], 0.0f+rDuck[2]};
    float colorDuck[] = {1.0, 1.0, 0.0};
    drawMesh(OBJ_DUCK, false, translateDuck, 10, rotateDuck, colorDuck);

    float translateTiger[] = {-5.0f+tTiger[0], 5.0f+tTiger[1], -10.0f+tTiger[2]};
    float rotateTiger[] = {-90.0f+rTiger[0], 0.0f+rTiger[1], 115.0f+rTiger[2]};
    float colorTiger[] = {1.0, 1.0, 1.0};
    glBindTexture(GL_TEXTURE_2D, textureTiger);
    drawMesh(OBJ_TIGER, false, translateTiger, 20, rotateTiger, colorTiger);

    // Specify how texture values combine with current surface color values.
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // Map the grass texture onto a rectangle along the xz-plane.
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glBegin(GL_POLYGON);
    glTexCoord2f(0.0, 0.0); glVertex3f(-100.0, 0.0, 100.0);
    glTexCoord2f(8.0, 0.0); glVertex3f(100.0, 0.0, 100.0);
    glTexCoord2f(8.0, 8.0); glVertex3f(100.0, 0.0, -100.0);
    glTexCoord2f(0.0, 8.0); glVertex3f(-100.0, 0.0, -100.0);
    glEnd();

    movement();

    glutSwapBuffers();
}

// Used for checking whether the mouse button is pressed.
void checkMouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            if (!canMoveCamera) {
                canMoveCamera = true;
                glutSetCursor(GLUT_CURSOR_NONE); // hide cursor
                glutWarpPointer(windowWidth / 2, windowHeight / 2);
            }
            else {
                canMoveCamera = false;
                glutSetCursor(GLUT_CURSOR_INHERIT); // reshow cursor
            }
        }
    }
}

// Rotate camera when mouse is pressed and moving.
void moveCamera(int x, int y) {
    int centerX = windowWidth/2, centerY = windowHeight/2;
    if (canMoveCamera) {
        float deltaX = (float)(x - centerX) * sensitivity;
        float deltaY = (float)(y - centerY) * sensitivity;
        yaw -= deltaX;
        pitch -= deltaY;

        if (pitch > PI/2 - 0.01) pitch = PI/2 - 0.01;
        if (pitch < -PI/2 + 0.01) pitch = -PI/2 + 0.01; // limit on pitch angle

        // change look-at coordinate
        lookatX = cameraX + 15.0f * sin(yaw) * cos(pitch);
        lookatZ = cameraZ + 15.0f * cos(yaw) * cos(pitch);
        lookatY = cameraY + 15.0f * sin(pitch);

        glutWarpPointer(centerX, centerY); // lock mouse in the center
        glutPostRedisplay(); // update canvas
    }
}

// OpenGL window reshape routine.
void resize(int w, int h)
{
    // update window size
    windowWidth = glutGet(GLUT_WINDOW_WIDTH);
    windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-5.0, 5.0, -5.0, 5.0, 5.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// camera movement routine.
void movement()
{
    float forwardX = moveSpeed * sin(yaw) * cos(pitch);
    float forwardY = moveSpeed * sin(pitch);
    float forwardZ = moveSpeed * cos(yaw) * cos(pitch);

    float rightX = -moveSpeed * cos(yaw);
    float rightZ = moveSpeed * sin(yaw);

    if (keyState['w']) {
        cameraX += forwardX; lookatX += forwardX;
        cameraY += forwardY; lookatY += forwardY;
        cameraZ += forwardZ; lookatZ += forwardZ;
    }
    if (keyState['a']) {
        cameraX -= rightX; lookatX -= rightX;
        cameraZ -= rightZ; lookatZ -= rightZ;
    }

    if (keyState['s']) {
        cameraX -= forwardX; lookatX -= forwardX;
        cameraY -= forwardY; lookatY -= forwardY;
        cameraZ -= forwardZ; lookatZ -= forwardZ;
    }

    if (keyState['d']) {
        cameraX += rightX; lookatX += rightX;
        cameraZ += rightZ; lookatZ += rightZ;
    }

    if (keyState[' ']) {
        cameraY += moveSpeed; lookatY += moveSpeed;
    }

    if (keyState['c']) {
        cameraY -= moveSpeed; lookatY -= moveSpeed;
    }
    if (keyState['j']) {
        switch (controlModel) {
            case 0: tBunny[0] += 0.1f; break;
            case 1: tCat[0] += 0.1f; break;
            case 2: tDog[0] += 0.1f; break;
            case 3: tDuck[0] += 0.1f; break;
            case 4: tTiger[0] += 0.1f; break;
            default: break;
        }
    }

    if (keyState['J']) {
        switch (controlModel) {
            case 0: tBunny[0] -= 0.1f; break;
            case 1: tCat[0] -= 0.1f; break;
            case 2: tDog[0] -= 0.1f; break;
            case 3: tDuck[0] -= 0.1f; break;
            case 4: tTiger[0] -= 0.1f; break;
            default: break;
        }

    }
    if (keyState['k']) {
        switch (controlModel) {
            case 0: tBunny[1] += 0.1f; break;
            case 1: tCat[1] += 0.1f; break;
            case 2: tDog[1] += 0.1f; break;
            case 3: tDuck[1] += 0.1f; break;
            case 4: tTiger[1] += 0.1f; break;
            default: break;
        }
    }

    if (keyState['K']) {
        switch (controlModel) {
            case 0: tBunny[1] -= 0.1f; break;
            case 1: tCat[1] -= 0.1f; break;
            case 2: tDog[1] -= 0.1f; break;
            case 3: tDuck[1] -= 0.1f; break;
            case 4: tTiger[1] -= 0.1f; break;
            default: break;
        }
    }
    if (keyState['l']) {
        switch (controlModel) {
            case 0: tBunny[2] += 0.1f; break;
            case 1: tCat[2] += 0.1f; break;
            case 2: tDog[2] += 0.1f; break;
            case 3: tDuck[2] += 0.1f; break;
            case 4: tTiger[2] += 0.1f; break;
            default: break;
        }
    }
    if (keyState['L']) {
        switch (controlModel) {
            case 0: tBunny[2] -= 0.1f; break;
            case 1: tCat[2] -= 0.1f; break;
            case 2: tDog[2] -= 0.1f; break;
            case 3: tDuck[2] -= 0.1f; break;
            case 4: tTiger[2] -= 0.1f; break;
            default: break;
        }
    }
    if (keyState['y']) {
        switch (controlModel) {
            case 0: rBunny[0] -= 1.0f; break;
            case 1: rCat[0] -= 1.0f; break;
            case 2: rDog[0] -= 1.0f; break;
            case 3: rDuck[0] -= 1.0f; break;
            case 4: rTiger[0] -= 1.0f; break;
            default: break;
        }
    }
    if (keyState['u']) {
        switch (controlModel) {
            case 0: rBunny[0] += 1.0f; break;
            case 1: rCat[0] += 1.0f; break;
            case 2: rDog[0] += 1.0f; break;
            case 3: rDuck[0] += 1.0f; break;
            case 4: rTiger[0] += 1.0f; break;
            default: break;
        }
    }
    if (keyState['i']) {
        switch (controlModel) {
            case 0: rBunny[1] += 1.0f; break;
            case 1: rCat[1] += 1.0f; break;
            case 2: rDog[1] += 1.0f; break;
            case 3: rDuck[1] += 1.0f; break;
            case 4: rTiger[1] += 1.0f; break;
            default: break;
        }
    }
    if (keyState['o']) {
        switch (controlModel) {
            case 0: rBunny[1] -= 1.0f; break;
            case 1: rCat[1] -= 1.0f; break;
            case 2: rDog[1] -= 1.0f; break;
            case 3: rDuck[1] -= 1.0f; break;
            case 4: rTiger[1] -= 1.0f; break;
            default: break;
        }
    }
    glutPostRedisplay();
}

// when certain keys are pressed down
void keyDown(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 27: exit(0);
        case 'w':
        case 'W': keyState['w'] = true; break;
        case 'a':
        case 'A': keyState['a'] = true; break;
        case 's':
        case 'S': keyState['s'] = true; break;
        case 'd':
        case 'D': keyState['d'] = true; break;
        case ' ': keyState[' '] = true; break;
        case 'c':
        case 'C': keyState['c'] = true; break;
        case '1': controlModel = 0; cout << "Now controlling: bunny." << endl; break;
        case '2': controlModel = 1; cout << "Now controlling: cat." << endl; break;
        case '3': controlModel = 2; cout << "Now controlling: dog." << endl; break;
        case '4': controlModel = 3; cout << "Now controlling: duck." << endl; break;
        case '5': controlModel = 4; cout << "Now controlling: tiger." << endl; break;
        case 'j': keyState['j'] = true; break;
        case 'J': keyState['J'] = true; break;
        case 'k': keyState['k'] = true; break;
        case 'K': keyState['K'] = true; break;
        case 'l': keyState['l'] = true; break;
        case 'L': keyState['L'] = true; break;
        default: break;
    }
}

// when certain keys are released
void keyUp(unsigned char key, int x, int y) {
    switch (key)
    {
        case 'w':
        case 'W': keyState['w'] = false; break;
        case 'a':
        case 'A': keyState['a'] = false; break;
        case 's':
        case 'S': keyState['s'] = false; break;
        case 'd':
        case 'D': keyState['d'] = false; break;
        case ' ': keyState[' '] = false; break;
        case 'c':
        case 'C': keyState['c'] = false; break;
        case 'j':
        case 'J': keyState['J'] = false; keyState['j'] = false; break;
        case 'k':
        case 'K': keyState['K'] = false; keyState['k'] = false; break;
        case 'l':
        case 'L': keyState['L'] = false; keyState['l'] = false; break;
        default: break;
    }
}

// Callback routine for non-ASCII key entry.
void specialKeyInput(int key, int x, int y)
{
    if (key == GLUT_KEY_SHIFT_L) keyState['c'] = true;
    if (key == GLUT_KEY_UP) keyState['y'] = true;
    if (key == GLUT_KEY_DOWN) keyState['u'] = true;
    if (key == GLUT_KEY_LEFT) keyState['i'] = true;
    if (key == GLUT_KEY_RIGHT) keyState['o'] = true;
    glutPostRedisplay();
}

// When you release these keys
void specialKeyUpInput(int key, int x, int y) {
    if (key == GLUT_KEY_SHIFT_L) keyState['c'] = false;
    if (key == GLUT_KEY_UP) keyState['y'] = false;
    if (key == GLUT_KEY_DOWN) keyState['u'] = false;
    if (key == GLUT_KEY_LEFT) keyState['i'] = false;
    if (key == GLUT_KEY_RIGHT) keyState['o'] = false;
    glutPostRedisplay();
}

// light adjustment menu
void lightMenu(int id)
{
    switch (id) {
        case ID_LIGHT_ON:
            enableLight = true;
            break;
        case ID_LIGHT_OFF:
            enableLight = false;
            break;
        case ID_LIGHT_TURN_WHITE:
            enableLight = true;
            lightDifAndSpec[0] = 1.0;
            lightDifAndSpec[1] = 1.0;
            lightDifAndSpec[2] = 1.0;
            lightDifAndSpec[3] = 1.0;
            break;
        case ID_LIGHT_TURN_DIM:
            enableLight = true;
            lightDifAndSpec[0] = 0.5;
            lightDifAndSpec[1] = 0.5;
            lightDifAndSpec[2] = 0.5;
            lightDifAndSpec[3] = 0.5;
            break;
        case ID_LIGHT_TURN_RED:
            enableLight = true;
            lightDifAndSpec[0] = 1.0;
            lightDifAndSpec[1] = 0.0;
            lightDifAndSpec[2] = 0.0;
            lightDifAndSpec[3] = 1.0;
            break;
        case ID_LIGHT_TURN_GREEN:
            enableLight = true;
            lightDifAndSpec[0] = 0.0;
            lightDifAndSpec[1] = 1.0;
            lightDifAndSpec[2] = 0.0;
            lightDifAndSpec[3] = 1.0;
            break;
        case ID_LIGHT_TURN_BLUE:
            enableLight = true;
            lightDifAndSpec[0] = 0.0;
            lightDifAndSpec[1] = 0.0;
            lightDifAndSpec[2] = 1.0;
            lightDifAndSpec[3] = 1.0;
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

// right click menu function
void rightMenu(int id)
{
    if (id == ID_QUIT) exit(0);
}

// create right click menu
void makeMenu()
{
    int light_sub_menu = glutCreateMenu(lightMenu);
    glutAddMenuEntry("Turn light on", ID_LIGHT_ON);
    glutAddMenuEntry("Turn light off", ID_LIGHT_OFF);
    glutAddMenuEntry("Turn light color to white", ID_LIGHT_TURN_WHITE);
    glutAddMenuEntry("Make light a bit dark", ID_LIGHT_TURN_DIM);
    glutAddMenuEntry("Turn light color to red", ID_LIGHT_TURN_RED);
    glutAddMenuEntry("Turn light color to green", ID_LIGHT_TURN_GREEN);
    glutAddMenuEntry("Turn light color to blue", ID_LIGHT_TURN_BLUE);

    glutCreateMenu(rightMenu);
    glutAddSubMenu("Light", light_sub_menu);
    glutAddMenuEntry("Quit", ID_QUIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

// timer of refreshing canvas, refreshes canvas every 15ms (or 66.67 fps)
void timer(int extra) {
    glutPostRedisplay();
    glutTimerFunc(15, timer, 0);
}

// Routine to output interaction instructions to the C++ window.
void printInteraction()
{
    std::cout << "Interaction:" << std::endl;
    std::cout << "Press w, a, s, d to move around, left click mouse to toggle see-around mode on & off." << std::endl;
    std::cout << "Press 1, 2, 3, 4, 5 to choose a model and use arrow keys to rotate them, use j, k, l, J, K, L (NOTE: USE RIGHT SHIFT or CAPSLOCK) to move them." << std::endl;
    std::cout << "Press c or shift to move down, space to move up, right click to bring up the light menu." << std::endl;
    std::cout << "You can freely resize the window." << std::endl;
}

// Main routine.
int main(int argc, char **argv)
{
    printInteraction();
    glutInit(&argc, argv);

    glutInitContextVersion(4, 3);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 800);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("fieldAndSky.cpp");
    glutDisplayFunc(drawScene);
    glutReshapeFunc(resize);

    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialKeyInput);
    glutSpecialUpFunc(specialKeyUpInput);

    glutMouseFunc(checkMouse);
    glutPassiveMotionFunc(moveCamera);

    glutTimerFunc(0, timer, 0);

    glewExperimental = GL_TRUE;
    glewInit();

    setup();

    glutMainLoop();
}
