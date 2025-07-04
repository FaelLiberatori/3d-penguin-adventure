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
const float PENGUIN_MOVE_SPEED = 0.2f;
const float PENGUIN_ROTATE_SPEED = 3.0f;
const float PENGUIN_COLLISION_RADIUS = 0.5f;
const float FISH_COLLISION_RADIUS = 0.5f;
const float CHICK_COLLISION_RADIUS = 1.0f;
const float HOLE_RADIUS = 1.5f;
const int TIME_BONUS_FROM_FEEDING = 60;
const int INITIAL_CHICK_LIFE = 60;
const int MAX_SESSION_DURATION = 300; // Nome da variável mantido.

// --- Estruturas de Dados ---

// Representa um objeto no mundo do jogo.
struct GameObject
{
    float x, y, z;
};

// Representa um buraco no gelo.
struct Hole
{
    float x, z, radius;
};

// --- NOVA ESTRUTURA PARA ORGANIZAR AS TEXTURAS ---
// Agrupa todos os IDs de textura em um só lugar.
struct TextureManager
{
    GLuint iceTexture;
    GLuint penguinBodyTexture;
    GLuint penguinBellyTexture;
    GLuint fishTexture;
    GLuint holeTexture;
    GLuint skyboxTextures[6];
};

// --- Estado do Jogo (Variáveis Globais) ---
GameObject motherPenguin = {0.0f, 0.75f, 5.0f};
float penguinRotationY = 180.0f;
bool motherHasFish = false;
GameObject chick = {0.0f, 0.5f, 0.0f};
std::vector<GameObject> fishes;
std::vector<Hole> holes;
int chickLifeTimer;
int sessionTimer;
bool isGameOver = false;
bool playerWon = false;
std::string endMessage = "";

// Variáveis de animação.
float wingAngle = 0.0f;
float legAngle = 0.0f;
bool isMoving = false;

// IDs das Janelas.
int window_top, window_chase, window_side, window_free, window_front;
std::vector<int> window_ids;

// --- Variáveis de Textura e Skybox ---
TextureManager gameTextures; // Única variável para gerenciar todas as texturas.
GLUquadric *quadric = nullptr;

// --- Protótipos das Funções ---
void initializeGame();
void init();
GLuint loadTexture(const char *path);
void loadAllTextures(); // Nova função para carregar as texturas.
void drawSkybox();
void drawPenguin(bool isChick, bool hasFish);
void drawFish();
void drawScene();
void drawUI();
void display_top();
void display_chase();
void display_side();
void display_free();
void display_front();
void reshape(int w, int h);
void timer(int value);
void keyboard(unsigned char key, int x, int y);
void specialKeyboard(int key, int x, int y);
void updateGameLogic();

// --- Funções Auxiliares ---

// Formata o tempo total em segundos para o formato MM:SS.
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

// --- Função para Carregar Texturas ---

// Carrega uma única textura de um arquivo de imagem.
GLuint loadTexture(const char *path)
{
    int w, h, ch;
    // Usa a biblioteca stb_image para carregar a imagem.
    unsigned char *img = stbi_load(path, &w, &h, &ch, 3);
    if (!img)
    {
        printf("Falha ao carregar a textura: %s\n", path);
        exit(1);
    }

    GLuint tex;
    glGenTextures(1, &tex);            // Gera um ID para a textura.
    glBindTexture(GL_TEXTURE_2D, tex); // Ativa a textura.
    // Define os parâmetros da textura.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Envia os dados da imagem para a GPU.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
    stbi_image_free(img); // Libera a memória da imagem.
    return tex;
}

// --- NOVA FUNÇÃO PARA CENTRALIZAR O CARREGAMENTO ---
// Carrega todas as texturas necessárias para o jogo de uma vez.
void loadAllTextures()
{
    gameTextures.iceTexture = loadTexture("textures/snow.jpg");
    gameTextures.penguinBodyTexture = loadTexture("textures/penguin_body.png");
    gameTextures.penguinBellyTexture = loadTexture("textures/penguin_belly.png");
    gameTextures.fishTexture = loadTexture("textures/fish.png");
    gameTextures.holeTexture = loadTexture("textures/hole.jpg");

    // Carrega a mesma textura para todas as faces do skybox.
    gameTextures.skyboxTextures[0] = loadTexture("textures/sky.jpg"); // Direita
    gameTextures.skyboxTextures[1] = loadTexture("textures/sky.jpg"); // Esquerda
    gameTextures.skyboxTextures[2] = loadTexture("textures/sky.jpg"); // Topo
    gameTextures.skyboxTextures[3] = loadTexture("textures/sky.jpg"); // Base
    gameTextures.skyboxTextures[4] = loadTexture("textures/sky.jpg"); // Frente
    gameTextures.skyboxTextures[5] = loadTexture("textures/sky.jpg"); // Trás
}

// --- Funções Principais ---

// Inicia ou reinicia as variáveis principais do estado do jogo.
void initializeGame()
{
    srand(static_cast<unsigned int>(time(0))); // Inicia o gerador de números aleatórios.
    fishes.clear();
    holes.clear();

    // Cria 5 peixes em posições aleatórias.
    for (int i = 0; i < 5; ++i)
    {
        float fx = (rand() % (int)ICE_SHEET_SIZE) - ICE_SHEET_SIZE / 2.0f;
        float fz = (rand() % (int)ICE_SHEET_SIZE) - ICE_SHEET_SIZE / 2.0f;
        fishes.push_back({fx, 0.2f, fz});
    }

    // Reseta a posição e estado do jogador e do jogo.
    motherPenguin = {0.0f, 0.75f, 5.0f};
    penguinRotationY = 180.0f;
    motherHasFish = false;
    isGameOver = false;
    playerWon = false;
    endMessage = "";
    chickLifeTimer = INITIAL_CHICK_LIFE;
    sessionTimer = MAX_SESSION_DURATION;
}

// Atualiza a lógica do jogo, como colisões e tempo.
void updateGameLogic()
{
    static int lastUpdateTime = 0;
    if (lastUpdateTime == 0)
    {
        lastUpdateTime = glutGet(GLUT_ELAPSED_TIME);
    }

    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    int deltaTime = currentTime - lastUpdateTime;
    lastUpdateTime = currentTime;

    // Acumulador para atualizar a lógica a cada segundo.
    static int timeAccumulator = 0;
    timeAccumulator += deltaTime;

    if (timeAccumulator >= 1000)
    {
        int secondsPassed = timeAccumulator / 1000;

        // Decrementa o tempo de vida do filhote.
        if (chickLifeTimer > 0)
            chickLifeTimer -= secondsPassed;
        else
        {
            isGameOver = true;
            endMessage = "GAME OVER: The chick did not survive!";
        }

        // Decrementa o tempo da sessão.
        if (sessionTimer > 0)
            sessionTimer -= secondsPassed;
        else if (!isGameOver)
        {
            playerWon = true;
            endMessage = "YOU WIN!";
        }

        // Lógica para criar novos buracos periodicamente.
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
        timeAccumulator %= 1000;
    }

    // Controla a animação das asas e pernas quando o pinguim se move.
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

    // Verifica a colisão do pinguim com os peixes.
    if (!motherHasFish)
    {
        for (auto it = fishes.begin(); it != fishes.end(); ++it)
        {
            float dist = sqrt(pow(motherPenguin.x - it->x, 2) + pow(motherPenguin.z - it->z, 2));
            if (dist < PENGUIN_COLLISION_RADIUS + FISH_COLLISION_RADIUS)
            {
                motherHasFish = true;
                fishes.erase(it);
                break;
            }
        }
    }
    else
    { // Verifica colisão com o filhote para alimentá-lo.
        float dist = sqrt(pow(motherPenguin.x - chick.x, 2) + pow(motherPenguin.z - chick.z, 2));
        if (dist < PENGUIN_COLLISION_RADIUS + CHICK_COLLISION_RADIUS)
        {
            motherHasFish = false;
            chickLifeTimer += TIME_BONUS_FROM_FEEDING;
            // Cria um novo peixe em um lugar aleatório.
            float fx = (rand() % (int)ICE_SHEET_SIZE) - ICE_SHEET_SIZE / 2.0f;
            float fz = (rand() % (int)ICE_SHEET_SIZE) - ICE_SHEET_SIZE / 2.0f;
            fishes.push_back({fx, 0.2f, fz});
        }
    }

    // Verifica se o pinguim caiu em um buraco.
    for (const auto &hole : holes)
    {
        float dist = sqrt(pow(motherPenguin.x - hole.x, 2) + pow(motherPenguin.z - hole.z, 2));
        if (dist < hole.radius)
        {
            isGameOver = true;
            endMessage = "GAME OVER: You fell into a hole!";
        }
    }
}

// --- Função de Desenho do Peixe ---
void drawFish()
{
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, gameTextures.fishTexture); // Usa a textura do peixe.

    // Corpo do peixe.
    glPushMatrix();
    glScalef(2.0f, 0.7f, 1.0f);
    gluSphere(quadric, 0.5, 15, 15);
    glPopMatrix();

    // Cauda do peixe.
    glPushMatrix();
    glTranslatef(-1.0f, 0.0f, 0.0f);
    glRotatef(90, 0, 1, 0);
    gluCylinder(quadric, 0.3, 0.0, 0.5, 10, 2);
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

// --- Função de Desenho do Pinguim ---
void drawPenguin(bool isChick, bool hasFish)
{
    glPushMatrix();
    // Diminui a escala se for o filhote.
    glScalef(isChick ? 0.7f : 1.0f, isChick ? 0.7f : 1.0f, isChick ? 0.7f : 1.0f);

    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);

    // Corpo (costas) com textura.
    glBindTexture(GL_TEXTURE_2D, gameTextures.penguinBodyTexture);
    glPushMatrix();
    glScalef(1.0f, 1.3f, 1.0f);
    gluSphere(quadric, 0.5, 20, 20);
    glPopMatrix();

    // Cabeça com textura.
    glBindTexture(GL_TEXTURE_2D, gameTextures.penguinBodyTexture);
    glPushMatrix();
    glTranslatef(0.0f, 0.75f, 0.0f);
    gluSphere(quadric, 0.35, 20, 20);
    glPopMatrix();

    // Barriga (frente) com textura.
    glBindTexture(GL_TEXTURE_2D, gameTextures.penguinBellyTexture);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.25f);
    glScalef(0.8f, 1.1f, 0.8f);
    gluSphere(quadric, 0.5, 20, 20);
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);

    // Bico do pinguim.
    glColor3f(1.0f, 0.6f, 0.0f);
    glPushMatrix();
    glTranslatef(0.0f, 0.85f, 0.3f);
    glutSolidCone(0.1, 0.5, 10, 10);
    // Se o pinguim estiver segurando um peixe, desenha o peixe no bico.
    if (hasFish)
    {
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.4f);
        glScalef(0.2f, 0.2f, 0.2f);
        drawFish();
        glPopMatrix();
    }
    glPopMatrix();

    // Pés do pinguim.
    glColor3f(1.0f, 0.6f, 0.0f);
    glPushMatrix();
    glTranslatef(-0.2f, -0.6f, 0.0f);
    if (!isChick)
        glRotatef(legAngle, 1, 0, 0); // Anima a perna.
    glScalef(0.15f, 0.08f, 0.4f);
    glutSolidCube(1.0);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.2f, -0.6f, 0.0f);
    if (!isChick)
        glRotatef(-legAngle, 1, 0, 0); // Anima a outra perna.
    glScalef(0.15f, 0.08f, 0.4f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Olhos (parte branca).
    glColor3f(1.0f, 1.0f, 1.0f);
    glPushMatrix();
    glTranslatef(-0.1f, 0.85f, 0.3f);
    glutSolidSphere(0.07, 10, 10);
    glTranslatef(0.2f, 0.0f, 0.0f);
    glutSolidSphere(0.07, 10, 10);
    glPopMatrix();

    // Pupilas (parte preta).
    glColor3f(0.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(-0.1f, 0.85f, 0.32f);
    glutSolidSphere(0.03, 10, 10);
    glTranslatef(0.2f, 0.0f, 0.0f);
    glutSolidSphere(0.03, 10, 10);
    glPopMatrix();

    // Asas (apenas para o pinguim mãe).
    if (!isChick)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, gameTextures.penguinBodyTexture);
        glColor3f(1.0f, 1.0f, 1.0f);

        // Asa esquerda.
        glPushMatrix();
        glTranslatef(-0.5f, 0.0f, 0.0f);
        glRotatef(-20, 0, 0, 1);
        glRotatef(wingAngle, 1, 0, 0); // Anima a asa.
        glScalef(0.1f, 0.6f, 0.4f);
        glutSolidCube(1.0);
        glPopMatrix();

        // Asa direita.
        glPushMatrix();
        glTranslatef(0.5f, 0.0f, 0.0f);
        glRotatef(20, 0, 0, 1);
        glRotatef(-wingAngle, 1, 0, 0); // Anima a outra asa.
        glScalef(0.1f, 0.6f, 0.4f);
        glutSolidCube(1.0);
        glPopMatrix();

        glDisable(GL_TEXTURE_2D);
    }
    glPopMatrix();
}

// --- Função para Desenhar o Skybox ---
void drawSkybox()
{
    glPushMatrix();
    // Centraliza o skybox na posição do jogador para dar a ilusão de infinito.
    glTranslatef(motherPenguin.x, motherPenguin.y, motherPenguin.z);

    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);   // Desabilita a luz para o céu não ficar escuro.
    glDisable(GL_DEPTH_TEST); // Garante que o céu seja desenhado atrás de tudo.

    glColor3f(1.0, 1.0, 1.0);
    float s = 75.0f; // Define o tamanho do Skybox.

    // Desenha cada uma das 6 faces do cubo do skybox com sua textura.
    glBindTexture(GL_TEXTURE_2D, gameTextures.skyboxTextures[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex3f(s, -s, -s);
    glTexCoord2f(1, 0);
    glVertex3f(s, -s, s);
    glTexCoord2f(1, 1);
    glVertex3f(s, s, s);
    glTexCoord2f(0, 1);
    glVertex3f(s, s, -s);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, gameTextures.skyboxTextures[1]);
    glBegin(GL_QUADS);
    glTexCoord2f(1, 0);
    glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 1);
    glVertex3f(-s, s, -s);
    glTexCoord2f(0, 1);
    glVertex3f(-s, s, s);
    glTexCoord2f(0, 0);
    glVertex3f(-s, -s, s);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, gameTextures.skyboxTextures[2]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1);
    glVertex3f(-s, s, -s);
    glTexCoord2f(0, 0);
    glVertex3f(-s, s, s);
    glTexCoord2f(1, 0);
    glVertex3f(s, s, s);
    glTexCoord2f(1, 1);
    glVertex3f(s, s, -s);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, gameTextures.skyboxTextures[3]);
    glBegin(GL_QUADS);
    glTexCoord2f(1, 1);
    glVertex3f(s, -s, -s);
    glTexCoord2f(0, 1);
    glVertex3f(-s, -s, -s);
    glTexCoord2f(0, 0);
    glVertex3f(-s, -s, s);
    glTexCoord2f(1, 0);
    glVertex3f(s, -s, s);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, gameTextures.skyboxTextures[4]);
    glBegin(GL_QUADS);
    glTexCoord2f(1, 0);
    glVertex3f(s, -s, -s);
    glTexCoord2f(1, 1);
    glVertex3f(s, s, -s);
    glTexCoord2f(0, 1);
    glVertex3f(-s, s, -s);
    glTexCoord2f(0, 0);
    glVertex3f(-s, -s, -s);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, gameTextures.skyboxTextures[5]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex3f(s, -s, s);
    glTexCoord2f(1, 0);
    glVertex3f(-s, -s, s);
    glTexCoord2f(1, 1);
    glVertex3f(-s, s, s);
    glTexCoord2f(0, 1);
    glVertex3f(s, s, s);
    glEnd();

    glPopAttrib(); // Restaura os atributos do OpenGL.
    glPopMatrix();
}

// --- Função de Desenho da Cena ---
void drawScene()
{
    // Desenha o céu primeiro.
    drawSkybox();

    // Habilita e aplica a textura do chão de gelo.
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gameTextures.iceTexture);
    glColor3f(1.0f, 1.0f, 1.0f);

    glPushMatrix();
    float textureRepeat = 10.0f; // Faz a textura se repetir no chão.
    glBegin(GL_QUADS);
    glNormal3f(0.0, 1.0, 0.0);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(-ICE_SHEET_SIZE, 0.0, -ICE_SHEET_SIZE);
    glTexCoord2f(textureRepeat, 0.0);
    glVertex3f(ICE_SHEET_SIZE, 0.0, -ICE_SHEET_SIZE);
    glTexCoord2f(textureRepeat, textureRepeat);
    glVertex3f(ICE_SHEET_SIZE, 0.0, ICE_SHEET_SIZE);
    glTexCoord2f(0.0, textureRepeat);
    glVertex3f(-ICE_SHEET_SIZE, 0.0, ICE_SHEET_SIZE);
    glEnd();
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);

    // Desenha os buracos com a textura de buraco.
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gameTextures.holeTexture);
    glDisable(GL_LIGHTING); // Desabilita luz para o buraco parecer mais escuro.
    glColor3f(1.0f, 1.0f, 1.0f);
    for (const auto &hole : holes)
    {
        glPushMatrix();
        glTranslatef(hole.x, 0.01f, hole.z); // Um pouco acima do chão para evitar z-fighting.
        glRotatef(90, 1, 0, 0);
        gluDisk(quadric, 0, hole.radius, 32, 1);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    // Desenha o filhote.
    glPushMatrix();
    glTranslatef(chick.x, chick.y, chick.z);
    drawPenguin(true, false);
    glPopMatrix();

    // Desenha os peixes.
    for (const auto &fish : fishes)
    {
        // Posiciona uma luz de destaque (spotlight) acima de cada peixe.
        GLfloat light1_position[] = {fish.x, 5.0f, fish.z, 1.0f};
        glEnable(GL_LIGHT1);
        glLightfv(GL_LIGHT1, GL_POSITION, light1_position);

        // Desenha o peixe.
        glPushMatrix();
        glTranslatef(fish.x, fish.y, fish.z);
        drawFish();
        glPopMatrix();

        // Desativa a luz para não afetar outros objetos.
        glDisable(GL_LIGHT1);
    }

    // Desenha a mamãe pinguim.
    glPushMatrix();
    glTranslatef(motherPenguin.x, motherPenguin.y, motherPenguin.z);
    glRotatef(penguinRotationY, 0, 1, 0);
    drawPenguin(false, motherHasFish);
    glPopMatrix();
}

// Desenha a Interface do Usuário (textos na tela).
void drawUI()
{
    // Muda para o modo de projeção 2D para desenhar texto.
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

    // Se o jogo acabou, mostra a mensagem de fim de jogo.
    if (playerWon || isGameOver)
    {
        if (playerWon)
            glColor3f(0.1f, 0.8f, 0.1f); // Verde para vitória.
        else
            glColor3f(0.8f, 0.1f, 0.1f); // Vermelho para derrota.

        int msgWidth = endMessage.length() * 14;
        glRasterPos2i(w / 2 - msgWidth / 2, h / 2);
        for (char c : endMessage)
        {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
        }

        // Mostra a mensagem para reiniciar.
        glColor3f(1.0f, 1.0f, 1.0f);
        std::string restartMsg = "Press 'R' to restart";
        int restartMsgWidth = restartMsg.length() * 9;
        glRasterPos2i(w / 2 - restartMsgWidth / 2, h / 2 - 30);
        for (char c : restartMsg)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
    }
    else // Se o jogo está rolando, mostra os contadores de tempo.
    {
        glColor3f(1.0f, 1.0f, 1.0f);
        std::string lifeStr = "Chick's Life: " + formatTime(chickLifeTimer);
        glRasterPos2i(10, h - 30);
        for (char c : lifeStr)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
        std::string sessionStr = "Victory in: " + formatTime(sessionTimer);
        glRasterPos2i(w - 200, h - 30);
        for (char c : sessionStr)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
    }

    // Restaura os modos de matriz e iluminação.
    glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

// --- Funções de display para cada câmera ---

// Função de display para a câmera de topo.
void display_top()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-30 * aspect, 30 * aspect, -30, 30, 1, 200); // Projeção ortográfica.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(motherPenguin.x, 50.0, motherPenguin.z, motherPenguin.x, 0, motherPenguin.z, 0, 0, -1);
    drawScene();
    drawUI();
    glutSwapBuffers();
}

// Função de display para a câmera de perseguição.
void display_chase()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 200.0); // Projeção em perspectiva.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    float camX = motherPenguin.x - 10 * sin(penguinRotationY * M_PI / 180.0);
    float camZ = motherPenguin.z - 10 * cos(penguinRotationY * M_PI / 180.0);
    gluLookAt(camX, motherPenguin.y + 5, camZ, motherPenguin.x, motherPenguin.y, motherPenguin.z, 0, 1, 0);
    drawScene();
    drawUI();
    glutSwapBuffers();
}

// Função de display para a câmera lateral.
void display_side()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 200.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(motherPenguin.x + 20, motherPenguin.y + 5, motherPenguin.z, motherPenguin.x, motherPenguin.y, motherPenguin.z, 0, 1, 0);
    drawScene();
    drawUI();
    glutSwapBuffers();
}

// Função de display para a câmera livre.
void display_free()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 200.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(25.0, 25.0, 25.0, motherPenguin.x, motherPenguin.y, motherPenguin.z, 0, 1, 0);
    drawScene();
    drawUI();
    glutSwapBuffers();
}

// Função de display para a câmera frontal.
void display_front()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 200.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(motherPenguin.x, motherPenguin.y + 5, motherPenguin.z + 20, motherPenguin.x, motherPenguin.y, motherPenguin.z, 0, 1, 0);
    drawScene();
    drawUI();
    glutSwapBuffers();
}

// Chamada quando a janela é redimensionada.
void reshape(int w, int h)
{
    if (h == 0)
        h = 1;
    glViewport(0, 0, w, h);
}

// Função de temporizador que atualiza e redesenha a tela.
void timer(int value)
{
    updateGameLogic(); // Atualiza a lógica do jogo.
    // Pede para todas as janelas se redesenharem.
    for (int id : window_ids)
    {
        glutSetWindow(id);
        glutPostRedisplay();
    }
    // Agenda a próxima chamada do timer.
    glutTimerFunc(1000 / FPS, timer, 0);
}

// Lida com as teclas normais do teclado.
void keyboard(unsigned char key, int x, int y)
{
    if (key == 27)
        exit(0); // Fecha o jogo com a tecla ESC.
    // Se o jogo acabou, a tecla 'R' reinicia.
    if ((isGameOver || playerWon) && (key == 'r' || key == 'R'))
    {
        initializeGame();
    }
}

// Lida com as teclas especiais (setas).
void specialKeyboard(int key, int x, int y)
{
    if (isGameOver || playerWon)
        return;      // Não faz nada se o jogo acabou.
    isMoving = true; // Ativa a animação de movimento.
    float angleRad = penguinRotationY * M_PI / 180.0f;
    switch (key)
    {
    case GLUT_KEY_UP:
        motherPenguin.x += sin(angleRad) * PENGUIN_MOVE_SPEED;
        motherPenguin.z += cos(angleRad) * PENGUIN_MOVE_SPEED;
        break;
    case GLUT_KEY_DOWN:
        motherPenguin.x -= sin(angleRad) * PENGUIN_MOVE_SPEED;
        motherPenguin.z -= cos(angleRad) * PENGUIN_MOVE_SPEED;
        break;
    case GLUT_KEY_LEFT:
        penguinRotationY += PENGUIN_ROTATE_SPEED;
        break;
    case GLUT_KEY_RIGHT:
        penguinRotationY -= PENGUIN_ROTATE_SPEED;
        break;
    }

    // Impede que o pinguim saia da área de gelo.
    float limit = ICE_SHEET_SIZE - PENGUIN_COLLISION_RADIUS;
    if (motherPenguin.x > limit)
        motherPenguin.x = limit;
    if (motherPenguin.x < -limit)
        motherPenguin.x = -limit;
    if (motherPenguin.z > limit)
        motherPenguin.z = limit;
    if (motherPenguin.z < -limit)
        motherPenguin.z = -limit;
}

// Função de inicialização do OpenGL.
void init()
{
    glClearColor(0.0f, 0.2f, 0.5f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    // Configura a luz ambiente global.
    GLfloat global_ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

    // Configura a luz principal (sol).
    GLfloat light0_ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat light0_diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat light0_specular[] = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat light0_position[] = {10.0f, 20.0f, 10.0f, 0.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
    glEnable(GL_LIGHT0);

    // Configura a luz de destaque para os peixes.
    GLfloat light1_ambient[] = {0.1f, 0.1f, 0.0f, 1.0f};
    GLfloat light1_diffuse[] = {1.0f, 1.0f, 0.5f, 1.0f};
    GLfloat light1_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
    GLfloat spot_direction[] = {0.0f, -1.0f, 0.0f};
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, spot_direction);
    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 45.0f);
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 5.0f);

    // --- Carregamento de Texturas Centralizado ---
    glEnable(GL_TEXTURE_2D);
    loadAllTextures(); // Chama a nova função para carregar tudo.

    // Cria o objeto quadric para desenhar esferas e cilindros.
    quadric = gluNewQuadric();
    gluQuadricTexture(quadric, GL_TRUE); // Habilita texturas no quadric.
}

// Função principal do programa.
int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    const int spacing = 50;

    // --- Criação das Janelas ---

    // Janela 1: Perseguição (Principal)
    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(spacing, spacing);
    window_chase = glutCreateWindow("Penguin Adventure - Chase Camera");
    init();           // Inicializa o OpenGL e as texturas.
    initializeGame(); // Inicializa o estado do jogo.
    glutDisplayFunc(display_chase);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    window_ids.push_back(window_chase);

    // Janela 2: Superior (Eixo Y)
    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(INITIAL_WIDTH + 2 * spacing, spacing);
    window_top = glutCreateWindow("Penguin Adventure - Top Camera (Y-axis)");
    init();
    glutDisplayFunc(display_top);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    window_ids.push_back(window_top);

    // Janela 3: Lateral (Eixo X)
    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(spacing, INITIAL_HEIGHT + 2 * spacing);
    window_side = glutCreateWindow("Penguin Adventure - Side Camera (X-axis)");
    init();
    glutDisplayFunc(display_side);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    window_ids.push_back(window_side);

    // Janela 4: Livre
    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(INITIAL_WIDTH + 2 * spacing, INITIAL_HEIGHT + 2 * spacing);
    window_free = glutCreateWindow("Penguin Adventure - Free Camera");
    init();
    glutDisplayFunc(display_free);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    window_ids.push_back(window_free);

    // Janela 5: Frontal (Eixo Z)
    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(2 * (INITIAL_WIDTH + spacing) + spacing, spacing);
    window_front = glutCreateWindow("Penguin Adventure - Front Camera (Z-axis)");
    init();
    glutDisplayFunc(display_front);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    window_ids.push_back(window_front);

    // Inicia o loop do temporizador.
    glutTimerFunc(1000 / FPS, timer, 0);

    // Inicia o loop principal do GLUT.
    glutMainLoop();

    // Limpa os recursos antes de sair.
    if (quadric)
    {
        gluDeleteQuadric(quadric);
    }
    return 0;
}
