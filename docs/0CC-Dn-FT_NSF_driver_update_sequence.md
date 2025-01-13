# 0CC-Dn-FT NSF driver update sequence

This doc aims to detail the update order as observed in the NSF driver.

Ideally, the NSF driver should be the de-facto standard in how FT modules should
behave, but since the tracker has desynced in feature parity, this may not be
the case.

Regardless, this should serve as a reference as to how the driver updates in
sequence.

```
ft_music_play
    (delay handling)
    (tempo handling)
    ft_do_row_update
        (frame handling)
        ft_read_channels (foreach channels do:)
            ft_read_pattern
                ft_read_note
                    (switch case pattern command)
                        (handle volume commands)
                        (handle instrument commands)
                        (handle effect commands)
                    ft_push_echo_buffer
                    (handle note off)
                    (handle note release)
                    (load echo buffer)
                    ft_push_echo_buffer
                    (handle note)
            ft_read_is_done
    ft_skip_row_update (when no updates are available)
        (tempo handling)
    ft_loop_fx_state
        (Sxx handling)
        (delayed transpose/release handling)
    ft_loop_channels (foreach channels do:)
        ft_run_effects (handles the rest of the other effects)
        ft_run_instrument
        ft_calc_period
        (Nxy handling)
    ft_update_<chip> (foreach chip do:)
        (register writes)
```