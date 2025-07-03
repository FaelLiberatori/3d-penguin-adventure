# 3D Penguin Adventure

Este documento fornece uma visão geral do projeto **3D Penguin Adventure**, um jogo 3D simples desenvolvido com C++ e OpenGL.

## 1\. Descrição

**3D Penguin Adventure** é um jogo onde o jogador controla uma mamãe pinguim em uma grande placa de gelo. O objetivo é garantir a sobrevivência do seu filhote, que está com fome e perde vida com o tempo.

## 2\. Objetivo do Jogo

Para vencer, o jogador deve sobreviver por 5 minutos (`MAX_SESSION_DURATION`). Para evitar que o filhote morra de fome, a mamãe pinguim deve coletar peixes espalhados pelo cenário e levá-los até ele. Cada peixe entregue aumenta o tempo de vida do filhote.

O jogo termina se:

- O tempo de vida do filhote chegar a zero.
- O jogador cair em um dos buracos que surgem no gelo.
- O jogador sobreviver por 5 minutos (vitória).

## 3\. Funcionalidades

- **Gráficos 3D:** Renderização da cena utilizando OpenGL e a biblioteca GLUT.
- **Múltiplas Câmeras:** 5 janelas simultâneas com diferentes perspectivas da cena:
  1. **Câmera de Perseguição:** Segue a mamãe pinguim.
  2. **Câmera Superior:** Visão de cima, posicionada no eixo Y.
  3. **Câmera Lateral:** Visão de lado, posicionada no eixo X.
  4. **Câmera Frontal:** Visão de frente, posicionada no eixo Z.
  5. **Câmera Livre:** Visão estática de um ponto fixo.
- **Texturização:** Uso de texturas para o gelo, pinguins, peixes, buracos e um skybox para o céu.
- **Controles do Jogador:** Movimentação e rotação da pinguim pelo cenário.
- **Interface Simples (UI):** Exibição do tempo de vida do filhote e o tempo restante para a vitória.

## 4\. Controles

| Tecla                | Ação                           |
| :------------------- | :----------------------------- |
| `Seta para Cima`     | Mover para frente              |
| `Seta para Baixo`    | Mover para trás                |
| `Seta para Esquerda` | Girar para a esquerda          |
| `Seta para Direita`  | Girar para a direita           |
| `R`                  | Reiniciar o jogo (após o fim)  |
| `ESC`                | Fechar todas as janelas e sair |

## 5\. Estrutura do Projeto

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

## 6\. Como Compilar e Executar

O projeto utiliza um `Makefile` para simplificar a compilação e execução. Certifique-se de ter as dependências do OpenGL/GLUT instaladas no seu sistema.

Com tudo configurado, abra um terminal na pasta raiz do projeto e execute o comando:

```sh
make run
```

Este comando irá compilar o código-fonte e, se a compilação for bem-sucedida, iniciará o jogo automaticamente.
