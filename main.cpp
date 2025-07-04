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

//  Constantes e Configurações do Jogo 
const int INITIAL_WIDTH = 600;                // Largura inicial da janela em pixels.
const int INITIAL_HEIGHT = 600;               // Altura inicial da janela em pixels.
const int FPS = 60;                           // Taxa de quadros por segundo desejada para a animação.
const float ICE_SHEET_SIZE = 40.0f;           // Metade do tamanho do cenário de gelo (o tamanho total é 2 * ICE_SHEET_SIZE).
const float PENGUIN_MOVE_SPEED = 0.2f;        // Velocidade de movimento do pinguim.
const float PENGUIN_ROTATE_SPEED = 3.0f;      // Velocidade de rotação do pinguim.
const float PENGUIN_COLLISION_RADIUS = 0.5f;  // Raio de colisão para o pinguim.
const float FISH_COLLISION_RADIUS = 0.5f;     // Raio de colisão para os peixes.
const float CHICK_COLLISION_RADIUS = 1.0f;    // Raio de colisão para o filhote.
const float HOLE_RADIUS = 1.5f;               // Raio dos buracos no gelo.
const int TIME_BONUS_FROM_FEEDING = 60;       // Bônus de tempo (em segundos) que o filhote ganha ao ser alimentado.
const int INITIAL_CHICK_LIFE = 60;            // Tempo de vida inicial do filhote (em segundos).
const int MAX_SESSION_DURATION = 300;         // Duração máxima da sessão de jogo para vencer (em segundos).

//  Estruturas de Dados 

// Representa um objeto no mundo do jogo.
struct GameObject
{
    float x, y, z; // Coordenadas do objeto no espaço 3D.
};

// Representa um buraco no gelo.
struct Hole
{
    float x, z, radius; // Coordenadas X, Z e raio do buraco.
};

//  NOVA ESTRUTURA PARA ORGANIZAR AS TEXTURAS 
// Agrupa todos os IDs de textura em um só lugar.
struct TextureManager
{
    GLuint iceTexture;         // ID da textura do gelo.
    GLuint penguinBodyTexture; // ID da textura do corpo do pinguim.
    GLuint penguinBellyTexture;// ID da textura da barriga do pinguim.
    GLuint fishTexture;        // ID da textura do peixe.
    GLuint holeTexture;        // ID da textura do buraco.
    GLuint skyboxTextures[6];  // Array de IDs para as texturas do skybox.
};

//  Estado do Jogo (Variáveis Globais) 
GameObject motherPenguin = {0.0f, 0.75f, 5.0f}; // Objeto que representa a pinguim mãe, com sua posição inicial.
float penguinRotationY = 180.0f;               // Rotação inicial da pinguim mãe em torno do eixo Y (em graus).
bool motherHasFish = false;                    // Flag que indica se a pinguim mãe está carregando um peixe.
GameObject chick = {0.0f, 0.5f, 0.0f};         // Objeto que representa o filhote de pinguim, com sua posição.
std::vector<GameObject> fishes;                // Vetor para armazenar todos os objetos de peixe no cenário.
std::vector<Hole> holes;                       // Vetor para armazenar todos os buracos no gelo.
int chickLifeTimer;                            // Temporizador para a vida do filhote.
int sessionTimer;                              // Temporizador para a duração total da sessão de jogo.
bool isGameOver = false;                       // Flag que indica se o jogo terminou com derrota.
bool playerWon = false;                        // Flag que indica se o jogador venceu o jogo.
std::string endMessage = "";                   // Mensagem a ser exibida na tela de fim de jogo.

// Variáveis de animação.
float wingAngle = 0.0f; // Ângulo atual de animação das asas.
float legAngle = 0.0f;  // Ângulo atual de animação das pernas.
bool isMoving = false;  // Flag que indica se o pinguim está em movimento.

// IDs das Janelas.
int window_top, window_chase, window_side, window_free, window_front; // Variáveis para armazenar os IDs das janelas GLUT.
std::vector<int> window_ids;                                          // Vetor para armazenar todos os IDs de janela para fácil iteração.

//  Variáveis de Textura e Skybox 
TextureManager gameTextures; // Única variável para gerenciar todas as texturas.
GLUquadric *quadric = nullptr; // Ponteiro para um objeto "quadric" do GLU.

//  Protótipos das Funções 
void initializeGame();                     // Protótipo da função que inicializa o estado do jogo.
void init();                               // Protótipo da função de inicialização do OpenGL.
GLuint loadTexture(const char *path);      // Protótipo da função que carrega uma textura.
void loadAllTextures();                    // Nova função para carregar as texturas. // Protótipo da função que carrega todas as texturas.
void drawSkybox();                         // Protótipo da função que desenha o skybox.
void drawPenguin(bool isChick, bool hasFish); // Protótipo da função que desenha um pinguim.
void drawFish();                           // Protótipo da função que desenha um peixe.
void drawScene();                          // Protótipo da função que desenha a cena completa do jogo.
void drawUI();                             // Protótipo da função que desenha a interface do usuário (texto na tela).
void display_top();                        // Protótipo da função de display para a câmera superior.
void display_chase();                      // Protótipo da função de display para a câmera de perseguição.
void display_side();                       // Protótipo da função de display para a câmera lateral.
void display_free();                       // Protótipo da função de display para a câmera livre.
void display_front();                      // Protótipo da função de display para a câmera frontal.
void reshape(int w, int h);                // Protótipo da função de callback para redimensionamento da janela.
void timer(int value);                     // Protótipo da função de callback do temporizador para o loop do jogo.
void keyboard(unsigned char key, int x, int y); // Protótipo da função de callback para teclas normais.
void specialKeyboard(int key, int x, int y); // Protótipo da função de callback para teclas especiais (setas).
void updateGameLogic();                    // Protótipo da função que atualiza a lógica do jogo.

//  Funções Auxiliares 

// Formata o tempo total em segundos para o formato MM:SS.
std::string formatTime(int totalSeconds)
{
    // Garante que o tempo não seja negativo.
    if (totalSeconds < 0)
        totalSeconds = 0;
    // Calcula os minutos.
    int minutes = totalSeconds / 60;
    // Calcula os segundos restantes.
    int seconds = totalSeconds % 60;
    // Usa um ostringstream para construir a string formatada.
    std::ostringstream oss;
    // Formata a string para ter sempre dois dígitos para minutos e segundos, preenchendo com '0' se necessário.
    oss << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2) << std::setfill('0') << seconds;
    // Retorna a string formatada.
    return oss.str();
}

//  Função para Carregar Texturas 

// Carrega uma única textura de um arquivo de imagem.
GLuint loadTexture(const char *path)
{
    int w, h, ch; // Variáveis para armazenar largura, altura e número de canais da imagem.
    // Usa a biblioteca stb_image para carregar a imagem.
    unsigned char *img = stbi_load(path, &w, &h, &ch, 3);
    // Verifica se a imagem foi carregada com sucesso.
    if (!img)
    {
        // Se falhar, imprime uma mensagem de erro e encerra o programa.
        printf("Falha ao carregar a textura: %s\n", path);
        exit(1);
    }

    GLuint tex; // Variável para armazenar o ID da textura OpenGL.
    glGenTextures(1, &tex);            // Gera um ID para a textura.
    glBindTexture(GL_TEXTURE_2D, tex); // Ativa a textura, tornando-a a textura 2D atual.
    // Define os parâmetros da textura.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Filtro para quando a textura é diminuída.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Filtro para quando a textura é aumentada.
    // Envia os dados da imagem para a GPU.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
    stbi_image_free(img); // Libera a memória da imagem carregada na CPU.
    return tex;           // Retorna o ID da textura gerada.
}

//  NOVA FUNÇÃO PARA CENTRALIZAR O CARREGAMENTO 
// Carrega todas as texturas necessárias para o jogo de uma vez.
void loadAllTextures()
{
    gameTextures.iceTexture = loadTexture("textures/snow.jpg");
    gameTextures.penguinBodyTexture = loadTexture("textures/penguin_body.png");
    gameTextures.penguinBellyTexture = loadTexture("textures/penguin_belly.png");
    gameTextures.fishTexture = loadTexture("textures/fish.png");
    gameTextures.holeTexture = loadTexture("textures/hole.jpg");

    // Carrega a textura para todas as faces do skybox.
    gameTextures.skyboxTextures[0] = loadTexture("textures/sky.jpg");
    gameTextures.skyboxTextures[1] = loadTexture("textures/sky.jpg");
    gameTextures.skyboxTextures[2] = loadTexture("textures/sky.jpg");
    gameTextures.skyboxTextures[3] = loadTexture("textures/sky.jpg");
    gameTextures.skyboxTextures[4] = loadTexture("textures/sky.jpg");
    gameTextures.skyboxTextures[5] = loadTexture("textures/sky.jpg");
}

//  Funções Principais 

// Inicia ou reinicia as variáveis principais do estado do jogo.
void initializeGame()
{
    srand(static_cast<unsigned int>(time(0))); // Inicia o gerador de números aleatórios com a hora atual.
    fishes.clear();  // Limpa o vetor de peixes.
    holes.clear();   // Limpa o vetor de buracos.

    // Cria 5 peixes em posições aleatórias.
    for (int i = 0; i < 5; ++i)
    {
        // Gera coordenadas aleatórias dentro dos limites do cenário de gelo.
        float fx = (rand() % (int)ICE_SHEET_SIZE) - ICE_SHEET_SIZE / 2.0f;
        float fz = (rand() % (int)ICE_SHEET_SIZE) - ICE_SHEET_SIZE / 2.0f;
        // Adiciona o novo peixe ao vetor de peixes.
        fishes.push_back({fx, 0.2f, fz});
    }

    // Reseta a posição e estado do jogador e do jogo.
    motherPenguin = {0.0f, 0.75f, 5.0f}; // Reposiciona a pinguim mãe.
    penguinRotationY = 180.0f;           // Reseta a rotação da pinguim.
    motherHasFish = false;               // A pinguim começa sem peixe.
    isGameOver = false;                  // Reseta a flag de fim de jogo.
    playerWon = false;                   // Reseta a flag de vitória.
    endMessage = "";                     // Limpa a mensagem de fim de jogo.
    chickLifeTimer = INITIAL_CHICK_LIFE; // Reseta o tempo de vida do filhote.
    sessionTimer = MAX_SESSION_DURATION; // Reseta o tempo da sessão.
}

// Atualiza a lógica do jogo, como colisões e tempo.
void updateGameLogic()
{
    // Variável estática para armazenar o tempo da última atualização.
    static int lastUpdateTime = 0;
    // Se for a primeira chamada, inicializa o tempo.
    if (lastUpdateTime == 0)
    {
        // Obtém o tempo decorrido desde o início do programa.
        lastUpdateTime = glutGet(GLUT_ELAPSED_TIME);
    }

    // Obtém o tempo atual.
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    // Calcula o tempo decorrido desde a última atualização (delta time).
    int deltaTime = currentTime - lastUpdateTime;
    // Atualiza o tempo da última atualização para o tempo atual.
    lastUpdateTime = currentTime;

    // Acumulador para atualizar a lógica a cada segundo.
    static int timeAccumulator = 0;
    // Adiciona o tempo decorrido ao acumulador.
    timeAccumulator += deltaTime;

    // Se o acumulador atingir 1000 milissegundos (1 segundo), atualiza os temporizadores.
    if (timeAccumulator >= 1000)
    {
        // Calcula quantos segundos se passaram.
        int secondsPassed = timeAccumulator / 1000;

        // Decrementa o tempo de vida do filhote.
        if (chickLifeTimer > 0)
            // Subtrai os segundos que se passaram.
            chickLifeTimer -= secondsPassed;
        else
        {
            // Se o tempo de vida acabar, o jogo termina.
            isGameOver = true;
            // Define a mensagem de derrota.
            endMessage = "FIM DE JOGO: O filhote nao sobreviveu!";
        }

        // Decrementa o tempo da sessão.
        if (sessionTimer > 0)
            // Subtrai os segundos que se passaram.
            sessionTimer -= secondsPassed;
        // Se o tempo da sessão acabar e o jogo não terminou por derrota, o jogador vence.
        else if (!isGameOver)
        {
            // Define que o jogador venceu.
            playerWon = true;
            // Define a mensagem de vitória.
            endMessage = "VOCE VENCEU!";
        }

        // Lógica para criar novos buracos periodicamente.
        static int holeSpawnTimer = 0;
        // Incrementa o temporizador de criação de buracos.
        holeSpawnTimer += secondsPassed;
        // Se passaram 10 segundos e ainda há espaço para mais buracos...
        if (holeSpawnTimer >= 10 && holes.size() < 10)
        {
            // Gera uma posição aleatória para o novo buraco, longe das bordas.
            float hx = (rand() % (int)(ICE_SHEET_SIZE - 4)) - (ICE_SHEET_SIZE / 2.0f - 2);
            float hz = (rand() % (int)(ICE_SHEET_SIZE - 4)) - (ICE_SHEET_SIZE / 2.0f - 2);
            // Verifica se o novo buraco não está muito perto do filhote.
            if (sqrt(hx * hx + hz * hz) > CHICK_COLLISION_RADIUS + HOLE_RADIUS)
            {
                // Adiciona o novo buraco à lista.
                holes.push_back({hx, hz, HOLE_RADIUS});
            }
            // Reseta o temporizador de criação de buracos.
            holeSpawnTimer = 0;
        }
        // Subtrai os segundos processados do acumulador.
        timeAccumulator %= 1000;
    }

    // Controla a animação das asas e pernas quando o pinguim se move.
    if (isMoving)
    {
        // Calcula um valor de tempo para a função seno, criando uma oscilação.
        float time = glutGet(GLUT_ELAPSED_TIME) * 0.01;
        // Define o ângulo da asa com base na função seno.
        wingAngle = 40.0f * sin(time);
        // Define o ângulo da perna com base na função seno.
        legAngle = 35.0f * sin(time);
    }
    else
    {
        // Se o pinguim não está se movendo, reseta os ângulos.
        wingAngle = 0;
        legAngle = 0;
    }
    // Reseta a flag de movimento para a próxima verificação de input.
    isMoving = false;

    // Verifica a colisão do pinguim com os peixes.
    if (!motherHasFish)
    {
        // Itera sobre todos os peixes usando um iterador.
        for (auto it = fishes.begin(); it != fishes.end(); ++it)
        {
            // Calcula a distância entre a pinguim e o peixe atual.
            float dist = sqrt(pow(motherPenguin.x - it->x, 2) + pow(motherPenguin.z - it->z, 2));
            // Se a distância for menor que a soma dos raios de colisão...
            if (dist < PENGUIN_COLLISION_RADIUS + FISH_COLLISION_RADIUS)
            {
                // A pinguim agora tem um peixe.
                motherHasFish = true;
                // Remove o peixe do vetor.
                fishes.erase(it);
                // Sai do loop, pois já pegou um peixe.
                break;
            }
        }
    }
    else
    { // Verifica colisão com o filhote para alimentá-lo.
        // Calcula a distância entre a pinguim e o filhote.
        float dist = sqrt(pow(motherPenguin.x - chick.x, 2) + pow(motherPenguin.z - chick.z, 2));
        // Se a distância for menor que a soma dos raios de colisão...
        if (dist < PENGUIN_COLLISION_RADIUS + CHICK_COLLISION_RADIUS)
        {
            // A pinguim entrega o peixe.
            motherHasFish = false;
            // Aumenta o tempo de vida do filhote.
            chickLifeTimer += TIME_BONUS_FROM_FEEDING;
            // Cria um novo peixe em um lugar aleatório.
            float fx = (rand() % (int)ICE_SHEET_SIZE) - ICE_SHEET_SIZE / 2.0f;
            float fz = (rand() % (int)ICE_SHEET_SIZE) - ICE_SHEET_SIZE / 2.0f;
            // Adiciona o novo peixe à lista.
            fishes.push_back({fx, 0.2f, fz});
        }
    }

    // Verifica se o pinguim caiu em um buraco.
    for (const auto &hole : holes)
    {
        // Calcula a distância entre a pinguim e o buraco atual.
        float dist = sqrt(pow(motherPenguin.x - hole.x, 2) + pow(motherPenguin.z - hole.z, 2));
        // Se a distância for menor que o raio do buraco...
        if (dist < hole.radius)
        {
            // O jogo termina.
            isGameOver = true;
            // Define a mensagem de derrota.
            endMessage = "FIM DE JOGO: Voce caiu em um buraco!";
        }
    }
}

//  Função de Desenho do Peixe 
void drawFish()
{
    // Salva a matriz de transformação atual.
    glPushMatrix();
    // Habilita o uso de texturas 2D.
    glEnable(GL_TEXTURE_2D);
    // Define a cor do material para branco, para não distorcer a cor da textura.
    glColor3f(1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, gameTextures.fishTexture); // Usa a textura do peixe.

    // Corpo do peixe.
    // Salva a matriz de transformação.
    glPushMatrix();
    // Escala a esfera para formar um corpo de peixe alongado.
    glScalef(2.0f, 0.7f, 1.0f);
    // Desenha o corpo como uma esfera texturizada.
    gluSphere(quadric, 0.5, 15, 15);
    // Restaura a matriz de transformação.
    glPopMatrix();

    // Cauda do peixe.
    // Salva a matriz de transformação.
    glPushMatrix();
    // Posiciona a cauda na parte de trás do corpo.
    glTranslatef(-1.0f, 0.0f, 0.0f);
    // Rotaciona para alinhar a cauda.
    glRotatef(90, 0, 1, 0);
    // Desenha a cauda como um cilindro que afina até a ponta.
    gluCylinder(quadric, 0.3, 0.0, 0.5, 10, 2);
    // Restaura a matriz de transformação.
    glPopMatrix();

    // Desabilita o uso de texturas 2D.
    glDisable(GL_TEXTURE_2D);
    // Restaura a matriz de transformação original da função.
    glPopMatrix();
}

//  Função de Desenho do Pinguim 
void drawPenguin(bool isChick, bool hasFish)
{
    // Salva a matriz de transformação atual.
    glPushMatrix();
    // Diminui a escala se for o filhote.
    glScalef(isChick ? 0.7f : 1.0f, isChick ? 0.7f : 1.0f, isChick ? 0.7f : 1.0f);

    // Habilita o uso de texturas 2D.
    glEnable(GL_TEXTURE_2D);
    // Define a cor do material para branco, para não alterar a cor da textura.
    glColor3f(1.0f, 1.0f, 1.0f);

    // Corpo (costas) com textura.
    // Associa a textura do corpo do pinguim.
    glBindTexture(GL_TEXTURE_2D, gameTextures.penguinBodyTexture);
    // Salva a matriz de transformação.
    glPushMatrix();
    // Escala a esfera para formar um corpo oval.
    glScalef(1.0f, 1.3f, 1.0f);
    // Desenha o corpo como uma esfera texturizada.
    gluSphere(quadric, 0.5, 20, 20);
    // Restaura a matriz de transformação.
    glPopMatrix();

    // Cabeça com textura.
    // Associa a textura do corpo do pinguim (reutilizada para a cabeça).
    glBindTexture(GL_TEXTURE_2D, gameTextures.penguinBodyTexture);
    // Salva a matriz de transformação.
    glPushMatrix();
    // Posiciona a cabeça acima do corpo.
    glTranslatef(0.0f, 0.75f, 0.0f);
    // Desenha a cabeça como uma esfera texturizada.
    gluSphere(quadric, 0.35, 20, 20);
    // Restaura a matriz de transformação.
    glPopMatrix();

    // Barriga (frente) com textura.
    // Associa a textura da barriga do pinguim.
    glBindTexture(GL_TEXTURE_2D, gameTextures.penguinBellyTexture);
    // Salva a matriz de transformação.
    glPushMatrix();
    // Posiciona a barriga um pouco à frente do corpo.
    glTranslatef(0.0f, 0.0f, 0.25f);
    // Escala a esfera para formar a barriga.
    glScalef(0.8f, 1.1f, 0.8f);
    // Desenha a barriga como uma esfera texturizada.
    gluSphere(quadric, 0.5, 20, 20);
    // Restaura a matriz de transformação.
    glPopMatrix();

    // Desabilita as texturas, pois as próximas partes serão coloridas.
    glDisable(GL_TEXTURE_2D);

    // Bico do pinguim.
    // Define a cor do bico para laranja.
    glColor3f(1.0f, 0.6f, 0.0f);
    // Salva a matriz de transformação.
    glPushMatrix();
    // Posiciona o bico na frente da cabeça.
    glTranslatef(0.0f, 0.85f, 0.3f);
    // Desenha o bico como um cone sólido.
    glutSolidCone(0.1, 0.5, 10, 10);
    // Se o pinguim estiver segurando um peixe, desenha o peixe no bico.
    if (hasFish)
    {
        // Salva a matriz de transformação.
        glPushMatrix();
        // Posiciona o peixe na ponta do bico.
        glTranslatef(0.0f, 0.0f, 0.4f);
        // Reduz a escala do peixe para caber no bico.
        glScalef(0.2f, 0.2f, 0.2f);
        // Chama a função para desenhar o peixe.
        drawFish();
        // Restaura a matriz de transformação.
        glPopMatrix();
    }
    // Restaura a matriz de transformação.
    glPopMatrix();

    // Pés do pinguim.
    // Define a cor dos pés para laranja.
    glColor3f(1.0f, 0.6f, 0.0f);
    // Salva a matriz de transformação para o pé esquerdo.
    glPushMatrix();
    // Posiciona o pé esquerdo.
    glTranslatef(-0.2f, -0.6f, 0.0f);
    // Se não for o filhote, aplica a animação de rotação da perna.
    if (!isChick)
        glRotatef(legAngle, 1, 0, 0); // Anima a perna.
    // Escala um cubo para formar o pé.
    glScalef(0.15f, 0.08f, 0.4f);
    // Desenha o pé.
    glutSolidCube(1.0);
    // Restaura a matriz de transformação.
    glPopMatrix();

    // Salva a matriz de transformação para o pé direito.
    glPushMatrix();
    // Posiciona o pé direito.
    glTranslatef(0.2f, -0.6f, 0.0f);
    // Se não for o filhote, aplica a animação de rotação da outra perna.
    if (!isChick)
        glRotatef(-legAngle, 1, 0, 0); // Anima a outra perna.
    // Escala um cubo para formar o pé.
    glScalef(0.15f, 0.08f, 0.4f);
    // Desenha o pé.
    glutSolidCube(1.0);
    // Restaura a matriz de transformação.
    glPopMatrix();

    // Olhos (parte branca).
    // Define a cor para branco.
    glColor3f(1.0f, 1.0f, 1.0f);
    // Salva a matriz de transformação.
    glPushMatrix();
    // Posiciona o olho esquerdo.
    glTranslatef(-0.1f, 0.85f, 0.3f);
    // Desenha a parte branca do olho esquerdo.
    glutSolidSphere(0.07, 10, 10);
    // Move para a posição do olho direito.
    glTranslatef(0.2f, 0.0f, 0.0f);
    // Desenha a parte branca do olho direito.
    glutSolidSphere(0.07, 10, 10);
    // Restaura a matriz de transformação.
    glPopMatrix();

    // Pupilas (parte preta).
    // Define a cor para preto.
    glColor3f(0.0f, 0.0f, 0.0f);
    // Salva a matriz de transformação.
    glPushMatrix();
    // Posiciona a pupila esquerda um pouco à frente do olho.
    glTranslatef(-0.1f, 0.85f, 0.32f);
    // Desenha a pupila esquerda.
    glutSolidSphere(0.03, 10, 10);
    // Move para a posição da pupila direita.
    glTranslatef(0.2f, 0.0f, 0.0f);
    // Desenha a pupila direita.
    glutSolidSphere(0.03, 10, 10);
    // Restaura a matriz de transformação.
    glPopMatrix();

    // Asas (apenas para o pinguim mãe).
    if (!isChick)
    {
        // Habilita texturas para as asas.
        glEnable(GL_TEXTURE_2D);
        // Usa a mesma textura do corpo para as asas.
        glBindTexture(GL_TEXTURE_2D, gameTextures.penguinBodyTexture);
        // Define a cor do material para branco.
        glColor3f(1.0f, 1.0f, 1.0f);

        // Asa esquerda.
        // Salva a matriz de transformação.
        glPushMatrix();
        // Posiciona a asa no lado esquerdo do corpo.
        glTranslatef(-0.5f, 0.0f, 0.0f);
        // Inclina a asa ligeiramente.
        glRotatef(-20, 0, 0, 1);
        // Aplica a animação de bater de asas.
        glRotatef(wingAngle, 1, 0, 0); // Anima a asa.
        // Escala um cubo para formar a asa.
        glScalef(0.1f, 0.6f, 0.4f);
        // Desenha a asa.
        glutSolidCube(1.0);
        // Restaura a matriz de transformação.
        glPopMatrix();

        // Asa direita.
        // Salva a matriz de transformação.
        glPushMatrix();
        // Posiciona a asa no lado direito do corpo.
        glTranslatef(0.5f, 0.0f, 0.0f);
        // Inclina a asa ligeiramente.
        glRotatef(20, 0, 0, 1);
        // Aplica a animação de bater de asas (em direção oposta).
        glRotatef(-wingAngle, 1, 0, 0); // Anima a outra asa.
        // Escala um cubo para formar a asa.
        glScalef(0.1f, 0.6f, 0.4f);
        // Desenha a asa.
        glutSolidCube(1.0);
        // Restaura a matriz de transformação.
        glPopMatrix();

        // Desabilita as texturas.
        glDisable(GL_TEXTURE_2D);
    }
    // Restaura a matriz de transformação original da função.
    glPopMatrix();
}

//  Função para Desenhar o Skybox 
void drawSkybox()
{
    // Salva a matriz de transformação atual.
    glPushMatrix();
    // Centraliza o skybox na posição do jogador para dar a ilusão de infinito.
    glTranslatef(motherPenguin.x, motherPenguin.y, motherPenguin.z);

    // Salva os atributos de estado do OpenGL, como iluminação e teste de profundidade.
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);     // Habilita texturas.
    glDisable(GL_LIGHTING);      // Desabilita a luz para o céu não ficar escuro.
    glDisable(GL_DEPTH_TEST);    // Garante que o céu seja desenhado atrás de tudo.

    // Define a cor para branco para não alterar a cor da textura do céu.
    glColor3f(1.0, 1.0, 1.0);
    float s = 75.0f; // Define o tamanho do Skybox.

    // Desenha cada uma das 6 faces do cubo do skybox com sua textura.
    // Face Direita (+X)
    glBindTexture(GL_TEXTURE_2D, gameTextures.skyboxTextures[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(s, s, s);
    glTexCoord2f(0, 1); glVertex3f(s, s, -s);
    glEnd();

    // Face Esquerda (-X)
    glBindTexture(GL_TEXTURE_2D, gameTextures.skyboxTextures[1]);
    glBegin(GL_QUADS);
    glTexCoord2f(1, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(-s, s, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, s);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, s);
    glEnd();

    // Face Superior (+Y)
    glBindTexture(GL_TEXTURE_2D, gameTextures.skyboxTextures[2]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex3f(-s, s, -s);
    glTexCoord2f(0, 0); glVertex3f(-s, s, s);
    glTexCoord2f(1, 0); glVertex3f(s, s, s);
    glTexCoord2f(1, 1); glVertex3f(s, s, -s);
    glEnd();

    // Face Inferior (-Y)
    glBindTexture(GL_TEXTURE_2D, gameTextures.skyboxTextures[3]);
    glBegin(GL_QUADS);
    glTexCoord2f(1, 1); glVertex3f(s, -s, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, -s, -s);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, s);
    glEnd();

    // Face Traseira (-Z)
    glBindTexture(GL_TEXTURE_2D, gameTextures.skyboxTextures[4]);
    glBegin(GL_QUADS);
    glTexCoord2f(1, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, s, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, -s);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, -s);
    glEnd();

    // Face Frontal (+Z)
    glBindTexture(GL_TEXTURE_2D, gameTextures.skyboxTextures[5]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(-s, s, s);
    glTexCoord2f(0, 1); glVertex3f(s, s, s);
    glEnd();

    glPopAttrib(); // Restaura os atributos do OpenGL.
    glPopMatrix(); // Restaura a matriz de transformação.
}

//  Função de Desenho da Cena 
void drawScene()
{
    // Desenha o céu primeiro.
    drawSkybox();

    // Habilita e aplica a textura do chão de gelo.
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gameTextures.iceTexture);
    glColor3f(1.0f, 1.0f, 1.0f); // Cor branca para não tingir a textura.

    glPushMatrix(); // Salva a matriz atual.
    float textureRepeat = 10.0f; // Faz a textura se repetir no chão.
    glBegin(GL_QUADS); // Inicia o desenho do chão.
    glNormal3f(0.0, 1.0, 0.0); // Define a normal do plano (apontando para cima).
    // Define os vértices e as coordenadas de textura para o plano do chão.
    glTexCoord2f(0.0, 0.0); glVertex3f(-ICE_SHEET_SIZE, 0.0, -ICE_SHEET_SIZE);
    glTexCoord2f(textureRepeat, 0.0); glVertex3f(ICE_SHEET_SIZE, 0.0, -ICE_SHEET_SIZE);
    glTexCoord2f(textureRepeat, textureRepeat); glVertex3f(ICE_SHEET_SIZE, 0.0, ICE_SHEET_SIZE);
    glTexCoord2f(0.0, textureRepeat); glVertex3f(-ICE_SHEET_SIZE, 0.0, ICE_SHEET_SIZE);
    glEnd();        // Termina o desenho do chão.
    glPopMatrix();  // Restaura a matriz.
    glDisable(GL_TEXTURE_2D); // Desabilita texturas.

    // Desenha os buracos com a textura de buraco.
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gameTextures.holeTexture);
    glDisable(GL_LIGHTING); // Desabilita luz para o buraco parecer mais escuro.
    glColor3f(1.0f, 1.0f, 1.0f);
    // Itera sobre todos os buracos no vetor.
    for (const auto &hole : holes)
    {
        glPushMatrix(); // Salva a matriz.
        glTranslatef(hole.x, 0.01f, hole.z); // Um pouco acima do chão para evitar z-fighting.
        glRotatef(90, 1, 0, 0); // Rotaciona o disco para que fique deitado no plano XZ.
        gluDisk(quadric, 0, hole.radius, 32, 1); // Desenha o buraco como um disco texturizado.
        glPopMatrix(); // Restaura a matriz.
    }
    glEnable(GL_LIGHTING); // Reabilita a iluminação.
    glDisable(GL_TEXTURE_2D); // Desabilita as texturas.

    // Desenha o filhote.
    glPushMatrix(); // Salva a matriz.
    glTranslatef(chick.x, chick.y, chick.z); // Posiciona o filhote na sua localização.
    drawPenguin(true, false); // Chama a função para desenhar um pinguim (filhote, sem peixe).
    glPopMatrix(); // Restaura a matriz.

    // Desenha os peixes.
    for (const auto &fish : fishes)
    {
        // Posiciona uma luz de destaque (spotlight) acima de cada peixe.
        GLfloat light1_position[] = {fish.x, 5.0f, fish.z, 1.0f};
        glEnable(GL_LIGHT1); // Habilita a luz de destaque.
        glLightfv(GL_LIGHT1, GL_POSITION, light1_position); // Define a posição da luz.

        // Desenha o peixe.
        glPushMatrix(); // Salva a matriz.
        glTranslatef(fish.x, fish.y, fish.z); // Posiciona o peixe na sua localização.
        drawFish(); // Chama a função para desenhar o peixe.
        glPopMatrix(); // Restaura a matriz.

        // Desativa a luz para não afetar outros objetos.
        glDisable(GL_LIGHT1);
    }

    // Desenha a mamãe pinguim.
    glPushMatrix(); // Salva a matriz.
    glTranslatef(motherPenguin.x, motherPenguin.y, motherPenguin.z); // Posiciona a pinguim mãe.
    glRotatef(penguinRotationY, 0, 1, 0); // Rotaciona a pinguim de acordo com sua direção.
    drawPenguin(false, motherHasFish); // Chama a função para desenhar um pinguim (mãe, com ou sem peixe).
    glPopMatrix(); // Restaura a matriz.
}

// Desenha a Interface do Usuário (textos na tela).
void drawUI()
{
    // Muda para o modo de projeção 2D para desenhar texto.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); // Salva a matriz de projeção atual.
    glLoadIdentity(); // Reseta a matriz de projeção.
    gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT)); // Define uma projeção ortográfica 2D.
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); // Salva a matriz de modelo-visão atual.
    glLoadIdentity(); // Reseta a matriz de modelo-visão.
    glDisable(GL_LIGHTING); // Desabilita a iluminação para o texto.

    // Obtém as dimensões da janela atual.
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    // Se o jogo acabou, mostra a mensagem de fim de jogo.
    if (playerWon || isGameOver)
    {
        // Define a cor da mensagem com base no resultado.
        if (playerWon)
            glColor3f(0.1f, 0.8f, 0.1f); // Verde para vitória.
        else
            glColor3f(0.8f, 0.1f, 0.1f); // Vermelho para derrota.

        // Calcula a largura da mensagem para centralizá-la.
        int msgWidth = endMessage.length() * 14;
        // Define a posição do texto no centro da tela.
        glRasterPos2i(w / 2 - msgWidth / 2, h / 2);
        // Itera sobre a string da mensagem e desenha cada caractere.
        for (char c : endMessage)
        {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
        }

        // Mostra a mensagem para reiniciar.
        glColor3f(0.0f, 0.0f, 0.0f); // Cor preta.
        std::string restartMsg = "Pressione 'R' para reiniciar";
        // Calcula a largura da mensagem de reinício.
        int restartMsgWidth = restartMsg.length() * 14;
        // Define a posição do texto abaixo da mensagem principal.
        glRasterPos2i(w / 2 - restartMsgWidth / 2, h / 2 - 40);
        // Itera e desenha a mensagem de reinício.
        for (char c : restartMsg)
        {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
        }
    }
    else // Se o jogo está rolando, mostra os contadores de tempo.
    {
        glColor3f(0.0f, 0.0f, 0.0f); // Cor preta para o texto.
        // Cria a string para a vida do filhote.
        std::string lifeStr = "Vida do Filhote: " + formatTime(chickLifeTimer);
        // Define a posição no canto superior esquerdo.
        glRasterPos2i(10, h - 30);
        // Desenha o texto.
        for (char c : lifeStr)
        {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
        }
        // Cria a string para o tempo de vitória.
        std::string sessionStr = "Vitoria em: " + formatTime(sessionTimer);
        // Define a posição no canto superior direito.
        glRasterPos2i(w - 280, h - 30);
        // Desenha o texto.
        for (char c : sessionStr)
        {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
        }
    }

    // Restaura os modos de matriz e iluminação.
    glEnable(GL_LIGHTING); // Reabilita a iluminação.
    glMatrixMode(GL_PROJECTION);
    glPopMatrix(); // Restaura a matriz de projeção anterior.
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix(); // Restaura a matriz de modelo-visão anterior.
}

//  Funções de display para cada câmera 

// Função de display para a câmera de topo.
void display_top()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Limpa os buffers de cor e profundidade.
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT); // Calcula a proporção da janela.
    glMatrixMode(GL_PROJECTION); // Muda para a matriz de projeção.
    glLoadIdentity(); // Reseta a matriz.
    glOrtho(-30 * aspect, 30 * aspect, -30, 30, 1, 200); // Projeção ortográfica (sem perspectiva).
    glMatrixMode(GL_MODELVIEW); // Muda para a matriz de modelo-visão.
    glLoadIdentity(); // Reseta a matriz.
    // Posiciona a câmera diretamente acima do pinguim, olhando para baixo.
    gluLookAt(motherPenguin.x, 50.0, motherPenguin.z, motherPenguin.x, 0, motherPenguin.z, 0, 0, -1);
    drawScene(); // Desenha a cena do jogo.
    drawUI();    // Desenha a interface do usuário.
    glutSwapBuffers(); // Troca os buffers para exibir o que foi desenhado.
}

// Função de display para a câmera de perseguição.
void display_chase()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Limpa os buffers.
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT); // Calcula a proporção.
    glMatrixMode(GL_PROJECTION); // Muda para a matriz de projeção.
    glLoadIdentity(); // Reseta a matriz.
    gluPerspective(60.0, aspect, 1.0, 200.0); // Projeção em perspectiva.
    glMatrixMode(GL_MODELVIEW); // Muda para a matriz de modelo-visão.
    glLoadIdentity(); // Reseta a matriz.
    // Calcula a posição da câmera atrás do pinguim, baseada na sua rotação.
    float camX = motherPenguin.x - 10 * sin(penguinRotationY * M_PI / 180.0);
    float camZ = motherPenguin.z - 10 * cos(penguinRotationY * M_PI / 180.0);
    // Posiciona a câmera atrás e um pouco acima do pinguim, olhando para ele.
    gluLookAt(camX, motherPenguin.y + 5, camZ, motherPenguin.x, motherPenguin.y, motherPenguin.z, 0, 1, 0);
    drawScene(); // Desenha a cena.
    drawUI();    // Desenha a UI.
    glutSwapBuffers(); // Troca os buffers.
}

// Função de display para a câmera lateral.
void display_side()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Limpa os buffers.
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT); // Calcula a proporção.
    glMatrixMode(GL_PROJECTION); // Muda para a matriz de projeção.
    glLoadIdentity(); // Reseta a matriz.
    gluPerspective(60.0, aspect, 1.0, 200.0); // Projeção em perspectiva.
    glMatrixMode(GL_MODELVIEW); // Muda para a matriz de modelo-visão.
    glLoadIdentity(); // Reseta a matriz.
    // Posiciona a câmera ao lado do pinguim (no eixo X positivo), olhando para ele.
    gluLookAt(motherPenguin.x + 20, motherPenguin.y + 5, motherPenguin.z, motherPenguin.x, motherPenguin.y, motherPenguin.z, 0, 1, 0);
    drawScene(); // Desenha a cena.
    drawUI();    // Desenha a UI.
    glutSwapBuffers(); // Troca os buffers.
}

// Função de display para a câmera livre.
void display_free()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Limpa os buffers.
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT); // Calcula a proporção.
    glMatrixMode(GL_PROJECTION); // Muda para a matriz de projeção.
    glLoadIdentity(); // Reseta a matriz.
    gluPerspective(60.0, aspect, 1.0, 200.0); // Projeção em perspectiva.
    glMatrixMode(GL_MODELVIEW); // Muda para a matriz de modelo-visão.
    glLoadIdentity(); // Reseta a matriz.
    // Posiciona a câmera em um ponto fixo diagonal, olhando para o pinguim.
    gluLookAt(25.0, 25.0, 25.0, motherPenguin.x, motherPenguin.y, motherPenguin.z, 0, 1, 0);
    drawScene(); // Desenha a cena.
    drawUI();    // Desenha a UI.
    glutSwapBuffers(); // Troca os buffers.
}

// Função de display para a câmera frontal.
void display_front()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Limpa os buffers.
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT); // Calcula a proporção.
    glMatrixMode(GL_PROJECTION); // Muda para a matriz de projeção.
    glLoadIdentity(); // Reseta a matriz.
    gluPerspective(60.0, aspect, 1.0, 200.0); // Projeção em perspectiva.
    glMatrixMode(GL_MODELVIEW); // Muda para a matriz de modelo-visão.
    glLoadIdentity(); // Reseta a matriz.
    // Posiciona a câmera na frente do pinguim (no eixo Z positivo), olhando para ele.
    gluLookAt(motherPenguin.x, motherPenguin.y + 5, motherPenguin.z + 20, motherPenguin.x, motherPenguin.y, motherPenguin.z, 0, 1, 0);
    drawScene(); // Desenha a cena.
    drawUI();    // Desenha a UI.
    glutSwapBuffers(); // Troca os buffers.
}

// Chamada quando a janela é redimensionada.
void reshape(int w, int h)
{
    // Evita divisão por zero.
    if (h == 0)
        h = 1;
    // Define o viewport para cobrir a nova janela.
    glViewport(0, 0, w, h);
}

// Função de temporizador que atualiza e redesenha a tela.
void timer(int value)
{
    updateGameLogic(); // Atualiza a lógica do jogo.
    // Pede para todas as janelas se redesenharem.
    for (int id : window_ids)
    {
        glutSetWindow(id); // Define a janela atual.
        glutPostRedisplay(); // Solicita um redesenho para a janela atual.
    }
    // Agenda a próxima chamada do timer.
    glutTimerFunc(1000 / FPS, timer, 0);
}

// Lida com as teclas normais do teclado.
void keyboard(unsigned char key, int x, int y)
{
    // Se a tecla ESC (código ASCII 27) for pressionada, sai do programa.
    if (key == 27)
        exit(0); // Fecha o jogo com a tecla ESC.
    // Se o jogo acabou, a tecla 'R' reinicia.
    if ((isGameOver || playerWon) && (key == 'r' || key == 'R'))
    {
        // Chama a função para reiniciar o jogo.
        initializeGame();
    }
}

// Lida com as teclas especiais (setas).
void specialKeyboard(int key, int x, int y)
{
    // Se o jogo acabou, ignora o input de movimento.
    if (isGameOver || playerWon)
        return;      // Não faz nada se o jogo acabou.
    isMoving = true; // Ativa a animação de movimento.
    // Converte o ângulo de rotação do pinguim de graus para radianos.
    float angleRad = penguinRotationY * M_PI / 180.0f;
    // Verifica qual tecla foi pressionada.
    switch (key)
    {
    case GLUT_KEY_UP:
        // Move o pinguim para frente na direção que ele está olhando.
        motherPenguin.x += sin(angleRad) * PENGUIN_MOVE_SPEED;
        motherPenguin.z += cos(angleRad) * PENGUIN_MOVE_SPEED;
        break;
    case GLUT_KEY_DOWN:
        // Move o pinguim para trás.
        motherPenguin.x -= sin(angleRad) * PENGUIN_MOVE_SPEED;
        motherPenguin.z -= cos(angleRad) * PENGUIN_MOVE_SPEED;
        break;
    case GLUT_KEY_LEFT:
        // Rotaciona o pinguim para a esquerda.
        penguinRotationY += PENGUIN_ROTATE_SPEED;
        break;
    case GLUT_KEY_RIGHT:
        // Rotaciona o pinguim para a direita.
        penguinRotationY -= PENGUIN_ROTATE_SPEED;
        break;
    }

    // Impede que o pinguim saia da área de gelo.
    float limit = ICE_SHEET_SIZE - PENGUIN_COLLISION_RADIUS;
    // Verifica e corrige a posição nos limites do eixo X.
    if (motherPenguin.x > limit) motherPenguin.x = limit;
    if (motherPenguin.x < -limit) motherPenguin.x = -limit;
    // Verifica e corrige a posição nos limites do eixo Z.
    if (motherPenguin.z > limit) motherPenguin.z = limit;
    if (motherPenguin.z < -limit) motherPenguin.z = -limit;
}

// Função de inicialização do OpenGL.
void init()
{
    glClearColor(0.0f, 0.2f, 0.5f, 1.0f); // Define a cor de fundo (azul escuro).
    glEnable(GL_DEPTH_TEST);             // Habilita o teste de profundidade para renderização 3D correta.
    glEnable(GL_LIGHTING);               // Habilita o cálculo de iluminação.
    glEnable(GL_COLOR_MATERIAL);         // Permite que a cor do material seja definida por glColor.
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE); // Aplica a cor definida por glColor às propriedades ambiente e difusa.

    // Configura a luz ambiente global.
    GLfloat global_ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

    // Configura a luz principal (sol).
    GLfloat light0_ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat light0_diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat light0_specular[] = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat light0_position[] = {10.0f, 20.0f, 10.0f, 0.0f}; // Luz direcional.
    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
    glEnable(GL_LIGHT0); // Habilita a luz 0.

    // Configura a luz de destaque para os peixes.
    GLfloat light1_ambient[] = {0.1f, 0.1f, 0.0f, 1.0f};
    GLfloat light1_diffuse[] = {1.0f, 1.0f, 0.5f, 1.0f}; // Luz amarelada.
    GLfloat light1_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
    GLfloat spot_direction[] = {0.0f, -1.0f, 0.0f}; // Aponta para baixo.
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, spot_direction);
    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 45.0f); // Ângulo do cone de luz.
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 5.0f); // Concentração da luz no centro.

    //  Carregamento de Texturas Centralizado 
    glEnable(GL_TEXTURE_2D); // Habilita o uso de texturas 2D.
    loadAllTextures(); // Chama a nova função para carregar tudo.

    // Cria o objeto quadric para desenhar esferas e cilindros.
    quadric = gluNewQuadric();
    gluQuadricTexture(quadric, GL_TRUE); // Habilita texturas no quadric.
}

// Função principal do programa.
int main(int argc, char **argv)
{
    glutInit(&argc, argv); // Inicializa a biblioteca GLUT.
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); // Define o modo de display (double buffer, cores RGB, com buffer de profundidade).
    const int spacing = 50; // Espaçamento entre as janelas.

    //  Criação das Janelas 

    // Janela 1: Perseguição (Principal)
    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT); // Define o tamanho da janela.
    glutInitWindowPosition(spacing, spacing); // Define a posição da janela.
    window_chase = glutCreateWindow("3D Penguin Adventure - Camera de Perseguicao"); // Cria a janela e define seu título.
    init();           // Inicializa o OpenGL e as texturas.
    initializeGame(); // Inicializa o estado do jogo.
    glutDisplayFunc(display_chase); // Registra a função de display.
    glutReshapeFunc(reshape);       // Registra a função de redimensionamento.
    glutKeyboardFunc(keyboard);     // Registra a função de teclado.
    glutSpecialFunc(specialKeyboard); // Registra a função de teclas especiais.
    window_ids.push_back(window_chase); // Adiciona o ID da janela ao vetor.

    // Janela 2: Superior (Eixo Y)
    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(INITIAL_WIDTH + 2 * spacing, spacing);
    window_top = glutCreateWindow("3D Penguin Adventure - Camera Superior (Eixo Y)");
    init(); // A inicialização do OpenGL deve ser feita para cada contexto.
    glutDisplayFunc(display_top);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    window_ids.push_back(window_top);

    // Janela 3: Lateral (Eixo X)
    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(spacing, INITIAL_HEIGHT + 2 * spacing);
    window_side = glutCreateWindow("3D Penguin Adventure - Camera Lateral (Eixo X)");
    init();
    glutDisplayFunc(display_side);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    window_ids.push_back(window_side);

    // Janela 4: Livre
    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(INITIAL_WIDTH + 2 * spacing, INITIAL_HEIGHT + 2 * spacing);
    window_free = glutCreateWindow("3D Penguin Adventure - Camera Livre");
    init();
    glutDisplayFunc(display_free);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    window_ids.push_back(window_free);

    // Janela 5: Frontal (Eixo Z)
    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(2 * (INITIAL_WIDTH + spacing) + spacing, spacing);
    window_front = glutCreateWindow("3D Penguin Adventure - Camera Frontal (Eixo Z)");
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
        gluDeleteQuadric(quadric); // Libera a memória alocada para o objeto quadric.
    }
    return 0; // Retorna 0 para indicar que o programa terminou com sucesso.
}
