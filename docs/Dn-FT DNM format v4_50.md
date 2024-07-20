# Dn-FamiTracker module file (.dnm) format specification (v4.50)

- Not yet complete!
- Last updated April 20, 2024

## About

- This text aims to document the binary module file format as of Dn-FamiTracker v0.5.0.1.
- Each section is declared sequentially as it appears in the file.
- Unless specified otherwise, all data is stored in **little-endian** format.
- Strings are stored in **ASCII** encoding unless specified otherwise.

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
  - for Sequence item count:
    - Value

### Frames

- for m_iTrackCount:
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
- for m_iChannelsAvailable:
  - Channel type index (unused)
  - for m_iTrackCount:
    - Effect column count
- for m_iTrackCount:
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
  - for Sequence item count:
    - Value

### Sequences N163

- N163 sequence count
  - Sequence index
  - Sequence type
  - Sequence item count
  - Sequence loop point
  - Sequence release point
  - Settings
  - for Sequence item count:
    - Value

### Sequences S5B

- 5B sequence count
  - Sequence index
  - Sequence type
  - Sequence item count
  - Sequence loop point
  - Sequence release point
  - Settings
  - for Sequence item count:
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
- for patches:
  - m_iOPLLPatchBytes[8]
  - m_strOPLLPatchNames

## Data Types

- `char`
	- Signed 8-bit integer. Used for storing bit flags, strings, and other misc. data.
- `unsigned int`
	- Unsigned 32-bit integer. Seems to be only used in `HEADER`?
- `int`
	- Signed 32-bit integer. Used for storing numerical data.
- `CInstrument`
	- CInstrument class object.

## Header

### File Header

<table><thead><tr><th>

Data type

</th><th>

Unit size (bytes)

</th><th>

Object name

</th><th>

Typical values

</th><th>

Description

</th><th>

Notes

</th></tr></thead><tbody><tr><td>

char[256]

</td><td>

Non-padded length of char[] string

</td><td>

`FILE_HEADER_ID` or `FILE_HEADER_ID_DN`

</td><td>

`FamiTracker Module`, `Dn-FamiTracker Module`

</td><td>

Identifier string

</td><td>

In Dn-FT v0.5.0.0, this was changed to `Dn-FamiTracker Module` for non-backwards compatibility of new features.

</td></tr><tr><td>

unsigned int

</td><td>

4

</td><td>

`m_iFileVersion`

</td><td>

0x00000450

</td><td>

Module version

</td><td>

Version number is formatted as BCD (ex. 4.50)

</td></tr></tbody></table>

#### Notes

- Information is based on `DocumentFile.cpp CDocumentFile::BeginDocument()` and `FamiTrackerDoc.cpp CFamiTrackerDoc::SaveDocument()`
- Modules saved in Dn-FamiTracker v0.5.0.0 and later may save modules with `FILE_HEADER_ID_DN` identifiers, to prevent older FamiTracker versions from reading non-backwards compatible data such as `FILE_BLOCK_JSON` and `FILE_BLOCK_PARAMS_EMU`.

## Block header

Each block has a 24-byte header consisting of a block ID, block version, and block size.

<table><thead><tr><th>

Data type

</th><th>

Unit size (bytes)

</th><th>

Object name

</th><th>

Typical values

</th><th>

Description

</th><th>

Notes

</th></tr></thead><tbody><tr><td>

char[16]

</td><td>

16

</td><td>

`m_cBlockID`

</td><td>

`PARAMS`

</td><td>

Identifier string

</td><td>

Length is zero-padded to 16 bytes

</td></tr><tr><td>

int

</td><td>

4

</td><td>

`m_iBlockVersion`

</td><td>

0x00000006

</td><td>

Block version

</td><td>

Block version number is ANDed with 0xFFFF, but is still written as 32-bit signed integer

</td></tr><tr><td>

int

</td><td>

4

</td><td>

`m_iBlockPointer`

</td><td>

0x1D000000

</td><td>

Block size, not counting the block header

</td></tr></tbody></table>

## Internal blocks

### Parameters block

Block ID: `PARAMS`

<table><thead><tr><th>

Data type

</th><th>

Unit size (bytes)

</th><th>

Repeat

</th><th>

Object name

</th><th>

Typical values

</th><th>

Description

</th><th>

Notes

</th><th>

Present in block version

</th></tr></thead><tbody><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_iSongSpeed`

</td><td>

6

</td><td>

Song speed of track 0

</td><td></td><td>

1

</td></tr><tr><td>

char

</td><td>

1

</td><td></td><td>

`m_iExpansionChip`

</td><td>

0x00

</td><td>

Expansion audio bitmask

</td><td>

Same format as expansion audio bitmask found in the NSF format

</td><td>

2+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_iChannelsAvailable`

</td><td>

5

</td><td>

Number of channels added

</td><td>

Includes the 5 channels from 2A03.

</td><td>

1+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_iMachine`

</td><td>

0

</td><td>

NTSC or PAL

</td><td></td><td>

1+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_iPlaybackRateType`

</td><td>

0

</td><td>

Playback rate type, 0 = default, 1 = custom, 2 = video

</td><td></td><td>

7+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_iPlaybackRate`

</td><td>

16639

</td><td>

NSF playback rate, in microseconds

</td><td></td><td>

7+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_iEngineSpeed`

</td><td>

60

</td><td>

Engine refresh rate, in Hz

</td><td></td><td>

1-6

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_iVibratoStyle`

</td><td>

1

</td><td>

0 = old style, 1 = new style

</td><td></td><td>

3+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

?

</td><td>

1

</td><td>

Hardware sweep pitch reset

</td><td>

Not implemented? Vanilla 0.5b stuff. Probably boolean type.

</td><td>

7+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_vHighlight.First`

</td><td>

4

</td><td>

1st row highlight

</td><td></td><td>

3-6

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_vHighlight.Second`

</td><td>

16

</td><td>

2nd row highlight

</td><td></td><td>

3-6

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_iNamcoChannels`

</td><td>

8

</td><td>

Number of N163 channels used

</td><td>

Only written/read when N163 is enabled.

</td><td>

5+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_iSpeedSplitPoint`

</td><td>

32

</td><td>

Fxx speed/tempo split-point

</td><td></td><td>

6+

</td></tr><tr><td>

char

</td><td>

1

</td><td></td><td>

`m_iDetuneSemitone`

</td><td>

0

</td><td>

Semitone detuning, range from -12 to 12

</td><td>

Moved to Tuning block in FT 050B 2020. Also in Extra Parameters block.

</td><td>

8

</td></tr><tr><td>

char

</td><td>

1

</td><td></td><td>

`m_iDetuneCent`

</td><td>

0

</td><td>

Cent detuning, range from -100 to 100

</td><td>

Moved to Tuning block in FT 050B 2020. Also in Extra Parameters block.

</td><td>

8

</td></tr></tbody></table>

#### Notes

- Information is based on `CFamiTrackerDoc::WriteBlock_Parameters()`

### Song Info block

Block ID: `INFO`

<table><thead><tr><th>

Data type

</th><th>

Unit size (bytes)

</th><th>

Repeat

</th><th>

Object name

</th><th>

Typical values

</th><th>

Description

</th><th>

Notes

</th><th>

Present in block version

</th></tr></thead><tbody><tr><td>

char[32]

</td><td>

32

</td><td></td><td>

`m_strName`

</td><td>

`(title)`

</td><td>

Module name

</td><td>

Length is zero-padded to 32 bytes

</td><td>

1+

</td></tr><tr><td>

char[32]

</td><td>

32

</td><td></td><td>

`m_strArtist`

</td><td>

`(author)`

</td><td>

Module artist

</td><td>

Length is zero-padded to 32 bytes

</td><td>

1+

</td></tr><tr><td>

char[32]

</td><td>

32

</td><td></td><td>

`m_strCopyright`

</td><td>

`(copyright)`

</td><td>

Module copyright

</td><td>

Length is zero-padded to 32 bytes

</td><td>

1+

</td></tr></tbody></table>

#### Notes

- Information is based on `CFamiTrackerDoc::WriteBlock_SongInfo()`

### Tuning block

Block ID: `TUNING`

<table><thead><tr><th>

Data type

</th><th>

Unit size (bytes)

</th><th>

Repeat

</th><th>

Object name

</th><th>

Typical values

</th><th>

Description

</th><th>

Notes

</th><th>

Present in block version

</th></tr></thead><tbody><tr><td>

char

</td><td>

1

</td><td></td><td>

`m_iDetuneSemitone`

</td><td>

0

</td><td>

Semitone detuning, range from -12 to 12

</td><td>

Also in Extra Parameters block.

</td><td>

1+

</td></tr><tr><td>

char

</td><td>

1

</td><td></td><td>

`m_iDetuneCent`

</td><td>

0

</td><td>

Cent detuning, range from -100 to 100

</td><td>

Also in Extra Parameters block.

</td><td>

1+

</td></tr></tbody></table>

#### Notes

- Information is based on `CFamiTrackerDoc::WriteBlock_Tuning()`
- Added in FamiTracker 0.5.0 beta 2020

### Header block

Block ID: `HEADER`

<table><thead><tr><th>

Data type

</th><th>

Unit size (bytes)

</th><th>

Repeat

</th><th>

Object name

</th><th>

Typical values

</th><th>

Description

</th><th>

Notes

</th><th>

Present in block version

</th></tr></thead><tbody><tr><td>

char

</td><td>

1

</td><td></td><td>

`m_iTrackCount`

</td><td>

0x06

</td><td>

Number of tracks added.

</td><td>

Stores track count - 1. (i.e. `m_iTrackCount` - 1)

</td><td>

2+

</td></tr><tr><td>

char[]

</td><td>

Length of zero-terminated char[] string

</td><td>

Per number of tracks added (`m_iTrackCount`)

</td><td>

`m_pTracks[]-&gt;m_sTrackName`

</td><td>

`New song`

</td><td>

Names of each track

</td><td>

Each track name is zero terminated, and therefore delineated by a byte of 0.

</td><td>

3+

</td></tr><tr><td>

char

</td><td>

1

</td><td rowspan=2>

Per number of channels added (`m_iChannelsAvailable`)

</td><td>

`m_iChannelTypes[]`

</td><td>

`CHANID_SQUARE1`

</td><td>

Channel ID (`enum chan_id_t`)

</td><td></td><td>

1+

</td></tr><tr><td>

char[]

</td><td>

Number of tracks added (`m_iTrackCount`)

</td><td>

`m_pTracks[]-&gt;m_iEffectColumns[]`

</td><td>

{0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x00}

</td><td>

Number of additional effect columns on a given channel, per track

</td><td>

On version 1, this was restricted to the first track.

</td><td>

1+

</td></tr></tbody></table>

#### Notes

- Information is based on `CFamiTrackerDoc::WriteBlock_Header()`

### Instruments block

Block ID: `INSTRUMENTS`

<table><thead><tr><th>

Data type

</th><th>

Unit size (bytes)

</th><th>

Repeat

</th><th>

Object name

</th><th>

Typical values

</th><th>

Description

</th><th>

Notes

</th><th>

Present in block version

</th></tr></thead><tbody><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_pInstrumentManager-&gt;GetInstrumentCount()`

</td><td>

23

</td><td>

Instrument count

</td><td>

Count of existing instruments

</td><td>

1+

</td></tr><tr><td>

int

</td><td>

4

</td><td rowspan=5>

Per instrument

</td><td>

Instrument index

</td><td>

0 to `MAX_INSTRUMENTS - 1`

</td><td>

Instrument index

</td><td></td><td>

1+

</td></tr><tr><td>

char

</td><td>

1

</td><td>

`m_pInstrumentManager-&gt;GetInstrumentCount()-&gt;m_iType`

</td><td>

0x01

</td><td>

Instrument type

</td><td></td><td>

1+

</td></tr><tr><td>

CInstrument

</td><td>

size of CInstrument object

</td><td>

`m_pInstrumentManager-&gt;GetInstrument()`

</td><td>

(CInstrument object)

</td><td>

Instrument definition

</td><td>

See tables below for instrument definitions

</td><td>

1+

</td></tr><tr><td>

int

</td><td>

4

</td><td>

`strlen()` of `m_pInstrumentManager-&gt;GetInstrument()-&gt;GetName()`

</td><td>

10

</td><td>

Instrument name length

</td><td></td><td>

1+

</td></tr><tr><td>

char[]

</td><td>

Instrument name length

</td><td>

`m_pInstrumentManager-&gt;GetInstrument()-&gt;GetName()`

</td><td>

`bass pluck`

</td><td>

Instrument name

</td><td></td><td>

1+

</td></tr></tbody></table>

#### Notes

- If FDS is used then version must be at least 4 or recent files won't load
- v6 adds DPCM delta settings
- This block is only written if any instruments exist

#### Instrument sequences format

<table><thead><tr><th>

Data type

</th><th>

Unit size (bytes)

</th><th>

Repeat

</th><th>

Object name

</th><th>

Typical values

</th><th>

Description

</th><th>

Notes

</th><th>

Present in block version

</th></tr></thead><tbody><tr><td>

int

</td><td>

4

</td><td></td><td>

`SEQ_COUNT`

</td><td>

`enum sequence_t`

</td><td>

Sequence count

</td><td>

Number of defined sequence types. Volume, Arpeggio, Pitch, Hi-Pitch, Duty cycle

</td><td>

1+

</td></tr><tr><td>

char

</td><td>

1

</td><td rowspan=2>

Per sequence count

</td><td>

`CSeqInstrument::m_iSeqEnable[]`

</td><td>

1

</td><td>

0 = sequence is disabled, 1 = sequence is enabled

</td><td></td><td>

1+

</td></tr><tr><td>

char

</td><td>

1

</td><td>

`CSeqInstrument::m_iSeqIndex[]`

</td><td>

2

</td><td>

Sequence index

</td><td></td><td>

1+

</td></tr></tbody></table>

##### Notes

- This format is shared between 2A03, VRC6, N163, and S5B instruments.
- In 0CC-FamiTracker and its forks, this format was converted into a subclass of `CInstrument`, `CSeqInstrument` to deduplicate code.

#### 2A03

Instrument type value: `0x01`

<table><thead><tr><th>

Data type

</th><th>

Unit size (bytes)

</th><th>

Repeat

</th><th>

Object name

</th><th>

Typical values

</th><th>

Description

</th><th>

Notes

</th><th>

Present in block version

</th></tr></thead><tbody><tr><td>

int

</td><td>

4

</td><td></td><td>

`SEQ_COUNT`

</td><td>

`enum sequence_t`

</td><td>

Sequence count

</td><td>

Volume, Arpeggio, Pitch, Hi-Pitch, Duty / Noise

</td><td>

1+

</td></tr><tr><td>

char

</td><td>

1

</td><td rowspan=2>

Per sequence count

</td><td>

`CSeqInstrument::m_iSeqEnable[]`

</td><td>

1

</td><td>

0 = sequence is disabled, 1 = sequence is enabled

</td><td></td><td>

1+

</td></tr><tr><td>

char

</td><td>

1

</td><td>

`CSeqInstrument::m_iSeqIndex[]`

</td><td>

2

</td><td>

Sequence index

</td><td></td><td>

1+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`CInstrument2A03::GetSampleCount()`

</td><td>

5

</td><td>

DPCM sample count

</td><td></td><td>

7+

</td></tr><tr><td>

char

</td><td>

1

</td><td rowspan=4>

Per note

</td><td>

`Note`

</td><td>

0 to `NOTE_COUNT - 1`

</td><td>

DPCM sample assignment note index

</td><td>

Written only when a sample exists at that note.

</td><td>

7+

</td></tr><tr><td>

char

</td><td>

1

</td><td>

`CInstrument2A03::m_cSamples[Octave][Note]`

</td><td>

0x00

</td><td>

DPCM sample assignment index

</td><td>

Written only when a sample exists at that note in version 7+. 

</td><td>

1+

</td></tr><tr><td>

char

</td><td>

1

</td><td>

`CInstrument2A03::m_cSamplePitch[Octave][Note]`

</td><td>

0x00

</td><td>

DPCM sample pitch of a given note

</td><td>

Written only when a sample exists at that note in version 7+. 

</td><td>

1+

</td></tr><tr><td>

char

</td><td>

1

</td><td>

`CInstrument2A03::m_cSampleDelta[Octave][Note]`

</td><td>

0xFF

</td><td>

DPCM delta offset of a given note

</td><td>

Written only when a sample exists at that note in version 7+. 

</td><td>

6+

</td></tr></tbody></table>

##### Notes

- Information is based on `CInstrument2A03::Store()`
- Only 72 notes are defined in version 1. Version 2+ has all 96 notes defined.
- In version 7, the DPCM format has been changed to only count notes with DPCM assignments.

#### VRC6

Instrument type value: `0x02`

<table><thead><tr><th>

Data type

</th><th>

Unit size (bytes)

</th><th>

Repeat

</th><th>

Object name

</th><th>

Typical values

</th><th>

Description

</th><th>

Notes

</th><th>

Present in block version

</th></tr></thead><tbody><tr><td>

int

</td><td>

4

</td><td></td><td>

`SEQ_COUNT`

</td><td>

5

</td><td>

Sequence count

</td><td>

Volume, Arpeggio, Pitch, Hi-pitch, Pulse width.

</td><td>

2+

</td></tr><tr><td>

char

</td><td>

1

</td><td rowspan=2>

Per sequence count

</td><td>

`CSeqInstrument::m_iSeqEnable[]`

</td><td>

1

</td><td>

0 = sequence is disabled, 1 = sequence is enabled

</td><td></td><td>

2+

</td></tr><tr><td>

char

</td><td>

1

</td><td>

`CSeqInstrument::m_iSeqIndex[]`

</td><td>

2

</td><td>

Sequence index

</td><td></td><td>

2+

</td></tr></tbody></table>

##### Notes

- Information is based on `CSeqInstrument::Store()`
- Similar to 2A03 instruments but with no special considerations for DPCM

#### VRC7

Instrument type value: `0x03`

<table><thead><tr><th>

Data type

</th><th>

Unit size (bytes)

</th><th>

Repeat

</th><th>

Name

</th><th>

Typical values

</th><th>

Description

</th><th>

Notes

</th><th>

Present in block version

</th></tr></thead><tbody><tr><td>

int

</td><td>

4

</td><td></td><td>

`CInstrumentVRC7::m_iPatch`

</td><td>

12

</td><td>

VRC7 patch number

</td><td>

Hardware patch number of the instrument.

</td><td>

2+

</td></tr><tr><td>

char[8]

</td><td>

8

</td><td></td><td>

`CInstrumentVRC7::m_iRegs[]`

</td><td>

{0x01, 0x21, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x0F}

</td><td>

Custom patch settings

</td><td>

Patch settings of hardware patch 0.

</td><td>

2+

</td></tr></tbody></table>

##### Notes

- Information is based on `CInstrumentVRC7::Store()`

#### FDS

Instrument type value: `0x04`

<table><thead><tr><th>

Data type

</th><th>

Unit size (bytes)

</th><th>

Repeat

</th><th>

Object name

</th><th>

Typical values

</th><th>

Description

</th><th>

Notes

</th><th>

Present in block version

</th></tr></thead><tbody><tr><td>

char[64]

</td><td>

64

</td><td></td><td>

`CInstrumentFDS::m_iSamples[]`

</td><td>

&lt;64 byte sequence of a waveform&gt;

</td><td>

Wave data

</td><td></td><td>

3+

</td></tr><tr><td>

char[32]

</td><td>

32

</td><td></td><td>

`CInstrumentFDS::m_iModulation[]`

</td><td>

&lt;32 byte sequence&gt;

</td><td>

Modulation table

</td><td></td><td>

3+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`CInstrumentFDS::m_iModulationSpeed`

</td><td>

0

</td><td>

Instrument modulation rate

</td><td></td><td>

3+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`CInstrumentFDS::m_iModulationDepth`

</td><td>

0

</td><td>

Instrument modulation depth

</td><td></td><td>

3+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`CInstrumentFDS::m_iModulationDelay`

</td><td>

0

</td><td>

Instrument modulation delay

</td><td></td><td>

3+

</td></tr><tr><td>

char

</td><td>

1

</td><td rowspan=5>

Per sequence count

</td><td>

`CSequence::m_iItemCount`

</td><td></td><td>

Sequence length

</td><td></td><td>

3+

</td></tr><tr><td>

int

</td><td>

4

</td><td>

`CSequence::m_iLoopPoint`

</td><td></td><td>

Sequence loop point

</td><td></td><td>

3+

</td></tr><tr><td>

int

</td><td>

4

</td><td>

`CSequence::m_iReleasePoint`

</td><td></td><td>

Sequence release point

</td><td></td><td>

4+

</td></tr><tr><td>

int

</td><td>

4

</td><td>

`CSequence::m_iSetting`

</td><td></td><td>

Sequence setting type

</td><td></td><td>

4+

</td></tr><tr><td>

char[]

</td><td>

Sequence length

</td><td>

`CSequence::m_cValues[Index]`

</td><td></td><td>

Sequence data

</td><td></td><td>

3+

</td></tr></tbody></table>

##### Notes

- Information is based on `CInstrumentFDS::Store()`
- FDS instruments stores its own sequences via `CInstrumentFDS::StoreSequence()`, separate from `CSeqInstrument::Store()`
	- These sequences only store the first 3 types

#### N163

Instrument type value: `0x05`

<table><thead><tr><th>

Data type

</th><th>

Unit size (bytes)

</th><th>

Repeat

</th><th>

Object name

</th><th>

Typical values

</th><th>

Description

</th><th>

Notes

</th><th>

Present in block version

</th></tr></thead><tbody><tr><td>

int

</td><td>

4

</td><td></td><td>

`SEQ_COUNT`

</td><td>

5

</td><td>

Sequence count

</td><td>

Volume, Arpeggio, Pitch, Hi-pitch, Wave Index.

</td><td>

2+

</td></tr><tr><td>

char

</td><td>

1

</td><td>

Per sequence count

</td><td>

`CSeqInstrument::m_iSeqEnable[]`

</td><td>

1

</td><td>

0 = sequence is disabled, 1 = sequence is enabled

</td><td></td><td>

2+

</td></tr><tr><td>

char

</td><td>

1

</td><td></td><td>

`CSeqInstrument::m_iSeqIndex[]`

</td><td>

2

</td><td>

Sequence index

</td><td></td><td>

2+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_iWaveSize`

</td><td>

32

</td><td>

Wave data size

</td><td></td><td>

2+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_iWavePos`

</td><td>

0

</td><td>

Wave data RAM position

</td><td>

Ignored if automatic wave data ram allocation is enabled

</td><td>

2+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_bAutoWavePos`

</td><td>

0x00 / 0x01

</td><td>

Automatic wave data RAM allocation

</td><td></td><td>

?+

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`m_iWaveCount`

</td><td>

1

</td><td>

Wave count

</td><td></td><td>

2+

</td></tr><tr><td>

char[]

</td><td>

Wave data size

</td><td>

Per wave count

</td><td>

`m_iSamples[MAX_WAVE_COUNT][MAX_WAVE_SIZE]`

</td><td>

&lt;byte sequence of wave data&gt;

</td><td>

Wave data

</td><td></td><td>

2+

</td></tr></tbody></table>

##### Notes

- Information is based on `CInstrumentN163::Store()`
- Automatic wave data RAM allocation feature is from FT 0.5.0 beta

#### S5B

Instrument type value: `0x06`

<table><tr><td>

Data type

</td><td>

Unit size (bytes)

</td><td>

Repeat

</td><td>

Object name

</td><td>

Typical values

</td><td>

Description

</td><td>

Notes

</td><td>

Present in block version

</td></tr><tr><td>

int

</td><td>

4

</td><td></td><td>

`SEQ_COUNT`

</td><td>

5

</td><td>

Sequence count

</td><td>

Volume, Arpeggio, Pitch, Hi-pitch, Noise / Mode.

</td><td>

2+

</td></tr><tr><td>

char

</td><td>

1

</td><td rowspan=2>

Per sequence count

</td><td>

`CSeqInstrument::m_iSeqEnable[]`

</td><td>

1

</td><td>

0 = sequence is disabled, 1 = sequence is enabled

</td><td></td><td>

2+

</td></tr><tr><td>

char

</td><td>

1

</td><td>

`CSeqInstrument::m_iSeqIndex[]`

</td><td>

2

</td><td>

Sequence index

</td><td></td><td>

2+

</td></tr></table>

##### Notes

- Information is based on `CSeqInstrument::Store()`
- Similar to 2A03 instruments but with no special considerations for DPCM
### Sequences block

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
