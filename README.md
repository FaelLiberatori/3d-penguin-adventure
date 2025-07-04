# Documentação do Jogo "3D Penguin Adventure"

**Versão:** 1.0

**Linguagem:** C++ com OpenGL (GLUT)

**Autor:** Rafael Liberatori e João Pedro

---

### 1. Visão Geral do Jogo

**3D Penguin Adventure** é um jogo 3D onde o jogador controla uma mamãe pinguim em uma vasta placa de gelo. O objetivo principal é garantir a sobrevivência de seu filhote, que perde vida constantemente com o passar do tempo. Para vencer, o jogador deve manter o filhote vivo por 5 minutos, coletando peixes e evitando perigos no cenário. O jogo é apresentado em cinco janelas simultâneas, cada uma com uma perspectiva de câmera diferente.

**Objetivos:**
1.  Coletar peixes espalhados pelo cenário de gelo.
2.  Levar os peixes até o filhote para adicionar tempo bônus ao seu contador de vida.
3.  Evitar que o contador de vida do filhote chegue a zero.
4.  Desviar dos buracos que surgem periodicamente no gelo.
5.  Sobreviver por 5 minutos para ganhar o jogo.

**Condições de Fim de Jogo:**
*   **Derrota (Fim de Jogo):**
    *   O contador "Vida do Filhote" chega a zero.
    *   A mamãe pinguim cai em um dos buracos no gelo.
*   **Vitória:**
    *   O contador "Vitória em:" (que representa a duração da sessão de 5 minutos) chega a zero E o contador "Vida do Filhote" ainda é maior que zero.

Ao final do jogo (vitória ou derrota), uma mensagem apropriada é exibida, e o jogador pode reiniciar a partida pressionando a tecla `R`.

---

### 2. Estrutura do Código

O código é organizado em várias seções lógicas:

*   **Includes e Constantes Globais:** Inclusão das bibliotecas necessárias (GLUT, cmath, stb_image, etc.) e definição de valores fixos que governam a jogabilidade (velocidades, raios de colisão, duração do jogo).
*   **Estruturas de Dados:**
    *   `struct GameObject`: Estrutura genérica para representar qualquer objeto com posição 3D (x, y, z), como pinguins e peixes.
    *   `struct Hole`: Define as propriedades de um buraco no gelo (posição x, z e raio).
    *   `struct TextureManager`: Agrupa todos os identificadores de textura (GLuint) em um único local para fácil gerenciamento.
*   **Variáveis Globais de Estado:** Variáveis que mantêm o estado atual do jogo, como a posição dos personagens (`motherPenguin`, `chick`), vetores de objetos (`fishes`, `holes`), temporizadores (`chickLifeTimer`, `sessionTimer`) e flags de estado (`isGameOver`, `playerWon`, `motherHasFish`).
*   **Funções de Inicialização:**
    *   `initializeGame()`: Reseta todas as variáveis de estado do jogo, reposiciona os personagens e gera novos peixes e buracos para o início de uma nova partida.
    *   `init()`: Realiza a configuração inicial do ambiente OpenGL, como cor de fundo, iluminação, profundidade e o carregamento de todas as texturas.
*   **Funções de Lógica e Atualização:**
    *   `updateGameLogic()`: O coração do jogo. Controla os temporizadores, o surgimento de novos buracos, a animação de movimento do pinguim e a detecção de todas as colisões.
    *   `timer()`: Função de callback do GLUT, chamada em intervalos regulares (definidos por `FPS`), que invoca `updateGameLogic()` e solicita o redesenho de todas as janelas.
*   **Funções de Desenho (`draw...`)**: Responsáveis por renderizar todos os elementos visuais na cena.
    *   Objetos: `drawPenguin()`, `drawFish()`.
    *   Cenário: `drawScene()`, `drawSkybox()`.
    *   Interface: `drawUI()`.
*   **Funções de Display e Câmera:**
    *   Cada uma das 5 câmeras (`display_chase`, `display_top`, `display_side`, `display_front`, `display_free`) tem sua própria função de display, que configura a matriz de projeção e a visão da câmera antes de chamar `drawScene()` e `drawUI()`.
*   **Funções de Callback do GLUT:**
    *   `reshape()`: Gerencia o redimensionamento da janela.
    *   `keyboard()` e `specialKeyboard()`: Processam a entrada do teclado do usuário para movimento, reinício e saída do jogo.
*   **`main()`:** Ponto de entrada do programa. Inicializa o GLUT, cria as 5 janelas do jogo, registra todas as funções de callback e inicia o loop principal do jogo.

---

### 3. Detalhes das Principais Mecânicas

#### 3.1. Mamãe Pinguim (Jogador)
*   **Movimento:** Controlada pelas setas do teclado. As setas para esquerda e direita giram a pinguim sobre seu eixo Y. As setas para cima e para baixo a movem para frente ou para trás na direção em que está olhando.
*   **Animação:** Quando o jogador pressiona uma tecla de movimento, a flag `isMoving` é ativada, o que aciona uma animação de balanço para as asas e pernas na função `updateGameLogic`.
*   **Coleta de Peixe:** Ao se aproximar de um peixe, a colisão é detectada (baseada em `PENGUIN_COLLISION_RADIUS` e `FISH_COLLISION_RADIUS`). O peixe é removido do cenário, a flag `motherHasFish` se torna verdadeira, e o modelo do peixe é desenhado no bico da pinguim. Ela só pode carregar um peixe por vez.
*   **Alimentar Filhote:** Com um peixe no bico, ao se aproximar do filhote, a colisão é detectada. O peixe é entregue, `motherHasFish` se torna falsa, um bônus de tempo (`TIME_BONUS_FROM_FEEDING`) é adicionado ao `chickLifeTimer`, e um novo peixe é gerado em uma posição aleatória no cenário.

#### 3.2. Filhote
*   Posicionado estaticamente no centro do mapa.
*   Sua "vida" é representada pelo `chickLifeTimer`, que diminui a cada segundo. Se chegar a zero, o jogo termina.
*   Funciona como o objetivo principal para a entrega dos peixes.

#### 3.3. Peixes
*   São gerados em posições aleatórias no início do jogo e toda vez que a mãe alimenta o filhote.
*   Permanecem estáticos no cenário até serem coletados.
*   Uma luz de destaque (spotlight) é posicionada acima de cada peixe para torná-los mais visíveis e destacá-los no ambiente.

#### 3.4. Buracos no Gelo (Obstáculos)
*   São o principal perigo do jogo.
*   Surgem periodicamente no cenário (a cada 10 segundos, até um máximo de 10 buracos).
*   Se a mamãe pinguim se mover para uma posição onde a distância ao centro de um buraco é menor que o raio do buraco, o jogo termina instantaneamente.

#### 3.5. Múltiplas Câmeras
Uma funcionalidade central do projeto é a visualização simultânea da cena a partir de 5 perspectivas diferentes:
1.  **Câmera de Perseguição:** A janela principal. A câmera segue a pinguim por trás, sempre olhando para ela.
2.  **Câmera Superior:** Uma visão ortográfica (sem perspectiva) de cima, centrada no jogador. Ideal para ter uma noção espacial do mapa.
3.  **Câmera Lateral:** Uma visão em perspectiva a partir de uma posição fixa no eixo X em relação à pinguim.
4.  **Câmera Frontal:** Uma visão em perspectiva a partir de uma posição fixa no eixo Z em relação à pinguim.
5.  **Câmera Livre:** Uma visão estática, posicionada em um ponto diagonal elevado, que observa a pinguim se mover pelo cenário.

#### 3.6. Skybox
O cenário é envolvido por um "skybox", um grande cubo com texturas de céu em suas faces internas. Este cubo se move junto com o jogador, criando a ilusão de um horizonte e um céu distantes e infinitos.

---

### 4. Controles

| Tecla                | Ação                           |
| :------------------- | :----------------------------- |
| `Seta para Cima`     | Mover para frente              |
| `Seta para Baixo`    | Mover para trás                |
| `Seta para Esquerda` | Girar para a esquerda          |
| `Seta para Direita`  | Girar para a direita           |
| `R`                  | Reiniciar o jogo (após o fim)  |
| `ESC`                | Fechar todas as janelas e sair |

---

### 5. Estrutura do Projeto

Para que o jogo funcione, os arquivos devem estar organizados da seguinte forma:

```txt
.
├── Makefile
├── main.cpp
├── stb_image.h
└── textures/
    ├── fish.png
    ├── hole.jpg
    ├── penguin_belly.png
    ├── penguin_body.png
    ├── sky.jpg
    └── snow.jpg
```
---

### 6. Como Compilar e Executar

O projeto utiliza um `Makefile` para simplificar a compilação. Certifique-se de ter as dependências do OpenGL/GLUT instaladas no seu sistema.

**Via Makefile (Recomendado):**
Abra um terminal na pasta raiz do projeto e execute o comando:
```sh
make run
```
Este comando irá compilar o código-fonte e, se a compilação for bem-sucedida, iniciará o jogo automaticamente.

**Manualmente (Exemplo Linux/g++):**
Se preferir compilar manualmente, use o seguinte comando:
```sh
g++ main.cpp -o penguin_adventure_3d -lGL -lglut -lm
```
E para executar:
```sh
./penguin_adventure_3d
```
