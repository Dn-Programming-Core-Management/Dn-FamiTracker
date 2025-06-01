#!/usr/bin/env python3
# NSF engine kernel compiler
# Python port of build_engine.lua
# Copyright 2025 Persune, GPL-3.0
# build_engine.lua Copyright 2017 HertzDevil, GPL-2.0

import re, subprocess, os, time, argparse, sys

start_time = time.time()

parser = argparse.ArgumentParser()
parser.add_argument("-d", "--debug", action="store_true")
args = parser.parse_args()

DEBUG = args.debug

def resolvelabel(label: str) -> str:
    match label:
        case "ft_vibrato_table":
            return "VIBRATO"
        case "ft_note_table_vrc7_l":
            return "CDetuneTable::DETUNE_VRC7"
        case "ft_update_ext":
            return "UPDATE_EXT"
        case "ft_channel_enable":
            return "CH_ENABLE"
        case "ft_channel_type":
            return "CH_TYPE"
    # Detune table locations
    chip = re.search(r"ft_periods_(.*)", label)
    if chip is not None:
        chipname = chip.group(1).upper()
        if chipname == "SAWTOOTH": chipname = "SAW"
        return "CDetuneTable::DETUNE_" + chipname
    return label

def build(chip: str):
    print("Building NSF driver for " + chip + "...")

    # compile assembly to object
    out = subprocess.run(f"ca65 ../driver.s -l out_{chip}.lst -D USE_{chip} -D NAMCO_CHANNELS=8 -D PACKAGE -D RELOCATE_MUSIC -D USE_BANKSWITCH -D USE_OLDVIBRATO -D USE_LINEARPITCH -o driver.o", shell=True, capture_output=True, text=True)
    print(out.stdout, end="")
    print(out.stderr, end="")
    # compile object with shifted memory config to determine pointer locations
    out = subprocess.run(f"ld65 -o c0_{chip}.bin driver.o -C c0.cfg", shell=True, capture_output=True, text=True)
    print(out.stdout, end="")
    print(out.stderr, end="")
    out = subprocess.run(f"ld65 -o c1_{chip}.bin driver.o -C c1.cfg", shell=True, capture_output=True, text=True)
    print(out.stdout, end="")
    print(out.stderr, end="")

    adr = {}
    pos = {}
    reloc_lo = {}
    reloc_hi = {}
    nsfdrv_size = 8
    # parse resulting .lst file
    with open(f"out_{chip}.lst", "rt") as asmlist:
        for line in asmlist:
            # search for tables to patch
            patchsearch = re.search(r"(......).*(?:\s)([\w]*):.*;; Patch$", line)
            if patchsearch is not None:
                hexpos, label = patchsearch.group(1, 2)
                hexpos = int(hexpos, base=16) - nsfdrv_size
                label = resolvelabel(label)
                pos[label] = hexpos
                adr[hexpos] = label
            
            # search for label pointers to relocate
            relocsearch = re.search(r"(......).....(..)(.*[><])([\w]*).*;; Reloc$", line)
            if relocsearch is not None:
                hexpos, used, mid, label = relocsearch.group(1, 2, 3, 4)
                if hexpos is not None and re.search(r"\S", used) is not None:
                    hexpos = int(hexpos, base=16)
                    assert used == "A9", "relocated pointer byte is not used" # assert LDA is used?
                    lo = re.search("<", mid) is not None
                    hi = re.search(">", mid) is not None
                    label = resolvelabel(label)
                    ptr_location = hexpos - nsfdrv_size + 1
                    if lo:
                        assert not hi, "cannot determine label pointer endianness"
                        reloc_lo[ptr_location] = label
                    else:
                        assert hi, "cannot determine label pointer endianness"
                        reloc_hi[ptr_location] = label


    # formatted string buffers to be written
    rel_str = ""
    hed_str = ""
    nsfdrv_str = ""

    # intermediate unformatted string buffers
    t_rel, t_hed, t_nsfdrv = [], [], []

    # parse the compiled binaries to be formatted
    with open(f"c1_{chip}.bin", "rb") as in1:
        with open(f"c0_{chip}.bin", "rb") as in2:
            while True:
                x, y = in1.read(1), in2.read(1)
                if not x: break
                if x == y:
                    # NSFDRV
                    if len(t_nsfdrv) < 8:
                        t_nsfdrv.append("0x%02X" % int.from_bytes(x))
                    # driver kernel
                    else:
                        t_hed.append("0x%02X" % int.from_bytes(x))
                else:
                    # pointer relocation
                    t_hed.append("0x%02X" % (int.from_bytes(x)-0xC1))
                    reloc_query = in1.tell() - 1 - nsfdrv_size
                    if not reloc_lo.get(reloc_query) and not reloc_hi.get(reloc_query):
                        t_rel.append("0x%04X" % (reloc_query - 1))

    def prettyprintarray(tableval: list, mod: int):
        outstring: str = ""
        length = len(tableval)
        for i in range(0, length, mod):
            outstring += "\t"+", ".join(tableval[i:i+mod])
            if i+mod < length: outstring += ","
            outstring += "\n"
        return outstring

    if len(t_nsfdrv) > 0:
        nsfdrv_str += "\t"+", ".join(t_nsfdrv)+"\n"
    if len(t_rel) > 0:
        rel_str += prettyprintarray(t_rel, 12)
    if len(t_hed) > 0:
        hed_str += prettyprintarray(t_hed, 16)

    # write the final driver file
    with open(f"drivers/drv_{chip.lower()}.h", "w", newline="\n") as drv:
        drv.write("const unsigned char NSFDRV_%s[] = {\t\t// !! !!\n" % chip)
        drv.write(nsfdrv_str)
        drv.write("};\n\nconst unsigned char DRIVER_%s[] = {\t\t// // //\n" % chip)
        drv.write(hed_str)
        drv.write("};\n\nconst int DRIVER_RELOC_WORD_%s[] = {\n" % chip)
        drv.write(rel_str)
        drv.write("};\n\nconst int DRIVER_FREQ_TABLE_%s[] = {\n" % chip)
        temp = []
        for k, v in adr.items():
            if v.find("DETUNE") != -1:
                temp.append("0x%04X" % pos[v])
                temp.append("%s" % v)
        drv.write(prettyprintarray(temp, 2))
        drv.write("};\n\nconst int DRIVER_RELOC_ADR_%s[] = {\n" % chip)
        temp.clear()
        for k,v in reloc_lo.items():
            for k2,v2 in reloc_hi.items():
                if v2==v:
                    temp.append("0x%04X" % k)
                    temp.append("0x%04X" % k2)
        drv.write(prettyprintarray(temp, 2))
        drv.write("};\n\nconst unsigned int VIBRATO_TABLE_LOCATION_"+chip+" = 0x%X;"%pos["VIBRATO"])
        if chip == "N163":
            drv.write("\n\nconst int FT_CH_TYPE_ADR = 0x%X;" % pos["CH_TYPE"])
        if chip == "ALL":
            drv.write("\n\nconst int FT_UPDATE_EXT_ADR = 0x%X;" % pos["UPDATE_EXT"])
            drv.write("\n\nconst int FT_CH_ENABLE_ADR = 0x%X;" % pos["CH_ENABLE"])
        drv.write("\n")

    # remove temp files
    if not DEBUG:
        os.remove("out_"+chip+".lst")
        os.remove("c0_"+chip+".bin")
        os.remove("c1_"+chip+".bin")
        os.remove("driver.o")



cfgstr: str= """MEMORY {
  ZP:  start = $00,   size = $100,   type = rw, file = "";
  RAM: start = $200,  size = $600,   type = rw, file = "";
  PRG: start = $%04X, size = $40000, type = rw, file = %%O;
}

SEGMENTS {
  ZEROPAGE: load = ZP,  type = zp;
  BSS:      load = RAM, type = bss;
  CODE:     load = PRG, type = rw;
}"""

with open("c0.cfg", "w", newline='') as cfgfile:
    cfgfile.write(cfgstr % 0xC000)

with open("c1.cfg", "w", newline='') as cfgfile:
    cfgfile.write(cfgstr % 0xC100)

subprocess.run("mkdir drivers", shell=True)

for chip in ["2A03", "VRC6", "VRC7", "FDS", "MMC5", "N163", "S5B", "ALL"]:
    build(chip)

if not DEBUG:
    os.remove("c1.cfg")
    os.remove("c0.cfg")

end_time = time.time()

print(f"All driver headers created in {end_time - start_time} seconds.")
