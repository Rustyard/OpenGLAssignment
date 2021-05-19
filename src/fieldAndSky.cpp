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

#include <GL/glew.h>
#include <GL/freeglut.h>

#include "../include/getBMP.h"

#define ID_LIGHT_OFF 0
#define ID_LIGHT_ON 1
#define ID_LIGHT_TURN_RED 2
#define ID_LIGHT_TURN_GREEN 3
#define ID_LIGHT_TURN_BLUE 4
#define ID_LIGHT_TURN_WHITE 5
#define ID_LIGHT_TURN_GREY 6

#define ID_QUIT 7


// Function declaration.
void makeMenu();

// Globals.
static unsigned int texture[2]; // Array of texture indices.
static float d = 0.0; // Distance parameter in gluLookAt().
static float lr = 0.0;

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
