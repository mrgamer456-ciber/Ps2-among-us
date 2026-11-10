# Backrooms PS2

Projeto de terror em primeira pessoa para PlayStation 2, desenvolvido em C/C++ com **PS2SDK + gsKit**, sem depender da engine Tyra.

Esta pasta é independente do jogo Among Us e da antiga pasta \`tyra\`. Não alterar esses projetos ao trabalhar no Backrooms.

## Primeira implementação: sala 3D real

O primeiro protótipo já desenha uma sala formada por superfícies 3D independentes:

- piso horizontal em geometria 3D;
- paredes laterais e parede do fundo em geometria 3D;
- teto em geometria 3D;
- texturas procedurais em memória: papel de parede amarelo, carpete gasto e teto com painéis fluorescentes;
- projeção em perspectiva e teste de profundidade do GS.

As texturas são geradas pelo próprio código nesta etapa, então não dependemos ainda de arquivos BMP externos. O piso não é uma imagem de fundo: é um quad 3D projetado com perspectiva.

## Compilar pelo GitHub Actions

1. Abra a aba **Actions** do repositório.
2. Escolha **Build Backrooms PS2**.
3. Toque em **Run workflow**.
4. Quando terminar, baixe o artefato **backrooms-ps2-first-room**. Ele contém \`backrooms.elf\`.

O workflow inicializa o submódulo gsKit automaticamente.

## Estrutura

\`\`\`
tyrarooms/
├── CMakeLists.txt
├── README.md
├── src/
│   └── main.cpp
└── vendor/
    └── gsKit/   # submódulo oficial ps2dev/gsKit
\`\`\`

## Próximas etapas

1. Validar a compilação do ELF.
2. Adicionar controle analógico e câmera em primeira pessoa.
3. Melhorar o mapeamento de texturas e a iluminação.
4. Adicionar portas e móveis com geometria própria e colisão.
5. Implementar personagem, animações, áudio e expansão procedural do mapa.

## Dependência

- gsKit oficial: https://github.com/ps2dev/gsKit
- Commit do submódulo: \`8ef73d05f022d14b3ded7c70cba670fe333ed61a\`
