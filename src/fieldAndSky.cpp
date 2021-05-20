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


#define PI 3.14159265358979

using namespace std;

// Function declaration.
void makeMenu();

// Globals.
static unsigned int texture[2]; // Array of texture indices.
static float d = 0.0; // Distance parameter in gluLookAt().

// Vectors used in model processing.
/**
 * Vectors of vertices of different objects, in this structure:
 * {x0, y0, z0, x1, y1, z1, ... , xN, yN, zN, ... }, { ... }, {...}
 * ^object1                                         ^object2 ^object3 and so on
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
 * ^object1                                   ^object2 and so on
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
static vector<vector<float>> faceCentersOf;

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

/**
 * Compute the bounding box for a specified object.
 * @param thisObj Indicating which object is being processed.
 */
void ComputeBoundingBox(int thisObj)
{
    float minX,maxX, minY,maxY, minZ,maxZ;
    // read value from vertices vector
    for (int i = 0; i < verticesOf[thisObj].size() / 3; i += 3)
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
}

/**
 * Compute the face normal and the volume of each face.
 * @param thisObj Indicating which object is being processed.
 */
void ComputeFaceNormalsAndVolumes(int thisObj)
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

        // compute center point of each face
        tempCenterX = (firstPoint[0] + secondPoint[0] + thirdPoint[0]) / 3;
        tempCenterY = (firstPoint[1] + secondPoint[1] + thirdPoint[1]) / 3;
        tempCenterZ = (firstPoint[2] + secondPoint[2] + thirdPoint[2]) / 3;

        faceCentersOf[thisObj].push_back(tempCenterX);
        faceCentersOf[thisObj].push_back(tempCenterY);
        faceCentersOf[thisObj].push_back(tempCenterZ);

        // compute face volume for each face
        float temp1P = (secondPoint[1] - firstPoint[1]) * (thirdPoint[2] - firstPoint[2]);
        float temp2P = (secondPoint[2] - firstPoint[2]) * (thirdPoint[0] - firstPoint[0]);
        float temp3P = (secondPoint[0] - firstPoint[0]) * (thirdPoint[1] - firstPoint[1]);
        float temp1N = (secondPoint[1] - firstPoint[1]) * (thirdPoint[0] - firstPoint[0]);
        float temp2N = (secondPoint[0] - firstPoint[0]) * (thirdPoint[2] - firstPoint[2]);
        float temp3N = (secondPoint[2] - firstPoint[2]) * (thirdPoint[1] - firstPoint[1]);
        float tempFaceVolume = (float)0.5 * abs(temp1P + temp2P + temp3P - temp1N - temp2N - temp3N);
        faceVolumesOf[thisObj].push_back(tempFaceVolume);

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

// TODO: improve this algorithm and make it fast (below quadratic time)
void ComputeVertexNormals(int thisObj)
{
    unsigned int vertexCount = verticesOf[thisObj].size() / 3;

    for (int i = 0; i < vertexCount; i++) {
        int j = 0;
        float resultNormalX = 0.0;
        float resultNormalY = 0.0;
        float resultNormalZ = 0.0;
        std::vector<int> vertexPositionInFace;
        std::vector<float> faceVolume;
        std::vector<float> faceNormal;
        float totalVolume = 0.0;

        // get the position where vertice i appeared OK
        while (j < facesOf[thisObj].size()) {
            if (facesOf[thisObj][j] == i) {
                vertexPositionInFace.push_back(j);
                j += 3 - (j % 3); // move to next face
            }
            else {
                j++; // move to next point
            }
        }

        // get the face volumes for each face which contains vertice i
        // get the normals too
        for (j = 0; j < vertexPositionInFace.size(); j++) {
            faceVolume.push_back(faceVolumesOf[thisObj][vertexPositionInFace[j] / 3]);
            faceNormal.push_back(faceNormalsOf[thisObj][(vertexPositionInFace[j] / 3) * 3]);
            faceNormal.push_back(faceNormalsOf[thisObj][(vertexPositionInFace[j] / 3) * 3 + 1]);
            faceNormal.push_back(faceNormalsOf[thisObj][(vertexPositionInFace[j] / 3) * 3 + 2]); // bug fixed
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
 * Load a OBJ file into verticesOf[thisObj] and facesOf[thisObj].
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
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);

    // Create texture ids.
    glGenTextures(2, texture);

    // Load external textures.
    loadTextures();

    // Specify how texture values combine with current surface color values.
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // Turn on OpenGL texturing.
    glEnable(GL_TEXTURE_2D);

    // Create menu.
    makeMenu();
}

// Drawing routine.
void drawScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    gluLookAt(0.0, 10.0, 15.0 + d, 0.0, 10.0, d, 0.0, 1.0, 0.0);

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
    switch (key)
    {
        case 27:
            exit(0);
            break;
        default:
            break;
    }
}

// Callback routine for non-ASCII key entry.
void specialKeyInput(int key, int x, int y)
{
    if (key == GLUT_KEY_UP) d -= 0.1;
    if (key == GLUT_KEY_DOWN) d += 0.1;
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
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("fieldAndSky.cpp");
    glutDisplayFunc(drawScene);
    glutReshapeFunc(resize);
    glutKeyboardFunc(keyInput);
    glutSpecialFunc(specialKeyInput);

    glewExperimental = GL_TRUE;
    glewInit();

    setup();

    glutMainLoop();
}
