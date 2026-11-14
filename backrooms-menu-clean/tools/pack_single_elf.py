#!/usr/bin/env python3
"""Append menu audio assets to an ELF without loading the WAV into EE RAM.

The runtime locates the payloads using the 16-byte footer:
  magic[8] = b"BKAUD01\\0"
  wav_size = uint32 little-endian
  irx_size = uint32 little-endian
Payload order: WAV bytes, audsrv IRX bytes, footer.
"""
import argparse
import pathlib
import struct

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--elf", required=True)
    ap.add_argument("--wav", required=True)
    ap.add_argument("--irx", required=True)
    ap.add_argument("--output", required=True)
    a = ap.parse_args()

    elf = pathlib.Path(a.elf)
    wav = pathlib.Path(a.wav)
    irx = pathlib.Path(a.irx)
    out = pathlib.Path(a.output)
    for p in (elf, wav, irx):
        if not p.is_file() or p.stat().st_size == 0:
            raise SystemExit(f"Missing or empty input: {p}")

    wav_size = wav.stat().st_size
    irx_size = irx.stat().st_size
    if wav_size > 0xFFFFFFFF or irx_size > 0xFFFFFFFF:
        raise SystemExit("Embedded payload too large")

    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("wb") as dst:
        for p in (elf, wav, irx):
            with p.open("rb") as src:
                while True:
                    block = src.read(1024 * 1024)
                    if not block:
                        break
                    dst.write(block)
        dst.write(struct.pack("<8sII", b"BKAUD01\\0", wav_size, irx_size))

    print(f"Created {out} ({out.stat().st_size} bytes)")
if __name__ == "__main__":
    main()
