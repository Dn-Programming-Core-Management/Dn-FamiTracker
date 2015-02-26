#ifndef FDSSOUND_H
#define FDSSOUND_H

void __fastcall FDSSoundReset(void);
uint8 __fastcall FDSSoundRead(uint16 address);
void __fastcall FDSSoundWrite(uint16 address, uint8 value);
int32 __fastcall FDSSoundRender(void);
void __fastcall FDSSoundVolume(unsigned int volume);
void FDSSoundInstall3(void);

#endif /* FDSSOUND_H */
