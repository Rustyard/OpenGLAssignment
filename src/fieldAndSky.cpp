////////////////////////////////////////////////////////////////////////////////////////////
// fieldAndSky.cpp
//
// This program shows a grass-textured field and a textured sky. The viewpoint can be moved.
//
// Interaction:
// Press the up and down arrow keys to move the viewpoint over the field.
//
// Sumanta Guha
//
// Texture Credits: See ExperimenterSource/Textures/TEXTURE_CREDITS.txt
////////////////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <sstream>

#include "../include/getBMP.h"

#define ID_LIGHT_OFF 0
#define ID_LIGHT_ON 1
#define ID_LIGHT_TURN_RED 2
#define ID_LIGHT_TURN_GREEN 3
#define ID_LIGHT_TURN_BLUE 4
#define ID_LIGHT_TURN_WHITE 5
#define ID_LIGHT_TURN_GREY 6
#define ID_QUIT 7

#define MODEL_NUMBERS 5

#define OBJ_BUNNY 0
#define OBJ_CAT 1
#define OBJ_DOG 2
#define OBJ_DUCK 3
#define OBJ_TIGER 4


#define PI 3.14159265358979

using namespace std;

// Function declaration.
void makeMenu();
void enableLighting();

// Globals.
const int windowWidth = 800, windowHeight = 800;
static unsigned int texture[2]; // Array of texture indices.
static float cameraX = 0.0f, cameraY = 10.0f, cameraZ = 15.0f; // Camera position.
static float lookatX = 0.0f, lookatY = 10.0f, lookatZ = 0.0f; // Camera look at position.
static float upX = 0.0f, upY = 1.0f, upZ = 0.0f; // Camera upward vector.
static float yaw = PI, pitch = 0.0f; // Camera rotation angle.
static bool canMoveCamera = false;
static float sensitivity = 0.001f; // mouse sensitivity

// global lighting
static float lightAmb[] = { 0.0, 0.0, 0.0, 1.0 };
static float lightDifAndSpec[] = { 1.0, 1.0, 1.0, 1.0 };
static float lightPos[] = { 0, 5, 0, 0.0 }; // fourth value: 0 for spot light, 1 for directional light
static float globAmb[] = { 0.2, 0.2, 0.2, 1.0 };

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
 * Stores the center point for each face of different objects.
 * Similar to facesOf, but for each face fN, we store xN, yN, zN as the center
 * coordinate.
 * Used for drawing face normals as 3d lines.
 * {x0, y0, z0, x1, y1, z1, ... , xN, yN, zN, ... }, { ... }, ...
 * e.g. {x1,y1,z1} is the center point coordinate for face 1.
 */
// static vector<vector<float>> faceCentersOf;

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

    float tempCenterX, tempCenterY, tempCenterZ;
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

//        // compute center point of each face (no longer needed here)
//        tempCenterX = (firstPoint[0] + secondPoint[0] + thirdPoint[0]) / 3;
//        tempCenterY = (firstPoint[1] + secondPoint[1] + thirdPoint[1]) / 3;
//        tempCenterZ = (firstPoint[2] + secondPoint[2] + thirdPoint[2]) / 3;
//
//        faceCentersOf[thisObj].push_back(tempCenterX);
//        faceCentersOf[thisObj].push_back(tempCenterY);
//        faceCentersOf[thisObj].push_back(tempCenterZ);

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

        // Nothing other than vertex and face data is processed.
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
    // Local storage for bmp image data.
    imageFile *image[2];

    // Load the images.
    image[0] = getBMP("../textures/grass.bmp");
    image[1] = getBMP("../textures/sky.bmp");

    // Bind grass image to texture object texture[0].
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image[0]->width, image[0]->height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, image[0]->data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Bind sky image to texture object texture[1]
    glBindTexture(GL_TEXTURE_2D, texture[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image[1]->width, image[1]->height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, image[1]->data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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

    glClearColor(1.0, 1.0, 1.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_CLAMP);

    // load obj models
    loadOBJAndProcess("../models/Bunny.obj", OBJ_BUNNY);
    loadOBJAndProcess("../models/Cat.obj", OBJ_CAT);
    loadOBJAndProcess("../models/Dog.obj", OBJ_DOG);
    loadOBJAndProcess("../models/Duck.obj", OBJ_DUCK);
    // TODO: get tiger and it's texture coordinates

    // Create texture ids.
    glGenTextures(2, texture);

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
            glVertex3f(verticesOf[thisObj][facesOf[thisObj][i]*3],
                       verticesOf[thisObj][facesOf[thisObj][i]*3+1],
                       verticesOf[thisObj][facesOf[thisObj][i]*3+2]);
            glVertex3f(verticesOf[thisObj][facesOf[thisObj][i+1]*3],
                       verticesOf[thisObj][facesOf[thisObj][i+1]*3+1],
                       verticesOf[thisObj][facesOf[thisObj][i+1]*3+2]);
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
            glVertex3f(verticesOf[thisObj][facesOf[thisObj][i]*3],
                       verticesOf[thisObj][facesOf[thisObj][i]*3+1],
                       verticesOf[thisObj][facesOf[thisObj][i]*3+2]);
            glNormal3f(vertexNormalsOf[thisObj][facesOf[thisObj][i+1]*3],
                       vertexNormalsOf[thisObj][facesOf[thisObj][i+1]*3+1],
                       vertexNormalsOf[thisObj][facesOf[thisObj][i+1]*3+2]);
            glVertex3f(verticesOf[thisObj][facesOf[thisObj][i+1]*3],
                       verticesOf[thisObj][facesOf[thisObj][i+1]*3+1],
                       verticesOf[thisObj][facesOf[thisObj][i+1]*3+2]);
            glNormal3f(vertexNormalsOf[thisObj][facesOf[thisObj][i+2]*3],
                       vertexNormalsOf[thisObj][facesOf[thisObj][i+2]*3+1],
                       vertexNormalsOf[thisObj][facesOf[thisObj][i+2]*3+2]);
            glVertex3f(verticesOf[thisObj][facesOf[thisObj][i+2]*3],
                       verticesOf[thisObj][facesOf[thisObj][i+2]*3+1],
                       verticesOf[thisObj][facesOf[thisObj][i+2]*3+2]);
        }
        glEnd();
    }


    glPopMatrix();
}

/**
 * Enable OpenGL light0.
 */
void enableLighting()
{
    glEnable(GL_LIGHTING);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDifAndSpec);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightDifAndSpec);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
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

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // TODO: fix "lighting follows camera" problem
    // Enable light.
    enableLighting();

    gluLookAt(cameraX, cameraY, cameraZ,
              lookatX, lookatY, lookatZ, upX, upY, upZ);


    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // transforms on object
    float translateBunny[] = {0.0, 3.0, 0.0};
    float rotateBunny[] = {0.0, 0.0, 0.0};
    float colorBunny[] = {1.0, 0.0, 1.0};
    drawMesh(OBJ_BUNNY, false, translateBunny, 10, rotateBunny, colorBunny);

    float translateCat[] = {5.0, 5.0, 0.0};
    float rotateCat[] = {-90.0, 0.0, 60.0};
    float colorCat[] = {1.0, 0.0, 0.0};
    drawMesh(OBJ_CAT, true, translateCat, 10, rotateCat, colorCat);

    float translateDog[] = {-6.0, 5.0, 0.0};
    float rotateDog[] = {-90.0, 0.0, 30.0};
    float colorDog[] = {0.0, 1.0, 0.0};
    drawMesh(OBJ_DOG, true, translateDog, 10, rotateDog, colorDog);

    float translateDuck[] = {0.0, 3.0, 6.0};
    float rotateDuck[] = {-90.0, 0.0, 0.0};
    float colorDuck[] = {1.0, 1.0, 0.0};
    drawMesh(OBJ_DUCK, false, translateDuck, 10, rotateDuck, colorDuck);

    // Specify how texture values combine with current surface color values.
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // Map the grass texture onto a rectangle along the xz-plane.
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glBegin(GL_POLYGON);
    glTexCoord2f(0.0, 0.0); glVertex3f(-100.0, 0.0, 100.0);
    glTexCoord2f(8.0, 0.0); glVertex3f(100.0, 0.0, 100.0);
    glTexCoord2f(8.0, 8.0); glVertex3f(100.0, 0.0, -100.0);
    glTexCoord2f(0.0, 8.0); glVertex3f(-100.0, 0.0, -100.0);
    glEnd();

    // Map the sky texture onto a rectangle parallel to the xy-plane.
    glBindTexture(GL_TEXTURE_2D, texture[1]);
    glBegin(GL_POLYGON);
    glTexCoord2f(0.0, 0.0); glVertex3f(-100.0, 0.0, -70.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(100.0, 0.0, -70.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(100.0, 120.0, -70.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(-100.0, 120.0, -70.0);
    glEnd();

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
                glutSetCursor(GLUT_CURSOR_INHERIT);
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
        yaw += deltaX;
        pitch += deltaY;

        if (pitch > PI/2 - 0.01) pitch = PI/2 - 0.01;
        if (pitch < -PI/2 + 0.01) pitch = -PI/2 + 0.01; // limit on pitch angle

        // change look-at coordinate
        lookatX = cameraX + 15.0f * sin(yaw) * cos(pitch);
        lookatZ = cameraZ + 15.0f * cos(yaw) * cos(pitch);
        lookatY = cameraY + 15.0f * sin(pitch);

        // change upward vector
//        upY = sin(pitch);
//        upX = -sin(yaw) * cos(pitch);
//        upZ = -cos(yaw) * cos(pitch);


        glutWarpPointer(centerX, centerY); // lock mouse in the center
        glutPostRedisplay(); // update canvas
    }
}

// OpenGL window reshape routine.
void resize(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-5.0, 5.0, -5.0, 5.0, 5.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Keyboard input processing routine.
void keyInput(unsigned char key, int x, int y)
{
    float forwardX = 0.5f * sin(yaw) * cos(pitch);
    float forwardY = 0.5f * sin(pitch);
    float forwardZ = 0.5f * cos(yaw) * cos(pitch);

    float rightX = -0.5f * cos(yaw);
    float rightZ = 0.5f * sin(yaw);

    switch (key)
    {
        case 27:
            exit(0);
            break;
        case 'w':
        case 'W':
            cameraX += forwardX; lookatX += forwardX;
            cameraY += forwardY; lookatY += forwardY;
            cameraZ += forwardZ; lookatZ += forwardZ;
            break;
        case 'a':
        case 'A':
            cameraX -= rightX; lookatX -= rightX;
            cameraZ -= rightZ; lookatZ -= rightZ;
            break;
        case 's':
        case 'S':
            cameraX -= forwardX; lookatX -= forwardX;
            cameraY -= forwardY; lookatY -= forwardY;
            cameraZ -= forwardZ; lookatZ -= forwardZ;
            break;
        case 'd':
        case 'D':
            cameraX += rightX; lookatX += rightX;
            cameraZ += rightZ; lookatZ += rightZ;
            break;
        default:
            break;
    }
}

// Callback routine for non-ASCII key entry.
void specialKeyInput(int key, int x, int y)
{
    if (key == GLUT_KEY_UP) {
        cameraZ -= 0.5;
        lookatZ -= 0.5;
    }
    if (key == GLUT_KEY_DOWN) {
        cameraZ += 0.5;
        lookatZ += 0.5;
    }
    glutPostRedisplay();
}

// light adjustment menu
// TODO: make this functioning
void lightMenu(int id)
{
    switch (id) {
        case ID_LIGHT_ON:
            break;
        case ID_LIGHT_OFF:
            break;
        case ID_LIGHT_TURN_WHITE:
            break;
        case ID_LIGHT_TURN_GREY:
            break;
        case ID_LIGHT_TURN_RED:
            break;
        case ID_LIGHT_TURN_GREEN:
            break;
        case ID_LIGHT_TURN_BLUE:
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
    glutAddMenuEntry("Turn light color to gray", ID_LIGHT_TURN_GREY);
    glutAddMenuEntry("Turn light color to red", ID_LIGHT_TURN_RED);
    glutAddMenuEntry("Turn light color to green", ID_LIGHT_TURN_GREEN);
    glutAddMenuEntry("Turn light color to blue", ID_LIGHT_TURN_BLUE);

    glutCreateMenu(rightMenu);
    glutAddSubMenu("Light", light_sub_menu);
    glutAddMenuEntry("Quit", ID_QUIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

// Routine to output interaction instructions to the C++ window.
void printInteraction()
{
    std::cout << "Interaction:" << std::endl;
    std::cout << "Press the up and down arrow keys to move the viewpoint over the field." << std::endl;
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
    glutKeyboardFunc(keyInput);
    glutSpecialFunc(specialKeyInput);
    glutMouseFunc(checkMouse);
    glutPassiveMotionFunc(moveCamera);

    glewExperimental = GL_TRUE;
    glewInit();

    setup();

    glutMainLoop();
}
