// ============================================================
//  Interactive Underwater Ecosystem Visualization
//  OpenGL + FreeGLUT  |  Single-file C++ source
//
//  Compile (Windows + MinGW):
//    g++ underwater_ecosystem.cpp -o underwater_ecosystem.exe
//        -lfreeglut -lopengl32 -lglu32 -std=c++17
//
//  Compile (Linux):
//    g++ underwater_ecosystem.cpp -o underwater_ecosystem
//        -lGL -lGLU -lglut -std=c++17
//
//  Controls:
//    W/S/A/D        - move camera
//    Arrow keys     - rotate view
//    B              - toggle bubbles
//    F              - toggle fog
//    N              - day / night mode
//    C              - cycle camera modes
//    P / Space      - pause animation
//    R              - reset scene
//    Mouse Left     - rotate camera (free mode)
//    Mouse Right    - zoom camera (free mode)
//    ESC            - exit
// ============================================================

#include <GL/freeglut.h>
#include <cmath>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

// ─── Constants ───────────────────────────────────────────────
const float PI = 3.14159265358979323846f;

// ─── Simple vector helper ────────────────────────────────────
struct Vec3 {
    float x, y, z;
};

// ─── Fish data ───────────────────────────────────────────────
struct Fish {
    float orbitRadius;   // radius of circular swim path
    float orbitSpeed;    // angular speed (radians / sec)
    float orbitAngle;    // current angle on path
    float height;        // Y position
    float bodyLength;    // body scale
    float r, g, b;       // color
    float phase;         // tail-wave phase offset

    float targetX;
    float targetZ;

    float dirX;
    float dirZ;
};

// ─── Sea-plant data ──────────────────────────────────────────
struct Plant {

    float x, z;

    float height;

    float swayOffset;

    int bladeCount;
};

// ─── Rock / coral data ──────────────────────────────────────
struct Rock {
    float x, z;
    float scale;
    float r, g, b;
};

struct Coral {
    float x, z;
    float height;
    float r, g, b;
};

// ─── Bubble data ─────────────────────────────────────────────
struct Bubble {
    float x, z;
    float y;             // current height
    float speed;
    float size;
    float wobble;        // horizontal wobble phase
};

struct Crab {
    float x, z;
    float angle;
    float speed;
    float scale;
    float phase;
};

struct Prawn {
    float orbitRadius;
    float orbitSpeed;
    float orbitAngle;
    float height;
    float scale;
    float phase;

    float x, z;
    float targetX, targetZ;
};

// ─── Window ──────────────────────────────────────────────────
int winW = 1280;
int winH = 720;

// ─── Animation ───────────────────────────────────────────────
float simTime   = 0.0f;
bool  paused    = false;

// ─── Mode flags ──────────────────────────────────────────────
bool nightMode    = false;
bool bubblesOn    = true;
bool fogOn        = true;
int  cameraMode   = 0; // 0=free, 1=follow fish, 2=top-down
// ─── Shark system ────────────────────────────────────────────
bool sharkMode = false;
bool fishPanic = false;
float panicTimer = 0.0f;

float sharkAngle  = 0.0f;
float sharkRadius = 28.0f;
float sharkHeight = 6.0f;

bool effectPlaying = false;
float effectTimer = 0.0f;

// ─── Camera (free mode) ──────────────────────────────────────
float camYaw      = 25.0f;
float camPitch    = 18.0f;
float camDist     = 45.0f;
const float MIN_CAM_DIST = 18.0f;
const float MAX_CAM_DIST = 70.0f;
Vec3  camTarget   = {0.0f, 4.0f, 0.0f};

// ─── Mouse state ─────────────────────────────────────────────
bool leftMouseDown  = false;
bool rightMouseDown = false;
int  lastMouseX = 0;
int  lastMouseY = 0;

// ─── Keyboard state ──────────────────────────────────────────
bool keys[256]        = {false};
bool specialKeys[256] = {false};

// ─── Scene objects ───────────────────────────────────────────
std::vector<Fish>   fishes;
std::vector<Plant>  plants;
std::vector<Rock>   rocks;
std::vector<Coral>  corals;
std::vector<Bubble> bubbles;
std::vector<Crab>   crabs;
std::vector<Prawn>  prawns;

// ─── Floating particles (plankton) ───────────────────────────
struct Plankton { float x, y, z, phase; };
std::vector<Plankton> planktons;

// ============================================================
//  Utility
// ============================================================
float degToRad(float d) { return d * PI / 180.0f; }

float clampVal(float v, float lo, float hi) {
    return std::max(lo, std::min(v, hi));
}

void playEffect(const char* file, float duration)
{
    
    PlaySound(NULL, 0, 0);

   
    PlaySound(
        file,
        NULL,
        SND_ASYNC | SND_FILENAME
    );

    effectPlaying = true;
    effectTimer = duration;
}

// Draw a simple text string at 2-D screen coordinates
void drawText2D(float x, float y, const std::string& s) {
    glRasterPos2f(x, y);
    for (char c : s)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
}

// Draw a GLU cylinder with caps
void drawCylinder(float radius, float height, int slices = 16) {
    GLUquadric* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);
    // bottom cap
    glPushMatrix();
    glRotatef(180.0f, 1,0,0);
    gluDisk(q, 0.0, radius, slices, 1);
    glPopMatrix();
    // side
    gluCylinder(q, radius, radius, height, slices, 1);
    // top cap
    glPushMatrix();
    glTranslatef(0,0,height);
    gluDisk(q, 0.0, radius, slices, 1);
    glPopMatrix();
    gluDeleteQuadric(q);
}

// Draw a tapered cone
void drawCone(float baseR, float topR, float height, int slices = 14) {
    GLUquadric* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);
    gluCylinder(q, baseR, topR, height, slices, 1);
    glPushMatrix();
    glRotatef(180.0f, 1,0,0);
    gluDisk(q, 0.0, baseR, slices, 1);
    glPopMatrix();
    gluDeleteQuadric(q);
}


//  Scene generation

void generateFish() {
    fishes.clear();

  
    struct FishTemplate {
        float orbitR, speed, y, bodyLen;
        float r, g, b;
    };

    FishTemplate templates[] = {
        { 18.0f, 0.55f, 5.0f,  1.4f,  1.0f, 0.55f, 0.05f },  // orange
        { 25.0f, 0.38f, 3.5f,  1.8f,  0.2f, 0.6f,  0.9f  },  // blue
        { 12.0f, 0.75f, 7.0f,  1.0f,  0.9f, 0.2f,  0.3f  },  // red
        { 30.0f, 0.28f, 2.5f,  2.2f,  0.4f, 0.85f, 0.5f  },  // green
        { 20.0f, 0.50f, 6.0f,  1.2f,  0.9f, 0.85f, 0.15f },  // yellow
        {  8.0f, 0.90f, 8.0f,  0.7f,  0.7f, 0.3f,  0.9f  },  // purple
    };

    int count = sizeof(templates) / sizeof(templates[0]);

    for (int t = 0; t < count; t++) {
        int numFish = 3 + rand() % 3; // 3-5 fish per group
        for (int i = 0; i < numFish; i++) {
            Fish f;
            f.orbitRadius = templates[t].orbitR + (rand() % 5 - 2);
            f.orbitSpeed  = templates[t].speed * (0.85f + (rand() % 30) / 100.0f);
            f.orbitAngle  = (float)(rand() % 360) * PI / 180.0f;
            f.height      = templates[t].y + (rand() % 4 - 2);
            f.bodyLength  = templates[t].bodyLen * (0.8f + (rand() % 40) / 100.0f);
            f.r           = templates[t].r;
            f.g           = templates[t].g;
            f.b           = templates[t].b;
            f.phase       = (float)(rand() % 628) / 100.0f;

            f.targetX = -30 + rand() % 60;
            f.targetZ = -30 + rand() % 60;

            f.dirX = 0;
            f.dirZ = 0;

            fishes.push_back(f);
        }
    }
}

void generatePlants() {

    plants.clear();

    for (int i = 0; i < 35; i++) {

        Plant p;

        p.x = -35.0f + rand() % 70;
        p.z = -35.0f + rand() % 70;

        // shorter natural grass
        p.height =
            2.0f + (rand() % 10) / 10.0f;

        p.swayOffset =
            (float)(rand() % 628) / 100.0f;

        // FIXED blade count
        p.bladeCount =
            7 + rand() % 4;

        plants.push_back(p);
    }
}

void generateRocks() {
    rocks.clear();
    for (int i = 0; i < 18; i++) {
        Rock r;
        r.x     = -40.0f + rand() % 80;
        r.z     = -40.0f + rand() % 80;
        r.scale = 0.6f + (rand() % 25) / 10.0f;
        r.r     = 0.35f + (rand() % 20) / 100.0f;
        r.g     = 0.35f + (rand() % 15) / 100.0f;
        r.b     = 0.4f  + (rand() % 15) / 100.0f;
        rocks.push_back(r);
    }
}

void generateCorals() {
    corals.clear();

    float coralColors[6][3] = {
        {0.9f, 0.3f, 0.3f},  // red coral
        {1.0f, 0.6f, 0.1f},  // orange coral
        {0.8f, 0.2f, 0.8f},  // purple coral
        {0.3f, 0.9f, 0.7f},  // teal coral
        {0.9f, 0.9f, 0.2f},  // yellow coral
        {0.5f, 0.4f, 0.9f},  // blue coral
    };

    for (int i = 0; i < 22; i++) {
        Coral c;
        c.x  = -38.0f + rand() % 76;
        c.z  = -38.0f + rand() % 76;
        c.height = 1.2f + (rand() % 20) / 10.0f;
        int col = rand() % 6;
        c.r  = coralColors[col][0];
        c.g  = coralColors[col][1];
        c.b  = coralColors[col][2];
        corals.push_back(c);
    }
}

void generateBubbles() {
    bubbles.clear();
    for (int i = 0; i < 80; i++) {
        Bubble b;
        b.x      = -40.0f + rand() % 80;
        b.z      = -40.0f + rand() % 80;
        b.y      = (float)(rand() % 1200) / 100.0f;  // spread heights at start
        b.speed  = 1.5f + (rand() % 25) / 10.0f;
        b.size   = 0.08f + (rand() % 18) / 100.0f;
        b.wobble = (float)(rand() % 628) / 100.0f;
        bubbles.push_back(b);
    }
}

void generatePlankton() {
    planktons.clear();
    for (int i = 0; i < 60; i++) {
        Plankton p;
        p.x     = -40.0f + rand() % 80;
        p.y     =   1.0f + rand() % 15;
        p.z     = -40.0f + rand() % 80;
        p.phase = (float)(rand() % 628) / 100.0f;
        planktons.push_back(p);
    }
}

void generateCrabs() {

    crabs.clear();

    for (int i = 0; i < 10; i++) {

        Crab c;

        c.x = -40.0f + rand() % 80;
        c.z = -40.0f + rand() % 80;

        c.angle =
            (float)(rand() % 360) * PI / 180.0f;

        c.speed =
            0.3f + (rand() % 20) / 100.0f;

        c.scale =
            0.7f + (rand() % 30) / 100.0f;

        c.phase =
            (float)(rand() % 628) / 100.0f;

        crabs.push_back(c);
    }
}

void generatePrawns() {

    prawns.clear();

    for (int i = 0; i < 12; i++) {

        Prawn p;

        p.orbitRadius =
            10.0f + rand() % 25;

        p.orbitSpeed =
            0.4f + (rand() % 20) / 100.0f;

        p.orbitAngle =
            (float)(rand() % 360) * PI / 180.0f;

        p.height =
            2.0f + rand() % 5;

        p.x = -20 + rand() % 40;
        p.z = -20 + rand() % 40;

        p.targetX = -25 + rand() % 50;
        p.targetZ = -25 + rand() % 50;

        p.scale =
            0.5f + (rand() % 20) / 100.0f;

        p.phase =
            (float)(rand() % 628) / 100.0f;

        prawns.push_back(p);
    }
}


//  Draw: Ocean Floor

void drawOceanFloor() {
    // Sandy floor
    glColor3f(0.72f, 0.65f, 0.42f);
    glBegin(GL_QUADS);
        glNormal3f(0, 1, 0);
        glVertex3f(-60, 0, -60);
        glVertex3f( 60, 0, -60);
        glVertex3f( 60, 0,  60);
        glVertex3f(-60, 0,  60);
    glEnd();

   
    glColor3f(0.38f, 0.32f, 0.22f);
    for (int side = 0; side < 4; side++) {
        glPushMatrix();
        glRotatef(side * 90.0f, 0, 1, 0);
        glBegin(GL_QUADS);
            glNormal3f(0, 1, 0);
            glVertex3f(-60, 0,  60);
            glVertex3f( 60, 0,  60);
            glVertex3f( 60, 0, 120);
            glVertex3f(-60, 0, 120);
        glEnd();
        glPopMatrix();
    }
}

// ============================================================
//  Draw: Rocks
// ============================================================
void drawRocks() {
    for (const auto& r : rocks) {
        glPushMatrix();
        glTranslatef(r.x, 0.0f, r.z);
        glColor3f(r.r, r.g, r.b);

        // Main rock body (squashed sphere)
        glPushMatrix();
        glScalef(r.scale, r.scale * 0.65f, r.scale * 0.85f);
        glutSolidSphere(1.0, 14, 10);
        glPopMatrix();

        // Small pebble next to it
        glPushMatrix();
        glTranslatef(r.scale * 0.7f, 0.0f, r.scale * 0.3f);
        glScalef(r.scale * 0.4f, r.scale * 0.3f, r.scale * 0.4f);
        glutSolidSphere(1.0, 10, 8);
        glPopMatrix();

        // algae on rocks

        glColor3f(0.08f, 0.45f, 0.12f);

        for (int a = 0; a < 6; a++) {

            glPushMatrix();

            glTranslatef(
                sinf(a * 2.3f) * 0.3f,
                r.scale * 0.4f,
                cosf(a * 1.7f) * 0.3f
            );

            glRotatef(rand()%360, 0,1,0);

            glBegin(GL_TRIANGLES);

                glVertex3f(0,0,0);
                glVertex3f(-0.15f,0.4f,0);
                glVertex3f(0.15f,0.4f,0);

            glEnd();

            glPopMatrix();
        }

        glPopMatrix();
    }
}

// ============================================================
//  Draw: Coral
// ============================================================
void drawSingleCoral(float height, float r, float g, float b) {
    glColor3f(r, g, b);

    int branches = 3;

    // Draw a branching coral using cylinders + spheres
    // Central stalk
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    drawCylinder(0.12f, height * 0.6f, 10);
    glPopMatrix();

    // Branch 1
    glPushMatrix();
    glTranslatef(0, height * 0.55f, 0);
    glRotatef(35.0f, 0, 0, 1);
    glRotatef(-90, 1, 0, 0);
    drawCylinder(0.08f, height * 0.4f, 8);
    glPopMatrix();

    // Branch 2
    glPushMatrix();
    glTranslatef(0, height * 0.55f, 0);
    glRotatef(-35.0f, 0, 0, 1);
    glRotatef(-90, 1, 0, 0);
    drawCylinder(0.08f, height * 0.4f, 8);
    glPopMatrix();

    // Branch 3
    glPushMatrix();
    glTranslatef(0, height * 0.55f, 0);
    glRotatef(35.0f, 1, 0, 0);
    glRotatef(-90, 1, 0, 0);
    drawCylinder(0.07f, height * 0.35f, 8);
    glPopMatrix();

    // Tip spheres
    float tipColors[3][3] = {{r*1.2f,g*1.2f,b*1.2f},{r*0.9f,g*1.1f,b*1.1f},{r*1.1f,g*0.9f,b*1.2f}};
    float offsets[3][3]   = {{height*0.38f, height*0.95f, 0},
                              {-height*0.35f, height*0.95f, 0},
                              {0, height*0.9f, height*0.35f}};
    for (int i = 0; i < 3; i++) {
        glPushMatrix();
        glTranslatef(offsets[i][0], offsets[i][1] + height * 0.55f, offsets[i][2]);
        glColor3f(
            clampVal(tipColors[i][0], 0, 1),
            clampVal(tipColors[i][1], 0, 1),
            clampVal(tipColors[i][2], 0, 1)
        );
        glutSolidSphere(0.13f, 8, 6);
        glPopMatrix();
    }
}

void drawCorals() {
    for (const auto& c : corals) {
        glPushMatrix();
        glTranslatef(c.x, 0.0f, c.z);
        drawSingleCoral(c.height, c.r, c.g, c.b);
        glPopMatrix();
    }
}

// ============================================================
//  Draw: Sea Plants (waving) - fixed geometry clusters
// ============================================================
void drawPlants() {

    for (const auto& p : plants) {

        glPushMatrix();

        glTranslatef(p.x, 0.0f, p.z);

        for (int b = 0; b < p.bladeCount; b++) {

            glPushMatrix();

            // FIXED offsets
            float angle =
                (360.0f / p.bladeCount) * b;

            float radius =
                0.15f + b * 0.03f;

            float offsetX =
                cosf(degToRad(angle)) * radius;

            float offsetZ =
                sinf(degToRad(angle)) * radius;

            glTranslatef(offsetX, 0, offsetZ);

            glRotatef(angle, 0,1,0);

            // stable slow sway
            float sway =
                sinf(simTime * 0.45f
                + p.swayOffset
                + b)
                * 0.25f;

            float bladeHeight =
                p.height * (0.9f + b * 0.03f);

            float baseWidth = 0.10f;

            int segments = 7;

            glBegin(GL_QUAD_STRIP);

            for (int i = 0; i <= segments; i++) {

                float t =
                    (float)i / segments;

                float y =
                    bladeHeight * t;

                // smooth curve
                float curve =
                    sway * t * t;

                // rounded grass shape
                float width =
                    baseWidth * (1.0f - t * 0.7f);

                glColor3f(
                    0.08f,
                    0.35f + t * 0.15f,
                    0.10f
                );

                glVertex3f(
                    curve - width,
                    y,
                    0
                );

                glVertex3f(
                    curve + width,
                    y,
                    0
                );
            }

            glEnd();

            glPopMatrix();
        }

        glPopMatrix();
    }
}

// ============================================================
//  Draw: Fish
// ============================================================
void drawSingleFish(float bodyLen, float tailWave, float r, float g, float b) {
    // Body (elongated sphere)
    glColor3f(r, g, b);
    glPushMatrix();
    glScalef(bodyLen, bodyLen * 0.35f, bodyLen * 0.25f);
    glutSolidSphere(1.0, 14, 10);
    glPopMatrix();

    // Slightly lighter belly
    glColor3f(clampVal(r+0.2f,0,1), clampVal(g+0.2f,0,1), clampVal(b+0.2f,0,1));
    glPushMatrix();
    glTranslatef(0, -bodyLen * 0.12f, 0);
    glScalef(bodyLen * 0.7f, bodyLen * 0.12f, bodyLen * 0.18f);
    glutSolidSphere(1.0, 10, 6);
    glPopMatrix();

    // Dorsal fin (triangle-ish, flattened cone)
    glColor3f(r * 0.8f, g * 0.8f, b * 0.8f);
    glPushMatrix();
    glTranslatef(-bodyLen * 0.1f, bodyLen * 0.28f, 0);
    glRotatef(-90, 1, 0, 0);
    drawCone(0.0f, bodyLen * 0.22f, bodyLen * 0.28f, 6);
    glPopMatrix();

    // Tail (two lobes) — wags with tailWave
    float tailAngle = sinf(tailWave) * 25.0f;
    glPushMatrix();
    glTranslatef(-bodyLen * 1.0f, 0, 0);
    glRotatef(tailAngle, 0, 1, 0);

    // Upper lobe
    glColor3f(r * 0.85f, g * 0.85f, b * 0.85f);
    glPushMatrix();
    glRotatef(35, 0, 0, 1);
    glRotatef(-90, 1, 0, 0);
    drawCone(0.0f, bodyLen * 0.18f, bodyLen * 0.45f, 6);
    glPopMatrix();
    // Lower lobe
    glPushMatrix();
    glRotatef(-35, 0, 0, 1);
    glRotatef(-90, 1, 0, 0);
    drawCone(0.0f, bodyLen * 0.18f, bodyLen * 0.45f, 6);
    glPopMatrix();

    glPopMatrix();

    // Eye
    glColor3f(0.05f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(bodyLen * 0.75f, bodyLen * 0.08f, bodyLen * 0.22f);
    glutSolidSphere(bodyLen * 0.07f, 8, 6);
    glPopMatrix();
    // Eye highlight
    glColor3f(1.0f, 1.0f, 1.0f);
    glPushMatrix();
    glTranslatef(bodyLen * 0.77f, bodyLen * 0.10f, bodyLen * 0.24f);
    glutSolidSphere(bodyLen * 0.025f, 5, 4);
    glPopMatrix();
}

void drawFish() {

    for (const auto& f : fishes) {

        // panic speed
        float speedMultiplier =
            fishPanic ? 3.5f : 1.0f;

        // fish spread outward
        float panicRadius =
            fishPanic
            ? (f.orbitRadius + panicTimer * 6.0f)
            : f.orbitRadius;

        // fish orbit
        float cx = f.dirX;
        float cz = f.dirZ;

        // fish disappear after panic
        if (fishPanic && panicTimer > 2.5f)
            continue;

        // facing direction
        float facing =
            f.orbitAngle + PI / 2.0f;

        // tail movement
        float tailWave =
            simTime * 4.0f + f.phase;

        // floating effect
        float bob =
            sinf(simTime * 1.5f + f.phase)
            * 0.3f;

        glPushMatrix();

        glTranslatef(
            cx,
            f.height + bob,
            cz
        );

        glRotatef(
            facing * 180.0f / PI,
            0, 1, 0
        );

        drawSingleFish(
            f.bodyLength,
            tailWave,
            f.r,
            f.g,
            f.b
        );

        glPopMatrix();
    }
}

void drawShark() {

    if (!sharkMode) return;

    // shark circular movement
    float x = cosf(sharkAngle) * sharkRadius;
    float z = sinf(sharkAngle) * sharkRadius;

    glPushMatrix();

    glTranslatef(x, sharkHeight, z);

    // face movement direction
    glRotatef(sharkAngle * 180.0f / PI + 90.0f, 0, 1, 0);

    // gentle swimming motion
    float swim = sinf(simTime * 3.0f) * 0.2f;
    glTranslatef(0, swim, 0);

    // =====================================================
    // BODY
    // =====================================================

    glColor3f(0.35f, 0.35f, 0.4f);

    glPushMatrix();
    glScalef(3.2f, 1.0f, 1.2f);
    glutSolidSphere(1.0f, 24, 24);
    glPopMatrix();

    // =====================================================
    // NOSE
    // =====================================================

    glPushMatrix();

    glTranslatef(3.0f, 0.0f, 0.0f);

    glRotatef(90, 0, 1, 0);

    glScalef(1.0f, 0.8f, 0.8f);

    glutSolidCone(1.0f, 2.0f, 20, 20);

    glPopMatrix();

    // =====================================================
    // TAIL
    // =====================================================

    glPushMatrix();

    glTranslatef(-3.2f, 0.0f, 0.0f);

    float tailSwing = sinf(simTime * 6.0f) * 25.0f;

    glRotatef(tailSwing, 0, 1, 0);

    glDisable(GL_LIGHTING);

    glColor3f(0.3f, 0.3f, 0.35f);

    glBegin(GL_TRIANGLES);

        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(-2.0f, 1.2f, 0.0f);
        glVertex3f(-2.0f, -1.2f, 0.0f);

    glEnd();

    glEnable(GL_LIGHTING);

    glPopMatrix();

    // =====================================================
    // TOP FIN
    // =====================================================

    glDisable(GL_LIGHTING);

    glColor3f(0.28f, 0.28f, 0.33f);

    glBegin(GL_TRIANGLES);

        glVertex3f(-0.5f, 1.0f, 0.0f);
        glVertex3f(0.4f, 2.3f, 0.0f);
        glVertex3f(1.3f, 1.0f, 0.0f);

    glEnd();

    // =====================================================
    // SIDE FINS
    // =====================================================

    glBegin(GL_TRIANGLES);

        // left fin
        glVertex3f(0.0f, -0.2f, 1.0f);
        glVertex3f(-1.2f, -0.8f, 2.0f);
        glVertex3f(0.7f, -0.4f, 1.0f);

        // right fin
        glVertex3f(0.0f, -0.2f, -1.0f);
        glVertex3f(-1.2f, -0.8f, -2.0f);
        glVertex3f(0.7f, -0.4f, -1.0f);

    glEnd();

    glEnable(GL_LIGHTING);

    // =====================================================
    // EYES
    // =====================================================

    glColor3f(0.0f, 0.0f, 0.0f);

    // left eye
    glPushMatrix();
    glTranslatef(2.8f, 0.35f, 0.55f);
    glutSolidSphere(0.10f, 10, 10);
    glPopMatrix();

    // right eye
    glPushMatrix();
    glTranslatef(2.8f, 0.35f, -0.55f);
    glutSolidSphere(0.10f, 10, 10);
    glPopMatrix();

    glPopMatrix();
}

void drawCrabs() {

    for (auto& c : crabs) {

        glPushMatrix();

        glTranslatef(c.x, 0.6f, c.z);

        glRotatef(
            c.angle * 180.0f / PI,
            0, 1, 0
        );

        glScalef(c.scale, c.scale, c.scale);

        // body
        glColor3f(0.8f, 0.25f, 0.1f);

        glPushMatrix();
        glScalef(1.2f, 0.5f, 1.0f);
        glutSolidSphere(1.0, 16, 12);
        glPopMatrix();

        // eyes
        glColor3f(0,0,0);

        glPushMatrix();
        glTranslatef(0.4f, 0.5f, 0.4f);
        glutSolidSphere(0.08f, 8, 8);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.4f, 0.5f, -0.4f);
        glutSolidSphere(0.08f, 8, 8);
        glPopMatrix();

        // legs
        glColor3f(0.7f, 0.2f, 0.1f);

        for (int i = -1; i <= 1; i += 2) {

            for (int j = 0; j < 3; j++) {

                glPushMatrix();

                glTranslatef(0, 0, i * 0.5f);

                glRotatef(
                    sinf(simTime * 5.0f + c.phase)
                    * 20.0f,
                    1,0,0
                );

                glBegin(GL_LINES);

                    glVertex3f(0,0,0);
                    glVertex3f(-1.0f,-0.3f,i*0.7f);

                glEnd();

                glPopMatrix();
            }
        }

        glPopMatrix();
    }
}

void drawPrawns() {

    for (auto& p : prawns) {

        float x = p.x;
        float z = p.z;

        float tailMove =
            sinf(simTime * 8.0f + p.phase);

        glPushMatrix();

        glTranslatef(x, p.height, z);

        glRotatef(
            p.orbitAngle * 180.0f / PI,
            0,1,0
        );

        glScalef(p.scale, p.scale, p.scale);

        // realistic shrimp body

        for (int i = 0; i < 7; i++) {

            glPushMatrix();

            glTranslatef(-i * 0.28f, 0, 0);

            float scale =
                0.24f - i * 0.015f;

            glColor3f(
                1.0f,
                0.55f + i * 0.03f,
                0.35f
            );

            glScalef(1.0f, 0.7f, 0.7f);

            glutSolidSphere(scale, 12, 10);

            glPopMatrix();
        }

        // head
        glColor3f(1.0f, 0.65f, 0.45f);

        glPushMatrix();

        glTranslatef(0.2f, 0, 0);

        glScalef(1.2f, 0.9f, 0.9f);

        glutSolidSphere(0.28f, 12, 10);

        glPopMatrix();

        // eyes
        glColor3f(0,0,0);

        glPushMatrix();
        glTranslatef(0.35f, 0.08f, 0.10f);
        glutSolidSphere(0.03f, 8, 8);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.35f, 0.08f, -0.10f);
        glutSolidSphere(0.03f, 8, 8);
        glPopMatrix();

        // legs
        glColor3f(1.0f, 0.45f, 0.3f);

        for (int i = 0; i < 5; i++) {

            glBegin(GL_LINES);

                glVertex3f(-i * 0.25f, -0.05f, 0);
                glVertex3f(-i * 0.25f - 0.12f, -0.18f, 0.15f);

                glVertex3f(-i * 0.25f, -0.05f, 0);
                glVertex3f(-i * 0.25f - 0.12f, -0.18f, -0.15f);

            glEnd();
        }

        // tail
        glPushMatrix();

        glTranslatef(-2.0f,0,0);

        glRotatef(
            tailMove * 25.0f,
            0,1,0
        );

        glBegin(GL_TRIANGLES);

            glVertex3f(0,0,0);
            glVertex3f(-0.5f,0.3f,0);
            glVertex3f(-0.5f,-0.3f,0);

        glEnd();

        glPopMatrix();

        glPopMatrix();
    }
}


// ============================================================
//  Draw: Bubbles
// ============================================================
void drawBubbles() {
    if (!bubblesOn) return;

    // Use blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (const auto& b : bubbles) {
        // Slight horizontal wobble
        float wx = sinf(simTime * 2.0f + b.wobble) * 0.3f;
        float wz = cosf(simTime * 1.7f + b.wobble) * 0.3f;

        glPushMatrix();
        glTranslatef(b.x + wx, b.y, b.z + wz);

        // Bubble: white-blue semi-transparent sphere
        glColor4f(0.75f, 0.90f, 1.0f, 0.35f);
        glutSolidSphere(b.size, 10, 8);

        // Highlight ring
        glColor4f(1.0f, 1.0f, 1.0f, 0.55f);
        glutSolidSphere(b.size * 0.35f, 6, 5);

        glPopMatrix();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// ============================================================
//  Draw: Plankton (tiny floating particles)
// ============================================================
void drawPlankton() {
    glDisable(GL_LIGHTING);
    glPointSize(2.5f);
    glBegin(GL_POINTS);
    for (const auto& p : planktons) {
        float glow = 0.5f + 0.5f * sinf(simTime * 2.0f + p.phase);
        glColor3f(0.4f * glow, 0.8f * glow, 0.5f * glow);
        float drift = sinf(simTime * 0.8f + p.phase) * 0.5f;
        glVertex3f(p.x + drift, p.y, p.z + drift * 0.5f);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

// ============================================================
//  Draw: Water surface (semi-transparent rippled plane)
// ============================================================
void drawWaterSurface() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    float surfaceY = 18.0f;

    // Draw in a grid so we can ripple normals
    glColor4f(0.05f, 0.35f, 0.65f, 0.30f);
    glBegin(GL_QUADS);
    int step = 5;
    for (int xi = -60; xi < 60; xi += step) {
        for (int zi = -60; zi < 60; zi += step) {
            // Simple wave normal
            float nx = sinf(simTime * 1.5f + xi * 0.3f) * 0.15f;
            float nz = sinf(simTime * 1.2f + zi * 0.3f) * 0.15f;
            glNormal3f(nx, 1.0f, nz);
            glVertex3f((float)xi,        surfaceY, (float)zi       );
            glVertex3f((float)xi + step, surfaceY, (float)zi       );
            glVertex3f((float)xi + step, surfaceY, (float)zi + step);
            glVertex3f((float)xi,        surfaceY, (float)zi + step);
        }
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// ============================================================
//  Lighting setup
// ============================================================
void setupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    // Ambient — slightly brighter in day
    float ambStr = nightMode ? 0.05f : 0.18f;
    GLfloat ambient[] = { ambStr * 0.6f, ambStr * 0.8f, ambStr * 1.0f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

    // Main sunlight / moonlight coming from above
    float lightY  = nightMode ? 50.0f : 80.0f;
    float diffStr = nightMode ? 0.25f : 0.65f;
    float specStr = nightMode ? 0.10f : 0.35f;

    GLfloat light0_pos[]  = { 10.0f, lightY, 15.0f, 0.0f };
    GLfloat light0_diff[] = { diffStr * 0.55f, diffStr * 0.75f, diffStr, 1.0f };
    GLfloat light0_spec[] = { specStr, specStr, specStr, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light0_diff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_spec);

    // Secondary fill light from slightly below (caustic feel)
    float fillStr = nightMode ? 0.04f : 0.20f;
    GLfloat light1_pos[]  = { -20.0f, 2.0f, -20.0f, 0.0f };
    GLfloat light1_diff[] = { fillStr * 0.4f, fillStr * 0.7f, fillStr, 1.0f };
    GLfloat light1_spec[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  light1_diff);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light1_spec);

    // Material defaults
    GLfloat mat_spec[] = { 0.30f, 0.30f, 0.30f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  mat_spec);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 28.0f);
}

// ============================================================
//  Fog
// ============================================================
void setupFog() {
if (!fogOn) {
glDisable(GL_FOG);
return;
}
glEnable(GL_FOG);

// Better underwater fog mode
glFogi(GL_FOG_MODE, GL_EXP2);

GLfloat fogColor[4];

if (nightMode) {
    fogColor[0] = 0.01f;
    fogColor[1] = 0.04f;
    fogColor[2] = 0.10f;
    fogColor[3] = 1.0f;

    glFogf(GL_FOG_DENSITY, 0.035f);
}
else {
    fogColor[0] = 0.04f;
    fogColor[1] = 0.22f;
    fogColor[2] = 0.45f;
    fogColor[3] = 1.0f;

    glFogf(GL_FOG_DENSITY, 0.018f);
}

glFogfv(GL_FOG_COLOR, fogColor);

// smoother fog quality
glHint(GL_FOG_HINT, GL_NICEST);
}


// ============================================================
//  Camera
// ============================================================
void updateCamera() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float yawRad   = degToRad(camYaw);
    float pitchRad = degToRad(camPitch);

    if (cameraMode == 0) {
        // Free orbital camera
        float cx = camTarget.x + camDist * cosf(pitchRad) * sinf(yawRad);
        float cy = camTarget.y + camDist * sinf(pitchRad);
        float cz = camTarget.z + camDist * cosf(pitchRad) * cosf(yawRad);
        gluLookAt(cx, cy, cz,
                  camTarget.x, camTarget.y, camTarget.z,
                  0, 1, 0);
    }
    else if (cameraMode == 1) {
        // Follow first fish
        if (!fishes.empty()) {
            const Fish& f = fishes[0];
            float fx = cosf(f.orbitAngle) * f.orbitRadius;
            float fz = sinf(f.orbitAngle) * f.orbitRadius;
            float fy = f.height;

            float behind = f.orbitAngle + PI;  // camera behind fish
            float camX = fx + cosf(behind) * 10.0f;
            float camZ = fz + sinf(behind) * 10.0f;

            gluLookAt(camX, fy + 3.0f, camZ,
                      fx,   fy,         fz,
                      0, 1, 0);
        }
    }
    else {
        // Top-down camera
        gluLookAt(0, 55.0f, 0.01f,
                  0,  0.0f, 0,
                  0,  1,    0);
    }
}

// ============================================================
//  HUD
// ============================================================
void drawHUD() {
    // Switch to 2-D orthographic projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    // Semi-transparent black background bar at the top
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.45f);
    glBegin(GL_QUADS);
        glVertex2f(0, winH);
        glVertex2f((float)winW, winH);
        glVertex2f((float)winW, winH - 52.0f);
        glVertex2f(0, winH - 52.0f);
    glEnd();
    glDisable(GL_BLEND);

    // Status text
    glColor3f(0.5f, 0.95f, 1.0f);

    const char* camNames[] = { "Free", "Follow Fish", "Top-Down" };
    std::string camStr  = std::string("Camera: ") + camNames[cameraMode];
    std::string bubStr  = std::string("Bubbles: ") + (bubblesOn ? "ON" : "OFF");
    std::string fogStr  = std::string("Fog: ")     + (fogOn     ? "ON" : "OFF");
    std::string modeStr = std::string("Mode: ")    + (nightMode ? "Night" : "Day");
    std::string sharkStr = std::string("Shark: ") + (sharkMode ? "ON" : "OFF");
    std::string pauseStr = paused ? "  [PAUSED]" : "";

    drawText2D(10, winH - 18,
        "Underwater Ecosystem  |  " + camStr + "  |  " + bubStr +
        "  |  " + fogStr + "  |  " + modeStr + "  |  " + sharkStr + pauseStr);

    glColor3f(0.75f, 0.90f, 1.0f);
    drawText2D(10, winH - 36,
        "W/S/A/D: Move   Arrows: Rotate   B: Bubbles   F: Fog   N: Day/Night   H: Shark");
    drawText2D(10, winH - 51,
        "C: Camera   P: Pause   R: Reset   Mouse-L: Rotate   Mouse-R: Zoom   ESC: Exit");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================
//  Animation update
// ============================================================
void updateAnimation(float dt) {
    if (paused) return;

    if (effectPlaying)
    {
        effectTimer -= dt;

        if (effectTimer <= 0.0f)
        {
            effectPlaying = false;

            // restart ocean ambience
            PlaySound(
                TEXT("ocean.wav"),
                NULL,
                SND_ASYNC | SND_LOOP | SND_FILENAME
            );
        }
    }

    simTime += dt;

    // fish panic timer
    if (fishPanic) {
        panicTimer += dt;
    }

    // Update fish positions
    for (auto& f : fishes) {

        float dx = f.targetX - f.dirX;
        float dz = f.targetZ - f.dirZ;

        float dist = sqrt(dx * dx + dz * dz);

        // choose new random target
        if (dist < 2.0f) {

            f.targetX = -35 + rand() % 70;
            f.targetZ = -35 + rand() % 70;
        }

        // normalize direction
        if (dist > 0.01f) {

            dx /= dist;
            dz /= dist;
        }

        // movement
        f.dirX += dx * f.orbitSpeed * 4.0f * dt;
        f.dirZ += dz * f.orbitSpeed * 4.0f * dt;

        // smooth rotation
        f.orbitAngle = atan2(
            f.dirZ - f.targetZ,
            f.dirX - f.targetX
        );
    }

    // crab movement
    for (auto& c : crabs) {

        c.angle += c.speed * dt;

        c.x += cosf(c.angle) * c.speed * dt;
        c.z += sinf(c.angle) * c.speed * dt;

        // keep inside area
        if (c.x < -45 || c.x > 45)
            c.angle = PI - c.angle;

        if (c.z < -45 || c.z > 45)
            c.angle = -c.angle;
    }

    // prawn movement
    for (auto& p : prawns) {
        float dx = p.targetX - p.x;
        float dz = p.targetZ - p.z;

        float dist = sqrt(dx * dx + dz * dz);

        if (dist < 2.0f) {

            p.targetX = -35 + rand() % 70;
            p.targetZ = -35 + rand() % 70;
        }

        if (dist > 0.01f) {

            dx /= dist;
            dz /= dist;
        }

        p.x += dx * p.orbitSpeed * 4.0f * dt;
        p.z += dz * p.orbitSpeed * 4.0f * dt;

        p.orbitAngle = atan2(dz, dx);
    }

    // shark movement
    if (sharkMode) {
        sharkAngle += 0.35f * dt;
    }

    // Update bubbles — rise and reset when they reach surface
    for (auto& b : bubbles) {
        b.y += b.speed * dt;
        if (b.y > 17.5f) {
            b.y  = 0.1f;
            b.x  = -40.0f + rand() % 80;
            b.z  = -40.0f + rand() % 80;
        }
    }
}

// ============================================================
//  Camera movement from keyboard
// ============================================================
void processKeys(float dt) {
    float moveSpeed = 10.0f * dt;
    float rotSpeed  = 55.0f * dt;

    // WASD: move camera target
    if (keys['w'] || keys['W']) {
        camTarget.z -= moveSpeed * cosf(degToRad(camYaw));
        camTarget.x -= moveSpeed * sinf(degToRad(camYaw));
    }
    if (keys['s'] || keys['S']) {
        camTarget.z += moveSpeed * cosf(degToRad(camYaw));
        camTarget.x += moveSpeed * sinf(degToRad(camYaw));
    }
    if (keys['a'] || keys['A']) {
        camTarget.x -= moveSpeed * cosf(degToRad(camYaw));
        camTarget.z += moveSpeed * sinf(degToRad(camYaw));
    }
    if (keys['d'] || keys['D']) {
        camTarget.x += moveSpeed * cosf(degToRad(camYaw));
        camTarget.z -= moveSpeed * sinf(degToRad(camYaw));
    }

    // Arrow keys: rotate in free mode
    if (cameraMode == 0) {
        if (specialKeys[GLUT_KEY_LEFT])  camYaw   -= rotSpeed;
        if (specialKeys[GLUT_KEY_RIGHT]) camYaw   += rotSpeed;
        if (specialKeys[GLUT_KEY_UP])    camPitch += rotSpeed * 0.6f;
        if (specialKeys[GLUT_KEY_DOWN])  camPitch -= rotSpeed * 0.6f;
        camPitch = clampVal(camPitch, -15.0f, 75.0f);
    }
}

// ============================================================
//  Display callback
// ============================================================
void display() {
    // Clear with underwater tint
    if (nightMode)
        glClearColor(0.01f, 0.03f, 0.09f, 1.0f);
    else
        glClearColor(0.04f, 0.18f, 0.40f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)winW / winH, 0.1, 300.0);

    updateCamera();
    setupLighting();
    setupFog();

    // Draw scene
    drawOceanFloor();
    drawRocks();
    drawCorals();
    drawPlants();
    drawFish();
    drawPrawns();
    drawCrabs();
    drawShark();
    drawPlankton();
    drawBubbles();
    drawWaterSurface();

    drawHUD();

    glutSwapBuffers();
}

// ============================================================
//  Idle callback
// ============================================================
void idle() {
    static int lastTime = glutGet(GLUT_ELAPSED_TIME);
    int currentTime     = glutGet(GLUT_ELAPSED_TIME);

    float dt = (currentTime - lastTime) / 1000.0f;
    lastTime  = currentTime;
    if (dt > 0.05f) dt = 0.05f;  // cap to avoid big jumps

    updateAnimation(dt);
    processKeys(dt);
    glutPostRedisplay();
}

// ============================================================
//  Reshape callback
// ============================================================
void reshape(int w, int h) {
    if (h == 0) h = 1;
    winW = w;
    winH = h;
    glViewport(0, 0, w, h);
}

// ============================================================
//  Reset scene
// ============================================================
void resetScene() {
    simTime    = 0.0f;
    paused     = false;
    nightMode  = false;
    bubblesOn  = true;
    fogOn      = true;
    cameraMode = 0;

    sharkMode = false;
    fishPanic = false;
    panicTimer = 0.0f;
    sharkAngle = 0.0f;

    camYaw   = 25.0f;
    camPitch = 18.0f;
    camDist  = 45.0f;
    camTarget = {0.0f, 4.0f, 0.0f};

    // Re-seed bubbles to spread heights
    for (auto& b : bubbles) {
        b.y = (float)(rand() % 1800) / 100.0f;
    }
}

// ============================================================
//  Keyboard callbacks
// ============================================================
void keyDown(unsigned char key, int /*x*/, int /*y*/) {
    keys[key] = true;

    switch (key) {
        case 27:                      exit(0);               break;
        case 'b': case 'B':

            // sound only when enabling
            if (!bubblesOn) {

                playEffect("bubble.wav", 1.5f);
            }

            bubblesOn = !bubblesOn;

            break;
        case 'f': case 'F':           fogOn     = !fogOn;     break;
        case 'n': case 'N':           nightMode = !nightMode; break;
        case 's': case 'S':

            sharkMode = !sharkMode;

            if (sharkMode) {

                playEffect("boom.wav", 2.5f);

                fishPanic = true;
                panicTimer = 0.0f;
            }
            else {

                playEffect("sonar.wav", 2.2f);

                fishPanic = false;
                panicTimer = 0.0f;
            }

            break;
        case 'c': case 'C':           cameraMode = (cameraMode + 1) % 3; break;
        case 'p': case 'P': case ' ': paused    = !paused;   break;
        case 'r': case 'R':           resetScene();           break;
    }
}

void keyUp(unsigned char key, int /*x*/, int /*y*/) {
    keys[key] = false;
}

void specialDown(int key, int /*x*/, int /*y*/) {
    specialKeys[key] = true;
}

void specialUp(int key, int /*x*/, int /*y*/) {
    specialKeys[key] = false;
}

// ============================================================
//  Mouse callbacks
// ============================================================
void mouseButton(int button, int state, int x, int y) {
    leftMouseDown  = (button == GLUT_LEFT_BUTTON  && state == GLUT_DOWN);
    rightMouseDown = (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN);

    // mouse wheel zoom in
    if (button == 3 && state == GLUT_DOWN) {

        camDist -= 2.0f;

        camDist = clampVal(
            camDist,
            MIN_CAM_DIST,
            MAX_CAM_DIST
        );
    }

    // mouse wheel zoom out
    if (button == 4 && state == GLUT_DOWN) {

        camDist += 2.0f;

        camDist = clampVal(
            camDist,
            MIN_CAM_DIST,
            MAX_CAM_DIST
        );
    }

    lastMouseX = x;
    lastMouseY = y;
}

void mouseMotion(int x, int y) {
    int dx = x - lastMouseX;
    int dy = y - lastMouseY;

    if (cameraMode == 0) {
        if (leftMouseDown) {
            camYaw   += dx * 0.35f;
            camPitch -= dy * 0.25f;
            camPitch  = clampVal(camPitch, -15.0f, 75.0f);
        }
        if (rightMouseDown) {
            camDist += dy * 0.12f;
            camDist = clampVal(
                camDist,
                MIN_CAM_DIST,
                MAX_CAM_DIST
            );
        }
    }

    lastMouseX = x;
    lastMouseY = y;
}

// ============================================================
//  OpenGL initialisation
// ============================================================
void initOpenGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);

    GLfloat mat_spec[]  = { 0.30f, 0.30f, 0.30f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  mat_spec);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 28.0f);
}

// ============================================================
//  Main
// ============================================================
int main(int argc, char** argv) {
    srand((unsigned int)time(nullptr));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Interactive Underwater Ecosystem Visualization");

    initOpenGL();

    // Audio playback is disabled in this build to avoid
    // linker/platform issues. Use `playEffect` (no-op).
    PlaySound(
        TEXT("ocean.wav"),
        NULL,
        SND_ASYNC | SND_LOOP | SND_FILENAME
    );

    // Generate all scene objects
    generateFish();
    generatePlants();
    generateRocks();
    generateCorals();
    generateBubbles();
    generatePlankton();
    generateCrabs();
    generatePrawns();

    // Register GLUT callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);

    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);

    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);

    glutMainLoop();
    return 0;
}