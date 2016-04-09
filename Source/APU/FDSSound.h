#ifndef FDSSOUND_H
#define FDSSOUND_H

void __fastcall FDSSoundReset(void);
uint8_t __fastcall FDSSoundRead(uint16_t address);
void __fastcall FDSSoundWrite(uint16_t address, uint8_t value);
int32_t __fastcall FDSSoundRender(void);
void __fastcall FDSSoundVolume(unsigned int volume);
void FDSSoundInstall3(void);

#endif /* FDSSOUND_H */
