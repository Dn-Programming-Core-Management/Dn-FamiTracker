# 0CC vs FT effect type order

0CC for some reason uses a slightly different effects type order within the tracker, but converts to FT 050B+ effects type order when saved to a file.

In Dn-FT v.0.5.0.0, this conversion logic was disturbed, resulting in 0CC effects type order not being properly converted back to FT 050B+ when saving. This issue has been fixed in [commit df78460](https://github.com/Dn-Programming-Core-Management/Dn-FamiTracker/commit/df78460aae403daf2bb68891c788248bbc8a8a02).

For devs: please increment PATTERNS block version immediately.

```
After EF_SUNSOFT_ENV_LO (Jxx S5B),
FT order:
    EF_NOTE_RELEASE         (Lxx)
    EF_GROOVE               (Oxx)
    EF_TRANSPOSE            (Txy)
    EF_N163_WAVE_BUFFER     (Zxx N163)
    EF_FDS_VOLUME           (Exx FDS)
    EF_FDS_MOD_BIAS         (Zxx FDS)
    EF_SUNSOFT_NOISE        (Wxx S5B)
    EF_VRC7_PORT            (Hxx VRC7)
    EF_VRC7_WRITE           (Ixx VRC7)

0CC order:
    EF_SUNSOFT_NOISE        (Wxx S5B)
    EF_VRC7_PORT            (Hxx VRC7)
    EF_VRC7_WRITE           (Ixx VRC7)
    EF_NOTE_RELEASE         (Lxx)
    EF_GROOVE               (Oxx)
    EF_TRANSPOSE            (Txy)
    EF_N163_WAVE_BUFFER     (Zxx N163)
    EF_FDS_VOLUME           (Exx FDS)
    EF_FDS_MOD_BIAS         (Zxx FDS)
```
