# Texturas do mapa Backrooms (PS2SDK + gsKit)

Coloque aqui as três texturas do cenário:

- `floor.tga` — textura do chão.
- `wall.tga` — textura das paredes.
- `ceiling.tga` — textura do teto.

Use imagens quadradas (recomendado: 128x128 ou 256x256) e mantenha os nomes exatamente como acima. O carregador do jogo será preparado para essas três texturas separadas.

A pasta pertence ao projeto independente `meu-jogo-gskit`; não usa Tyra.

## Geometria planejada

O mapa será uma área Backrooms bem grande, gerada proceduralmente com:
- um salão principal amplo;
- divisórias e corredores retangulares em posições aleatórias;
- paredes, piso e teto feitos como superfícies planas retangulares;
- teto contínuo e iluminação simples.

Não serão usados modelos com aparência facetada/triangular. As superfícies serão planas e retangulares, com as texturas repetidas para cobrir áreas grandes.

Quando adicionar as imagens, não altere os nomes dos arquivos.
