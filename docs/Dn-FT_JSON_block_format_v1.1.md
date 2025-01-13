# Dn-FT JSON block format version 1.1
Must always be backwards compatible using a never remove, only add strategy.

Listed below are all the recognized keywords, with their default values.

**JSON data is only for optional settings, do not add any tracker or emulator crucial data here!**

```JSON
{
	// Device mixing offsets, described in centibels. too late to change to millibels.
	// range is +- 12 db.
	"apu1-offset": 0,
	"apu2-offset": 0,
	"fds-offset": 0,
	"mmc5-offset": 0,
	"n163-offset": 0,
	"s5b-offset": 0,
	"vrc6-offset": 0,
	"vrc7-offset": 0,

	// Use better mixing values derived from survey: https://forums.nesdev.org/viewtopic.php?f=2&t=17741
	"use-survey-mix": false
}
```