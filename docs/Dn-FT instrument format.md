# Dn-FamiTracker Instrument format

- Document updated 2025-01-06
- Version 2.4

---

## File header

| _Data type_  | _Unit size (bytes)_    | _Repeat_ | _Object/relevant variable in code_                               | _Description_          | _Valid values_                     | _Notes_                |
| ------------ | ---------------------- | -------- | ---------------------------------------------------------------- | ---------------------- | ---------------------------------- | ---------------------- |
| char[4]      | 4                      |          | `INST_HEADER`                                                    | Identifier string      | `FTI`                              | Null-terminated string |
| unsigned int | 4                      |          | `INST_VERSION`                                                   | Module version         | `2.4`                              | Null-terminated string |
| char         | 1                      |          | `m_pInstrumentManager->GetInstrument()->m_iType`                 | Instrument type        | `enum inst_type_t`                 | See table.             |
| int          | 4                      |          | `strlen()` of `m_pInstrumentManager->GetInstrument()->GetName()` | Instrument name length | 0 to `CInstrument::INST_NAME_MAX`. |                        |
| char[]       | Instrument name length |          | `m_pInstrumentManager->GetInstrument()->GetName()`               | Instrument name        |                                    |                        |

## Instrument type value

| *Chip* | *Index* |
| ---- | ----- |
| 2A03 | 0x01  |
| VRC6 | 0x02  |
| VRC7 | 0x03  |
| FDS  | 0x04  |
| N163 | 0x05  |
| S5B  | 0x06  |

## Base instrument format

| _Data type_ | _Unit size (bytes)_ | _Repeat_           | _Object/relevant variable in code_ | _Description_                                     | _Valid range_     | _Notes_                                                                              | _Present in block version_ |
| ----------- | ------------------- | ------------------ | ---------------------------------- | ------------------------------------------------- | ----------------- | ------------------------------------------------------------------------------------ | -------------------------- |
| int         | 4                   |                    | `SEQ_COUNT`                        | Sequence count                                    | `enum sequence_t` | Number of defined sequence types.  <br>Volume, Arpeggio, Pitch, Hi-Pitch, Duty cycle | 1+                         |
| char        | 1                   | Per sequence count | `CSeqInstrument::m_iSeqEnable[]`   | 0 = sequence is disabled, 1 = sequence is enabled | `enum sequence_t` |                                                                                      | 1+                         |
| char        | 1                   | ^                  | `CSeqInstrument::m_iSeqIndex[]`    | Sequence index                                    | `enum sequence_t` |                                                                                      | 1+                         |

### 2A03 instruments

Instrument type value: `0x01`

| _Data type_ | _Unit size (bytes)_ | _Repeat_                     | _Object/relevant variable in code_              | _Description_                                     | _Valid range_         | _Notes_                                                      | _Present in block version_ |
| ----------- | ------------------- | ---------------------------- | ----------------------------------------------- | ------------------------------------------------- | --------------------- | ------------------------------------------------------------ | -------------------------- |
| int         | 4                   |                              | `SEQ_COUNT`                                     | Sequence count                                    | `enum sequence_t`     | Volume, Arpeggio, Pitch, Hi-Pitch, Duty / Noise              | 1+                         |
| char        | 1                   | Per sequence count           | `CSeqInstrument::m_iSeqEnable[]`                | 0 = sequence is disabled, 1 = sequence is enabled | `enum sequence_t`     |                                                              | 1+                         |
| char        | 1                   | ^                            | `CSeqInstrument::m_iSeqIndex[]`                 | Sequence index                                    | `enum sequence_t`     |                                                              | 1+                         |
| int         | 4                   |                              | `CInstrument2A03::GetSampleCount()`             | DPCM sample assignment count                      | 0 to `MAX_DSAMPLES`   |                                                              | 7+                         |
| char        | 1                   | DPCM sample assignment count | `Note`                                          | DPCM sample assignment note index                 | 0 to `NOTE_COUNT - 1` | Written only when a sample exists at that note.              | 7+                         |
| char        | 1                   | ^                            | `CInstrument2A03::m_cSamples[Octave][Note]`     | DPCM sample assignment index                      | 0 to `MAX_DSAMPLES`   | Written only when a sample exists at that note in FT 050b1+. | 1+                         |
| char        | 1                   | ^                            | `CInstrument2A03::m_cSamplePitch[Octave][Note]` | DPCM sample pitch                                 | 0x0 to 0xF            | Written only when a sample exists at that note in FT 050b1+. | 1+                         |
| char        | 1                   | ^                            | `CInstrument2A03::m_cSampleDelta[Octave][Note]` | DPCM delta offset of a given note                 |                       | Written only when a sample exists at that note in FT 050b1+. | 6+                         |
#### Notes

- Information is based on `CInstrument2A03::Store()`
- Only 72 notes are defined in version 1. Version 2+ has all 96 notes defined.
- In FamiTracker 0.5.0 beta, the DPCM format has been changed to only count notes with DPCM assignments.

### VRC6 instruments

- Same format as base instrument sequences format.
- Fifth sequence type is named `Pulse Width`.

#### Notes

- Information is based on `CSeqInstrument::Store()`
- Similar to 2A03 instruments but with no special considerations for DPCM

### VRC7 instruments

| _Data type_ | _Unit size (bytes)_ | _Repeat_ | _Object/relevant variable in code_ | _Description_         | _Valid range_ | _Notes_                                  | _Present in block version_ |
| ----------- | ------------------- | -------- | ---------------------------------- | --------------------- | ------------- | ---------------------------------------- | -------------------------- |
| int         | 4                   |          | `CInstrumentVRC7::m_iPatch`        | VRC7 patch number     |               | Hardware patch number of the instrument. | 2+                         |
| char[8]     | 8                   |          | `CInstrumentVRC7::m_iRegs[]`       | Custom patch settings |               | Patch settings of hardware patch 0.      | 2+                         |

#### Notes

- Information is based on `CInstrumentVRC7::Store()`

### FDS instruments

| _Data type_ | _Unit size (bytes)_ | _Repeat_                         | _Object/relevant variable in code_   | _Description_               | _Valid range_       | _Notes_ | _Present in block version_ |
| ----------- | ------------------- | -------------------------------- | ------------------------------------ | --------------------------- | ------------------- | ------- | -------------------------- |
| char[64]    | 64                  |                                  | `CInstrumentFDS::m_iSamples[]`       | Wave data                   |                     |         | 3+                         |
| char[32]    | 32                  |                                  | `CInstrumentFDS::m_iModulation[]`    | Modulation table            |                     |         | 3+                         |
| int         | 4                   |                                  | `CInstrumentFDS::m_iModulationSpeed` | Instrument modulation rate  |                     |         | 3+                         |
| int         | 4                   |                                  | `CInstrumentFDS::m_iModulationDepth` | Instrument modulation depth |                     |         | 3+                         |
| int         | 4                   |                                  | `CInstrumentFDS::m_iModulationDelay` | Instrument modulation delay |                     |         | 3+                         |
| char        | 1                   | `CInstrumentFDS::SEQUENCE_COUNT` | `CSequence::m_iItemCount`            | Sequence item count         | 0 to 255            |         | 3+                         |
| int         | 4                   | ^                                | `CSequence::m_iLoopPoint`            | Sequence loop point         | -1 to`SeqCount - 1` |         | 3+                         |
| int         | 4                   | ^                                | `CSequence::m_iReleasePoint`         | Sequence release point      | -1 to`SeqCount - 1` |         | 4+                         |
| int         | 4                   | ^                                | `CSequence::m_iSetting`              | Sequence setting type       | `seq_setting_t`     |         | 4+                         |
| char[]      | Sequence item count | ^                                | `CSequence::m_cValues[Index]`        | Sequence data               |                     |         | 3+                         |

#### Notes

- Information is based on `CInstrumentFDS::Store()`
- FDS instruments stores its own sequences via `CInstrumentFDS::StoreSequence()`, separate from `CSeqInstrument::Store()`
	- These sequences only store 3 types, Volume, Arpeggio, and Pitch.
- In version 3, volume range was 0-15. Later versions have volume ranges from 0-31.
- In version 2, FDS sequences were stored incorrectly.

### N163 instruments

| _Data type_ | _Unit size (bytes)_ | _Repeat_           | _Object/relevant variable in code_          | _Description_                                     | _Valid range_            | _Notes_                                                                              | _Present in block version_ |
| ----------- | ------------------- | ------------------ | ------------------------------------------- | ------------------------------------------------- | ------------------------ | ------------------------------------------------------------------------------------ | -------------------------- |
| int         | 4                   |                    | `SEQ_COUNT`                                 | Sequence count                                    | `enum sequence_t`        | Number of defined sequence types.  <br>Volume, Arpeggio, Pitch, Hi-Pitch, Duty cycle | 1+                         |
| char        | 1                   | Per sequence count | `CSeqInstrument::m_iSeqEnable[]`            | 0 = sequence is disabled, 1 = sequence is enabled | `enum sequence_t`        |                                                                                      | 1+                         |
| char        | 1                   | ^                  | `CSeqInstrument::m_iSeqIndex[]`             | Sequence index                                    | `enum sequence_t`        |                                                                                      | 1+                         |
| int         | 4                   |                    | `AutoPosition`                              | Automatic wave data RAM allocation                | 0, nonzero               |                                                                                      | 8+                         |
| int         | 4                   |                    | `m_iWaveSize`                               | N163 wave size                                    | 4 to `MAX_WAVE_SIZE`     | In FT 0.5.0 beta, `m_iWaveSize` is determined by `m_iWaveCount / remaining_bytes`    | 2-6                        |
| int         | 4                   |                    | `m_iWavePos`                                | N163 wave position                                | 0 to `MAX_WAVE_SIZE - 1` | Ignored if automatic wave data ram allocation is enabled                             | 2-6                        |
| int         | 4                   |                    | `m_iWaveCount`                              | N163 wave count                                   | 1 to `MAX_WAVE_COUNT`    |                                                                                      | 2+                         |
| char[]      | Wave data size      | Per wave count     | `m_iSamples[MAX_WAVE_COUNT][MAX_WAVE_SIZE]` | N163 wave sample                                  | 0 to 15                  |                                                                                      | 2+                         |

#### Notes

- Information is based on `CInstrumentN163::Store()` and instrument file binary analysis
- Automatic wave data RAM allocation feature is from FT 0.5.0 beta.

### S5B instruments

- Same format as base instrument sequences format.
- Fifth sequence type is named `Noise / Mode`.

#### Notes

- Information is based on `CSeqInstrument::Store()`
- Similar to 2A03 instruments but with no special considerations for DPCM
