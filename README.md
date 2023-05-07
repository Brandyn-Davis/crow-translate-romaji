# ![Crow Translate logo](data/icons/app/48-apps-crow-translate.png) Crow Translate w/ Romaji

Converts Japanese source text to romaji in [Crow Translate](https://github.com/crow-translate/crow-translate) using [Cutlet](https://github.com/polm/cutlet)

<p align="center">
  <img src="https://raw.githubusercontent.com/Brandyn-Davis/crow-translate-romaji/master/data/screenshots/crow-romaji-screenshot.png">
</p>

## Details
**main.cpp** creates a Python file containing a function that returns romaji with Cutlet. **translationedit.cpp** then uses the Python/C API to run the Python file and displays the text

The code is from Crow Translate's crow-translate-2.10.4-source.tar.gz release which has all the submodules

## Releases
I have one Debian build in [Releases](https://github.com/Brandyn-Davis/crow-translate-romaji/releases)

I'll pay $15 to anyone who can get it running on Windows
