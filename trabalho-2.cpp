#include <GL/glut.h>
#include <math.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>

// (constantes e outras definições
const int INITIAL_WIDTH = 600; const int INITIAL_HEIGHT = 600; const int FPS = 60; const int DELAY = 1000 / FPS; const float ICE_SHEET_SIZE = 40.0f; const float PINGUIN_MOVE_SPEED = 0.2f; const float PINGUIN_ROTATE_SPEED = 3.0f; const float PINGUIN_COLLISION_RADIUS = 0.5f; const float FISH_COLLISION_RADIUS = 0.5f; const float CHICK_COLLISION_RADIUS = 1.0f; const float HOLE_RADIUS = 1.5f; const int TIME_BONUS_FROM_FEEDING = 60; const int INITIAL_CHICK_LIFE = 60; const int MAX_SESSION_DURATION = 300;
struct Fish { float x, y, z; bool isVisible; Fish(float _x, float _y, float _z) : x(_x), y(_y), z(_z), isVisible(true) {} };
struct Hole { float x, z, radius; Hole(float _x, float _z, float _r) : x(_x), z(_z), radius(_r) {} };
float pinguinX = 0.0f, pinguinY = 0.75f, pinguinZ = 5.0f; float pinguinRotationY = 180.0f; bool pinguinHasFish = false; float wingAngle = 0.0f, legAngle = 0.0f; bool moving = false;
const float chickX = 0.0f, chickY = 0.5f, chickZ = 0.0f;
int chickLifeTimer, sessionTimer, framesSinceLastSecond = 0; bool isGameOver = false, playerWon = false; std::string gameOverReason = "";
std::vector<Fish> fishes; std::vector<Hole> holes; int holeSpawnTimer = 0;
int window_top, window_chase, window_side, window_free; std::vector<int> window_ids;

// Protótipos
void init(void); void display_top(); void display_chase(); void display_side(); void display_free();
void reshape(int w, int h); void timer(int value);
void keyboard(unsigned char key, int x, int y); void specialKeyboard(int key, int x, int y);
void updateGameLogic(); void checkCollisions();
void drawPenguin(bool isChick, bool hasFish); void drawFishModel(); void drawIceSheet();
void drawScene(); void drawUI();

// Implementação
std::string formatTime(int totalSeconds) {
    if (totalSeconds < 0) totalSeconds = 0; int minutes = totalSeconds / 60; int seconds = totalSeconds % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2) << std::setfill('0') << seconds;
    return oss.str();
}

void initializeGame() {
    fishes.clear(); holes.clear(); srand(static_cast<unsigned int>(time(0)));
    for (int i = 0; i < 5; ++i) {
        float fx = (rand() % (int)(ICE_SHEET_SIZE)) - ICE_SHEET_SIZE / 2.0f;
        float fz = (rand() % (int)(ICE_SHEET_SIZE)) - ICE_SHEET_SIZE / 2.0f;
        fishes.push_back(Fish(fx, -1.0f, fz));
    }
    holes.push_back(Hole(5.0f, 5.0f, HOLE_RADIUS));
    pinguinX = 0.0f; pinguinY = 0.75f; pinguinZ = 5.0f; pinguinRotationY = 180.0f;
    pinguinHasFish = false; isGameOver = false; playerWon = false; gameOverReason = "";
    chickLifeTimer = INITIAL_CHICK_LIFE; sessionTimer = MAX_SESSION_DURATION;
    framesSinceLastSecond = 0; holeSpawnTimer = 0; moving = false; wingAngle = 0; legAngle = 0;
}

void init(void)
{
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLfloat light_ambient[] = { 0.3, 0.3, 0.3, 1.0 };
    GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    initializeGame();
}

void timer(int value) {
    if (!isGameOver && !playerWon) { updateGameLogic(); }
    for (int id : window_ids) { glutSetWindow(id); glutPostRedisplay(); }
    glutTimerFunc(DELAY, timer, 0);
}

void updateGameLogic() {
    framesSinceLastSecond++;
    if (framesSinceLastSecond >= FPS) {
        if (chickLifeTimer > 0) chickLifeTimer--; else { isGameOver = true; gameOverReason = "O filhote nao sobreviveu!"; }
        if (sessionTimer > 0) sessionTimer--; else { if (!isGameOver) playerWon = true; }
        framesSinceLastSecond = 0;
        holeSpawnTimer++;
        if (holeSpawnTimer >= 10 && holes.size() < 10) {
            float hx = (rand() % (int)(ICE_SHEET_SIZE - 4)) - (ICE_SHEET_SIZE / 2.0f - 2);
            float hz = (rand() % (int)(ICE_SHEET_SIZE - 4)) - (ICE_SHEET_SIZE / 2.0f - 2);
            if (sqrt(hx*hx + hz*hz) > CHICK_COLLISION_RADIUS + HOLE_RADIUS) { holes.push_back(Hole(hx, hz, HOLE_RADIUS)); }
            holeSpawnTimer = 0;
        }
    }
    if (moving) {
        wingAngle = 40.0f * sin(glutGet(GLUT_ELAPSED_TIME) * 0.01);
        legAngle = 35.0f * sin(glutGet(GLUT_ELAPSED_TIME) * 0.01);
    } else { wingAngle = 0; legAngle = 0; }
    moving = false;
    checkCollisions();
}

void checkCollisions() {
    if (pinguinHasFish) {
        float dist = sqrt(pow(pinguinX - chickX, 2) + pow(pinguinZ - chickZ, 2));
        if (dist < PINGUIN_COLLISION_RADIUS + CHICK_COLLISION_RADIUS) {
            pinguinHasFish = false; chickLifeTimer += TIME_BONUS_FROM_FEEDING;
            float fx = (rand() % (int)(ICE_SHEET_SIZE)) - ICE_SHEET_SIZE / 2.0f;
            float fz = (rand() % (int)(ICE_SHEET_SIZE)) - ICE_SHEET_SIZE / 2.0f;
            fishes.push_back(Fish(fx, -1.0f, fz));
        }
    } else {
        for (auto it = fishes.begin(); it != fishes.end(); ++it) {
            float dist = sqrt(pow(pinguinX - it->x, 2) + pow(pinguinZ - it->z, 2));
            if (dist < PINGUIN_COLLISION_RADIUS + FISH_COLLISION_RADIUS) { pinguinHasFish = true; it = fishes.erase(it); break; }
        }
    }
    for (const auto& hole : holes) {
        float dist = sqrt(pow(pinguinX - hole.x, 2) + pow(pinguinZ - hole.z, 2));
        if (dist < hole.radius) { isGameOver = true; gameOverReason = "Voce caiu em um buraco!"; }
    }
}

void drawPenguin(bool isChick, bool hasFish) {
    glPushMatrix();
    // Corpo
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix(); glScalef(1.0f, 1.3f, 1.0f); glutSolidSphere(isChick ? 0.3 : 0.5, 20, 20); glPopMatrix();
    // Barriga
    glColor3f(1.0f, 1.0f, 1.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.0f, 0.25f); glScalef(0.8f, 1.1f, 0.8f); glutSolidSphere(isChick ? 0.3 : 0.5, 20, 20); glPopMatrix();
    // Cabeça
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix(); glTranslatef(0.0f, isChick ? 0.45f : 0.75f, 0.0f); glutSolidSphere(isChick ? 0.25 : 0.35, 20, 20); glPopMatrix();
    // Bico
    glColor3f(1.0f, 0.6f, 0.0f);
    glPushMatrix();
        glTranslatef(0.0f, isChick ? 0.55f : 0.85f, isChick ? 0.2f : 0.3f);
        glRotatef(-10.0f, 1, 0, 0);
        glutSolidCone(isChick ? 0.08 : 0.1, isChick ? 0.3 : 0.5, 10, 10);
        if (hasFish) {
            glPushMatrix(); glTranslatef(0.0f, 0.0f, 0.3f); glScalef(0.3f, 0.3f, 0.3f); drawFishModel(); glPopMatrix();
        }
    glPopMatrix();
    // Olhos
    glColor3f(1.0f, 1.0f, 1.0f);
    glPushMatrix(); glTranslatef(-0.1f, isChick ? 0.55f : 0.85f, isChick ? 0.2f : 0.3f); glutSolidSphere(isChick ? 0.05 : 0.07, 10, 10); glPopMatrix();
    glPushMatrix(); glTranslatef(0.1f, isChick ? 0.55f : 0.85f, isChick ? 0.2f : 0.3f); glutSolidSphere(isChick ? 0.05 : 0.07, 10, 10); glPopMatrix();
    
    // Patas (agora com tamanho e posição ajustados para o filhote)
    glColor3f(1.0f, 0.6f, 0.0f);

    float legPosX = isChick ? 0.15f : 0.25f;  // Posição X da pata (mais perto para o filhote)
    float legPosY = isChick ? -0.4f : -0.6f;  // Posição Y da pata (mais para cima para o filhote)
    float legScaleX = isChick ? 0.15f : 0.2f; // Largura da pata (menor para o filhote)
    float legScaleY = isChick ? 0.08f : 0.1f; // Altura da pata (menor para o filhote)
    float legScaleZ = isChick ? 0.3f : 0.5f;  // Comprimento da pata (menor para o filhote)
    
    glPushMatrix(); // Pata esquerda
        glTranslatef(-legPosX, legPosY, 0.0f);
        if (!isChick) { glRotatef(legAngle, 1, 0, 0); } // Só anima a pata da mãe
        glScalef(legScaleX, legScaleY, legScaleZ);
        glutSolidCube(1.0);
    glPopMatrix();
    
    glPushMatrix(); // Pata direita
        glTranslatef(legPosX, legPosY, 0.0f);
        if (!isChick) { glRotatef(-legAngle, 1, 0, 0); } // Só anima a pata da mãe
        glScalef(legScaleX, legScaleY, legScaleZ);
        glutSolidCube(1.0);
    glPopMatrix();

    // Asas (apenas para a mãe)
    if (!isChick) {
        glColor3f(0.1f, 0.1f, 0.1f);
        glPushMatrix(); glTranslatef(-0.5f, 0.0f, 0.0f); glRotatef(-20, 0, 0, 1); glRotatef(wingAngle, 1, 0, 0); glScalef(0.1f, 0.6f, 0.4f); glutSolidCube(1.0); glPopMatrix();
        glPushMatrix(); glTranslatef(0.5f, 0.0f, 0.0f); glRotatef(20, 0, 0, 1); glRotatef(-wingAngle, 1, 0, 0); glScalef(0.1f, 0.6f, 0.4f); glutSolidCube(1.0); glPopMatrix();
    }
    
    glPopMatrix(); // Pop da matriz principal do pinguim
}

void drawFishModel() {
    glPushMatrix();
    glColor3f(0.8f, 0.8f, 0.8f);
    glPushMatrix(); glScalef(2.0f, 0.7f, 1.0f); glutSolidSphere(0.5, 15, 15); glPopMatrix();
    glColor3f(1.0f, 0.5f, 0.0f);
    glPushMatrix(); glTranslatef(-1.0f, 0.0f, 0.0f); glRotatef(90, 0, 1, 0); glScalef(1.0f, 2.0f, 1.0f); glutSolidCone(0.3, 0.5, 10, 2); glPopMatrix();
    glPopMatrix();
}

void drawIceSheet() {
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glColor4f(0.9f, 0.95f, 1.0f, 0.7f);
    glPushMatrix();
        glTranslatef(0.0f, -0.1f, 0.0f);
        glScalef(ICE_SHEET_SIZE, 0.2f, ICE_SHEET_SIZE);
        glutSolidCube(1.0);
    glPopMatrix();
    glEnable(GL_LIGHTING);
    glDepthMask(GL_TRUE);
    
    glDisable(GL_LIGHTING);
    glColor3f(0.0f, 0.1f, 0.2f);
    for (const auto& hole : holes) {
        glPushMatrix();
            glTranslatef(hole.x, 0.01f, hole.z);
            glRotatef(90, 1, 0, 0);
            gluDisk(gluNewQuadric(), 0, hole.radius, 20, 1);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

void drawScene() {
    glPushMatrix(); glTranslatef(chickX, chickY, chickZ); drawPenguin(true, false); glPopMatrix();
    for (const auto& fish : fishes) {
        if (fish.isVisible) { glPushMatrix(); glTranslatef(fish.x, fish.y, fish.z); drawFishModel(); glPopMatrix(); }
    }
    drawIceSheet();
    glPushMatrix();
        glTranslatef(pinguinX, pinguinY, pinguinZ);
        glRotatef(pinguinRotationY, 0, 1, 0);
        drawPenguin(false, pinguinHasFish);
    glPopMatrix();
}

void drawUI() {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_LIGHTING);

    glColor3f(0.0f, 0.0f, 0.0f);
    int w = glutGet(GLUT_WINDOW_WIDTH); int h = glutGet(GLUT_WINDOW_HEIGHT);

    if (playerWon || isGameOver) {
        std::string endMsg = playerWon ? "VOCE GANHOU!" : "GAME OVER";
        std::string reasonMsg = gameOverReason;
        std::string restartMsg = "Pressione 'R' para reiniciar";

        if(playerWon) glColor3f(0.1f, 0.8f, 0.1f); else glColor3f(0.8f, 0.1f, 0.1f);
        glRasterPos2i(w/2 - 100, h/2 + 20); for (char c : endMsg) { glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c); }
        if(isGameOver && !reasonMsg.empty()){
            glColor3f(0.8f, 0.1f, 0.1f);
            glRasterPos2i(w/2 - 120, h/2); for (char c : reasonMsg) { glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c); }
        }
        glColor3f(0.0f, 0.0f, 0.0f);
        glRasterPos2i(w/2 - 100, h/2 - 30); for (char c : restartMsg) { glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c); }
    } else {
        glRasterPos2i(10, h - 20); std::string lifeStr = "Vida do Filhote: " + formatTime(chickLifeTimer);
        for (char c : lifeStr) { glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c); }
        glRasterPos2i(10, h - 40); std::string sessionStr = "Vitoria em: " + formatTime(sessionTimer);
        for (char c : sessionStr) { glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c); }
    }
    glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW); glPopMatrix();
}

void reshape(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
}

void display_top() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    float ortho_size = 20.0f;
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(-ortho_size * aspect, ortho_size * aspect, -ortho_size, ortho_size, 1, 100);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(pinguinX, 50.0, pinguinZ, pinguinX, 0, pinguinZ, 0, 0, -1);
    drawScene(); drawUI(); glutSwapBuffers();
}

void display_chase() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    float camX = pinguinX - 10 * sin(pinguinRotationY * M_PI / 180.0);
    float camZ = pinguinZ - 10 * cos(pinguinRotationY * M_PI / 180.0);
    gluLookAt(camX, pinguinY + 5, camZ, pinguinX, pinguinY, pinguinZ, 0, 1, 0);
    drawScene(); drawUI(); glutSwapBuffers();
}

void display_side() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(pinguinX + 30, pinguinY + 10, pinguinZ, pinguinX, pinguinY, pinguinZ, 0, 1, 0);
    drawScene(); drawUI(); glutSwapBuffers();
}

void display_free() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(3.0, 20.0, 20.0, pinguinX, pinguinY, pinguinZ, 0, 1, 0);
    drawScene(); drawUI(); glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y)
{
    if (isGameOver || playerWon) { if (key == 'r' || key == 'R') { initializeGame(); } }
    if (key == 27) { exit(0); }
}

void specialKeyboard(int key, int x, int y)
{
    if (isGameOver || playerWon) return;
    moving = true;
    switch (key) {
        case GLUT_KEY_UP:
            pinguinX += sin(pinguinRotationY * M_PI / 180.0) * PINGUIN_MOVE_SPEED;
            pinguinZ += cos(pinguinRotationY * M_PI / 180.0) * PINGUIN_MOVE_SPEED;
            break;
        case GLUT_KEY_DOWN:
            pinguinX -= sin(pinguinRotationY * M_PI / 180.0) * PINGUIN_MOVE_SPEED;
            pinguinZ -= cos(pinguinRotationY * M_PI / 180.0) * PINGUIN_MOVE_SPEED;
            break;
        case GLUT_KEY_LEFT:
            pinguinRotationY += PINGUIN_ROTATE_SPEED;
            break;
        case GLUT_KEY_RIGHT:
            pinguinRotationY -= PINGUIN_ROTATE_SPEED;
            break;
    }
    float limit = ICE_SHEET_SIZE / 2.0f - PINGUIN_COLLISION_RADIUS;
    if (pinguinX > limit) pinguinX = limit; if (pinguinX < -limit) pinguinX = -limit;
    if (pinguinZ > limit) pinguinZ = limit; if (pinguinZ < -limit) pinguinZ = -limit;
}

int main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  
  glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
  glutInitWindowPosition(50, 50);

  window_chase = glutCreateWindow("3D Penguin Adventure - Chase Camera");
  init();
  glutDisplayFunc(display_chase); glutReshapeFunc(reshape); glutKeyboardFunc(keyboard); glutSpecialFunc(specialKeyboard); window_ids.push_back(window_chase);
  
  glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
  glutInitWindowPosition(700, 50);

  window_top = glutCreateWindow("3D Penguin Adventure - Top Camera");
  init();
  glutDisplayFunc(display_top); glutReshapeFunc(reshape); glutKeyboardFunc(keyboard); glutSpecialFunc(specialKeyboard); window_ids.push_back(window_top);

  glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
  glutInitWindowPosition(50, 700);

  window_side = glutCreateWindow("3D Penguin Adventure - Side Camera");
  init();
  glutDisplayFunc(display_side); glutReshapeFunc(reshape); glutKeyboardFunc(keyboard); glutSpecialFunc(specialKeyboard); window_ids.push_back(window_side);

  glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
  glutInitWindowPosition(700, 700);

  window_free = glutCreateWindow("3D Penguin Adventure - Free Camera");
  init();
  glutDisplayFunc(display_free); glutReshapeFunc(reshape); glutKeyboardFunc(keyboard); glutSpecialFunc(specialKeyboard); window_ids.push_back(window_free);


  glutTimerFunc(DELAY, timer, 0);
  glutMainLoop();
  return 0;
}
