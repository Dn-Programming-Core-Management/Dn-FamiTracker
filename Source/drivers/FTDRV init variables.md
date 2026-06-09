# Initialized variables

last updated: 2026-06-10

---

variables init by `ft_music_init`

- var_Linear_Counter `#$FF`
- var_Channels `#$FF`
- var_ch_DPCM_EffPitch `#$FF`
- var_ch_DPCMDAC `#$FF`
- var_ch_LengthCounter `#$08`
- var_ch_NoteRelease `#$00`
- var_ch_Transpose `#$00`
- var_ch_NoteCut `#$00`
- var_ch_Effect `#$00`
- var_ch_EffParam `#$00`
- var_ch_PortaToLo `#$00`
- var_ch_PortaToHi `#$00`
- var_ch_TimerPeriodLo `#$00`
- var_ch_TimerPeriodHi `#$00`
- var_ch_Trigger `#$00`
- var_ch_VibratoPos `#$00` (old vibrato)
- var_ch_VibratoPos `#$48` (new vibrato)
- var_ch_Note `#$80`

variables init by `ft_load_song`

- var_Song_list (address of song list)
- var_Wavetables (address of wavetable data)
- var_Tempo_Dec (region speed divider)
- var_NamcoChannels, var_NamcoChannelsReg (n163 channel count)

variables init by `ft_load_track`

- var_PlayerFlags `#$01`
- var_ch_VolColumn `#$7F`
- var_ch_VolDefault `#$7F`
- var_ch_FinePitch `#$80`
- var_ch_VolSlideTarget `#$80`
- var_ch_VibratoSpeed `#$00`
- var_ch_TremoloSpeed `#$00`
- var_ch_Effect `#$00`
- var_ch_VolSlide `#$00`
- var_ch_NoteDelay `#$00`
- var_ch_ArpeggioCycle `#$00`
- var_ch_PhaseReset `#$00`
- var_ch_DPCMPhaseReset `#$00`
- var_ch_Harmonic `#$01`
- var_ch_Note `#$01`
- var_ch_PrevFreqHigh `#$FFFF`
- var_ch_PrevFreqHighMMC5 `#$FFFF`
- var_Current_Frame

variables init by `ft_load_frame`

- var_ch_PatternAddrLo, var_ch_PatternAddrHi
- var_ch_NoteDelay `#$00`
- var_ch_Delay `#$00`
- var_ch_DefaultDelay `#$FF`
