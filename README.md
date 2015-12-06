PuTTY Patched
=============

This is a mirror of Simon's git repository with my PuTTY
patches in the putty-rdb branch.

The [master branch](https://github.com/rdebath/PuTTY/tree/master) of this
repository is a clone of the master from Simon's PuTTY repo at
[git://git.tartarus.org/simon/putty.git](http://tartarus.org/~simon-git/gitweb/?p=putty.git)
and I am pushing Binaries to [here](https://github.com/rdebath/PuTTY/tree/binary).

*NOTE* ...
I'm pushing to this repository using rebase to keep each patch as a single commit.

Bugfixes
========
* Fix WM_NETEVENT delayed processing.
* Repair handling of WM_NETEVENT (part 2)
* Repair GTK timer handling
* Minimise, maximise, etc are as annoying as other terminal resize operations.
* Adjust DEC-MCS character set.
* This is a fix for the direct to font DBCS cursor.
* Windows codepages do not have C1 controls
* Fix the scoasc translation on unix
* Function x11font_draw_text uses the UCS2 character set
* SCO Ansi Keyboard correction
* Make sure all ISO-2022 codes are checked.
* Timer starvation fix
* Fix support of Windows 256 colour palettes.

Performance Improvements
========================
* Pango font performance update
* Optimisation to delete windows code on unix
* Windows font drawing change.
* Don't let dopaint eat too much CPU
* Remove unneeded special case code for proportional fonts.

Small Features
==============
* Add XTerm extensions for VTTEST and reverse BS
* Add new interpretation for XTerm SGR 38/48
* Add XTerm/Konsole truecolour mode.
* Line character set for Window title on windows.
* Add wcwidth override sequence
* Add Ansi REP sequence and SCO CSI = g
* Add DIM, CONCEAL, BGBOLD and Blink control.
* Add Linux default attribute set.
* Adding better support for Windows MBCS (codepages)
* Add Linux default attribute set. Comments
* Reconfigure PuTTY colour scheme from Unix host.

Large Features
==============
* UTF-8 processing update

Code Cleanups
=============
* Changes to the internal usage of unicode areas for character set conversion.
* Clean up VT52 leftovers from the 256 colours patch
* Waaay overkill on uuid generation
* Changes for compiling under MinGW

Obviously, some of these will depend on earlier changes *plus* I am only
testing the full set so partial sets may have significant bugs.
