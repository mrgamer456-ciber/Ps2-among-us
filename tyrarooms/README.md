# TyraRooms — Backrooms para PS2

Projeto novo de Backrooms em C++ usando a engine **Tyra** para PlayStation 2.

Esta pasta é independente do jogo Among Us e do projeto AthenaEnv. O código, os recursos e os workflows do Among Us devem continuar intactos.

## Direção do jogo

A prioridade é criar uma experiência de horror liminal, não um jogo de tiro:

- Exploração em primeira pessoa com cenário 3D.
- Level 0: paredes amarelas, carpete úmido e teto com lâmpadas fluorescentes.
- Corredores e salas com proporções variadas, evitando aparência de mapas de Doom.
- Iluminação e atmosfera inquietantes, com zumbido ambiente.
- Monstro com aparições e perseguições pontuais.
- Objetivo de encontrar uma saída.

## Plano de desenvolvimento

1. Confirmar a compilação mínima da Tyra dentro desta pasta.
2. Criar uma cena 3D de teste com câmera em primeira pessoa.
3. Construir uma sala modular com paredes, chão e teto.
4. Adicionar texturas e iluminação.
5. Implementar áudio ambiente e efeitos sonoros.
6. Expandir para corredores, salas e progressão do Level 0.

## Estrutura planejada

```
tyrarooms/
├── README.md
├── Makefile
├── inc/
├── src/
└── res/
    ├── models/
    ├── textures/
    └── audio/
```

A estrutura acima é o destino planejado; os arquivos de compilação e jogo serão adicionados nas próximas etapas, após validar o template e as instruções oficiais da Tyra.

## Engine

- Tyra: https://github.com/h4570/tyra
- Tutoriais oficiais: https://github.com/h4570/tyra/tree/master/tutorials
