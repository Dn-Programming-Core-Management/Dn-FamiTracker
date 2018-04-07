0CC-FamiTracker Mod
Readme / Manual
Written by HertzDevil
Version 0.3.14.5 - Jan 16 2017

--------------------------------------------------------------------------------

                                  +=========+
                                  |  About  |
                                  +=========+

0CC-FamiTracker is a modified version of FamiTracker that incorporates numerous
new features that work in exported NSFs and interface improvements. The name
"0CC" comes from the author's favourite arpeggio effect command. The current
version includes:

- Partial FamiTracker 0.5.0 beta support
- Sound engine extensions:
   - Ad-hoc multichip NSF export
   - Echo buffer access
   - Polyphonic note preview
- New effects:
   - Hardware volume envelope effects
   - Delayed channel effects
   - FDS automatic FM effects
   - N163 wave buffer access effect
- Instrument extensions:
   - Arpeggio schemes
   - Instrument recorder
   - Compatible sequence instruments
- Interface extensions:
   - Find / replace tab
   - Transpose dialog
   - Split keyboard
- Extra module contents:
   - Detune settings
   - Groove settings
   - Bookmark manager
   - Linear pitch mode

See the change log for the full list of changes made in 0CC-FamiTracker.

This program and its source code are licensed under the GNU General Public
License Version 2. Differences to the original FamiTracker source are marked
with "// // //"; those to the ASM source with ";;; ;; ;" and "; ;; ;;;". Since
version 0.3.12, the source code is no longer included within the download;
always consult the Github page for up-to-date source code files.

The current build is based on the version 0.5.0 beta 5 release of the vanilla
FamiTracker. 0CC-FamiTracker will be ported to newer vanilla releases once they
become available; features added in 0CC-FamiTracker may not have identical
behaviour as the corresponding features on the vanilla branch.



                                  +=========+
                                  |  Links  |
                                  +=========+

- http://hertzdevil.info/programs/
   The download site for all versions of 0CC-Famitracker.
- http://0cc-famitracker.tumblr.com/
   The official development log of 0CC-FamiTracker.
- http://hertzdevil.info/bug/main_page.php
   The official bug tracker for all of HertzDevil's projects, including this
   tracker. Feature requests and bug reports can be sent here.
- http://github.com/HertzDevil/0CC-FamiTracker
   The Git source repository for the tracker.
- http://github.com/HertzDevil/0CC-FT-NSF-Driver
   The Git source repository for the NSF driver.



                      +==================================+
                      |  FamiTracker 0.5.0 Beta Support  |
                      +==================================+

0CC-FamiTracker 0.3.14.3 and above support the loading of modules created in any
beta build of vanilla FamiTracker 0.5.0. See
https://gist.github.com/HertzDevil/c158d826a344e5ffbc0c0989e1c96a24 for the
current status of porting 0.5.0 beta module features.

The current implementation of the Sunsoft 5B chip is different from the vanilla
beta in the following ways:

- Commands V00 - V03 disable the envelope output and V04 - V07 enable it.
- Hx0 does not affect whether the current channel produces envelope output.
   Only Vxx and the instrument noise / mode sequence may be used to control the
   envelope flag.
- The envelope generator is retriggered whenever a note triggers on a channel
   with envelope output enabled, or any Hxy effect command is issued.

0CC-FamiTracker continues to save modules in the format version that is readable
by vanilla version 0.4.6. There is no guarantee that any 0.5.0 beta module saved
in the current version of 0CC-FamiTracker can be opened in the beta again.



                       +===============================+
                       |  Ad-hoc Multichip NSF Export  |
                       +===============================+

The Module Properties dialog now uses the same expansion chip selector as ipi's
mod (http://famitracker.com/forum/posts.php?id=5235). Each expansion chip can be
individually toggled on or off using the check boxes. Unlike the original build,
after the expansion chip configuration is modified, the pattern data move to the
correct channel positions so that data loss is prevented. (See below for a
description of the pitch mode setting.)

Since 0.3.6, 0CC-FamiTracker also allows ad-hoc exporting of multichip NSFs by
temporarily enabling all expansion chips.



                            +======================+
                            |  Echo Buffer Access  |
                            +======================+

0CC-FamiTracker supports an echo buffer for each channel. Upon playing, the echo
buffer is emptied; whenever the channel encounters a note or a note halt, that
event is pushed into the echo buffer. Then, using the corresponding echo buffer
access note (which must be manually assigned to a key in the configuration menu
by default), entries in the echo buffer can be retrieved. Effectively, "^x"
skips x notes above and retrieves that note event. All access note events are
processed during run-time in exported NSFs.

The skip amount is determined by the octave number during inserting an echo
buffer access note, and is restricted between 0 and 3. These notes cannot be
modified while transposing in a selection, but can be "transposed" if the
selection contains only the access note.

When a transposing effect is used, the first entry in the echo buffer will also
change accordingly. Currently, these effects have this behaviour: Qxy, Rxy, Txy



                         +===========================+
                         |  Polyphonic Note Preview  |
                         +===========================+

In 0CC-FamiTracker 0.3.14.2 and above, when edit mode is disabled, notes that
are input at the pattern editor (not at the instrument editor) will be
automatically queued to groups of equivalent channels, so that multiple notes
can be played simultaneously without breaking channel limits. The following
channel groups are available:

- 2A03 pulse channels and MMC5 pulse channels
- 2A03 triangle
- 2A03 noise
- 2A03 DPCM
- VRC6 pulses
- VRC6 sawtooth
- VRC7 FM channels
- FDS channel
- N163 channels
- 5B square channels

Notes are always queued to the currently selected channel first, then the
channels to the right. If the number of played notes exceeds the channel count
within the current group, the earliest played notes will be interrupted by new
ones.

When previewing N163 instruments that load multiple waves, it is often necessary
to insert appropriate Zxx effects somewhere in the pattern, then play that
single row to reserve different positions in the wave buffer so that notes do
not interfere with each other. Similarly, channel volume commands might be
needed to reduce the volume of some sound chips while playing.



                     +====================================+
                     |  Hardware Volume Envelope Effects  |
                     +====================================+

The following effects have been added to access several features on the 2A03
chip and the MMC5 pulse channels:

- EE0 - EE3
   Bit 0 toggles the hardware envelope on the pulse channel or the noise
   channel, which causes the channel volume to affect the channel's decay rate
   instead of the output amplitude. Smaller values give a faster decay. If the
   length counter is disabled (see below), the output amplitude warps, otherwise
   it stays at 0 after the decay finishes. On the triangle channel, this bit
   also toggles the linear counter since both must be active at the same time.
   Bit 1 toggles the length counter on the pulse or noise channel. On the
   triangle channel, both this and the linear counter may cut the channel's
   output after a fixed number of counter clocks have elapsed, whichever comes
   first.
- E00 - E1F
   Sets the length counter to the value listed below, and enables the length
   counter if it was disabled. Works on the pulse, triangle, or noise channel.
   On the triangle channel, this effect also initializes the linear counter;
   whichever is shorter will terminate the triangle output first. On
   initialization, this value is set to be 0x01 (254).
   |  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
   ----+---------------------------------------------------------------
   +00 | 10 254  20   2  40   4  80   6 160   8  60  10  14  12  26  14
   +10 | 12  16  24  18  48  20  96  22 192  24  72  26  16  28  32  30
- Sxx
   On the triangle channel, resets the linear counter to (xx - 0x80). If xx is
   smaller than 0x80, this effect disables the linear counter and the length
   counter, then behaves as the usual delayed note cut effect. S80 - SFF as the
   original effect has been disabled on all channels since NSF export does not
   work.

Currently, the hardware envelope and the linear counter are clocked at 240 Hz,
and the length counter at 120 Hz on 2A03 channels, 240 Hz on MMC5 channels. All
are independent from the module's engine speed. Volume mixing between the
channel volume and the instrument sequence still takes place while hardware
envelope is enabled!

The following effects have been added to access the volume envelope unit of the
FDS sound hardware:

- E00 - E3F
   Enables the hardware volume envelope, sets the direction to positive (attack
   envelope), and sets the rate to the effect parameter. A smaller rate value
   corresponds to a faster envelope.
- E40 - E7F
   Enables the hardware volume envelope, sets the direction to negative (decay
   envelope), and sets the rate to the effect parameter, minus 0x40. A smaller
   rate value corresponds to a faster envelope.
- EE0
   Disables the hardware volume envelope and returns volume control to the
   tracker's sound driver.

When the hardware volume envelope is enabled on the FDS, every time a note is
triggered, the FDS channel returns to that initial volume, and then yields
control to the envelope unit. The envelope can also be enabled during a note, so
that the attack/decay takes effect immediately. All volume changes not due to
the envelope unit still remain in the memory; as soon as the EE0 effect is
issued, this volume value is instantly recalled.



                         +===========================+
                         |  Delayed Channel Effects  |
                         +===========================+

The following effects have been added to 0CC-FamiTracker that perform certain
actions after x or xx frames have elapsed:

- Lxx
   Issues a note release, triggering the release part of the sequences used by
   the current instrument, or triggering the release command of VRC7 patches, or
   halting DPCM samples without writing to the DPCM bias. The delay amount must
   be less than 0x80.
- Mxy
   Sets the channel volume to y. This effect has been reserved in the effects
   enumeration since vanilla 0.3.5. Both parameters must be nonzero for
   0CC-FamiTracker to recognize this effect.
   After a delayed channel volume effect is issued, the next note will restore
   the original channel volume if the Axy volume slide effect is not active. The
   effect can still set the channel volume even though new notes appear before
   the x frames that follow, but the counter is always reset when a new Mxy
   effect is encountered before the previous one overwrites the channel volume.
- Txy
   Transposes the channel by y semitones, upwards if bit 7 is clear, and
   downwards if bit 7 is set. Here x has an effective range of 0 to 7 frames, so
   any value larger than 7 would be subtracted by 8 and the direction would be
   taken as downward.



                         +============================+
                         |  FDS Automatic FM Effects  |
                         +============================+

0CC-FamiTracker overloads the FDS effects in FamiTracker so that the FM rate of
the FDS channel may be realized as a multiple of the carrier frequency during
run-time. These effects are:

- Ixy
   When x is not equal to 0, enables auto-FM, and sets the modulator frequency
   to the carrier frequency multiplied by x / (y + 1).
- Hxx
   When xx is 0x80 or larger, sets the modulator multiplier's numerator to xx,
   so that the multiplier becomes xx / (y + 1) where y is previously set by an
   Ixy command. This effect command does nothing if auto-FM is disabled.
   FamiTracker's effect evaluation order is from left to right (except for Gxx),
   thus no Ixy effects should appear to the right of any Hxx effect on the same
   row for it to become effective.
- Zxx
   Sets the modulator frequency bias, which is an FM analog of the Pxx effect;
   the bias is added to the value resulting from modulator multiplication as the
   final register value. The default value of the modulator bias is 0x80. No
   special handling is done when the current module uses linear pitch.

Automatic FM does not apply to the FDS channel if the current instrument uses a
non-zero FM rate. All effects are stateful, so they do not have to be issued for
each individual note; using the existing forms of the FDS effects will disable
auto-FM immediately, but the modulator bias value remains effective once auto-FM
is enabled again.



                      +==================================+
                      |  N163 Wave Buffer Access Effect  |
                      +==================================+

0CC-FamiTracker has a new effect, Zxx, exclusive to the N163 channels, that
allows controlling the wave buffer more effectively:

- Z00 - Z7E
   Sets the channel's wave buffer position to the effect parameter, overriding
   the wave position of N163 instruments. All read / write operations on the
   wave buffer use the sample position determined by the effect parameter times
   2 (each byte holds 2 samples) until the reset effect below is encountered.
   0CC-FamiTracker performs bounds checking to ensure that the N163 channels
   will not read from the non-wave registers, but the exported NSFs do NOT
   perform this check. Use this effect at your own risk.
- Z7F
   Returns the wave buffer control to N163 instruments, allowing N163
   instruments to read and write to their default wave positions. This command
   takes effect immediately; the current instrument will write to the wave
   buffer at the position designated by the current instrument as soon as this
   effect command is encountered.

This effect is shown as Yxx in version 0.3.7 and 0.3.8. These will be converted
to the chip-specific form of Zxx automatically upon loading a module.



                             +====================+
                             |  Arpeggio Schemes  |
                             +====================+

Arpeggio schemes are a generalization of the 0xy arpeggio effect command which
allows the arpeggio sequences of instruments to carry variable entries modified
by the 0xy effect.

To use arpeggio schemes, they must be input using the MML field of the arpeggio
sequence editor. (The use of the Sunsoft 5B noise sequence editor is planned.)
The MML field accepts terms formed by "x" added to, "y" added to, or "y"
subtracted from any numeral between -27 and +36 inclusive. These occurrences of
"x" and "y" will be substituted with the respective parameters of the 0xy effect
whenever FamiTracker encounters these in patterns. Alternatively, by holding any
numpad key as below while clicking on the graph editor, a specific arpeggio type
can be accessed from the graph:

- 0 -> None
- 1 -> +x
- 2 -> +y
- 3 -> -y

As an example, given the following absolute arpeggio sequences:

- {| 0 12 4 16 7 19}
- {| 0 12 3 15 7 19}
- {| 0 12 5 17 9 21}

These can be replaced with one single arpeggio scheme:

- {| 0 12 x x+12 y y+12}

By using 047, 037, and 059, all three sequences can be invoked from one arpeggio
scheme. "x" and "y" can go before or after the numerals in each sequence term.
"-y" is valid but "-x" is not; thus these terms are all effective:

 x+12  12+x  y+12  12+y -y+12  12-y
 x-12 -12+x  y-12 -12+y -y-12 -12-y



                           +=======================+
                           |  Instrument Recorder  |
                           +=======================+

0CC-FamiTracker allows logging the output of sound channels to new instruments,
providing a new way to create instruments from pattern data. Right-clicking any
channel header or going to the Tracker menu from the main menu bar shows an
option to mark the current channel as ready for recording; the next time the
tracker plays the song, temporary instruments are created, whereas data will be
continuously appended to the sequences used by these instruments, which are
added to the instrument list once the song stops or the sequence size reaches
the designated amount.

The final volume and pitch are logged to the current instrument on every tick;
except for the features listed below, this instrument would replicate the output
of the recorded channel by assuming a channel volume of F and the current detune
settings. Inconsistent tracker settings might produce different results.

The instrument recorder only allows logging output that may be represented by
the respective instrument type of the sound chip; in particular, only sequence
instruments are supported right now. The following features are recorded only
once on the _final_ tick of each instrument upon playing:

- The FDS instrument waveform;
- The frequency modulation parameters for an FDS instrument;
- The wave position for an N163 instrument.

The following features will not be recorded:

- The DPCM channel and any VRC7 channel;
- Half of the volume values for the VRC6 sawtooth channel, if accessible
   otherwise;
- Hardware features accessible only by pattern effect commands;
- The FDS modulation table; (might be cached in the future)
- FDS waveform changes;
- More than 63 waveforms per N163 instrument, as well as any waveform whose
   size is different from the first recorded value, in which case the waveform
   of the previous tick is used.

Below the "Record to Instrument" option, "Recorder Settings" brings up a dialog
where parameters for the instrument recorder can be changed:

- Sequence length
   The maximum number of sequence terms per instrument logged. This value is
   limited below the maximum sequence size (252 ticks), and customarily limited
   above 24 ticks.
- Maximum instrument count
   The number of instruments generated by the recorder before logging
   automatically stops during playing. The recorder may emit as many new
   instruments as possible until the current module cannot contain any more
   data.
- Re-initialize settings upon stopping
   If checked, the recorder resets to 252 ticks and 1 maximum instrument after
   the current recording session ends.

Extremely high refresh rates may cause the recorder to discharge instruments too
quickly or crash the tracker; Excessive use of this feature can easily lead to
instrument data overflow during NSF export; use with care.



                     +===================================+
                     |  Compatible Sequence Instruments  |
                     +===================================+

The NSF driver already allows instruments to be used interchangeably on several
chips to a certain degree in exported NSFs, namely those from 2A03, VRC6, N163,
FDS, or 5B; these sound chips and MMC5 share a unified sequence instrument type.
(N163 does not actually work because it injects data before sequence settings.)
Since version 0.3.13, 0CC-FamiTracker adds full tracker-side support for using
these sequence instruments across expansion chips, while handling the duty/wave
setting for the respective chip appropriately.

Instruments from the sound chips listed above can be used across these chips.
Their volume, arpeggio, and (hi)-pitch sequences are processed according to the
target chip configuration. The following rules for duty cycle conversion apply:

- 2A03, MMC5
   VRC6 duty cycles from 6.25% to 12.5% become 12.5%, duty cycles from 43.75% to
   50% become 50%, and the rest become 25%. N163 wave indices are processed as
   is. 5B duty cycles always become 50%. There is no special handling of the
   duty cycle value regarding the noise generator of 2A03 or 5B.
- VRC6
   2A03 75% becomes 25%, the rest remain the same. N163 wave indices are
   processed as is. 5B duty cycles always become 50%. There is no special
   handling related to the sawtooth channel.
- FDS
   Non-FDS sequence instruments do not write to the wave buffer nor alter the
   FDS channel's frequency modulation parameters. FDS instruments on non-FDS
   channels do not affect the wave buffer.
- N163
   Non-N163 sequence instruments do not write to the wave buffer nor alter the
   current channel's wave parameters. N163 instruments on non-N163 channels do
   not affect the wave buffer. Duty cycle does not affect the current channel
   when the currently playing instrument is not from N163.
- 5B
   All non-5B instrument duty values enable tone output, plus disable the noise
   and envelope outputs. 5B intruments on non-5B channels do not affect the
   envelope/noise generator.

Duty conversion occurs only through duty/wave instrument sequences; the Vxx
effect modifies the duty cycle/wave index directly, without invoking any
conversion method. Instruments may work even if the respective sound chip is
absent from the current module/NSF, and for this reason instrument sequences on
unused sound chips remain in the current module file when saved.

Except for the duty sequences, all other sequence types are raw; no conversion
takes place to account for the differences in the volume and pitch registers
between sound channels.



                            +======================+
                            |  Find / Replace Tab  |
                            +======================+

The Find / Replace tab is an addition to 0CC-FamiTracker, toggled by Ctrl+F by
default, that allows quick searching and replacement of pattern data in a
module.

The four query fields are:

- Note, the note field, which could be one of the following:
   - The note as notated in the pattern editor, possibly without an octave. The
      noise channel can be searched if and only if the note field is identical
      to any noise note (e.g. "4-#", "C-#");
   - "-" or "---" for note rest;
   - "=" or "===" for note release;
   - "^", possibly followed by a buffer index, for echo buffer access;
   - an integer between 0 and 95 representing the absolute note from C-0 to
      B-7;
   - "." for any note event, or ".-#" for any note on the noise channel.
- Inst, the instrument index, which could be any hexadecimal number between 0
   and 3F, or "." for any non-empty index;
- Vol, the channel volume, which could be any hexadecimal digit, or "." for any
   non-empty value;
- FX, the effect command, which could be any valid effect character or the
   character "." for any non-blank effect, possibly followed by a hexadecimal
   effect parameter.

Above each field is a button which, when not depressed, indicates that the
respective field will be ignored in the query. If the field is enabled and
empty, the query contains a blank field for that part of the note. For the
search query, both fields are required to be blank in order to search blank
fields.

Below three of the search query fields are optional fields that allow one to
specify the range of the respective query. Either field can be used if range
searching is not need; the top field does not necessarily have to represent a
greater value than the bottom field. For example, the query below finds all rows
with a note between A-3 and G-4, using instrument 05 and a volume of 7 or less:

    Note    Inst       Vol    FX
    A-3     (blank)    7      (disabled)
    G-4     05         0

There is currently no range searching for the effect parameter. Range search
queries do not allow wildcards of any form; for the note query this includes
commands with no specified octave value.

The search scope can be modified by using the dropdown menu, which can limit the
search area to the current track, channel, frame, pattern, or selection. The
menu to the right determines which effect columns to look for while searching
and replacing data; if set to "All", 0CC-FamiTracker finds rows containing at
least one visible effect that matches, and automatically determines the correct
effect columns for replacement.

The find / replace tab contains a few options:

- Vertical-first searching
   By default, 0CC-FamiTracker searches across the channels on a row, and then
   moves to the next row. When this option is checked, the search direction
   within a frame becomes moving along all the rows in a channel, then to the
   next channel.
- Remove original data
   When checked, replacement takes place after the target row is emptied.
- Negate search
   When checked, the tracker finds rows that do not match the search query. If
   the note query is used, the tracker never matches notes on a noise channel
   with a melodic note and vice versa, regardless of whether this option is
   enabled.



                             +====================+
                             |  Transpose Dialog  |
                             +====================+

0CC-FamiTracker 0.3.14.0 adds a transpose dialog under the Song menu, which
allows quickly transposing entire songs while keeping most non-melodic notes
unchanged. This is achieved by ignoring the noise and DPCM channels, as well as
selected instruments.

The following options are available:

- Semitones
   The number of half-steps to transpose.
- Raise / Lower
   Whether the track(s) is/are to be transposed up or down.
- Transpose all tracks
   If enabled, the transposition applies to all tracks in the current module;
   otherwise only the current track is transposed.
- Exclude these instruments
   An array of checkboxes next to instrument indices. If an instrument with a
   given index exists in the module, the corresponding checkbox will be enabled;
   checking the box will cause the transposition to ignore all notes containing
   the given instrument index, regardless of the channels these notes are on.
   These settings are remembered even after the dialog is closed.
- Reverse
   Changes all indices from enabled to disabled, and vice versa, including
   disabled checkboxes.
- Clear All
   Enables transposition for all instruments, including disabled checkboxes.

Out-of-bound notes clip at C-0 or B-7. The undo history will be reset after
transposition. (For transposition on selected channels, enable multi-frame
selection in the configuration menu.)



                              +==================+
                              |  Split Keyboard  |
                              +==================+

0CC-FamiTracker allows splitting keyboard notes into two ranges, similar to how
MIDI keyboards enable playing with two instruments at the same time. The split
keyboard settings dialog, which is accessible under the Edit menu, contains
these options:

- Enable split keyboard
   Unless this option is checked, all split keyboard functionality will be
   disabled.
- Split point
   The highest note of the split region.
- Channel
   The channel to play the notes in the split region when edit mode is disabled.
   This option does nothing when editing the module, or if this option is set to
   "Keep".
- Instrument
   The instrument to use for all notes within the split region. If set to
   "Keep", notes in the split region use the currently selected instrument;
   otherwise, this instrument index is shown on the status bar next to the
   selected instrument.
- Transpose
   The number of semitones to add to all notes within the split region.

When the noise channel is selected, notes could be moved outside using the
"Channel" option, and vice versa; but the "Instrument" and "Transpose" options
do not apply to notes on the noise channel.



                             +===================+
                             |  Detune Settings  |
                             +===================+

Since version 0.3.2, 0CC-FamiTracker is able to generate a detune settings block
for each individual module, containing information on adjusting the period or
frequency register of each individual note on each lookup table. Below "Module"
on the menu bar, clicking "Detune Settings" brings up a dialog where the detune
tables can be manipulated.

The octave slider, note slider and chip radio buttons select the current detune
table entry to modify. The note field accepts both "#" and "-" as accidentals.

The offset slider adjusts the fine pitch value of the current note on the lookup
table. Negative values have lower pitch and positive values have higher pitch.
The slider's range is -128 ~ 128, but using the offset field directly, any
integer can be used as the fine pitch value.

A value of 1 on these tables is equivalent to the finest pitch offset available
in FamiTracker's sound engine, which are in turn equivalent to Pxx effects
except for the N163 table, where Pxx on N163 channels is much coarser. The N163
detune table does not scale with the number of enabled N163 channels.

The Reset button initializes the target detune tables with zeroes. The Tune
button will overwrite the target detune tables with an equal temperament table
lower than the 440 Hz concert pitch by the number of cents specified. Either the
current sound chip's detune table or all six tables can be chosen as the target
detune tables.

The Import and Export buttons load and save external comma-separated values for
exchange of detune tables across modules.



                             +===================+
                             |  Groove Settings  |
                             +===================+

Since version 0.3.4, 0CC-FamiTracker modules may store an extra data block which
contains grooves as in Little Sound Dj, or speed sequences. While the sound
engine is loaded with a groove, at each row update the sound engine cycles
through the groove, reads a groove entry and then uses it as the song speed.
Below "Module" on the menu bar, clicking "Groove Settings" brings up a dialog
where the grooves can be manipulated.

The groove list is be used to reorder grooves, as well as delete one or all of
them. The groove editor is used to modify the entries in the selected groove.
Grooves may be copied and pasted as space-separated values.

Expanding the groove halves the groove's average speed and works only if all
entries are greater than or equal to 2. Shrinking the groove doubles the average
speed and works only when the groove length is a multiple of 2.

Padding inserts the specified entry after each groove entry, so that the average
speed is halved. All entries in the groove must be greater than the pad amount.

The Oxx effect applies the groove with index xx on the current row, starting
from the first entry; when the Fxx effect modifies the song speed, it cancels
the song groove at the same time.

Each track can now use either a default speed or a default groove; songs using a
default groove index of 0 cannot be opened in vanilla builds.

Each module may store up to:

- 32 grooves;
- 255 groove bytes; (each non-empty groove uses 1 byte per entry plus 2 bytes)
- 128 entries per groove.



                             +====================+
                             |  Bookmark Manager  |
                             +====================+

0CC-FamiTracker allows each module to contain its own list of bookmarks for
quick navigation. Bookmarks of the current track can be accessed and manipulated
by clicking "Bookmark Manager" below "Module" on the menu bar.

Each bookmark contains:

- A name, which defaults to "Bookmark" and comes with an index if created from
   the pattern editor directly;
- A frame index and a row index, which indicate the position of the bookmark.
   Bookmarks reposition themselves upon performing frame actions such as
   inserting or removing frames;
- Highlight settings which override the previous value of the track if enabled.
   Using bookmarks, it is possible to change the row highlight intervals in the
   middle of a track. Disabling "Apply to all following frames" will keep the
   highlight distance to the current frame, so that the next frame will not use
   the settings of this bookmark.

Pressing "Create New" appends the current bookmark to the bookmark list of the
current track, and the corresponding row of the pattern editor will be marked on
the row index column. Each song of a module contains its own bookmark list, and
there is no limit to how many bookmarks a module can contain. Bookmarks can be
sorted by position or by name.

The bookmark list supports these keyboard shortcuts:

- Ctrl + Up / Down for moving the currently selected bookmark;
- Insert for creating a new bookmark;
- Delete for removing the currently selected bookmark.

0CC-FamiTracker also supports pattern editor shortcuts for toggling on/off the
bookmark on the current row, and navigating to the next/previous bookmark. The
respective menu commands are available under [Edit] -> [Bookmarks].



                            +=====================+
                            |  Linear Pitch Mode  |
                            +=====================+

Vanilla FamiTracker 0.4.0 and above contain unused code that would treat all
pitch effects on melodic channels uniformly; as with many other sound engines,
it achieves this by right-shifting the current pitch register in order to
approximate a linear pitch space. The linear pitch mode in 0CC-FamiTracker
improves the accuracy of this behaviour by subdividing each semitone into 32
equal parts as the smallest pitch unit for all pitch calculations. For each
melodic channel, the final period / frequency register is calculated only after
all pitch effects have been applied in the linear space, which allows all
melodic channels to use the same pitch effects regardless of the current pitch
and their pitch register implementations (in particular, the number of enabled
N163 channels).

Additionally, the number 0x20 corresponds to exactly a semitone in pitch
effects, thus the following statements about pitch effects hold:

- 120 increases the current note by a semitone every tick, and similarly for
   220 and 320;
- All 4xy commands have the same amplitude regardless of the current pitch and
   the vibrato mode;
- PA0 transposes upward by one semitone, and P60 downward by one semitone;
- Since QFy and RFy both instantiate a 31F command to the target note, the
   slide speed will be almost the same as a semitone per tick;
- All instrument pitch sequences may use the value 32 to increase or decrease
   the current note by a semitone;
- As a result of the above and compatible instrument sequences, positive
   entries always denote an increase in pitch and vice versa, so existing
   instruments for sound chips using period registers need to have their
   sequence values inverted;
- The hi-pitch sequence corresponds exactly to quarter tone offsets.

The linear pitch mode depends on linear interpolation between two adjacent pitch
register values; the detune table must be assumed to be smooth enough so that
pitches are as evenly spaced as possible. In this mode, slide effects no longer
automatically silence the channel on zeroing the period or frequency register,
since there is always a highest or lowest note available on the pitch lookup
table.

If, during the pitch interpolation, the difference between two pitch registers
is less than 32, then the sound engine guarantees that the pitch of any detuned
note is different from that of the correctly tuned note below this pitch. Unless
the difference is less than 2, the pitch will also be different from the
correctly tuned note above. For example, P81 is always different from P80, but
P7F is the same as P80 for the entire octave 7 on any 2A03 channel.

The pitch mode setting can be selected on the module properties dialog. As this
mode was indeed planned in the vanilla build, the source code contains commented
out code that would change the format of the PARAMS data block; therefore,
0CC-FamiTracker saves this flag separately in a PARAMS_EXTRA data block whenever
this mode is used, to ensure that modules using linear pitch mode do not become
incompatible.



                     +===================================+
                     |  New Configuration Menu Settings  |
                     +===================================+

The following options have been added to the General tab of the configuration
menu:

- Warp pattern values
   When checked, using Shift+Mouse Wheel on the instrument, channel volume, or
   any effect parameter will cause these fields to overflow appropriately when
   the minimum or maximum is reached. Each digit of a two-parameter effect warps
   independently from the other unless they are inside a selection.
- Cut sub-volume
   In NSFs exported with older versions of FamiTracker, volume values between 0
   and 1 caused by the Axy and 7xy effects will be truncated, while in later
   versions these values round up to 1. When checked, the old behaviour will be
   used for all channels in the tracker. This option does not affect the volume
   table in exported NSFs. (As of version 2.11 of the NSF driver, only some
   expansion chips use the rounding-up behaviour.)
- Use old FDS volume table
   Since 0CC-FamiTracker 0.3.8, the tracker uses the same volume table as in
   exported NSFs, which is slightly louder, especially at high instrument volume
   and low channel volume. When checked, the existing volume table will be used
   in the tracker. This option does not affect the table in exported NSFs.
- Retrieve channel state
   When checked, 0CC-FamiTracker will search backward in the module to restore
   the effect parameters of all effect commands that have memory, and apply them
   at once before playing begins. This option does not check for global effects
   that affect the playing order (such as Bxx or Dxx with non-zero parameters).
- Overflow paste mode
   When checked, pattern data in the clipboard may be moved to subsequent frames
   if the destination row exceeds the number of rows in the current frame.
- Show skipped rows
   In previous versions of FamiTracker, rows truncated by skip effects are
   displayed if and only if "Preview next/previous frame" is disabled. Since
   0CC-FamiTracker 0.3.9 this behaviour is separated from the preview option.
- Hexadecimal keypad
   When checked, the following numpad keys are treated as hexadecimal digits A -
   F in the pattern editor: Divide, Multiply, Subtract, Add, Enter, Decimal.
   These keys are effective only if no shortcuts using them are defined in the
   configuration menu (in particular, Enter / Return must not be assigned to any
   shortcut).
- Multi-frame selection
   When checked, selections in the pattern editor can span across multiple
   frames. This behaviour is always enabled since 0.3.9, but requires manual
   enabling since 0.3.11.
- Check version on startup
   When checked, where an internet connection can be established,
   0CC-FamiTracker checks against the releases on the Github repository to see
   if any newer version is available, and prompts the user to download the new
   version if any.



                          +=========================+
                          |  New Shortcut Commands  |
                          +=========================+

The following shortcuts have been added to 0CC-FamiTracker: (parenthesized key
combinations are the default hotkeys if provided)

- Paste overwrite / insert
   Pastes the clipboard content at the given cursor position for pasting. In
   "Overwrite", all non-empty fields in the clipboard will be pasted, regardless
   of whether the target fields contain non-empty data; in "Insert", pattern
   data below the pasted content is pushed downwards.
- Deselect (Esc)
   Deactivates the current selection, if any. This shortcut exists in the
   vanilla build but is not assignable there.
- Select row/column/pattern/frame/channel/track
   Selects the entire area of the given scope containing the cursor. Multi-frame
   selections must be enabled for the "Channel" and "Track" scopes.
- Select in other editor
   If the pattern editor is active, moves the current selection to the frame
   editor, or vice versa.
- Go to row (Alt+G)
   Brings up a dialog which allows moving the cursor to any position in the
   current song.
- Toggle / Solo chip (Ctrl+Alt+F9, Ctrl+Alt+F10)
   Similar to "Toggle / Solo channel", but applies to the entire sound chip.
- Record to instrument
   Selects the current channel for recording to new instruments.
- Toggle bookmark (Ctrl+K)
   Creates a bookmark on the current row if none exists, otherwise removes all
   bookmarks on the current row.
- Next / Previous bookmark (Ctrl+PgDown, Ctrl+PgUp)
   Navigates to the first bookmark below / above the current row, and displays
   its information on the status bar.
- Type instrument number
   When pressed, the two following keypresses will be treated as a hexadecimal
   instrument index, and the tracker selects the instrument with the given index
   if it exists.
- Mask volume (Alt+V)
   Toggles the volume mask setting. This shortcut exists in the vanilla build
   but is not assignable there.
- Stretch patterns
   Stretches the data in the current active selection using a user-defined map.
- Duplicate current pattern (Alt+D)
   Copies the pattern below the cursor to an unused blank pattern, then modifies
   the current frame to use that new pattern index.
- Coarse decrease / increase values (Shift+F3 / Shift+F4)
   Increases or decreases all pattern fields except for note values by 16.
- Toggle find / replace tab (Ctrl+F)
   Shows or hides the find / replace tab.
- Find next / previous
   Finds the first occurrence of the search query after / before the current
   row.
- Recall channel state
   Searches backward in the module and accumulates the channel state, then
   reports it on the status bar, as if by the "Retrieve channel state" option.
   This command also reports the current groove position if a groove is used.
- Compact View
   Shrinks the pattern editor so that only the note field is displayed; if a row
   contains no notes, the fields to the right will be displayed grayed out.
- Toggle N163 multiplexer emulation (Ctrl+Shift+M; not configurable)
   Enables or disables the old N163 emulation without the multiplexer hiss found
   in early versions of FamiTracker.



                              +=================+
                              |  Miscellaneous  |
                              +=================+

The "Compact view" button hides all pattern columns except the note columns, and
displays greyed out text for the hidden columns for rows that do not contain a
note. This view is useful for recording videos with a lot of channels.

"Merge Duplicated Patterns" now applies only to the current track.



                               +================+
                               |  Known issues  |
                               +================+

- When the triangle channel's linear counter is enabled, the high byte of the
   period cannot be changed; this is intended behaviour because any write will
   reset the linear counter
- MMC5's length counter depends on the 2A03's frame counter
- In exported NSFs, the echo buffer is updated as Txy effects are applied; in
   the tracker this happens upon encountering Txy effects



                                 +===========+
                                 |  Credits  |
                                 +===========+

- jsr: Original developer
- Alexander283: Original arpeggio scheme proposal
- ImATrackMan: FDS / 5B NSF hardware recordings
- ipi: Original implementation of the Lxx effect and expansion chip selector,
   "Usual Day" demo module
- jsr: Partial implementation of the Sunsoft 5B chip and linear pitch mode
- m9m, MrKyurem, Patashu, Phroneris, retro_dpc, techel, Threxx, w7n,
   Xyz_39808, YobaHere: Bug testing
- jfbillingsley: N163 waveform manager design

--------------------------------------------------------------------------------

For enquiries mail to nicetas.c@gmail.com
