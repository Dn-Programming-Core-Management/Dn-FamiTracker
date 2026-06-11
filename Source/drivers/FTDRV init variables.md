# Initialized variables

last updated: 2026-06-11

---

## variables init by `ft_music_init`

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
- var_ch_VibratoPos `#$00` (old vibrato mode)
- var_ch_VibratoPos `#$48` (new vibrato mode)
- var_ch_Note `#$80`

### chip-specific init routines

- var_ch_vrc7_PatchFlag
- var_ch_vrc7_EffPatch
- var_ch_FDSVolume
- var_ch_ModBias
- var_ch_DutyDefault (S5B)
- var_Noise_Default
- var_Noise_Period
- var_Noise_Prev
- var_Pul_Noi
- var_EnvelopeRate
- var_EnvelopeType
- var_EnvelopeEnabled
- var_ch_WaveLen

## variables init by `ft_load_song`

- var_Song_list (address of song list)
- var_Wavetables (address of wavetable data)
- var_Tempo_Dec (region speed divider)
- var_NamcoChannels, var_NamcoChannelsReg (n163 channel count)
- var_EffChannels
- var_AllChannels
- var_Tempo_Accum `#$0000`
- var_Tempo_Count (in `ft_calculate_speed`)
- var_Tempo_Modulus (in `ft_calculate_speed`)
- var_ch_TremoloSpeed

## variables init by `ft_load_track`

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
- var_Current_Frame `#$FF`
- var_Frame_List (address of frame list)
- var_Frame_Count (frame count of track)

## variables init by `ft_load_frame`

- var_ch_PatternAddrLo, var_ch_PatternAddrHi (address of pattern)
- var_ch_NoteDelay `#$00` (separate init?)
- var_ch_Delay `#$00`
- var_ch_DefaultDelay `#$FF`
- var_Pattern_Pos `#$00`
- var_Jump `#$00`
- var_Skip `#$00`


## variables initialized during use (RAII)

- Scratch variables:
    - var_Temp
    - var_Temp2
    - var_Temp3
    - var_Temp4
    - var_Temp16
    - var_Temp_Pointer
    - var_Temp_Pointer2
    - var_Temp_Pattern
    - var_Note_Table
    - var_currentChannel
    - ACC, AUX, EXT (16-bit mul/div routine variables)
- APU variables
    - var_Sweep
- DPCM variables
    - var_ch_SamplePtr
    - var_ch_SampleLen
    - var_ch_SampleBank
    - var_ch_SamplePitch
    - var_ch_DPCM_Retrig
    - var_ch_DPCM_RetrigCntr
- VRC7 variables
    - var_ch_vrc7_FnumLo
    - var_ch_vrc7_FnumHi
    - var_ch_vrc7_Bnum
    - var_ch_vrc7_ActiveNote
    - var_ch_vrc7_Command
    - var_ch_vrc7_OldOctave
    - var_ch_vrc7_CustomLo, var_ch_vrc7_CustomHi (custom patch pointer temp var)
    - var_CustomPatchPtr
    - var_ch_vrc7_Port
    - var_ch_vrc7_Write
- FDS variables
    - var_ch_ModDelay
    - var_ch_ModDepth
    - var_ch_ModRate
    - var_ch_ModDelayTick
    - var_ch_ModEffDepth
    - var_ch_ModEffRate
    - var_ch_ModInstDepth
    - var_ch_ModInstRate
    - var_ch_ModEffWritten (during `ft_music_play`)
    - var_ch_ModTable
- N163 variables
    - var_ch_WavePtrLo, var_ch_WavePtrHi
    - var_ch_WavePos, var_ch_WavePosOld
    - var_NamcoInstrument
- S5B variables
    - var_EnvelopeTrigger
- Global player
    - var_Song_list
    - var_SongFlags
    - var_Speed
    - var_Tempo
    - var_GroovePointer
    - var_VolumeSlideStarted
    - var_sequence_result
    - var_ch_Bank (pattern bank)
    - var_ch_VolDelay
    - var_ch_State
    - var_ch_PeriodCalcLo, var_ch_PeriodCalcHi
    - var_ch_DutyCurrent (through `ft_run_instrument`)
    - var_ch_Sweep
    - var_ch_SeqVolume, var_ch_SeqArpeggio, var_ch_SeqPitch, var_ch_SeqHiPitch, var_ch_SeqDutyCycle
    - var_ch_Volume
    - var_ch_SequencePtr1, var_ch_SequencePtr2, var_ch_SequencePtr3, var_ch_SequencePtr4, var_ch_SequencePtr5
    - var_ch_InstType
    - var_ch_ArpFixed
    - var_ch_TremoloResult

## where are they initialized??

- var_Instrument_list
- var_dpcm_inst_list
- var_dpcm_pointers
- var_Groove_Table
- var_Pattern_Length
- var_InitialBank
- var_Load_Frame
- var_ch_TremoloPos
- var_ch_TremoloDepth
- var_ch_EchoBuffer

## unused

- var_sequence_ptr