```text
Dn-FamiTracker NSF Driver Copyright (C) 2020-2026 D.P.C.M.
0CC-FamiTracker NSF Driver Copyright (C) 2014-2018 HertzDevil
FamiTracker NSF music driver Copyright (C) 2005-2015 Jonathan Liss
```

The original FT NSF driver's source code is explicitly *not* under the
GPL.[^1][^2][^3]

It is instead source-available, under no explicit license. Based on its usecase,
and jsr's comments [^4], it is presumed that distribution and modification is
permitted, and attribution is recommended.

[^1]: <http://famitracker.com/forum/posts.php?id=787#5149>
[^2]: <http://forums.famitracker.com/viewtopic.php?t=2638>
[^3]: <http://famitracker.com/forum/posts.php?id=4337#44063>
[^4]: <http://famitracker.com/forum/posts.php?id=4337#44083>

0CC-FamiTracker's modified NSF driver source comes with a
[GPL v2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html) license
file. Although
[the output of the program is not covered by the GNU GPL](https://www.gnu.org/licenses/gpl-faq.en.html#WhatCaseIsOutputGPL),
the NSF/ROM output might be considered as an executable binary, which in theory
may restrict anyone sharing compiled NSFs and binaries without sharing the
assembly source, according to section 3.

Dn-FamiTracker's modifications are under [MIT-0](../../../LICENSE-MIT-0.txt).

Dn-FamiTracker NSF driver source as a whole is effectively under the
[GPL v2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html) license.
