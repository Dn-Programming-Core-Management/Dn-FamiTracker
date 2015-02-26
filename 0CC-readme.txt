0CC-FamiTracker Mod
Readme / Manual
Written by HertzDevil
Version 0.3.8 - Feb 16 2015

--------------------------------------------------------------------------------

	===
	About
	===

This is a modified version of FamiTracker that incorporates various new effects
and bug fixes which work in exported NSFs as well. The name "0CC" comes from the
author's favourite arpeggio effect. The current version includes:

 - Complete Sunsoft 5B support
 - Arpeggio schemes
 - Hardware volume envelopes
 - Find / replace tab
 - Detune settings
 - Groove settings
 - Echo buffer access
 - Delayed channel effects
 - N163 wave buffer access effect
 - Expansion chip selector & ad-doc multichip NSF export

See also the change log for a list of various tweaks and improvements.

This program and its source code are licensed under the GNU General Public
License Version 2. Differences to the original FamiTracker source are marked
with "// // //"; those to the ASM source with ";;; ;; ;" and "; ;; ;;;". The
current build is based on the 0.4.6 official release of FamiTracker and the 2.11
version of the NSF driver; 0CC-FamiTracker will continue to use that version in
future updates unless there is great need for update, but its behaviour may not
be identical if and when any of the features of this mod becomes part of the an
official FamiTracker release.

0CC-FamiTracker accepts feature requests and bug reports at the official
development log at http://0cc-famitracker.tumblr.com/ as well as the forum at
http://hertzdevil.info/forum/ where users may also share their 0CC-FT creations.

	===
	Sunsoft 5B Support
	===

While the official release of FamiTracker doesn't, 0CC-FamiTracker now fully
supports the Sunsoft 5B chip, which was used in Gimmick!, the only NES game that
uses this expansion chip. The Sunsoft 5B chip is functionally identical to a
YM2149F chip with half clock speed.

The 5B noise / mode sequence, as well as the Vxx effect, takes four parameters:
 - Bits 0 - 4 determine the 5B noise frequency, lower values have higher pitch;
    (Since there is only one noise output, noise period values from channels to
    the right will override those to the left. This is not thoroughly tested.)
 - Bit 5 selects the envelope output;
 - Bit 6 selects the square wave output;
 - Bit 7 selects the noise output.

The 5B channels also use these chip-exclusive effects:
 - Hxx sets the low byte of the envelope period;
 - Ixx sets the high byte of the envelope period;
 - Jxx sets the envelope shape.

See http://wiki.nesdev.com/w/index.php/Sunsoft_audio for more information.

	===
	Arpeggio Schemes
	===

Arpeggio schemes are a generalization of the 0xy arpeggio effect command which
allows the arpeggio sequences of instruments to carry variable entries modified
by the 0xy effect.

CAUTION: Arpeggio schemes WILL crash on official versions of FamiTracker!

To use arpeggio schemes, they must be input using the MML field of the arpeggio
sequence editor. (The use of the Sunsoft 5B noise sequence editor is planned.)
The MML field accepts terms formed by "x" added to, "y" added to, or "y"
subtracted from any numeral between -27 and +36 inclusive. These occurrences of
"x" and "y" will be substituted with the respective parameters of the 0xy effect
whenever FamiTracker encounters these in patterns.

As an example, given the following absolute arpeggio sequences:
{| 0 12 4 16 7 19}
{| 0 12 3 15 7 19}
{| 0 12 5 17 9 21}
These can be replaced with one single arpeggio scheme:
{| 0 12 x x+12 y y+12}
By using 047, 037, and 059, all three sequences can be invoked from one arpeggio
scheme. "x" and "y" can go before or after the numerals in each sequence term.
"-y" is valid but "-x" is not; thus these terms are all effective:
x+12  12+x y+12  12+y -y+12  12-y
x-12 -12+x y-12 -12+y -y-12 -12-y

What arpeggio schemes can do:
- Experience with more arpeggios with 0xy than the usual {| 0 x} and {| 0 x y}
- Always use the default 0xy effect in any given phase, e.g. {| x y 0} and
   {| y 0 x}
- Create extended chord arpeggio instruments, e.g. {|0 x y 11} and
   {| 0 0 x x y y 10 10}
- Input lead notes and then use one negative parameter to reference the
   countermelody, e.g. {| 0 -y -y 0 0}
- Use the 0xy as a parameter for "movable fixed" arpeggios, e.g. {x+12 x+12 | 0}
- Reduce the number of arpeggio instruments sharing the same pattern but
   different notes
- Change the notes of an arpeggio instruments without affecting their sequence
   positions
- Keep the FTM tidier (also reduce file size if no new effect columns are
   created; NSFs may become larger)

	===
	Hardware volume envelopes
	===

The following effects have been added to access several features on the 2A03
chip and the MMC5 pulse channels:

EE0 - EE3: Bit 0 toggles the hardware envelope on the pulse channel or the noise
            channel, which causes the channel volume to affect the channel's
            decay rate instead of the output amplitude. Smaller values give a
            faster decay. If the length counter is disabled (see below), the
            output amplitude warps, otherwise it stays at 0 after the decay
            finishes. On the triangle channel, this bit toggles the linear
            counter instead.
           Bit 1 toggles the length counter on the pulse channel or the noise
            channel. Both this and the triangle channel's linear counter cut the
            channel's output after a fixed number of counter clocks have
            elapsed, whichever comes first.

E00 - E1F: Sets the length counter to the value listed below, and enables the
            length counter if it was disabled. Works on the pulse, triangle, or
            noise channel. On the triangle channel, this effect also resets the
            linear counter; whichever is shorter will terminate the triangle
            output first. On initialization, this value is set to be 0x01 (254).
               |  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
           ----+---------------------------------------------------------------
           +00 | 10 254  20   2  40   4  80   6 160   8  60  10  14  12  26  14
           +10 | 12  16  24  18  48  20  96  22 192  24  72  26  16  28  32  30

Sxx:       On the triangle channel, resets the linear counter to (xx - 0x80). If
            xx is smaller than 0x80, this effect disables the linear counter and
            the length counter, then behaves as the usual delayed note cut
            effect. S80 - SFF as the original effect has been disabled on all
            channels since NSF export does not work.

Currently, the hardware envelope and the linear counter are clocked at 240 Hz,
and the length counter at 120 Hz on 2A03 channels, 240 Hz on MMC5 channels. All
are independent from the FTM's engine speed. Volume mixing between the channel
volume and the instrument sequence still takes place while hardware envelope is
enabled!

	===
	Find / Replace Tab
	===

The Find / Replace tab is an addition to 0CC-FamiTracker, toggled by Ctrl+F by
default, that allows quick search and replacement of pattern data in a FTM.
Currently, 0CC-FamiTracker only allows simple search and replacement queries.

The simple query fields are:
 - Note, the note field, which could be one of the following:
  - The note as notated in the pattern editor, possibly without an octave. The
     noise channel can be searched if and only if the note field is identical to
     any noise note (e.g. "4-#", "C-#");
  - "-" or "---" for note rest;
  - "=" or "===" for note release;
  - "^", possibly followed by a buffer index, for echo buffer access;
  - "-" or "---" for note rest;
  - an integer between 0 and 95 representing the absolute note from C-0 to B-7.
 - Inst, the instrument index, which could be any hexadecimal number between 0
    and 3F;
 - Vol, the channel volume, which could be any hexadecimal digit;
 - FX, the effect command, which could be any valid effect character, possibly
    followed by a hexadecimal effect parameter.

Above each field is a button which, when not depressed, indicates that the
respective field will be ignored (treated like a wildcard) in the query. If the
field is enabled and empty, it will be regarded as the corresponding blank
pattern data.

The search scope can be modified by using the dropdown menu, which can limit the
search area to the current track, channel, frame, or pattern. The menu to the
right determines which effect columns to look for while searching and replacing
data; if set to "Any", 0CC-FamiTracker automatically determines the correct
effect for replacement.

The find / replace tab contains a few options:
 - Vertical-first searching: By default, 0CC-FamiTracker searches across the
    channels on a row, and then moves to the next row. When this option is
    checked, the search direction within a frame becomes moving along a pattern
    first, then to the next channel;
 - Replace all: When checked, pressing the "Replace" button will iterate through
    the entire search area until all occurrences of the search query have been
    replaced, or an internal limit of 10,000 replacements is reached, at which
    point 0CC-FamiTracker stops the operation to avoid potential lockup;
 - Remove original data: When checked, any field in the replacement query that
    is disabled will be treated as the corresponding blank pattern data.

	===
	Detune settings
	===

Since version 0.3.2, 0CC-FamiTracker is able to generate a detune settings block
for each individual FTM, containing information on adjusting the period or
frequency register of each individual note on each lookup table. Below "Module"
on the menu bar, clicking "Detune settings" brings up a dialog where the detune
tables can be manipulated.

The octave slider, note slider and chip radio buttons select the current detune
table entry to modify. The note field accepts both # and - as accidentals.

The offset slider adjusts the fine pitch value of the current note on the lookup
table. Negative values have lower pitch and positive values have higher pitch.
The slider's range is -128 ~ 128, but using the offset field directly, any
integer can be used as the fine pitch value. A value of 1 on these tables is
equivalent to the smallest pitch offset available in FamiTracker's sound engine,
so they are in turn equivalent to Pxx effects except for the N163 table, where
Pxx on N163 channels is much coarser.

The same N163 detune table behaves less sensitively as the number of N163
channels increases. This may be fixed in a future version.

The Reset button initializes the target detune tables with zeroes. The Tune
button will overwrite the target detune tables with an equal temperament table
lower than the 440 Hz concert pitch by the number of cents specified. The Import
and Export buttons load and save external comma-separated values for exchange of
detune tables across FTMs.

Either the current sound chip's detune table, or all six tables, can be chosen
as the target detune tables.

	===
	Groove Settings
	===

Since version 0.3.4, 0CC-FamiTracker FTMs store an extra data block which stores
grooves as in Little Sound Dj, or speed sequences. While the sound engine is
loaded with a groove, at each row update the sound engine cycles through the
groove, reads a groove entry and then uses it as the song speed. Below "Module"
on the menu bar, clicking "Groove settings" brings up a dialog where the grooves
can be manipulated.

The groove list is be used to reorder grooves, as well as delete one or all of
them. The groove editor is used to modify the entries in the selected groove.
Grooves may be copied and pasted as space-separated values.

Expanding the groove halves the groove's average speed and works only if all
entries are greater than or equal to 2. Shrinking the groove doubles the average
speed and works only when the groove length is a multiple of n2.

Padding inserts the specified entry after each groove entry, so that the average
speed is halved. All groove entries must be greater than the pad amount.

The Oxx effect applies the groove with index xx on the current row; when the Fxx
effect modifies the song speed, it cancels the song groove at the same time.

Each track can now use either a default speed or a default groove; they both use
the same variable in 0CC-FT, so a groove index of 0 can be used to ensure that
the FTM cannot be opened in official releases, or a non-zero index may be used
for backward compatibility.

Each FTM may store up to:
 - 32 grooves;
 - 255 groove bytes; (each non-empty groove uses 1 byte per entry plus 2 bytes)
 - 128 entries per groove.

	===
	Echo Buffer Access
	===

0CC-FamiTracker supports an echo buffer for each channel. Upon playing, the echo
buffer is emptied; whenever the channel encounters a note or a note halt, that
event is pushed into the echo buffer. Then, using the corresponding echo buffer
access note (which must be manually assigned to a key in the configuration menu
by default), entries in the echo buffer can be retrieved. Effectively, this is
the same as skipping the number of notes above indicated by the access note and
retrieving the note event. All access notes are processed during run-time in
exported NSFs.

The skip amount is determined by the octave number during inserting an echo
buffer access note, and is restricted between 1 and 3. These notes cannot be
modified while transposing in a selection.

When a transposing effect is used, the first entry in the echo buffer will also
change accordingly. Currently, these effects have this behaviour: Qxy, Rxy, Txy.

	===
	Delayed Channel Effects
	===

The following effects have been added to 0CC-FamiTracker that perform certain
actions after x or xx frames have elapsed:

Lxx: Issues a note release, triggering the release part of the sequences used by
      the current instrument, or triggering the release command of VRC7 patches,
      or halting DPCM samples without writing to the DPCM bias. The delay amount
      must be less than 0x80.

Mxy: Sets the channel volume to y. This effect has been reserved in the effects
      enumeration since official 0.3.5. Both parameters must be nonzero for
      0CC-FamiTracker to recognize this effect.
     After a delayed channel volume effect is issued, the next note will restore
      the original channel volume if the Axy volume slide effect is not active.
      The effect can still set the channel volume even though new notes appear
      before the x frames that follow, but the counter is always reset when a
      new Mxy effect is encountered before the previous one overwrites the
      channel volume.

Txy: Transposes the channel by y semitones, upwards if bit 7 is clear, and
      downwards if bit 7 is set. Here x has an effective range of 0 to 7 frames,
      so any value larger than 7 would be subtracted by 8 and the direction
      would be taken as downward.

	===
	N163 Wave Buffer Access Effect
	===

0CC-FamiTracker has a new effect, Zxx, exclusive to the N163 channels, that
allows controlling the wave buffer more effectively:

Z00 - Y7E: Sets the channel's wave buffer position to the effect parameter,
            overriding the wave position of N163 instruments. All read / write
            operations on the wave buffer use the sample position determined by
            the effect parameter times 2 (each byte holds 2 samples) until the
            reset effect below is encountered. 0CC-FamiTracker performs bounds
            checking to ensure that the N163 channels will not read from the
            non-wave registers, but the exported NSFs do NOT perform this check.
            Use this effect at your own risk.

Z7F	 : Returns the wave buffer control to N163 instruments, allowing N163
            instruments to read and write to their default wave positions.

This effect is shown as Yxx in version 0.3.7 and 0.3.8. These will be converted
to the chip-specific form of Zxx automatically upon loading an FTM.

	===
	Expansion Chip Selector
	===

The Module Properties dialog now uses the same expansion chip selector as ipi's
mod (http://famitracker.com/forum/posts.php?id=5235). Each expansion chip can be
individually toggled on or off using the check boxes. Unlike the original build,
after the expansion chip configuration is modified, the pattern data move to the
correct channel positions so that data loss is prevented.

Since 0.3.6, 0CC-FamiTracker also allows ad-hoc exporting of multichip NSFs by
temporarily enabling all expansion chips.

	===
	Miscellaneous
	===

The following options have been added to the General tab of the configuration
menu:
 - Warp pattern values: When checked, using Shift+Mouse Wheel on the instrument,
    channel volume, or any effect parameter will cause these fields to overflow
    appropriately when the minimum or maximum is reached. Each digit of a two-
    parameter effect warps independently from the other unless they are inside a
    selection.
 - Cut sub-volume: In NSFs exported with older versions of FamiTracker, volume
    values between 0 and 1 caused by the Axy and 7xy effects will be truncated,
    while in later versions these values round up to 1. When checked, the old
    behaviour will be used for all channels. (As of version 2.11 of the NSF
    driver, only some expansion chips use the rounding-up behaviour.)
 - Use old FDS volume table: Since 0CC-FamiTracker 0.3.8, the tracker uses the
    same volume table as in exported NSFs, which is slightly louder, especially
    at high instrument volume and low channel volume. When checked, the existing
    volume table will be used.

The following shortcuts have been added to 0CC-FamiTracker:
 - Mask volume (Alt+V by default)
 - Toggle find / replace tab (Ctrl+F by default)
 - Find next (none by default)
 - Duplicate current pattern (Alt+C by default)
 - Toggle N163 multiplexer emulation (Ctrl+Shift+M)

	===
	Known issues
	===

- When the triangle channel's linear counter is enabled, the high byte of the
   period cannot be changed; this is intended behaviour because any write will
   reset the linear counter
- The arpeggio sequence editor cannot access "x" or "y" from the graph
- Pitch bend effects remove the 0xy parameters in NSFs (pitch bend effects do
   override arpeggio, however in the tracker the 0xy parameter is stored
   separately)
- MMC5's length counter depends on the 2A03's frame counter
- The behaviour of Qxy and Rxy on the noise channel is inconsistent between
   FamiTracker and NSF driver when the pitch overflows
- FDS and N163 sometimes load incorrect waves in multichip NSFs

	===
	Credits
	===

- ImATrackMan: Sunsoft 5B NSF hardware recordings
- ipi: Original implementation of the Lxx effect and expansion chip selector,
   "UsualDay.ftm" demo module
- jsr: Partial implementation of the Sunsoft 5B chip
- Xyz_39808: Bug testing
- Flaminglog: Easter egg

--------------------------------------------------------------------------------

For enquiries mail to nicetas.c@gmail.com