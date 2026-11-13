# Menu assets

Coloque aqui os arquivos do menu principal:

- `main.tga` — imagem de fundo. O workflow converte a imagem para o cabeçalho C++ `tyrarooms/src/menu_background.h` durante a compilação.
- `Main Menu -Credits Track.wav` — trilha sonora do menu. Ela ainda não está ligada ao código de áudio; não basta colocar o WAV nesta pasta para tocar no PS2.

Mantenha os nomes exatamente como acima. O fundo TGA é convertido automaticamente pelo script `tools/generate_menu_background.py` antes de compilar o ELF.
