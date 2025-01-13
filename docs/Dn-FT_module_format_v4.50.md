# Dn-FamiTracker module file (.dnm) format

- Not yet complete!
- Document updated 2025-01-06
- version 4.50

---

## About

- This text aims to document the binary module file format as of Dn-FamiTracker v0.5.1.0.
- Main sources comes from FamiTracker, 0CC and Dn tracker source code, as well as an archive of the FamiTracker wiki page on the module format
	- [http://famitracker.com/wiki/index.php?title=FamiTracker_module#SEQUENCES](https://web.archive.org/web/20201124070633/http://famitracker.com/wiki/index.php?title=FamiTracker_module#SEQUENCES)
- FT 0.5.0 beta features come from 0CC-FT file parsing, as well as module binary analysis for 0.5.0 beta 10.
- Each section is declared sequentially as it appears in the file.
- Unless specified otherwise, all data is stored in **little-endian** format.
- Strings are stored in **ASCII** encoding unless specified otherwise.

## File Header

| Data type    | Unit size (bytes) | Repeat | Object/relevant variable in code      | Description       | Valid values                                  | Notes                                                                                                     |
| ------------ | ----------------- | ------ | ------------------------------------- | ----------------- | --------------------------------------------- | --------------------------------------------------------------------------------------------------------- |
| char[256]    | 256               |        | `FILE_HEADER_ID`, `FILE_HEADER_ID_DN` | Identifier string | `FamiTracker Module`, `Dn-FamiTracker Module` | In Dn-FT v0.5.0.0, this was changed to `Dn-FamiTracker Module` to avoid collision with FT 0.5.0b modules. |
| unsigned int | 4                 |        | `m_iFileVersion`                      | Module version    |                                               | Version number is formatted as BCD (ex. 4.50)                                                             |

### Notes

- Information is based on `DocumentFile.cpp CDocumentFile::BeginDocument()` and `FamiTrackerDoc.cpp CFamiTrackerDoc::SaveDocument()`
- Modules saved in Dn-FamiTracker v0.5.0.0 and later may save modules with `FILE_HEADER_ID_DN` identifiers, to prevent older FamiTracker versions from reading non-backwards compatible data such as `FILE_BLOCK_JSON` and `FILE_BLOCK_PARAMS_EMU`.

## Block header

Each block has a 24-byte header consisting of a block ID, block version, and block size.

| Data type    | Unit size (bytes) | Repeat | Object/relevant variable in code | Description                               | Valid range      | Notes                                                                                    |
| ------------ | ----------------- | ------ | -------------------------------- | ----------------------------------------- | ---------------- | ---------------------------------------------------------------------------------------- |
| char[16]     | 16                |        | `m_cBlockID`                     | Identifier string                         | `char` data      | Length is zero-padded to 16 bytes                                                        |
| unsigned int | 4                 |        | `m_iBlockVersion`                | Block version                             | 0x0000 to 0xFFFF | Block version number is ANDed with 0xFFFF, but is still written as 32-bit signed integer |
| unsigned int | 4                 |        | `m_iBlockPointer`                | Block size, not counting the block header |                  |                                                                                          |

## Internal blocks

### Parameters block

Block ID: `PARAMS`

| _Data type_  | _Unit size (bytes)_ | _Repeat_ | _Object/relevant variable in code_ | _Description_                                          | _Valid range_                                      | _Notes_                                                                                        | _Present in block version_ |
| ------------ | ------------------- | -------- | ---------------------------------- | ------------------------------------------------------ | -------------------------------------------------- | ---------------------------------------------------------------------------------------------- | -------------------------- |
| unsigned int | 4                   |          | `m_iSongSpeed`                     | Song speed of track 0                                  |                                                    | Moved to FRAMES block. Off-by-one in module version 2.00 when less than speed-split point (20) | 1                          |
| char         | 1                   |          | `m_iExpansionChip`                 | Expansion audio bitmask                                |                                                    | Same format as expansion audio bitmask found in the NSF format                                 | 2+                         |
| unsigned int | 4                   |          | `m_iChannelsAvailable`             | Number of channels added                               | 1 to `MAX_CHANNELS`                                | Includes the 5 channels from 2A03.                                                             | 1+                         |
| unsigned int | 4                   |          | `m_iMachine`                       | NTSC or PAL                                            | `machine_t::NTSC`, `machine_t::PAL`                |                                                                                                | 1+                         |
| unsigned int | 4                   |          | `m_iPlaybackRateType`              | Playback rate type, 0 = default, 1 = custom, 2 = video | 0 to 2                                             |                                                                                                | 7+                         |
| unsigned int | 4                   |          | `m_iPlaybackRate`                  | NSF playback rate, in microseconds                     | 0x0000 to 0xFFFF                                   |                                                                                                | 7+                         |
| unsigned int | 4                   |          | `m_iEngineSpeed`                   | Engine refresh rate, in Hz                             |                                                    |                                                                                                | 1-6                        |
| unsigned int | 4                   |          | `m_iVibratoStyle`                  | 0 = old style, 1 = new style                           | `vibrato_t::VIBRATO_OLD`, `vibrato_t::VIBRATO_NEW` | Stored as `vibrato_t` in the tracker.                                                          | 3+                         |
| unsigned int | 4                   |          | `SweepReset`                       | Hardware sweep pitch reset                             |                                                    | Not implemented? Vanilla 0.5b stuff. Probably boolean type.                                    | 7+                         |
| int          | 4                   |          | `m_vHighlight.First`               | 1st row highlight                                      |                                                    |                                                                                                | 3-6                        |
| int          | 4                   |          | `m_vHighlight.Second`              | 2nd row highlight                                      |                                                    |                                                                                                | 3-6                        |
| unsigned int | 4                   |          | `m_iNamcoChannels`                 | Number of N163 channels used                           | 1 to 8                                             | Only accessed when N163 is enabled. Initialized to 4 when unused in version 10?                | 5+                         |
| unsigned int | 4                   |          | `m_iSpeedSplitPoint`               | Fxx speed/tempo split-point                            |                                                    | Default is `0x20`. Set to `0x15` in versions 5 and lower.                                      | 6+                         |
| char         | 1                   |          | `m_iDetuneSemitone`                | Semitone detuning, range from -12 to 12                | 0                                                  | Moved to Tuning block in FT 050B10.                                                            | 8-9                        |
| char         | 1                   |          | `m_iDetuneCent`                    | Cent detuning, range from -100 to 100                  | 0                                                  | Moved to Tuning block in FT 050B10.                                                            | 8-9                        |

#### Notes

- Information is based on `CFamiTrackerDoc::WriteBlock_Parameters()`
- Global semitone and cent detuning equivalent in Extra Parameters block.

### Song Info block

Block ID: `INFO`

| _Data type_ | _Unit size (bytes)_                     | _Repeat_ | _Object/relevant variable in code_ | _Description_    | _Valid range_ | _Notes_                                        | _Present in block version_ |
| ----------- | --------------------------------------- | -------- | ---------------------------------- | ---------------- | ------------- | ---------------------------------------------- | -------------------------- |
| char[32]    | 32                                      |          | `m_strName`                        | Module name      |               | Length is zero-padded to 32 bytes              | 1                          |
| char[32]    | 32                                      |          | `m_strArtist`                      | Module artist    |               | Length is zero-padded to 32 bytes              | 1                          |
| char[32]    | 32                                      |          | `m_strCopyright`                   | Module copyright |               | Length is zero-padded to 32 bytes              | 1                          |
| char[]      | Length of zero-terminated char[] string |          | `m_strName`                        | Module name      |               | Length of string must not exceed 32 characters | 2                          |
| char[]      | Length of zero-terminated char[] string |          | `m_strArtist`                      | Module artist    |               | Length of string must not exceed 32 characters | 2                          |
| char[]      | Length of zero-terminated char[] string |          | `m_strCopyright`                   | Module copyright |               | Length of string must not exceed 32 characters | 2                          |

#### Notes

- Information is based on `CFamiTrackerDoc::WriteBlock_SongInfo()` and module binary analysis
- in FamiTracker 0.5.0 beta 10, this was changed to be zero-terminated strings.

### Tuning block

Block ID: `TUNING`

| _Data type_ | _Unit size (bytes)_ | _Repeat_ | _Object/relevant variable in code_ | _Description_          | _Valid range_ | _Notes_ | _Present in block version_ |
| ----------- | ------------------- | -------- | ---------------------------------- | ---------------------- | ------------- | ------- | -------------------------- |
| char        | 1                   |          | `m_iDetuneSemitone`                | Global semitone tuning | -12 to 12     |         | 1                          |
| char        | 1                   |          | `m_iDetuneCent`                    | Global cent tuning     | -100 to 100   |         | 1                          |

#### Notes

- Information is based on `CFamiTrackerDoc::WriteBlock_Tuning()` and module binary analysis
- Added in FamiTracker 0.5.0 beta 10
- Global semitone and cent detuning equivalent in Extra Parameters block.

### Header block

Block ID: `HEADER`

| _Data type_ | _Unit size (bytes)_                     | _Repeat_               | _Object/relevant variable in code_ | _Description_                                                     | _Valid range_                    | _Notes_                                                                      | _Present in block version_ |
| ----------- | --------------------------------------- | ---------------------- | ---------------------------------- | ----------------------------------------------------------------- | -------------------------------- | ---------------------------------------------------------------------------- | -------------------------- |
| char        | 1                                       |                        | (unused)                           | Channel type index                                                | 0 to `chan_id_t::CHANNELS - 1`   | Unused. Version 1 only has one track, so it initializes `m_iTrackCount = 1`  | 1                          |
| char        | 1                                       |                        | `m_iTrackCount`                    | Number of tracks added.                                           | 0 to `MAX_TRACKS - 1`            | Stored as `m_iTrackCount` - 1                                                | 2-4                        |
| char[]      | Length of zero-terminated char[] string | `m_iTrackCount`        | `m_pTracks[]->m_sTrackName`        | Names of each track                                               |                                  | Each track name is zero terminated, and therefore delineated by a byte of 0. | 3-4                        |
| char        | 1                                       | `m_iChannelsAvailable` | `m_iChannelTypes[]`                | Channel type index                                                | 0 to `chan_id_t::CHANNELS - 1`   | See Channel ID table.                                                        | 1-4                        |
| char[]      | `m_iTrackCount`                         | ^                      | `m_pTracks[]->m_iEffectColumns[]`  | Number of additional effect columns on a given channel, per track | 0 to `MAX_EFFECT_COLUMNS - 1`    | On version 1, this was restricted to the first track.                        | 1-4                        |
| int         | 4                                       | `m_iTrackCount`        | `m_vHighlight.First`               | 1st row highlight                                                 | 0 to 255 (cast to unsigned char) | FT 050b1. Stores per-track row highlights.                                   | 4                          |
| int         | 4                                       | ^                      | `m_vHighlight.Second`              | 2nd row highlight                                                 | 0 to 255 (cast to unsigned char) | FT 050B1. Stores per-track row highlights.                                   | 4                          |

#### Channel ID

| Chip | Identifiers                    |
| ---- | ------------------------------ |
| 2A03 | 00, 01, 02, 03, 04             |
| VRC6 | 05, 06, 07                     |
| MMC5 | 08, 09, 0A                     |
| N163 | 0B, 0C, 0D, 0E, 0F, 10, 11, 12 |
| FDS  | 13                             |
| VRC7 | 14, 15, 16, 17, 18, 19         |
| S5B  | 1A, 1B, 1C                     |

#### Notes

- Information is based on `CFamiTrackerDoc::WriteBlock_Header()`

### Track block

Block ID: `TRACK`

| _Data type_  | _Unit size (bytes)_                     | _Repeat_        | _Object/relevant variable in code_ | _Description_                                                 | _Valid range_                 | _Notes_                        | _Present in block version_ |
| ------------ | --------------------------------------- | --------------- | ---------------------------------- | ------------------------------------------------------------- | ----------------------------- | ------------------------------ | -------------------------- |
| char         | 1                                       |                 |                                    | data: track name                                              | 0x00                          | Track name data indicator      | 1                          |
| char[]       | Length of zero-terminated char[] string |                 | `m_pTracks[]->m_sTrackName`        | Track name                                                    |                               | Track name is zero terminated. | 1                          |
| char         | 1                                       |                 |                                    | data: track settings                                          | 0x01                          | Track setting data indicator   | 1                          |
| unsigned int | 4                                       |                 |                                    | Track default row count                                       | 1 to `MAX_PATTERN_LENGTH`     |                                | 1                          |
| unsigned int | 4                                       |                 |                                    | Track default speed                                           | 0 to `MAX_TEMPO`              |                                | 1                          |
| unsigned int | 4                                       |                 |                                    | Track default tempo                                           | 0 to `MAX_TEMPO`              |                                | 1                          |
| char         | 1                                       |                 |                                    | data: track row highlight                                     | 0x02                          | Row highlight data indicator   | 1                          |
| char         | 1                                       |                 | `m_vHighlight.First`               | 1st row highlight                                             |                               | Stores track row highlights.   | 1                          |
| char         | 1                                       |                 | `m_vHighlight.Second`              | 2nd row highlight                                             |                               | Stores track row highlights.   | 1                          |
| char         | 1                                       |                 |                                    | data: track effect columns                                    | 0x03                          | Effect column data indicator   | 1                          |
| char[]       | `m_iTrackCount`                         |                 | `m_pTracks[]->m_iEffectColumns[]`  | Number of additional effect columns on a channel, per channel | 0 to `MAX_EFFECT_COLUMNS - 1` |                                | 1                          |
| char         | 1                                       |                 |                                    | data: track frame data                                        | 0x04                          | Denotes frames                 | 1                          |
| unsigned int | 4                                       |                 | `m_iFrameCount`                    | Track frame count                                             | 1 to `MAX_FRAMES`             |                                | 1                          |
| char[]       | `m_iChannelsAvailable`                  | `m_iFrameCount` |                                    | Pattern indices in a given frame                              | 0 to `MAX_PATTERN - 1`        |                                | 1                          |
| char         | 1                                       |                 |                                    | data: track pattern data                                      | 0x05                          | Pattern data indicator         | 1                          |
| unsigned int | 4                                       |                 |                                    | Pattern size                                                  |                               |                                | 1                          |
| char         | 1                                       | Pattern count   |                                    | Pattern channel index                                         |                               |                                | 1                          |
| char         | 1                                       | ^               |                                    | Pattern index                                                 |                               |                                | 1                          |
| uint16       | 2                                       | ^               |                                    | Pattern data count                                            | 0 to `MAX_PATTERN_LENGTH`     |                                | 1                          |
| char[]       |                                         | ^               |                                    | Row data                                                      |                               | See row data format.           | 1                          |

#### Row data format

| _Data type_ | _Unit size (bytes)_ | _Repeat_           | _Object/relevant variable in code_ | _Description_             | _Valid range_                | _Notes_                                                           | _Present in block version_ |
| ----------- | ------------------- | ------------------ | ---------------------------------- | ------------------------- | ---------------------------- | ----------------------------------------------------------------- | -------------------------- |
| char        | 1                   | Pattern data count |                                    | Row index                 |                              |                                                                   | 1                          |
| char        | 1                   | ^                  |                                    | Pending effect bitflag    |                              | `4321 xIVN` = Effect on col 1/2/3/4, Instrument, Volume, and Note | 1                          |
| char        | 1                   | ^                  |                                    | Note index                | 0 to `NOTE_COUNT`            | Only present when corresponding bitflag is set                    | 1                          |
| char        | 1                   | ^                  |                                    | Instrument index          | 0 to `MAX_INSTRUMENTS`       | Only present when corresponding bitflag is set                    | 1                          |
| char        | 1                   | ^                  |                                    | Volume index              | 0 to `MAX_VOLUME`            | Only present when corresponding bitflag is set                    | 1                          |
| char        | 1                   | ^                  |                                    | Column 1 effect index     | `EF_SPEED` to `EF_COUNT - 1` | Only present when corresponding bitflag is set                    | 1                          |
| char        | 1                   | ^                  |                                    | Column 1 effect parameter |                              | Only present when corresponding bitflag is set                    | 1                          |
| char        | 1                   | ^                  |                                    | Column 2 effect index     | `EF_SPEED` to `EF_COUNT - 1` | Only present when corresponding bitflag is set                    | 1                          |
| char        | 1                   | ^                  |                                    | Column 2 effect parameter |                              | Only present when corresponding bitflag is set                    | 1                          |
| char        | 1                   | ^                  |                                    | Column 3 effect index     | `EF_SPEED` to `EF_COUNT - 1` | Only present when corresponding bitflag is set                    | 1                          |
| char        | 1                   | ^                  |                                    | Column 3 effect parameter |                              | Only present when corresponding bitflag is set                    | 1                          |
| char        | 1                   | ^                  |                                    | Column 4 effect index     | `EF_SPEED` to `EF_COUNT - 1` | Only present when corresponding bitflag is set                    | 1                          |
| char        | 1                   | ^                  |                                    | Column 4 effect parameter |                              | Only present when corresponding bitflag is set                    | 1                          |

#### Notes

- Information is based on module binary analysis
- Added in FamiTracker 0.5.0 beta 10, this replaces the `HEADER`, `FRAMES`, and `PATTERNS` block.
- More research is needed.

### Instruments block

Block ID: `INSTRUMENTS`

| _Data type_ | _Unit size (bytes)_        | _Repeat_       | _Object/relevant variable in code_                               | _Description_          | _Valid range_              | _Notes_                                                        | _Present in block version_ |
| ----------- | -------------------------- | -------------- | ---------------------------------------------------------------- | ---------------------- | -------------------------- | -------------------------------------------------------------- | -------------------------- |
| int         | 4                          |                | `m_pInstrumentManager->GetInstrumentCount()`                     | Instrument count       | 0 to `MAX_INSTRUMENTS`     | Count of existing instruments                                  | 1+                         |
| int         | 4                          | Per instrument | Instrument index                                                 | Instrument index       | 0 to `MAX_INSTRUMENTS - 1` |                                                                | 1+                         |
| char        | 1                          | ^              | `m_pInstrumentManager->GetInstrument()->m_iType`                 | Instrument type        | `enum inst_type_t`         | See table.                                                     | 1+                         |
| CInstrument | size of CInstrument object | ^              | `m_pInstrumentManager->GetInstrument()`                          | Instrument definition  |                            | See [Dn-FT_instrument_format_v2.4](Dn-FT_instrument_format_v2.4.md). | 1+                         |
| int         | 4                          | ^              | `strlen()` of `m_pInstrumentManager->GetInstrument()->GetName()` | Instrument name length | 0 to `INST_NAME_MAX`       |                                                                | 1+                         |
| char[]      | Instrument name length     | ^              | `m_pInstrumentManager->GetInstrument()->GetName()`               | Instrument name        |                            |                                                                | 1+                         |

#### Instrument types

| *Chip* | *Index* |
| ---- | ----- |
| 2A03 | 0x01  |
| VRC6 | 0x02  |
| VRC7 | 0x03  |
| FDS  | 0x04  |
| N163 | 0x05  |
| S5B  | 0x06  |

#### Notes

- See [Dn-FT_instrument_format_v2.4](Dn-FT_instrument_format_v2.4.md) for more details.
- If FDS is used then version must be at least 4 or recent files won't load
- v6 adds DPCM delta settings
- This block is only written if any instruments exist

#### Instrument format

See [Dn-FT_instrument_format_v2.4](Dn-FT_instrument_format_v2.4.md).

##### Notes

- This format is shared between 2A03, VRC6, N163, and S5B instruments.
- In 0CC-FamiTracker and its forks, this format was converted into a subclass of `CInstrument`, `CSeqInstrument` to deduplicate code.

### Sequences block

| _Data type_ | _Unit size (bytes)_             | _Repeat_                | _Object/relevant variable in code_ | _Description_                | _Valid range_                  | _Notes_                                                                                | _Present in block version_ |
| ----------- | ------------------------------- | ----------------------- | ---------------------------------- | ---------------------------- | ------------------------------ | -------------------------------------------------------------------------------------- | -------------------------- |
| int         | 4                               |                         | `Count`                            | 2A03 sequence count          | 0 to `MAX_SEQUENCES - 1`       | Only used sequences are counted                                                        | 1+                         |
| int         | 4                               | Per 2A03 sequence count | `Index`                            | Sequence index               | 0 to `MAX_SEQUENCES - 1`       |                                                                                        | 1+                         |
| int         | 4                               | ^                       | `Type`                             | Sequence type                | 0 to `SEQ_COUNT - 1`           | See table.                                                                             | 2+                         |
| char        | 1                               | ^                       | `SeqCount`                         | Sequence run count           | 0 to `MAX_SEQUENCES_ITEMS - 1` | `SeqCount` is different in version 1-2                                                 | 1-2                        |
| char[]      | `SeqCount * 2` (number of runs) | ^                       | `Value`, `Length`                  | Sequence value, run length-1 |                                | RLE encoded sequence data?                                                             | 1-2                        |
| char        | 1                               | ^                       | `SeqCount`                         | Sequence item count          | 0 to `MAX_SEQUENCES_ITEMS - 1` | Size of sequence data                                                                  | 3+                         |
| int         | 4                               | ^                       | `LoopPoint`                        | Sequence loop point          | -1 to `SeqCount`               | -1 if it doesn't exist                                                                 | 3+                         |
| int         | 4                               | ^                       | `ReleasePoint`                     | Sequence release point       | -1 to `SeqCount`               | -1 if it doesn't exist                                                                 | 4, 7                       |
| int         | 4                               | ^                       | `Settings`                         | Sequence setting             | `seq_setting_t`                | See table.                                                                             | 4, 7                       |
| char[]      | `SeqCount` (length of sequence) | ^                       | `Value`                            | Sequence value               |                                |                                                                                        | 3+                         |
| int         | 4                               | Per 2A03 sequence       | `ReleasePoint`                     | Sequence release point       | -1 to `SeqCount - 1`           | Version 5 saved the release points incorrectly, fixed in ver 6. -1 if it doesn't exist | 5-6                        |
| int         | 4                               | ^                       | `Settings`                         | Sequence setting             | `seq_setting_t`                |                                                                                        | 5-6                        |

#### Sequence type

| Value | 0      | 1        | 2     | 3        | 4                                |
| ----- | ------ | -------- | ----- | -------- | -------------------------------- |
| Type  | Volume | Arpeggio | Pitch | Hi-pitch | Duty / Noise (resp. Pulse width) |

#### Sequence arpeggio type (only in version 4)

| Value | 0        | 1     | 2        | 0?                                   |
| ----- | -------- | ----- | -------- | ------------------------------------ |
| Type  | Absolute | Fixed | Relative | Sequence is not an arpeggio sequence |

Source: [http://famitracker.com/wiki/index.php?title=FamiTracker_module#SEQUENCES](https://web.archive.org/web/20201124070633/http://famitracker.com/wiki/index.php?title=FamiTracker_module#SEQUENCES)

#### Notes

- Information is based on `CFamiTrackerDoc::WriteBlock_Header()`, `CFamiTrackerDoc::ReadBlock_Sequences()`
- in FamiTracker 0.5.0 beta 10, this is only saved when any sequences exist in the module.
- in FamiTracker 0.5.0 beta 10, the sequence release point and setting has shifted back to version 4's place.

### Frames block

### Patterns block

### DPCM Samples block

### Comments block

## Expansion blocks

### VRC6 Sequences block

### N163 Sequences block

### S5B Sequences block

## 0CC-FamiTracker extension blocks

### Extra Parameters block

### Detune Tables block

### Grooves block

### Bookmarks block

## Dn-FamiTracker extension blocks

### JSON block

### Emulation Parameters block

## Format overview

### Parameters

- m_iExpansionChip
- m_iChannelsAvailable
- m_iMachine
- m_iEngineSpeed
- m_iVibratoStyle
- m_vHighlight
- m_iNamcoChannels
- m_iSpeedSplitPoint
- m_iDetuneSemitone
- m_iDetuneCent

### Song Info

- m_strName
- m_strArtist
- m_strCopyright

### Tuning

- m_iDetuneSemitone
- m_iDetuneCent

### Instruments

- Instrument index
- Instrument count
- pInstrument
  - Type
  - CInstrumentManager
  - Instrument name length
  - Name

### Sequences

- 2A03 sequence count
  - Sequence index
  - Sequence type
  - Sequence item count
  - Sequence loop point
  - Sequence release point
  - Settings
  - for each Sequence item count:
    - Value

### Frames

- for each m_iTrackCount:
  - Track frame count
    - m_iChannelsAvailable
      - Pattern index
  - Track default speed
  - Track default tempo
  - Track default row count (PatternLength)

### Patterns

- Pattern track index
- Pattern channel index
- Pattern index
- Pattern data count
  - Row index
    - stChanNote Note
      - Note value
      - Octave value
      - Instrument index
      - Channel volume
    - effect columns
      - Effect index
      - Effect Param

### DSamples

- DPCM sample count
  - DPCM sample index
  - DPCM sample name length
  - DPCM sample name
  - DPCM sample size
  - DPCM sample bytes

### Header

- m_iTrackCount
  - m_pTracks[i]
- for each m_iChannelsAvailable:
  - Channel type index (unused)
  - for each m_iTrackCount:
    - Effect column count
- for each m_iTrackCount:
  - m_vHighlight.First
  - m_vHighlight.Second

### Comments

- m_bDisplayComment
- m_strComment

### Sequences VRC6

- VRC6 sequence count
  - Sequence index
  - Sequence type
  - Sequence item count
  - Sequence loop point
  - Sequence release point
  - Settings
  - for each Sequence item count:
    - Value

### Sequences N163

- N163 sequence count
  - Sequence index
  - Sequence type
  - Sequence item count
  - Sequence loop point
  - Sequence release point
  - Settings
  - for each Sequence item count:
    - Value

### Sequences S5B

- 5B sequence count
  - Sequence index
  - Sequence type
  - Sequence item count
  - Sequence loop point
  - Sequence release point
  - Settings
  - for each Sequence item count:
    - Value

### Detune Tables

- Detune table count
  - Detune table index
    - Detune table note count
      - Detune table note index

### Grooves

- Groove count
  - Groove index
  - Groove size
    - Groove item
- Use-groove flag count
  - Use-groove flag

### Bookmarks

- Bookmark Count
  - CBookmark, which includes
    - Bookmark track index
    - Bookmark frame index
    - Bookmark row index
    - m_vHighlight.First
    - m_vHighlight.Second
    - m_bPersist
    - m_sName

### Params Extra

- m_bLinearPitch
- Global semitone tuning
- Global cent tuning

### JSON

- (optional JSON data)
  - APU1_OFFSET
  - APU2_OFFSET
  - VRC6_OFFSET
  - VRC7_OFFSET
  - FDS_OFFSET
  - MMC5_OFFSET
  - N163_OFFSET
  - S5B_OFFSET
  - USE_SURVEY_MIX

### ParamsEmu

- m_bUseExternalOPLLChips
- for each patches:
  - m_iOPLLPatchBytes[8]
  - m_strOPLLPatchNames