# Among Us para PlayStation 2

Projeto experimental de Among Us para PS2, desenvolvido com a engine **Tyra** e compilado usando **PS2SDK**.

> **Estado atual:** a compilação do ELF foi concluída com sucesso no GitHub Actions. O funcionamento no console real ainda precisa ser testado.

## Downloads rápidos

[![Código-fonte](https://img.shields.io/badge/C%C3%B3DIGO-FONTE_ABERTO-0969DA?style=for-the-badge&logo=github)](https://github.com/mrgamer456-ciber/Ps2-among-us)
[![ZIP completo](https://img.shields.io/badge/BAIXAR-amongus--ps2.zip_%7C_9%2C7_MB-0969DA?style=for-the-badge&logo=github)](https://github.com/mrgamer456-ciber/Ps2-among-us/actions/runs/42094757407/artifacts/13447412018)
[![Compilação](https://img.shields.io/badge/ELF-compila%C3%A7%C3%A3o_%23106-0969DA?style=for-the-badge)](https://github.com/mrgamer456-ciber/Ps2-among-us/actions/runs/42094757407)

## Downloads

| Etiqueta | Arquivo | Para que serve |
|---|---|---|
| 🟢 **CÓDIGO-FONTE ABERTO** | Este repositório | Código C++, Makefiles, workflow e recursos do projeto. |
| 🔵 **ELF COMPILADO** | `amongus.elf` | Executável do PS2 gerado pela compilação. |
| 🟣 **PACOTE ZIP COMPLETO** | `amongus-ps2.zip` | ELF acompanhado das pastas de imagens e música. |

### Baixar a compilação

- [Abrir a compilação #106 no GitHub Actions](https://github.com/mrgamer456-ciber/Ps2-among-us/actions/runs/42094757407)
- [Baixar o pacote amongus-ps2.zip (artifact da compilação #106)](https://github.com/mrgamer456-ciber/Ps2-among-us/actions/runs/42094757407/artifacts/13447412018)

O artifact do GitHub Actions é temporário e pode expirar. Se o link deixar de funcionar, abra a página da compilação e confira a seção **Artifacts** ou execute uma nova compilação.

## Conteúdo do pacote

```text
amongus-ps2/
├── amongus.elf
├── images/
│   ├── mapa.png
│   ├── minimapa.png
│   └── player.png
└── music/
    └── amongus.wav
```

Mantenha o ELF e as pastas `images/` e `music/` juntos. O jogo procura os recursos por esses caminhos relativos.

## Música

A música `music/amongus.wav` está configurada no código para tocar em loop (`engine->audio.song.inLoop = true`).

## Controles implementados atualmente

- **Setas do controle:** movimentam o personagem.
- **Quadrado:** mostra ou esconde o minimapa.

## Como a compilação foi feita

1. O GitHub Actions inicia o ambiente de compilação com Tyra/PS2SDK.
2. O Makefile prepara e compila a engine Tyra.
3. O código C++ do projeto é compilado e ligado para gerar `amongus.elf`.
4. Os recursos de `assets/` são copiados para a pasta de saída.
5. O workflow empacota o ELF e os recursos em `amongus-ps2.zip`.

## Créditos da compilação

- **Projeto e repositório:** mantidos por `mrgamer456-ciber`.
- **Assistência na correção do build, configuração do workflow e compilação #106:** ChatGPT (OpenAI), como assistente de IA, sob orientação do mantenedor.
- **Engine:** [Tyra](https://github.com/h4570/tyra).
- **Ferramentas:** PS2SDK e GitHub Actions.

## Aviso

Este é um build experimental. A compilação ter sido concluída não garante, por si só, que todas as funções estejam corretas no hardware. O ELF ainda precisa ser testado em um PlayStation 2 real.

Nenhuma licença de redistribuição foi declarada neste repositório até o momento. A presença do código público não significa que recursos de terceiros estejam automaticamente liberados para reutilização.
