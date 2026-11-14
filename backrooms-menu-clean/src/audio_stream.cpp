#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <audsrv.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

static const char *kElfPath = "mass:/backrooms-full-menu-v7.elf";
static unsigned char audioStack[8192] __attribute__((aligned(16)));
static int audioThreadId = -1;

struct __attribute__((packed)) AudioFooter {
  char magic[8];
  uint32_t wavSize;
  uint32_t irxSize;
};

static uint16_t rd16(const unsigned char *p) {
  return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t rd32(const unsigned char *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

struct WavInfo {
  int rate;
  int bits;
  int channels;
  long dataOffset;
  uint32_t dataSize;
};

static bool readWavInfo(FILE *f, long wavOffset, WavInfo *w) {
  if (fseek(f, wavOffset, SEEK_SET) != 0) return false;
  unsigned char head[12];
  if (fread(head, 1, 12, f) != 12) return false;
  if (memcmp(head, "RIFF", 4) || memcmp(head + 8, "WAVE", 4)) return false;

  bool gotFmt = false, gotData = false;
  uint16_t format = 0, channels = 0, bits = 0;
  uint32_t rate = 0, dataSize = 0;
  long dataOffset = 0;

  while (!gotData) {
    unsigned char chunk[8];
    if (fread(chunk, 1, 8, f) != 8) return false;
    uint32_t size = rd32(chunk + 4);
    long payload = ftell(f);

    if (!memcmp(chunk, "fmt ", 4)) {
      unsigned char fmt[16];
      if (size < 16 || fread(fmt, 1, 16, f) != 16) return false;
      format = rd16(fmt);
      channels = rd16(fmt + 2);
      rate = rd32(fmt + 4);
      bits = rd16(fmt + 14);
      gotFmt = true;
    } else if (!memcmp(chunk, "data", 4)) {
      dataOffset = payload;
      dataSize = size;
      gotData = true;
    }
    if (!gotData && fseek(f, payload + size + (size & 1), SEEK_SET) != 0)
      return false;
  }

  if (!gotFmt || format != 1 || (channels != 1 && channels != 2) ||
      (bits != 8 && bits != 16) || rate < 8000 || rate > 48000 ||
      dataSize == 0) return false;

  w->rate = (int)rate;
  w->bits = (int)bits;
  w->channels = (int)channels;
  w->dataOffset = dataOffset;
  w->dataSize = dataSize;
  return fseek(f, dataOffset, SEEK_SET) == 0;
}

static bool locateEmbeddedFiles(FILE *f, long *wavOffset, uint32_t *wavSize,
                                long *irxOffset, uint32_t *irxSize) {
  if (fseek(f, 0, SEEK_END) != 0) return false;
  long end = ftell(f);
  if (end < (long)sizeof(AudioFooter)) return false;

  struct AudioFooter footer;
  if (fseek(f, end - (long)sizeof(footer), SEEK_SET) != 0 ||
      fread(&footer, 1, sizeof(footer), f) != sizeof(footer)) return false;
  if (memcmp(footer.magic, "BKAUD01", 7) != 0) return false;

  long payloadStart = end - (long)sizeof(footer) -
                      (long)footer.wavSize - (long)footer.irxSize;
  if (payloadStart <= 0 || footer.wavSize == 0 || footer.irxSize == 0)
    return false;

  *wavOffset = payloadStart;
  *wavSize = footer.wavSize;
  *irxOffset = payloadStart + (long)footer.wavSize;
  *irxSize = footer.irxSize;
  return true;
}

static void audioWorker(void *arg) {
  (void)arg;
  FILE *elf = fopen(kElfPath, "rb");
  if (!elf) return;

  long wavOffset = 0, irxOffset = 0;
  uint32_t wavSize = 0, irxSize = 0;
  if (!locateEmbeddedFiles(elf, &wavOffset, &wavSize, &irxOffset, &irxSize)) {
    fclose(elf);
    return;
  }

  WavInfo info;
  if (!readWavInfo(elf, wavOffset, &info) ||
      info.dataOffset + (long)info.dataSize > wavOffset + (long)wavSize) {
    fclose(elf);
    return;
  }

  void *irx = memalign(16, irxSize);
  if (!irx || fseek(elf, irxOffset, SEEK_SET) != 0 ||
      fread(irx, 1, irxSize, elf) != irxSize) {
    if (irx) free(irx);
    fclose(elf);
    return;
  }

  struct audsrv_fmt_t fmt;
  fmt.freq = info.rate;
  fmt.bits = info.bits;
  fmt.channels = info.channels;

  SifInitRpc(0);
  if (SifLoadModule("rom0:LIBSD", 0, 0) < 0 ||
      SifExecModuleBuffer(irx, irxSize, 0, 0, 0) < 0 ||
      audsrv_init() != AUDSRV_ERR_NOERROR ||
      audsrv_set_format(&fmt) != AUDSRV_ERR_NOERROR) {
    free(irx);
    fclose(elf);
    return;
  }
  free(irx);
  audsrv_set_volume(70);

  unsigned char chunk[2048] __attribute__((aligned(16)));
  const int frameBytes = info.channels * (info.bits / 8);
  uint32_t remaining = info.dataSize;

  for (;;) {
    if (remaining == 0) {
      if (fseek(elf, info.dataOffset, SEEK_SET) != 0) break;
      remaining = info.dataSize;
    }

    int want = remaining > sizeof(chunk) ? sizeof(chunk) : (int)remaining;
    want -= want % frameBytes;
    if (want <= 0) {
      if (fseek(elf, info.dataOffset, SEEK_SET) != 0) break;
      remaining = info.dataSize;
      continue;
    }

    int got = (int)fread(chunk, 1, want, elf);
    if (got <= 0) {
      if (fseek(elf, info.dataOffset, SEEK_SET) != 0) break;
      remaining = info.dataSize;
      continue;
    }

    got -= got % frameBytes;
    if (got == 0) continue;
    audsrv_wait_audio(got);
    audsrv_play_audio((const char *)chunk, got);
    remaining -= (uint32_t)got;
  }

  audsrv_stop_audio();
  audsrv_quit();
  fclose(elf);
}

void startMenuAudio() {
  if (audioThreadId >= 0) return;
  ee_thread_t thread;
  memset(&thread, 0, sizeof(thread));
  thread.func = (void *)audioWorker;
  thread.stack = audioStack;
  thread.stack_size = sizeof(audioStack);
  thread.initial_priority = 40;
  audioThreadId = CreateThread(&thread);
  if (audioThreadId >= 0) StartThread(audioThreadId, 0);
}
