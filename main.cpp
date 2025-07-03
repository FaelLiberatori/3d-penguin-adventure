#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>

// --- Constantes e Configurações do Jogo ---
const int INITIAL_WIDTH = 600;
const int INITIAL_HEIGHT = 600;
const int FPS = 60;
const float ICE_SHEET_SIZE = 40.0f;
const float PINGUIN_MOVE_SPEED = 0.2f;
const float PINGUIN_ROTATE_SPEED = 3.0f;
const float PINGUIN_COLLISION_RADIUS = 0.5f;
const float FISH_COLLISION_RADIUS = 0.5f;
const float CHICK_COLLISION_RADIUS = 1.0f;
const float HOLE_RADIUS = 1.5f;
const int TIME_BONUS_FROM_FEEDING = 60;
const int INITIAL_CHICK_LIFE = 60;
const int MAX_SESSION_DURATION = 300;

// --- Estruturas de Dados ---
struct GameObject
{
    float x, y, z;
};

struct Hole
{
    float x, z, radius;
};

// --- Estado do Jogo (Variáveis Globais) ---
GameObject motherPenguin = {0.0f, 0.75f, 5.0f};
float pinguinRotationY = 180.0f;
bool motherHasFish = false;
GameObject chick = {0.0f, 0.5f, 0.0f};
std::vector<GameObject> fishes;
std::vector<Hole> holes;
int chickLifeTimer;
int sessionTimer;
bool isGameOver = false;
bool playerWon = false;
std::string endMessage = "";

// Animação
float wingAngle = 0.0f;
float legAngle = 0.0f;
bool isMoving = false;

// --- IDs das Janelas ---
int window_top, window_chase, window_side, window_free;
std::vector<int> window_ids;

// --- Protótipos das Funções ---
void initializeGame();
void init();
void drawPenguin(bool isChick, bool hasFish);
void drawFish();
void drawScene();
void drawUI();
void display_top();
void display_chase();
void display_side();
void display_free();
void reshape(int w, int h);
void timer(int value);
void keyboard(unsigned char key, int x, int y);
void specialKeyboard(int key, int x, int y);
void updateGameLogic();

// --- Funções Auxiliares ---
std::string formatTime(int totalSeconds)
{
    if (totalSeconds < 0)
        totalSeconds = 0;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2) << std::setfill('0') << seconds;
    return oss.str();
}

// --- Funções Principais ---
void initializeGame()
{
    srand(static_cast<unsigned int>(time(0)));
    fishes.clear();
    holes.clear();
    for (int i = 0; i < 5; ++i)
    {
        float fx = (rand() % (int)ICE_SHEET_SIZE) - ICE_SHEET_SIZE / 2.0f;
        float fz = (rand() % (int)ICE_SHEET_SIZE) - ICE_SHEET_SIZE / 2.0f;
        fishes.push_back({fx, 0.2f, fz});
    }
    motherPenguin = {0.0f, 0.75f, 5.0f};
    pinguinRotationY = 180.0f;
    motherHasFish = false;
    isGameOver = false;
    playerWon = false;
    endMessage = "";
    chickLifeTimer = INITIAL_CHICK_LIFE;
    sessionTimer = MAX_SESSION_DURATION;
}

void init()
{
    glClearColor(0.0f, 0.2f, 0.5f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    GLfloat light_pos[] = {5.0, 10.0, 5.0, 1.0};
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
}

// *** INÍCIO DA MODIFICAÇÃO: LÓGICA DE TEMPO CORRIGIDA ***
void updateGameLogic()
{
    static int lastUpdateTime = 0;
    // Inicializa o tempo na primeira chamada
    if (lastUpdateTime == 0)
    {
        lastUpdateTime = glutGet(GLUT_ELAPSED_TIME);
    }

    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    int deltaTime = currentTime - lastUpdateTime; // Tempo passado desde o último frame
    lastUpdateTime = currentTime;

    // Acumulador para contar o tempo para os eventos de 1 segundo
    static int timeAccumulator = 0;
    timeAccumulator += deltaTime;

    // Verifica se 1000ms (1 segundo) ou mais se passaram
    if (timeAccumulator >= 1000)
    {
        int secondsPassed = timeAccumulator / 1000; // Calcula quantos segundos se passaram

        // Atualiza os timers do jogo
        if (chickLifeTimer > 0)
            chickLifeTimer -= secondsPassed;
        else
        {
            isGameOver = true;
            endMessage = "GAME OVER: O filhote nao sobreviveu!";
        }

        if (sessionTimer > 0)
            sessionTimer -= secondsPassed;
        else if (!isGameOver)
        {
            playerWon = true;
            endMessage = "VOCE GANHOU!";
        }

        // Lógica para gerar buracos
        static int holeSpawnTimer = 0;
        holeSpawnTimer += secondsPassed;
        if (holeSpawnTimer >= 10 && holes.size() < 10)
        {
            float hx = (rand() % (int)(ICE_SHEET_SIZE - 4)) - (ICE_SHEET_SIZE / 2.0f - 2);
            float hz = (rand() % (int)(ICE_SHEET_SIZE - 4)) - (ICE_SHEET_SIZE / 2.0f - 2);
            if (sqrt(hx * hx + hz * hz) > CHICK_COLLISION_RADIUS + HOLE_RADIUS)
            {
                holes.push_back({hx, hz, HOLE_RADIUS});
            }
            holeSpawnTimer = 0;
        }

        timeAccumulator %= 1000; // Guarda o resto para não perder precisão
    }

    // A animação e colisões continuam rodando a cada frame
    if (isMoving)
    {
        float time = glutGet(GLUT_ELAPSED_TIME) * 0.01;
        wingAngle = 40.0f * sin(time);
        legAngle = 35.0f * sin(time);
    }
    else
    {
        wingAngle = 0;
        legAngle = 0;
    }
    isMoving = false;

    // Verificação de colisões (código inalterado)
    if (!motherHasFish)
    {
        for (auto it = fishes.begin(); it != fishes.end(); ++it)
        {
            float dist = sqrt(pow(motherPenguin.x - it->x, 2) + pow(motherPenguin.z - it->z, 2));
            if (dist < PINGUIN_COLLISION_RADIUS + FISH_COLLISION_RADIUS)
            {
                motherHasFish = true;
                fishes.erase(it);
                break;
            }
        }
    }
    else
    {
        float dist = sqrt(pow(motherPenguin.x - chick.x, 2) + pow(motherPenguin.z - chick.z, 2));
        if (dist < PINGUIN_COLLISION_RADIUS + CHICK_COLLISION_RADIUS)
        {
            motherHasFish = false;
            chickLifeTimer += TIME_BONUS_FROM_FEEDING;
            float fx = (rand() % (int)ICE_SHEET_SIZE) - ICE_SHEET_SIZE / 2.0f;
            float fz = (rand() % (int)ICE_SHEET_SIZE) - ICE_SHEET_SIZE / 2.0f;
            fishes.push_back({fx, 0.2f, fz});
        }
    }
    for (const auto &hole : holes)
    {
        float dist = sqrt(pow(motherPenguin.x - hole.x, 2) + pow(motherPenguin.z - hole.z, 2));
        if (dist < hole.radius)
        {
            isGameOver = true;
            endMessage = "GAME OVER: Voce caiu em um buraco!";
        }
    }
}
// *** FIM DA MODIFICAÇÃO ***

void drawFish()
{
    glPushMatrix();
    glColor3f(1.0f, 0.0f, 0.0f);
    glPushMatrix();
    glScalef(2.0f, 0.7f, 1.0f);
    glutSolidSphere(0.5, 15, 15);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-1.0f, 0.0f, 0.0f);
    glRotatef(90, 0, 1, 0);
    glutSolidCone(0.3, 0.5, 10, 2);
    glPopMatrix();
    glPopMatrix();
}

void drawPenguin(bool isChick, bool hasFish)
{
    glPushMatrix();
    glScalef(isChick ? 0.7f : 1.0f, isChick ? 0.7f : 1.0f, isChick ? 0.7f : 1.0f);

    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix();
    glScalef(1.0f, 1.3f, 1.0f);
    glutSolidSphere(0.5, 20, 20);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, 0.75f, 0.0f);
    glutSolidSphere(0.35, 20, 20);
    glPopMatrix();

    glColor3f(1.0f, 1.0f, 1.0f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.25f);
    glScalef(0.8f, 1.1f, 0.8f);
    glutSolidSphere(0.5, 20, 20);
    glPopMatrix();

    glColor3f(1.0f, 0.6f, 0.0f);
    glPushMatrix();
    glTranslatef(0.0f, 0.85f, 0.3f);
    glutSolidCone(0.1, 0.5, 10, 10);
    if (hasFish)
    {
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.4f);
        glScalef(0.2f, 0.2f, 0.2f);
        drawFish();
        glPopMatrix();
    }
    glPopMatrix();

    glColor3f(1.0f, 0.6f, 0.0f);
    glPushMatrix();
    glTranslatef(-0.2f, -0.6f, 0.0f);
    if (!isChick)
        glRotatef(legAngle, 1, 0, 0);
    glScalef(0.15f, 0.08f, 0.4f);
    glutSolidCube(1.0);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.2f, -0.6f, 0.0f);
    if (!isChick)
        glRotatef(-legAngle, 1, 0, 0);
    glScalef(0.15f, 0.08f, 0.4f);
    glutSolidCube(1.0);
    glPopMatrix();

    glColor3f(1.0f, 1.0f, 1.0f);
    glPushMatrix();
    glTranslatef(-0.1f, 0.85f, 0.3f);
    glutSolidSphere(0.07, 10, 10);
    glTranslatef(0.2f, 0.0f, 0.0f);
    glutSolidSphere(0.07, 10, 10);
    glPopMatrix();

    glColor3f(0.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(-0.1f, 0.85f, 0.32f);
    glutSolidSphere(0.03, 10, 10);
    glTranslatef(0.2f, 0.0f, 0.0f);
    glutSolidSphere(0.03, 10, 10);
    glPopMatrix();

    if (!isChick)
    {
        glColor3f(0.1f, 0.1f, 0.1f);
        glPushMatrix();
        glTranslatef(-0.5f, 0.0f, 0.0f);
        glRotatef(-20, 0, 0, 1);
        glRotatef(wingAngle, 1, 0, 0);
        glScalef(0.1f, 0.6f, 0.4f);
        glutSolidCube(1.0);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0.5f, 0.0f, 0.0f);
        glRotatef(20, 0, 0, 1);
        glRotatef(-wingAngle, 1, 0, 0);
        glScalef(0.1f, 0.6f, 0.4f);
        glutSolidCube(1.0);
        glPopMatrix();
    }
    glPopMatrix();
}

void drawScene()
{
    glColor3f(1.0f, 1.0f, 1.0f);
    glPushMatrix();
    glTranslatef(0.0f, -0.1f, 0.0f);
    glScalef(ICE_SHEET_SIZE, 0.2f, ICE_SHEET_SIZE);
    glutSolidCube(1.0);
    glPopMatrix();

    glDisable(GL_LIGHTING);
    glColor3f(0.0f, 0.0f, 0.0f);
    for (const auto &hole : holes)
    {
        glPushMatrix();
        glTranslatef(hole.x, 0.01f, hole.z);
        glRotatef(90, 1, 0, 0);
        gluDisk(gluNewQuadric(), 0, hole.radius, 20, 1);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);

    glPushMatrix();
    glTranslatef(chick.x, chick.y, chick.z);
    drawPenguin(true, false);
    glPopMatrix();

    for (const auto &fish : fishes)
    {
        glPushMatrix();
        glTranslatef(fish.x, fish.y, fish.z);
        drawFish();
        glPopMatrix();
    }

    glPushMatrix();
    glTranslatef(motherPenguin.x, motherPenguin.y, motherPenguin.z);
    glRotatef(pinguinRotationY, 0, 1, 0);
    drawPenguin(false, motherHasFish);
    glPopMatrix();
}

void drawUI()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_LIGHTING);

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    if (playerWon || isGameOver)
    {
        if (playerWon)
            glColor3f(0.1f, 0.8f, 0.1f);
        else
            glColor3f(0.8f, 0.1f, 0.1f);
        glRasterPos2i(w / 2 - 150, h / 2);
        for (char c : endMessage)
        {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
        }
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2i(w / 2 - 100, h / 2 - 30);
        std::string restartMsg = "Pressione 'R' para reiniciar";
        for (char c : restartMsg)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
    }
    else
    {
        glColor3f(1.0f, 1.0f, 1.0f);
        std::string lifeStr = "Vida do Filhote: " + formatTime(chickLifeTimer);
        glRasterPos2i(10, h - 30);
        for (char c : lifeStr)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
        std::string sessionStr = "Vitoria em: " + formatTime(sessionTimer);
        glRasterPos2i(w - 200, h - 30);
        for (char c : sessionStr)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
    }
    glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void display_top()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-20 * aspect, 20 * aspect, -20, 20, 1, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(motherPenguin.x, 50.0, motherPenguin.z, motherPenguin.x, 0, motherPenguin.z, 0, 0, -1);
    drawScene();
    drawUI();
    glutSwapBuffers();
}

void display_chase()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    float camX = motherPenguin.x - 10 * sin(pinguinRotationY * M_PI / 180.0);
    float camZ = motherPenguin.z - 10 * cos(pinguinRotationY * M_PI / 180.0);
    gluLookAt(camX, motherPenguin.y + 5, camZ, motherPenguin.x, motherPenguin.y, motherPenguin.z, 0, 1, 0);
    drawScene();
    drawUI();
    glutSwapBuffers();
}

void display_side()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(motherPenguin.x + 20, motherPenguin.y + 10, motherPenguin.z, motherPenguin.x, motherPenguin.y, motherPenguin.z, 0, 1, 0);
    drawScene();
    drawUI();
    glutSwapBuffers();
}

void display_free()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(10.0, 20.0, 20.0, motherPenguin.x, motherPenguin.y, motherPenguin.z, 0, 1, 0);
    drawScene();
    drawUI();
    glutSwapBuffers();
}

void reshape(int w, int h)
{
    if (h == 0)
        h = 1;
    glViewport(0, 0, w, h);
}

void timer(int value)
{
    updateGameLogic(); // A lógica do jogo é atualizada aqui

    // Pede para redesenhar todas as janelas
    for (int id : window_ids)
    {
        glutSetWindow(id);
        glutPostRedisplay();
    }
    glutTimerFunc(1000 / FPS, timer, 0);
}

void keyboard(unsigned char key, int x, int y)
{
    if (key == 27)
        exit(0);
    if ((isGameOver || playerWon) && (key == 'r' || key == 'R'))
    {
        initializeGame();
    }
}

void specialKeyboard(int key, int x, int y)
{
    if (isGameOver || playerWon)
        return;
    isMoving = true;
    float angleRad = pinguinRotationY * M_PI / 180.0f;
    switch (key)
    {
    case GLUT_KEY_UP:
        motherPenguin.x += sin(angleRad) * PINGUIN_MOVE_SPEED;
        motherPenguin.z += cos(angleRad) * PINGUIN_MOVE_SPEED;
        break;
    case GLUT_KEY_DOWN:
        motherPenguin.x -= sin(angleRad) * PINGUIN_MOVE_SPEED;
        motherPenguin.z -= cos(angleRad) * PINGUIN_MOVE_SPEED;
        break;
    case GLUT_KEY_LEFT:
        pinguinRotationY += PINGUIN_ROTATE_SPEED;
        break;
    case GLUT_KEY_RIGHT:
        pinguinRotationY -= PINGUIN_ROTATE_SPEED;
        break;
    }
    float limit = (ICE_SHEET_SIZE / 2.0f) - PINGUIN_COLLISION_RADIUS;
    if (motherPenguin.x > limit)
        motherPenguin.x = limit;
    if (motherPenguin.x < -limit)
        motherPenguin.x = -limit;
    if (motherPenguin.z > limit)
        motherPenguin.z = limit;
    if (motherPenguin.z < -limit)
        motherPenguin.z = -limit;
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(50, 50);
    window_chase = glutCreateWindow("Aventura Pinguim - Camera Perseguicao");
    init();
    initializeGame();
    glutDisplayFunc(display_chase);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    window_ids.push_back(window_chase);

    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(700, 50);
    window_top = glutCreateWindow("Aventura Pinguim - Camera Superior");
    init();
    glutDisplayFunc(display_top);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    window_ids.push_back(window_top);

    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(50, 650);
    window_side = glutCreateWindow("Aventura Pinguim - Camera Lateral");
    init();
    glutDisplayFunc(display_side);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    window_ids.push_back(window_side);

    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(700, 650);
    window_free = glutCreateWindow("Aventura Pinguim - Camera Livre");
    init();
    glutDisplayFunc(display_free);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    window_ids.push_back(window_free);

    glutTimerFunc(1000 / FPS, timer, 0);

    glutMainLoop();
    return 0;
}
