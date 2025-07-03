#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>

// Adiciona a implementação da biblioteca para carregar imagens
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

// --- NOVAS VARIÁVEIS PARA TEXTURA E SKYBOX ---
GLuint texturaGelo, texturaPinguimCorpo, texturaPinguimBarriga, texturaPeixe, texturaBuraco; // ADICIONADA texturaBuraco
GLuint texturasCeu[6]; // 0: direita, 1: esquerda, 2: topo, 3: base, 4: frente, 5: trás
GLUquadric* quadric = nullptr;


// --- Protótipos das Funções ---
void initializeGame();
void init();
GLuint carregarTextura(const char* caminho); // Nova função
void desenhaSkybox(); // Nova função
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

// --- NOVA FUNÇÃO PARA CARREGAR TEXTURAS ---
GLuint carregarTextura(const char* caminho) {
    int w, h, ch;
    unsigned char* img = stbi_load(caminho, &w, &h, &ch, 3);
    if (!img) {
        printf("Falha ao carregar a textura: %s\n", caminho);
        exit(1);
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
    stbi_image_free(img);
    return tex;
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

    // --- CARREGAMENTO DE TEXTURAS ---
    glEnable(GL_TEXTURE_2D);
    // **NOTA**: Certifique-se de que os arquivos de textura estão na pasta correta!
    // Você pode precisar ajustar os caminhos abaixo.
    texturaGelo = carregarTextura("textura/neve.jpg");
    texturaPinguimCorpo = carregarTextura("textura/pinguim-corpo.png");
    texturaPinguimBarriga = carregarTextura("textura/pinguim-barriga.png");
    texturaPeixe = carregarTextura("textura/peixe.png");
    texturaBuraco = carregarTextura("textura/buraco.jpg"); // TEXTURA DO BURACO CARREGADA
    texturasCeu[0] = carregarTextura("textura/ceu.jpg"); // Direita
    texturasCeu[1] = carregarTextura("textura/ceu.jpg"); // Esquerda
    texturasCeu[2] = carregarTextura("textura/ceu.jpg"); // Topo
    texturasCeu[3] = carregarTextura("textura/ceu.jpg"); // Base
    texturasCeu[4] = carregarTextura("textura/ceu.jpg"); // Frente
    texturasCeu[5] = carregarTextura("textura/ceu.jpg"); // Trás

    quadric = gluNewQuadric();
    gluQuadricTexture(quadric, GL_TRUE); // Habilita coordenadas de textura para o quadric
}

void updateGameLogic()
{
    static int lastUpdateTime = 0;
    if (lastUpdateTime == 0) {
        lastUpdateTime = glutGet(GLUT_ELAPSED_TIME);
    }

    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    int deltaTime = currentTime - lastUpdateTime;
    lastUpdateTime = currentTime;

    static int timeAccumulator = 0;
    timeAccumulator += deltaTime;

    if (timeAccumulator >= 1000)
    {
        int secondsPassed = timeAccumulator / 1000;

        if (chickLifeTimer > 0)
            chickLifeTimer -= secondsPassed;
        else {
            isGameOver = true;
            endMessage = "GAME OVER: O filhote nao sobreviveu!";
        }

        if (sessionTimer > 0)
            sessionTimer -= secondsPassed;
        else if (!isGameOver) {
            playerWon = true;
            endMessage = "VOCE GANHOU!";
        }

        static int holeSpawnTimer = 0;
        holeSpawnTimer += secondsPassed;
        if (holeSpawnTimer >= 10 && holes.size() < 10) {
            float hx = (rand() % (int)(ICE_SHEET_SIZE - 4)) - (ICE_SHEET_SIZE / 2.0f - 2);
            float hz = (rand() % (int)(ICE_SHEET_SIZE - 4)) - (ICE_SHEET_SIZE / 2.0f - 2);
            if (sqrt(hx * hx + hz * hz) > CHICK_COLLISION_RADIUS + HOLE_RADIUS) {
                holes.push_back({hx, hz, HOLE_RADIUS});
            }
            holeSpawnTimer = 0;
        }
        timeAccumulator %= 1000;
    }

    if (isMoving) {
        float time = glutGet(GLUT_ELAPSED_TIME) * 0.01;
        wingAngle = 40.0f * sin(time);
        legAngle = 35.0f * sin(time);
    } else {
        wingAngle = 0;
        legAngle = 0;
    }
    isMoving = false;

    if (!motherHasFish) {
        for (auto it = fishes.begin(); it != fishes.end(); ++it) {
            float dist = sqrt(pow(motherPenguin.x - it->x, 2) + pow(motherPenguin.z - it->z, 2));
            if (dist < PINGUIN_COLLISION_RADIUS + FISH_COLLISION_RADIUS) {
                motherHasFish = true;
                fishes.erase(it);
                break;
            }
        }
    } else {
        float dist = sqrt(pow(motherPenguin.x - chick.x, 2) + pow(motherPenguin.z - chick.z, 2));
        if (dist < PINGUIN_COLLISION_RADIUS + CHICK_COLLISION_RADIUS) {
            motherHasFish = false;
            chickLifeTimer += TIME_BONUS_FROM_FEEDING;
            float fx = (rand() % (int)ICE_SHEET_SIZE) - ICE_SHEET_SIZE / 2.0f;
            float fz = (rand() % (int)ICE_SHEET_SIZE) - ICE_SHEET_SIZE / 2.0f;
            fishes.push_back({fx, 0.2f, fz});
        }
    }

    for (const auto &hole : holes) {
        float dist = sqrt(pow(motherPenguin.x - hole.x, 2) + pow(motherPenguin.z - hole.z, 2));
        if (dist < hole.radius) {
            isGameOver = true;
            endMessage = "GAME OVER: Voce caiu em um buraco!";
        }
    }
}

// --- FUNÇÃO DE DESENHO DO PEIXE MODIFICADA ---
void drawFish()
{
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f); // Cor base branca para não tingir a textura
    glBindTexture(GL_TEXTURE_2D, texturaPeixe);

    // Corpo
    glPushMatrix();
    glScalef(2.0f, 0.7f, 1.0f);
    gluSphere(quadric, 0.5, 15, 15);
    glPopMatrix();

    // Cauda
    glPushMatrix();
    glTranslatef(-1.0f, 0.0f, 0.0f);
    glRotatef(90, 0, 1, 0);
    gluCylinder(quadric, 0.3, 0.0, 0.5, 10, 2);
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

// --- FUNÇÃO DE DESENHO DO PINGUIM MODIFICADA ---
void drawPenguin(bool isChick, bool hasFish)
{
    glPushMatrix();
    glScalef(isChick ? 0.7f : 1.0f, isChick ? 0.7f : 1.0f, isChick ? 0.7f : 1.0f);
    
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);

    // Corpo (costas)
    glBindTexture(GL_TEXTURE_2D, texturaPinguimCorpo);
    glPushMatrix();
    glScalef(1.0f, 1.3f, 1.0f);
    gluSphere(quadric, 0.5, 20, 20);
    glPopMatrix();

    // Cabeça
    glBindTexture(GL_TEXTURE_2D, texturaPinguimCorpo);
    glPushMatrix();
    glTranslatef(0.0f, 0.75f, 0.0f);
    gluSphere(quadric, 0.35, 20, 20);
    glPopMatrix();

    // Barriga (frente)
    glBindTexture(GL_TEXTURE_2D, texturaPinguimBarriga);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.25f);
    glScalef(0.8f, 1.1f, 0.8f);
    gluSphere(quadric, 0.5, 20, 20);
    glPopMatrix();
    
    glDisable(GL_TEXTURE_2D);

    // Bico
    glColor3f(1.0f, 0.6f, 0.0f);
    glPushMatrix();
    glTranslatef(0.0f, 0.85f, 0.3f);
    glutSolidCone(0.1, 0.5, 10, 10);
    if (hasFish) {
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.4f);
        glScalef(0.2f, 0.2f, 0.2f);
        drawFish();
        glPopMatrix();
    }
    glPopMatrix();

    // Pés
    glColor3f(1.0f, 0.6f, 0.0f);
    glPushMatrix();
    glTranslatef(-0.2f, -0.6f, 0.0f);
    if (!isChick) glRotatef(legAngle, 1, 0, 0);
    glScalef(0.15f, 0.08f, 0.4f);
    glutSolidCube(1.0);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.2f, -0.6f, 0.0f);
    if (!isChick) glRotatef(-legAngle, 1, 0, 0);
    glScalef(0.15f, 0.08f, 0.4f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Olhos Brancos
    glColor3f(1.0f, 1.0f, 1.0f);
    glPushMatrix();
    glTranslatef(-0.1f, 0.85f, 0.3f);
    glutSolidSphere(0.07, 10, 10);
    glTranslatef(0.2f, 0.0f, 0.0f);
    glutSolidSphere(0.07, 10, 10);
    glPopMatrix();

    // Pupilas
    glColor3f(0.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(-0.1f, 0.85f, 0.32f);
    glutSolidSphere(0.03, 10, 10);
    glTranslatef(0.2f, 0.0f, 0.0f);
    glutSolidSphere(0.03, 10, 10);
    glPopMatrix();

    // Asas
    if (!isChick) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texturaPinguimCorpo);
        glColor3f(1.0f, 1.0f, 1.0f);

        glPushMatrix();
        glTranslatef(-0.5f, 0.0f, 0.0f);
        glRotatef(-20, 0, 0, 1);
        glRotatef(wingAngle, 1, 0, 0);
        glScalef(0.1f, 0.6f, 0.4f);
        glutSolidCube(1.0); // Asas podem continuar como cubos simples
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.5f, 0.0f, 0.0f);
        glRotatef(20, 0, 0, 1);
        glRotatef(-wingAngle, 1, 0, 0);
        glScalef(0.1f, 0.6f, 0.4f);
        glutSolidCube(1.0);
        glPopMatrix();
        
        glDisable(GL_TEXTURE_2D);
    }
    glPopMatrix();
}

// --- NOVA FUNÇÃO PARA DESENHAR O SKYBOX ---
void desenhaSkybox() {
    glPushMatrix();
    // Centraliza o skybox na câmera para dar a ilusão de infinito
    glTranslatef(motherPenguin.x, motherPenguin.y, motherPenguin.z);

    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);   // O céu não deve ser afetado pela luz
    glDisable(GL_DEPTH_TEST); // O céu deve ser desenhado sempre no fundo

    glColor3f(1.0, 1.0, 1.0);

    float s = 75.0f; // Tamanho do Skybox

    // Face Direita (+X)
    glBindTexture(GL_TEXTURE_2D, texturasCeu[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(s, s, s);
    glTexCoord2f(0, 1); glVertex3f(s, s, -s);
    glEnd();

    // Face Esquerda (-X)
    glBindTexture(GL_TEXTURE_2D, texturasCeu[1]);
    glBegin(GL_QUADS);
    glTexCoord2f(1, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(-s, s, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, s);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, s);
    glEnd();

    // Face Superior (+Y)
    glBindTexture(GL_TEXTURE_2D, texturasCeu[2]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex3f(-s, s, -s);
    glTexCoord2f(0, 0); glVertex3f(-s, s, s);
    glTexCoord2f(1, 0); glVertex3f(s, s, s);
    glTexCoord2f(1, 1); glVertex3f(s, s, -s);
    glEnd();
    
    // Face Inferior (-Y) - Geralmente não é vista, mas incluída por completude
    glBindTexture(GL_TEXTURE_2D, texturasCeu[3]);
    glBegin(GL_QUADS);
    glTexCoord2f(1, 1); glVertex3f(s, -s, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, -s, -s);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, s);
    glEnd();

    // Face Frontal (-Z)
    glBindTexture(GL_TEXTURE_2D, texturasCeu[4]);
    glBegin(GL_QUADS);
    glTexCoord2f(1, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, s, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, -s);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, -s);
    glEnd();

    // Face Traseira (+Z)
    glBindTexture(GL_TEXTURE_2D, texturasCeu[5]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(-s, s, s);
    glTexCoord2f(0, 1); glVertex3f(s, s, s);
    glEnd();

    glPopAttrib();
    glPopMatrix();
}

// --- FUNÇÃO DE DESENHO DA CENA MODIFICADA ---
void drawScene()
{
    // Desenha o Skybox primeiro
    desenhaSkybox();

    // Habilita a textura para o chão de gelo
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaGelo);
    glColor3f(1.0f, 1.0f, 1.0f);

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.0f); // Chão na altura y=0
    float repeticao = 10.0f; // Quantas vezes a textura se repete
    glBegin(GL_QUADS);
        glNormal3f(0.0, 1.0, 0.0);
        glTexCoord2f(0.0, 0.0); glVertex3f(-ICE_SHEET_SIZE, 0.0, -ICE_SHEET_SIZE);
        glTexCoord2f(repeticao, 0.0); glVertex3f(ICE_SHEET_SIZE, 0.0, -ICE_SHEET_SIZE);
        glTexCoord2f(repeticao, repeticao); glVertex3f(ICE_SHEET_SIZE, 0.0, ICE_SHEET_SIZE);
        glTexCoord2f(0.0, repeticao); glVertex3f(-ICE_SHEET_SIZE, 0.0, ICE_SHEET_SIZE);
    glEnd();
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);

    // --- TRECHO MODIFICADO PARA DESENHAR OS BURACOS COM TEXTURA ---
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaBuraco);
    glDisable(GL_LIGHTING); // Desabilita iluminação para o buraco não ficar escuro
    glColor3f(1.0f, 1.0f, 1.0f); // Cor branca para não tingir a textura
    for (const auto &hole : holes)
    {
        glPushMatrix();
        glTranslatef(hole.x, 0.01f, hole.z);
        glRotatef(90, 1, 0, 0);
        // Usa o quadric global que já tem a geração de textura habilitada
        gluDisk(quadric, 0, hole.radius, 32, 1);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING); // Reabilita a iluminação para o resto da cena
    glDisable(GL_TEXTURE_2D); // Desabilita textura para não afetar outros objetos

    // Desenha o filhote
    glPushMatrix();
    glTranslatef(chick.x, chick.y, chick.z);
    drawPenguin(true, false);
    glPopMatrix();

    // Desenha os peixes
    for (const auto &fish : fishes)
    {
        glPushMatrix();
        glTranslatef(fish.x, fish.y, fish.z);
        drawFish();
        glPopMatrix();
    }

    // Desenha a mamãe pinguim
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
        
        // Centraliza a mensagem
        int msgWidth = endMessage.length() * 14; // Aproximação da largura
        glRasterPos2i(w / 2 - msgWidth / 2, h / 2);
        for (char c : endMessage)
        {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
        }
        glColor3f(1.0f, 1.0f, 1.0f);
        std::string restartMsg = "Pressione 'R' para reiniciar";
        int restartMsgWidth = restartMsg.length() * 9; // Aproximação
        glRasterPos2i(w / 2 - restartMsgWidth / 2, h / 2 - 30);
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

// Funções de display (alteradas para aumentar a distância de visão para o skybox)
void display_top() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-30 * aspect, 30 * aspect, -30, 30, 1, 200); // Aumentado o far plane
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(motherPenguin.x, 50.0, motherPenguin.z, motherPenguin.x, 0, motherPenguin.z, 0, 0, -1);
    drawScene();
    drawUI();
    glutSwapBuffers();
}

void display_chase() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 200.0); // Aumentado o far plane
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    float camX = motherPenguin.x - 10 * sin(pinguinRotationY * M_PI / 180.0);
    float camZ = motherPenguin.z - 10 * cos(pinguinRotationY * M_PI / 180.0);
    gluLookAt(camX, motherPenguin.y, camZ, motherPenguin.x, motherPenguin.y, motherPenguin.z, 0, 1, 0);
    drawScene();
    drawUI();
    glutSwapBuffers();
}

void display_side() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 200.0); // Aumentado o far plane
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(motherPenguin.x + 20, motherPenguin.y, motherPenguin.z, motherPenguin.x, motherPenguin.y, motherPenguin.z, 0, 1, 0);
    drawScene();
    drawUI();
    glutSwapBuffers();
}

void display_free() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 200.0); // Aumentado o far plane
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // Posição de câmera livre um pouco mais distante para ver o skybox melhor
    gluLookAt(25.0, 25.0, 25.0, motherPenguin.x, motherPenguin.y, motherPenguin.z, 0, 1, 0);
    drawScene();
    drawUI();
    glutSwapBuffers();
}


void reshape(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
}

void timer(int value)
{
    updateGameLogic();
    for (int id : window_ids) {
        glutSetWindow(id);
        glutPostRedisplay();
    }
    glutTimerFunc(1000 / FPS, timer, 0);
}

void keyboard(unsigned char key, int x, int y)
{
    if (key == 27) exit(0);
    if ((isGameOver || playerWon) && (key == 'r' || key == 'R')) {
        initializeGame();
    }
}

void specialKeyboard(int key, int x, int y)
{
    if (isGameOver || playerWon) return;
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
    // Limita o movimento à placa de gelo
    float limit = ICE_SHEET_SIZE - PINGUIN_COLLISION_RADIUS;
    if (motherPenguin.x > limit) motherPenguin.x = limit;
    if (motherPenguin.x < -limit) motherPenguin.x = -limit;
    if (motherPenguin.z > limit) motherPenguin.z = limit;
    if (motherPenguin.z < -limit) motherPenguin.z = -limit;
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    // Janela 1
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

    // Janela 2
    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(700, 50);
    window_top = glutCreateWindow("Aventura Pinguim - Camera Superior");
    init();
    glutDisplayFunc(display_top);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    window_ids.push_back(window_top);

    // Janela 3
    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(50, 700);
    window_side = glutCreateWindow("Aventura Pinguim - Camera Lateral");
    init();
    glutDisplayFunc(display_side);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    window_ids.push_back(window_side);

    // Janela 4
    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(700, 700);
    window_free = glutCreateWindow("Aventura Pinguim - Camera Livre");
    init();
    glutDisplayFunc(display_free);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    window_ids.push_back(window_free);

    glutTimerFunc(1000 / FPS, timer, 0);

    glutMainLoop();

    // Limpeza
    if (quadric) {
        gluDeleteQuadric(quadric);
    }
    return 0;
}
