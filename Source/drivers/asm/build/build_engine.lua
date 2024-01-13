local resolveLabel = function (str)
  if str == "ft_vibrato_table" then return "VIBRATO" end
  if str == "ft_note_table_vrc7_l" then return "CDetuneTable::DETUNE_VRC7" end
  if str == "ft_update_ext" then return "UPDATE_EXT" end
  if str == "ft_channel_enable" then return "CH_ENABLE" end
  
  local chip = str:match("ft_periods_(.*)")
  if chip then
    chip = chip:upper()
    if chip == "SAWTOOTH" then chip = "SAW" end
    return "CDetuneTable::DETUNE_" .. chip
  end
  return str
end

local build = function (chip)
  print("Building NSF driver for " .. chip .. "...")
  os.execute([[ca65 ../driver.s -l out_]]..chip..[[.lst -D USE_]]..chip..[[ -D PACKAGE -D USE_BANKSWITCH -D USE_OLDVIBRATO -D USE_LINEARPITCH -o driver.o]])
  os.execute([[ld65 -o c0_]] .. chip .. [[.bin driver.o -C c0.cfg]])
  os.execute([[ld65 -o c1_]] .. chip .. [[.bin driver.o -C c1.cfg]])
  
  local adr = {}
  local pos = {}
  local reloc_lo, reloc_hi = {}, {}
  local nsfdrvSize = 8
  for str in io.lines("out_" .. chip .. ".lst") do
    local hex, ident = str:match "(......)........*%f[_%w]([_%w]*):.*;; Patch$"
    if hex then
      hex = tonumber(hex, 16) - nsfdrvSize
      ident = resolveLabel(ident)
      pos[ident] = hex
      adr[hex] = ident
    end
    
    local hex, used, mid, ident = str:match "(......).....(..)(.*)%f[_%w]([_%w]*).*;; Reloc$"
    if hex and used:find "[^ ]" then
      assert(used == "A9")
      local lo, hi = mid:find "<", mid:find ">"
      ident = assert(resolveLabel(ident))
      if lo then
        assert(not hi)
        reloc_lo[tonumber(hex, 16) + 1 - nsfdrvSize] = ident
      else
        assert(hi)
        reloc_hi[tonumber(hex, 16) + 1 - nsfdrvSize] = ident
      end
    end
  end
  
  local in1 = io.open("c1_" .. chip .. ".bin", "rb")
  local in2 = io.open("c0_" .. chip .. ".bin", "rb")
  local relStr = {}
  local hedStr = {}
  local nsfdrvStr = {}
  local tRel, tHed, tnsfdrv = {}, {}, {}

  while true do
    local x, y = in1:read(1), in2:read(1)
    if not x then break end
    x, y = x:byte(), y:byte()
    if x == y then
      if #tnsfdrv < 8 then
        tnsfdrv[#tnsfdrv + 1] = ("0x%02X,"):format(x)
      else
        tHed[#tHed + 1] = ("0x%02X,"):format(x)
      end
    else
      tHed[#tHed + 1] = ("0x%02X,"):format(x - 0xC1)
      if not reloc_lo[in1:seek() - 1 - nsfdrvSize] and not reloc_hi[in1:seek() - 1 - nsfdrvSize] then
        tRel[#tRel + 1] = ("0x%04X,"):format(in1:seek() - 2 - 8)
        if #tRel == 12 then
          table.insert(relStr, "\t" .. table.concat(tRel, " ") .. "\n")
          tRel = {}
        end
      end
    end
    if #tHed == 16 then
      table.insert(hedStr, "\t" .. table.concat(tHed, " ") .. "\n")
      tHed = {}
    end
  end

  if #tnsfdrv > 0 then
    table.insert(nsfdrvStr, "\t" .. table.concat(tnsfdrv, " ") .. "\n")
  end

  if #tRel > 0 then
    table.insert(relStr, "\t" .. table.concat(tRel, " ") .. "\n")
  end
  if #tHed > 0 then
    table.insert(hedStr, "\t" .. table.concat(tHed, " ") .. "\n")
  end
  
  local final = io.open("drivers/drv_" .. string.lower(chip) .. ".h", "w")
  final:write("const unsigned char NSFDRV_", chip, "[] = {\t\t// !! !!\n")
  final:write(table.concat(nsfdrvStr))
  final:write("};\n\nconst unsigned char DRIVER_", chip, "[] = {\t\t// // //\n")
  final:write(table.concat(hedStr))
  final:write("};\n\nconst int DRIVER_RELOC_WORD_", chip, "[] = {\n")
  final:write(table.concat(relStr))
  final:write("};\n\nconst int DRIVER_FREQ_TABLE_", chip, "[] = {\n")
  for k, v in pairs(adr) do if v:find "DETUNE" then
    final:write(("\t0x%04X, %s,\n"):format(pos[v], v))
  end end
  final:write("};\n\nconst int DRIVER_RELOC_ADR_", chip, "[] = {\n")
  for k, v in pairs(reloc_lo) do
    for k2, v2 in pairs(reloc_hi) do if v2 == v then
      final:write(("\t0x%04X, 0x%04X,\n"):format(k, k2)); break
    end end
  end
  final:write("};\n\nconst unsigned int VIBRATO_TABLE_LOCATION_",
    chip, string.format(" = 0x%X;", pos.VIBRATO))
  
  if chip == "ALL" then
    final:write(("\n\nconst int FT_UPDATE_EXT_ADR = 0x%X;"):format(pos.UPDATE_EXT))
    final:write(("\n\nconst int FT_CH_ENABLE_ADR = 0x%X;"):format(pos.CH_ENABLE))
  end

  final:write '\n'

  in1:close()
  in2:close()
  final:close()
  os.remove("out_"..chip..".lst")
  os.remove("c0_"..chip..".bin")
  os.remove("c1_"..chip..".bin")
end

local cfgstr = [[MEMORY {
  ZP:  start = $00,   size = $100,   type = rw, file = "";
  RAM: start = $200,  size = $600,   type = rw, file = "";
  PRG: start = $%04X, size = $40000, type = rw, file = %%O;
}

SEGMENTS {
  ZEROPAGE: load = ZP,  type = zp;
  BSS:      load = RAM, type = bss, define = yes;
  CODE:     load = PRG, type = rw;
}]]

local cfg = io.open("c0.cfg", "w")
cfg:write(cfgstr:format(0xC000))
cfg:close()
cfg = io.open("c1.cfg", "w")
cfg:write(cfgstr:format(0xC100))
cfg:close()

os.execute("mkdir drivers")
for _, v in pairs {"2A03", "VRC6", "VRC7", "FDS", "MMC5", "N163", "S5B", "ALL"} do
  build(v)
end

-- os.execute("rmdir /S /Q ..\\..\\famitracker\\Source\\drivers >nul")
-- os.execute("move drivers ..\\..\\famitracker\\Source >nul")
-- os.remove("c1.cfg")
-- os.remove("c0.cfg")

print("All driver headers created in " .. tonumber(os.clock()) .. " seconds.")
-- os.execute("pause")
