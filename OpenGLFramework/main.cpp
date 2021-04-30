#include "GL/glew.h"
#include "GL/glut.h"

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>

#include "Planet.h"
#include "stb_image.h"

using namespace std;

// GLUT CALLBACK functions
void displayCB();
void reshapeCB(int w, int h);
void timerCB(int millisec);
void keyboardCB(unsigned char key, int x, int y);
void mouseCB(int button, int stat, int x, int y);
void mouseMotionCB(int x, int y);

void parseFile(string file);
string clean(const string& str, const string& fill = " ", const string& whitespace = " \t");
void initGL();
int  initGLUT(int argc, char **argv);
bool initSharedMem();
void initLights();
void setCamera(float posX, float posY, float posZ, float targetX, float targetY, float targetZ);
void drawString(const char *str, int x, int y, float color[4], void *font);
void drawString3D(const char *str, float pos[3], float color[4], void *font);
void toOrtho();
void toPerspective();
void background();
GLuint loadBackground();


// constants
const int   SCREEN_WIDTH    = 800;
const int   SCREEN_HEIGHT   = 600;
const float CAMERA_DISTANCE = 4.0f;
const int   TEXT_WIDTH      = 8;
const int   TEXT_HEIGHT     = 13;

// texture info
const char* textureFile     = "space.jpg";
unsigned char* textureData;
GLint textureDataLocation;
int textureWidth;
int textureHeight;
int textureComp;
GLuint spaceTexture;


// global variables
void *font = GLUT_BITMAP_8_BY_13;
int screenWidth;
int screenHeight;
bool mouseLeftDown;
bool mouseRightDown;
bool mouseMiddleDown;
float mouseX, mouseY;
float cameraAngleX;
float cameraAngleY;
float cameraDistance;
int drawMode;
int imageWidth;
int imageHeight;
Planet planet;
Params params;


int main(int argc, char **argv)
{
    string filename;

    cout << "Please enter the planet grammar filename: ";
    cin >> filename;
    // planet: min sector = 3, min stack = 2
    parseFile(filename);

    // init global vars
    initSharedMem();

    // init GLUT and GL
    initGLUT(argc, argv);
    initGL();

    GLuint result = loadBackground();

    glutMainLoop();

    return 0;
}



/* initialize planet from file */
void parseFile(string file)
{
    ifstream scene(file);

    /* initialize random number generator */
    time_t t;
    srand((unsigned)time(&t));

    // Check if file is openable
    if (!scene.is_open()) {
        cout << "Unable to open file \"" << file << "\"" << endl;
        cout << "Generating terrestrial planet instead." << endl;
        planet = Planet(params, 1.0f, 512, 256);
        return;
    }

    string line, token, b[4];
    string delim = " ";
    size_t pos;
    int k = 0;

    while (getline(scene, line)) {
        line = clean(line);  // remove unnecessary whitespace that may exist
        pos = line.find(delim);
        token = line.substr(0, pos);
        line.erase(0, pos + delim.length());

        string type;

        switch (token[0]) {
        case 'R':
            params.R = stod(line) * 1000.0; // convert to m
            break;
        case 'M':
            params.M = stod(line);
            break;
        case 'D':
            params.D = stod(line) * 3600;   // convert to s
            break;
        case 'S':
            params.S = stof(line);
            break;
        case 'T':
            params.T = stof(line);
            break;
        case 'W':
            params.W = stof(line);
            break;
        case 'C':
            while ((pos = line.find(delim)) != string::npos) {
                token = line.substr(0, pos);
                b[k++] = token;
                line.erase(0, pos + delim.length());
            }

            if (line.compare("terrestrial")) params.terrestrial = false;
            if (!line.compare("random")) {
                params.red = rand() % 100 * 0.01;
                params.green = rand() % 100 * 0.01;
                params.blue = rand() % 100 * 0.01;
            }
            else if (!b[0].compare("color")) {
                params.red = stof(b[1]) / 255.0;
                params.green = stof(b[2]) / 255.0;
                params.blue = stof(line) / 255.0;
            }
        }
    }

    planet = Planet(params, 1.0f, 512, 256);    // radius, sectors, stacks, non-smooth (flat) shading
}



/* get rid of excess whitespace that sometimes exists in files */
string clean(const string & str, const string & fill, const string & whitespace)
{
    // trim first
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == string::npos) return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    auto result = str.substr(strBegin, strRange);

    // replace sub ranges
    auto beginSpace = result.find_first_of(whitespace);
    while (beginSpace != string::npos)
    {
        const auto endSpace = result.find_first_not_of(whitespace, beginSpace);
        const auto range = endSpace - beginSpace;

        result.replace(beginSpace, range, fill);

        const auto newStart = beginSpace + fill.length();
        beginSpace = result.find_first_of(whitespace, newStart);
    }

    return result;
}



/* initialize GLUT for windowing */
int initGLUT(int argc, char **argv)
{
    // GLUT stuff for windowing
    // initialization openGL window.
    // it is called before any other GLUT routine
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);   // display mode

    glutInitWindowSize(screenWidth, screenHeight);  // window size

    glutInitWindowPosition(100, 100);               // window location

    // finally, create a window with openGL context
    // Window will not displayed until glutMainLoop() is called
    // it returns a unique ID
    int handle = glutCreateWindow(argv[0]);     // param is the title of window

    // register GLUT callback functions
    glutDisplayFunc(displayCB);
    glutTimerFunc(33, timerCB, 33);             // redraw only every given millisec
    glutReshapeFunc(reshapeCB);
    glutKeyboardFunc(keyboardCB);
    glutMouseFunc(mouseCB);
    glutMotionFunc(mouseMotionCB);

    return handle;
}



/*
 * initialize OpenGL
 * disable unused features
 */
void initGL()
{
    glShadeModel(GL_SMOOTH);                    // shading mathod: GL_SMOOTH or GL_FLAT
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);      // 4-byte pixel alignment

    // enable /disable features
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_CULL_FACE);

    // track material ambient and diffuse from surface color, call it before glEnable(GL_COLOR_MATERIAL)
    //glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    //glEnable(GL_COLOR_MATERIAL);

    glClearColor(0, 0, 0, 0);                   // background color
    glClearStencil(0);                          // clear stencil buffer
    glClearDepth(1.0f);                         // 0 is near, 1 is far
    glDepthFunc(GL_LEQUAL);

    initLights();
}



/*
 * write 2d text using GLUT
 * The projection matrix must be set to orthogonal before call this function.
 */
void drawString(const char *str, int x, int y, float color[4], void *font)
{
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT); // lighting and color mask
    glDisable(GL_LIGHTING);     // need to disable lighting for proper text color

    glColor4fv(color);          // set text color
    glRasterPos2i(x, y);        // place text position

    // loop all characters in the string
    while(*str)
    {
        glutBitmapCharacter(font, *str);
        ++str;
    }

    glEnable(GL_LIGHTING);
    glPopAttrib();
}



/* draw a string in 3D space */
void drawString3D(const char *str, float pos[3], float color[4], void *font)
{
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT); // lighting and color mask
    glDisable(GL_LIGHTING);     // need to disable lighting for proper text color

    glColor4fv(color);          // set text color
    glRasterPos3fv(pos);        // place text position

    // loop all characters in the string
    while(*str)
    {
        glutBitmapCharacter(font, *str);
        ++str;
    }

    glEnable(GL_LIGHTING);
    glPopAttrib();
}



/* initialize global variables */
bool initSharedMem()
{
    screenWidth = SCREEN_WIDTH;
    screenHeight = SCREEN_HEIGHT;

    mouseLeftDown = mouseRightDown = mouseMiddleDown = false;
    mouseX = mouseY = 0;

    cameraAngleX = cameraAngleY = 0.0f;
    cameraDistance = CAMERA_DISTANCE;

    drawMode = 0; // 0:fill, 1: wireframe, 2:points

    // debug
    // planet.printSelf();

    return true;
}



/* initialize lights */
void initLights()
{
    // set up light colors (ambient, diffuse, specular)
    GLfloat lightKa[] = {.3f, .3f, .3f, 1.0f};  // ambient light
    GLfloat lightKd[] = {.7f, .7f, .7f, 1.0f};  // diffuse light
    GLfloat lightKs[] = {1, 1, 1, 1};           // specular light
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightKa);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightKd);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightKs);

    // position the light
    float lightPos[4] = {0, 0, 1, 0}; // directional light
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glEnable(GL_LIGHT0);
}



/* set camera position and lookat direction */
void setCamera(float posX, float posY, float posZ, float targetX, float targetY, float targetZ)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(posX, posY, posZ, targetX, targetY, targetZ, 0, 1, 0); // eye(x,y,z), focal(x,y,z), up(x,y,z)
}



/* display info messages */
void showInfo()
{
    // backup current model-view matrix
    glPushMatrix();                     // save current modelview matrix
    glLoadIdentity();                   // reset modelview matrix

    // set to 2D orthogonal projection
    glMatrixMode(GL_PROJECTION);        // switch to projection matrix
    glPushMatrix();                     // save current projection matrix
    glLoadIdentity();                   // reset projection matrix
    //gluOrtho2D(0, screenWidth, 0, screenHeight); // set to orthogonal projection
    glOrtho(0, screenWidth, 0, screenHeight, -1, 1); // set to orthogonal projectionD

    float color[4] = {1, 1, 1, 1};

    stringstream ss;
    ss << fixed << setprecision(3);

    ss << "Planet Radius: " << params.R / 1000.0 << " km" << ends;
    drawString(ss.str().c_str(), 1, screenHeight-TEXT_HEIGHT, color, font);
    ss.str("");

    ss << "  Planet Mass: " << params.M << " kg" << ends;
    drawString(ss.str().c_str(), 1, screenHeight-(2*TEXT_HEIGHT), color, font);
    ss.str("");

    ss << " Sidereal Day: " << params.D / 3600.0 << " Earth hours" << ends;
    drawString(ss.str().c_str(), 1, screenHeight-(3*TEXT_HEIGHT), color, font);
    ss.str("");

    ss << "Average Temp.: " << params.T << " C" << ends;
    drawString(ss.str().c_str(), 1, screenHeight - (5 * TEXT_HEIGHT), color, font);
    ss.str("");

    ss << "Smooth Factor: " << params.S << ends;
    drawString(ss.str().c_str(), 1, screenHeight-(4*TEXT_HEIGHT), color, font);
    ss.str("");

    // unset floating format
    ss << resetiosflags(ios_base::fixed | ios_base::floatfield);

    // restore projection matrix
    glPopMatrix();                   // restore to previous projection matrix

    // restore modelview matrix
    glMatrixMode(GL_MODELVIEW);      // switch to modelview matrix
    glPopMatrix();                   // restore to previous modelview matrix
}



/* set projection matrix as orthogonal */
void toOrtho()
{
    // set viewport to be the entire window
    glViewport(0, 0, (GLsizei)screenWidth, (GLsizei)screenHeight);

    // set orthographic viewing frustum
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, screenWidth, 0, screenHeight, -1, 1);

    // switch to modelview matrix in order to set scene
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}



/* set the projection matrix as perspective */
void toPerspective()
{
    // set viewport to be the entire window
    glViewport(0, 0, (GLsizei)screenWidth, (GLsizei)screenHeight);

    // set perspective viewing frustum
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0f, (float)(screenWidth)/screenHeight, 1.0f, 1000.0f); // FOV, AspectRatio, NearClip, FarClip

    // switch to modelview matrix in order to set scene
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}



/* display space background */
void background()
{
    glPushMatrix();
    glTranslatef(-textureWidth / 2, -textureHeight / 2, -textureHeight / 2);
    glBindTexture(GL_TEXTURE_2D, spaceTexture);
    glEnable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
        glTexCoord2i(0, 0); glVertex2i(0, 0);
        glTexCoord2i(1, 0); glVertex2i(textureWidth, 0);
        glTexCoord2i(1, 1); glVertex2i(textureWidth, textureHeight);
        glTexCoord2i(0, 1); glVertex2i(0, textureHeight);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}



/* load background texture */
GLuint loadBackground()
{
    textureData = stbi_load(textureFile, &textureWidth, &textureHeight, &textureComp, STBI_rgb); 

    glGenTextures(1, &spaceTexture);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glBindTexture(GL_TEXTURE_2D, spaceTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);

    return spaceTexture;
}

/*
 * CALLBACKS
 */

void displayCB()
{
    // clear buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // save the initial ModelView matrix before modifying ModelView matrix
    glPushMatrix();

    // tramsform modelview matrix
    toPerspective();
    glTranslatef(0, 0, -cameraDistance);

    // display background
    background();

    // set material
    float ambient[]  = {0.6f, 0.6f, 0.6f, 1};
    float diffuse[]  = {0.7f, 0.7f, 0.7f, 1};
    float specular[] = {0.6f, 0.6f, 0.6f, 1};
    float shininess  = 128;
    glMaterialfv(GL_FRONT, GL_AMBIENT,   ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  specular);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess);

    // line color
    float lineColor[] = {0.2f, 0.2f, 0.2f, 1};

    // draw flat planet with lines
    glPushMatrix();
    glRotatef(cameraAngleX, 1, 0, 0);   // pitch
    glRotatef(cameraAngleY, 0, 1, 0);   // heading
    glRotatef(-90, 1, 0, 0);
    planet.draw();
    glPopMatrix();

    showInfo();     // print max range of glDrawRangeElements
    glPopMatrix();

    glutSwapBuffers();
}


void reshapeCB(int w, int h)
{
    screenWidth = w;
    screenHeight = h;
    toPerspective();
}


void timerCB(int millisec)
{
    glutTimerFunc(millisec, timerCB, millisec);
    glutPostRedisplay();
}


void keyboardCB(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 27: // escape
        exit(0);
        break;
    }
}


void mouseCB(int button, int state, int x, int y)
{
    mouseX = x;
    mouseY = y;

    if(button == GLUT_LEFT_BUTTON)
    {
        if(state == GLUT_DOWN)
        {
            mouseLeftDown = true;
        }
        else if(state == GLUT_UP)
            mouseLeftDown = false;
    }

    else if(button == GLUT_RIGHT_BUTTON)
    {
        if(state == GLUT_DOWN)
        {
            mouseRightDown = true;
        }
        else if(state == GLUT_UP)
            mouseRightDown = false;
    }

    else if(button == GLUT_MIDDLE_BUTTON)
    {
        if(state == GLUT_DOWN)
        {
            mouseMiddleDown = true;
        }
        else if(state == GLUT_UP)
            mouseMiddleDown = false;
    }
}


void mouseMotionCB(int x, int y)
{
    if(mouseLeftDown)
    {
        cameraAngleY += (x - mouseX);
        cameraAngleX += (y - mouseY);
        mouseX = x;
        mouseY = y;
    }
    if(mouseRightDown)
    {
        cameraDistance -= (y - mouseY) * 0.2f;
        mouseY = y;
    }
}
