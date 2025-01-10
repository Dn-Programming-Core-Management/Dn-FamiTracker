# Tracker player update sequence

Documents the update sequence of the player in the tracker.
```cpp
CSoundGen::PlayChannelNotes();
    CSoundGen::PlayNote();
        CSoundGen::EvaluateGlobalEffects();
        CChannelHandler::HandleDelay();
        CChannelHandler::HandleNoteData();
            // (handle echo buffer)
                CChannelHandler::WriteEchoBuffer();
            // (clear note cut and note release)
            // (handle effects)
                CChannelHandler::HandleEffect();
            // (handle volume command and Nxy)
            // (handle instrument command)
            switch (pNoteData->Note) {
                case NONE:
                    HandleEmptyNote();
                    break;
                case HALT:
                    m_bRelease = false;
                    HandleCut();
                    break;
                case RELEASE:
                    HandleRelease();
                    break;
                default:
                    HandleNote(pNoteData->Note, pNoteData->Octave);
                    break;
            }
            // (handle note slide)
            if (new_instrument || note_trigger)
                CChannelHandler::HandleInstrument();
                    if (new_instrument)
                        m_pInstHandler->LoadInstrument(pInstrument);
                    if (note_trigger)
                        m_pInstHandler->TriggerInstrument();
CSoundGen::UpdatePlayer();
CSoundGen::UpdateChannels();  // run instruments, effects, etc.
    CChannelHandler::ProcessChannel();
        UpdateDelay();
        UpdateNoteCut();
        UpdateNoteRelease();
        UpdateNoteVolume();           // // //
        UpdateTranspose();            // // //
        if (m_iVolSlideTarget < 0)    // // !!
            UpdateVolumeSlide();
        else
            UpdateTargetVolumeSlide();
        UpdateVibratoTremolo();
        UpdateEffects();
        if (m_pInstHandler) m_pInstHandler->UpdateInstrument();       // // //
CSoundGen::UpdateAPU();  // write to registers
	// this function also happens to blit the current audio buffer
	// to either the wave output or the sound driver
```