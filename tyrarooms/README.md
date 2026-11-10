# Backrooms PS2

Projeto de terror em primeira pessoa para PlayStation 2, desenvolvido em C/C++ com **PS2SDK + gsKit**, sem depender da engine Tyra.

Esta pasta é independente do jogo Among Us e da antiga pasta `tyra`. Não alterar esses projetos ao trabalhar no Backrooms.

## Direção do jogo

- Exploração 3D em primeira pessoa.
- Level 0: paredes amarelas, carpete úmido e teto com lâmpadas fluorescentes.
- Salas e corredores com formatos variados.
- Zumbido ambiente e efeitos sonoros.
- Monstro com aparições e perseguições pontuais.
- Objetivo de encontrar uma saída; sem armas.

## Base técnica

- **PS2SDK**: compilação ELF, controle, memória, I/O e integração com hardware do PS2.
- **gsKit**: interface C para o Graphics Synthesizer do PS2; será usada para renderização 2D/3D, texturas e apresentação da cena.
- **gsKit oficial**: https://github.com/ps2dev/gsKit
- Dependência fixada em `vendor/gsKit` como submódulo Git, apontando para o commit `8ef73d05f022d14b3ded7c70cba670fe333ed61a`.

## Plano de desenvolvimento

1. Preparar e validar o ambiente PS2SDK + gsKit.
2. Compilar um ELF mínimo que inicializa o gsKit e limpa a tela.
3. Desenhar uma única sala 3D de teste.
4. Adicionar câmera em primeira pessoa e controle.
5. Aplicar texturas de parede, carpete e teto.
6. Implementar áudio ambiente.
7. Expandir o cenário e adicionar o monstro.

## Estrutura planejada

```
tyrarooms/
├── README.md
└── vendor/
    └── gsKit/   # submódulo oficial ps2dev/gsKit
```

## Clonagem

Para baixar também o conteúdo do gsKit ao clonar o repositório:

```bash
git clone --recurse-submodules https://github.com/mrgamer456-ciber/Ps2-among-us.git
```

Se o repositório já estiver clonado:

```bash
git submodule update --init --recursive
```

O submódulo registra o código oficial do gsKit sem copiar nem modificar o projeto original dos mantenedores.
