#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <libpad.h>
#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "background.h"
#ifndef TEST_BACKGROUND
#define TEST_BACKGROUND 1
#endif
#ifndef TEST_FONT
#define TEST_FONT 1
#endif
#ifndef TEST_UI
#define TEST_UI 1
#endif
#ifndef TEST_FILTER
#define TEST_FILTER GS_FILTER_LINEAR
#endif
#ifndef TEST_MODE
#define TEST_MODE GS_MODE_NTSC
#endif
#ifndef TEST_AUDIO
#define TEST_AUDIO 0
#endif
#if TEST_AUDIO
extern void startMenuAudio(const char *elfPath);
extern int getMenuAudioStatus();
#endif

static GSGLOBAL *gs = 0;
static GSTEXTURE bg;
static GSTEXTURE fontAtlas;
static unsigned char padBuf[256] __attribute__((aligned(64)));

static const unsigned char glyphs[37][7] = {
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
 {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},{14,17,19,21,25,17,14},
 {4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
 {2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},
 {31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14},
 {0,4,4,0,4,4,0}
};

static int glyphIndex(char c) {
 if(c>='A' && c<='Z') return c-'A';
 if(c>='0' && c<='9') return 26+c-'0';
 if(c==':') return 36;
 return -1;
}

// Build a small 64x64 CT32 atlas: 8 columns x 5 rows, 8x8 per glyph.
// This keeps the current readable 5x7 font but renders one textured quad per glyph.
static bool buildFontAtlas() {
 memset(&fontAtlas,0,sizeof(fontAtlas));
 fontAtlas.Width=64; fontAtlas.Height=64; fontAtlas.TBW=1;
 fontAtlas.PSM=GS_PSM_CT32; fontAtlas.Filter=GS_FILTER_NEAREST;
 fontAtlas.Mem=(u32*)memalign(128,64*64*sizeof(u32));
 if(!fontAtlas.Mem) return false;
 for(int i=0;i<64*64;i++) fontAtlas.Mem[i]=0x00FFFFFFu;
 for(int g=0;g<37;g++) {
   int ox=(g%8)*8, oy=(g/8)*8;
   for(int y=0;y<7;y++) for(int x=0;x<5;x++) {
     if(glyphs[g][y]&(1<<(4-x)))
       fontAtlas.Mem[(oy+y)*64+ox+x]=0x80FFFFFFu;
   }
 }
 fontAtlas.Vram=gsKit_vram_alloc(gs,gsKit_texture_size(64,64,fontAtlas.PSM),GSKIT_ALLOC_USERBUFFER);
 if(fontAtlas.Vram==GSKIT_ALLOC_ERROR) return false;
 gsKit_texture_upload(gs,&fontAtlas);
 return true;
}

static bool loadBackground() {
 memset(&bg,0,sizeof(bg));
 bg.Width=512; bg.Height=512; bg.TBW=8;
 bg.PSM=GS_PSM_CT32; bg.Filter=TEST_FILTER;
 bg.Mem=(u32*)memalign(128,512*512*sizeof(uint32_t));
 if(!bg.Mem) return false;
 memcpy(bg.Mem,cleanMenuPixels,512*512*sizeof(uint32_t));
 bg.Vram=gsKit_vram_alloc(gs,gsKit_texture_size(bg.Width,bg.Height,bg.PSM),GSKIT_ALLOC_USERBUFFER);
 if(bg.Vram==GSKIT_ALLOC_ERROR) return false;
 gsKit_texture_upload(gs,&bg);
 return true;
}

static void drawText(const char *s,int x,int y,int scale,u64 color) {
 while(*s) {
   char c=*s++;
   if(c==' ') { x+=4*scale; continue; }
   int g=glyphIndex(c);
   if(g>=0) {
     int u=(g%8)*8, v=(g/8)*8;
     gsKit_prim_sprite_texture(gs,&fontAtlas,
       x,y,u,v,
       x+5*scale,y+7*scale,u+5,v+7,
       2,color);
   }
   x+=6*scale;
 }
}
static void centered(const char *s,int y,int scale,u64 color) {
 int n=0; for(const char *p=s;*p;p++) n++;
 drawText(s,(640-n*6*scale+scale)/2,y,scale,color);
}

#if TEST_AUDIO
static const char *audioStatusLabel() {
 switch(getMenuAudioStatus()) {
  case 1: return "AUDIO: STARTING";
  case 2: return "AUDIO: FILE FAIL";
  case 3: return "AUDIO: FOOTER FAIL";
  case 4: return "AUDIO: WAV FAIL";
  case 5: return "AUDIO: IRX FAIL";
  case 6: return "AUDIO: LIBSD FAIL";
  case 7: return "AUDIO: AUDSRV FAIL";
  case 8: return "AUDIO: FORMAT FAIL";
  case 9: return "AUDIO: VOLUME FAIL";
  case 10: return "AUDIO: PLAYING";
  case 11: return "AUDIO: STREAM FAIL";
  case 12: return "AUDIO: THREAD FAIL";
  default: return "AUDIO: WAIT";
 }
}

#endif

int main(int argc,char **argv) {
 gs=gsKit_init_global();
 if(!gs) { for(;;) {} }
 gs->Mode=TEST_MODE; gs->Width=640; gs->Height=448;
 gs->PSM=GS_PSM_CT32; gs->PSMZ=GS_PSMZ_16; gs->ZBuffering=GS_SETTING_OFF;
 gs->PrimAlphaEnable=GS_SETTING_ON;
 dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF,D_CTRL_STS_UNSPEC,D_CTRL_STD_OFF,D_CTRL_RCYC_8,1<<DMA_CHANNEL_GIF);
 dmaKit_chan_init(DMA_CHANNEL_GIF);
 gsKit_init_screen(gs); gsKit_vram_clear(gs); gsKit_set_clamp(gs,GS_CMODE_CLAMP);
 gsKit_mode_switch(gs,GS_PERSISTENT);
 padInit(0);
 if(padPortOpen(0,0,padBuf)) {
   for(int i=0;i<120 && padGetState(0,0)!=PAD_STATE_STABLE;i++){}
   if(padGetState(0,0)==PAD_STATE_STABLE)
     padSetMainMode(0,0,PAD_MMODE_DUALSHOCK,PAD_MMODE_LOCK);
 }
 bool backgroundReady=true;
 bool fontReady=true;
#if TEST_BACKGROUND
 backgroundReady=loadBackground();
#endif
#if TEST_FONT
 fontReady=buildFontAtlas();
#endif
#if TEST_AUDIO
 startMenuAudio((argc > 0 && argv) ? argv[0] : 0);
#endif

 int selected=0,page=0; bool oldUp=false,oldDown=false,oldX=false;
 for(;;) {
   if(padGetState(0,0)==PAD_STATE_STABLE) {
     padButtonStatus p;
     if(padRead(0,0,&p)!=0) {
       unsigned short b=p.btns;
       bool up=!(b&PAD_UP),down=!(b&PAD_DOWN),x=!(b&PAD_CROSS),o=!(b&PAD_CIRCLE);
       if(page==0) {
         if(up&&!oldUp) selected=(selected+2)%3;
         if(down&&!oldDown) selected=(selected+1)%3;
         if(x&&!oldX) page=selected+1;
       } else if(o) page=0;
       oldUp=up; oldDown=down; oldX=x;
     }
   }
   // Explicit texture blending: transparent atlas pixels stay invisible and glyph pixels blend.
   gsKit_set_primalpha(gs,GS_SETREG_ALPHA(0,1,0,1,0),0);
   gsKit_set_test(gs,GS_ATEST_OFF);
   gsKit_set_test(gs,GS_ZTEST_OFF);
   gsKit_clear(gs,GS_SETREG_RGBAQ(5,5,4,0x80,0));
#if TEST_BACKGROUND
   if(backgroundReady) gsKit_prim_sprite_texture(gs,&bg,0,0,0,0,640,448,512,512,1,
     GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0));
#endif
#if TEST_UI
#if TEST_FONT
   if(!fontReady || !backgroundReady) {
     gsKit_prim_sprite(gs,70,70,570,378,2,GS_SETREG_RGBAQ(65,8,8,0x80,0));
     gsKit_prim_sprite(gs,85,90,555,112,2,GS_SETREG_RGBAQ(220,35,25,0x80,0));
   } else {
   gsKit_prim_sprite(gs,145,36,495,414,2,GS_SETREG_RGBAQ(8,7,5,0x65,0));
   if(page==0) {
     centered("BACKROOMS",82,5,GS_SETREG_RGBAQ(250,225,170,0x80,0));
     centered("LEVEL 0",145,2,GS_SETREG_RGBAQ(230,220,190,0x80,0));
     const char *items[3]={"INICIAR","OPCOES","CREDITOS"};
     for(int i=0;i<3;i++) {
       int yy=220+i*62;
       gsKit_prim_sprite(gs,220,yy-12,420,yy+27,2,GS_SETREG_RGBAQ(28,24,16,0x75,0));
       if(i==selected) gsKit_prim_sprite(gs,220,yy-12,225,yy+27,2,GS_SETREG_RGBAQ(220,180,85,0x80,0));
       centered(items[i],yy,3,i==selected?GS_SETREG_RGBAQ(255,235,185,0x80,0):GS_SETREG_RGBAQ(215,210,190,0x80,0));
     }
     centered("D PAD MOVE  X SELECT",405,1,GS_SETREG_RGBAQ(245,230,200,0x80,0));
   } else if(page==1) {
     centered("MENU BASE PRONTO",190,3,GS_SETREG_RGBAQ(255,230,180,0x80,0));
     centered("O VOLTAR",390,2,GS_SETREG_RGBAQ(245,230,200,0x80,0));
   } else if(page==2) {
     centered("OPCOES",170,4,GS_SETREG_RGBAQ(255,230,180,0x80,0));
     centered("AINDA NAO CONFIGURADAS",240,2,GS_SETREG_RGBAQ(230,220,195,0x80,0));
     centered("O VOLTAR",390,2,GS_SETREG_RGBAQ(245,230,200,0x80,0));
   } else {
     centered("CREDITOS",155,4,GS_SETREG_RGBAQ(255,230,180,0x80,0));
     centered("BACKROOMS PS2",225,2,GS_SETREG_RGBAQ(235,225,200,0x80,0));
     centered("FEITO COM PS2SDK E GSKIT",265,1,GS_SETREG_RGBAQ(235,225,200,0x80,0));
     centered("CHATGPT",295,2,GS_SETREG_RGBAQ(235,225,200,0x80,0));
     centered("O VOLTAR",390,2,GS_SETREG_RGBAQ(245,230,200,0x80,0));
   }
   }
#else
   gsKit_prim_sprite(gs,80,80,560,368,2,GS_SETREG_RGBAQ(40,30,10,0x80,0));
#endif
#endif
#if TEST_AUDIO
   // Compact diagnostic in the bottom-left corner; scale 2 keeps the atlas crisp.
   drawText(audioStatusLabel(),8,430,2,GS_SETREG_RGBAQ(255,235,190,0x80,0));
#endif
   // Same persistent-mode queue order as the gsKit alpha.c sample.
   gsKit_sync_flip(gs);
   gsKit_queue_exec(gs);
   gsKit_queue_reset(gs->Per_Queue);
 }
 return 0;
}
