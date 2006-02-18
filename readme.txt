Nintendulator 0.960

------------
Introduction
------------
Nintendulator started out as NinthStar NES, written by David "Akilla"
De Regt. Written in C++, it was a reasonably accurate (and slow) NES
emulator which used NESten 0.61's mapper DLLs. Numerous other systems
were planned to be emulated within NinthStar (as well as complex
debuggers for each of them), but somewhere along the line, the project
was abandoned.

At that point, I took the existing NES sources and started improving
them. First, the PPU was rewritten to be much more accurate than
before, running cycle-by-cycle according to documentation that had been
released at the time. After that, the CPU was rewritten to execute
instructions more accurately. Then the APU was mostly completed, giving
the emulator proper sound. Somewhere along the line, it was determined
that the C++ usage in the code was very poorly done and was slowing the
program down, so I converted it to plain C and named the program
"Nintendulator".

The eventual goal of Nintendulator is to be *the* most accurate NES
emulator, right down to the hardware quirks. In the meanwhile, it can
certainly be used to test NES code with confidence that if it works
properly in Nintendulator, it will probably work properly on the real
hardware as well.

---------------
Version History
---------------

0.960
- Overall:
   Added proper Unicode support to Nintendulator - as such, there are
      now separate ANSI and Unicode builds.
- APU:
   Various optimizations to the pulse channels
   Removed the sound logging feature from the code, since AVI output
      allows sound logging easily.
   Re-enabled frame IRQs on reset - the only games that had problems
      with these were demos.
- Controllers:
   Added a global option to enable Left+Right and Up+Down in normal
      gameplay, rather than only allowing it during movie recording.
   Cleaned up device handling somewhat.
   Fixed several bugs with device enabling.
   Moved movie-related code into its own file.
   Fixed various controller 'Frame' procedures to use the MovieMode
      parameter rather than the global setting.
   Improved cursor positioning for the Oeka Kids Tablet controller.
   Updated Zapper support to behave more closely to the real thing
- Debugger:
   Enabling debugger with 1X size no longer sets the size multiplier to
      2x - when closing the debugger, it will now revert to 1X properly.
   Various minor fixes.
- Game Genie:
   Cleaned up initialization code.
   Updated Genie Enable code to use the 'optimized' code handlers.
   On savestate load, any active game genie codes will be reported in
      the Status window.
- Graphics:
   Graphics resources are now unloaded when an invalid bit depth is
      detected - this should prevent the emulator from crashing.
   Enumerated all available palettes to reduce the possibility of
      introducing bugs when newer VS palettes are added.
   Added dark grays to the 14th column of the Playchoice-10 palette.
   Added the palette for the VS unisystem PPU "RP2C04-0001".
   Cleaned up NTSC palette generation code slightly.
   Improved Colour Emphasis coefficients to more closely match the real
      hardware.
- Mapper Interface:
   Added the ability to select which DLL to use when multiple DLLs
      support the current game's mapper.
   Removed the "Full Reset" reset type.
   Added Unicode support, denoted by setting the high order bit of the
      CurrentMapperInterface variable.
   Moved mapper 'Load' call from NES "File Open" code into LoadMapper.
- NES:
   Fixed a bug where changing between NTSC and PAL emulation would load
      the "Custom Palette" for the wrong mode.
   Fixed a multithreading bug in the main emulation routine.
   Fixed a bug in the Load Settings code to perform special mapping for
      the Four-Score instead of the Power Pad.
- Main Program:
   Fixed command-line parameter handling to behave correctly when the
      program and/or ROM filenames are enclosed in quotes.
- PPU:
   Added a variable to keep track of fractional cycles in PAL emulation.
      For compatibility, this variable is stored in the upper bits of
      the "ClockTicks" state variable.
   Added preliminary code (currently disabled) to accurately emulate
      the sprite evaluation process.
   Adjusted the initial VRAM addres load to occur at cycle 304 on
      scanline -1.
   Delayed VRAM address horizontal component reload until cycle 257.
   Fixed a bug in Sprite 0 Hit handling during frameskip.
- Savestates:
   Moved controller savestate code into the Controllers module.
   Moved movie savestate code into the new Movie module.

- Mappers:
   Added a new mapper DLL for VS Unisystem games.
   Increased MMC3/MMC6 IRQ counter threshold for PPU address changes.
   Removed noise emulation from FME-7 sound code.
   Updated MMC5 configuration dialog to list unconfirmed Famicom game
      titles in Japanese.
   Improved mapper 32 slightly.
   Updated mapper 45 (and BMC-Super1Min1 UNIF board) to support RAM
      at $6000-$7FFF.
   Updated mapper 95 to use Namco 118 instead of MMC3.
   Made vast improvements to mapper 100 (debugging mapper).
   Upgraded MMC5 mappers to full compatibility.
   Added iNES mappers 74, 88, 116, 125, and 188.
   Added UNIF boards NES-SFROM, NES-TL1ROM. and UNL_A9712.
   Fixed UNL-H2288 board (Earthworm Jim 2 pirate) to work correctly.

0.950
- APU:
   Fixed behaviour of square channel pitch bends (silences properly in
      Mega Man games, Codemasters intros, etc.)
   Fixed behaviour pf square/noise channel volume envelope
   Fixed behaviour of triangle channel linear counter
   Rewrote DPCM IRQ code to work correctly
   Adjusted DPCM sample fetch to take up to 4 cycles
   Reduced PCM volume by 25%
   Reading $4015 now correctly acknowledges a DPCM IRQ
   Fixed behaviour of frame counter when in 5-stage mode
   Disabled frame IRQs on reset (fixes some games)
   Added full APU state information to savestate files
   Added simple linear downsampling filter to audio playback (improves
      noise channel)
   Audio output is now clamped to prevent clipping
   Improved audio playback code somewhat, should react more smoothly
      to brief system slowdowns
- Controllers:
   Added SNES controller (compatible with NES controller in most games,
      can be used with homebrew games designed for it)
   Added alternate Famicom keyboard (used in "Study & Game 32-in-1"
      multicart)
   Added Family Trainer (similar to Power Pad)
   Added Oeka Kids drawing tablet
   Individual controllers are now updated exactly once per frame, to
      ensure consistent results
   Redesigned controller configuration, now prompts to press a
      button/key/axis instead of having to select it from a list
   Made controller configuration much more flexible, allowing joypad
      axes to be used for ANY controller button
   Controllers are now kept acquired at all times, allowing players to
      control the game while the emulator is not selected.
- CPU:
   Removed 'B' (breakpoint) flag from processor status
   Set 'I' flag upon receiving NMI and RESET
   Various optimizations to individual instructions
   Updated sprite DMA to take 513 cycles instead of 512
   Fixed minor bugs in NMI and IRQ handling
   Implemented most of the 'useful' invalid opcodes
- Debugger:
   Changed window positions to be relative to each other, rather than
      fixed
   Debugger now forces window size to at least 2X size, removed
      half-size nametable viewer.
   Fixed crash bug with placing breakpoint on top instruction line
   Scrolling through I/O registers in the disassembly window no longer
      reads from them (since it would alter their states)
   No longer causes graphical glitches under Windows 9x
   Added bank status for CPU banks 6/7 and PPU nametables
- Graphics:
   Auto-Frameskip now updates the selection in the menu
   NTSC palette is now generated at runtime
   Added the Playchoice-10 RGB palette as well as its own unique way of
      applying colour emphasis
   Added a palette configuration dialog, allowing users to select
      between a parameterized NTSC palette, a fixed PAL palette,
      the Playchoice-10 RGB palette, or one loaded from an external
      file (.PAL format, as supported by other emulators)
   Separate palette settings are stored for both NTSC and PAL
      emulation, though NTSC only uses one set of parameters
   Miscellaneous fixes
- Mappers:
   Redesigned mapper interface to provide additional functionality
   * PPU I/O handlers
   * Allow setting PPU open bus
   * Allow mapping nametables in arbitrary PPU regions
   * Allow mapping custom data into the CPU and PPU
   * Removed SRAM save/load handling
   * Changed IRQ to select the state (high/low) instead of sending a
        pulse
   * Replaced Mapper menu with a single callback to let the mapper open
        its own configuration dialog
   * Removed MMC5 Attribute Cache update operation, as Nintendulator
        does not use an attribute cache
   * Moved ROM-specific information into a separate 'ROMInfo'
        structure, passed to the DLL when a game is loaded. Added
        detailed information for all supported file types. ROMs are now
        loaded using this information, rather than by only specifying a
        mapper number or board name
   * Replaced mapper-specific Init/Unload functions with Load, Reset,
        and Unload operations.
   * Added a global 'UnloadMapper' operation
- NES:
   Moved emulation to a thread separate from UI handling
   Failure to open a ROM now gives a reason why it failed.
   Added "Auto-Run on Load" option
   Rewrote file parsing code for all supported ROM types
   Added disk writing support for FDS games
   NSF player 'control' code now resides in the mapper DLL rather than
      in the emulator itself
   NSF playback speed is now adjusted to the actual NTSC/PAL video
      rates rather than exactly 60Hz/50Hz.
   Switching from PAL to NTSC no longer causes the program to lock
   Various optimizations to Game Genie support
   Allow compile-time disabling of debugger for improved speed
   Changed default stretch to 2X
   Added the ability to slow down emulation by a particular amount
   Added the ability to pause emulation and advance one frame at a time
- Overall:
   Allow specifying a ROM filename on the command-line
   Allow opening a ROM by dragging it onto the program window
   Opening a menu, resizing the window, or opening the Load File dialog
      no longer results in the last sound segment playing over and over
   Added a status window, where various messages are reported
   Paths for ROMs, movies, palettes, etc. are saved between sessions
   Added an iNES header editor
- PPU: 
   Added I/O handlers for all PPU memory accesses
   Made all PPU memory accesses synchronous with PPU emulation
   Fixed palette access functionality
   Attempting to read from SPR-RAM while rendering now simulates what
      a real PPU would return, though it assumes no sprites are drawn
   Various timing fixes and optimizations
- Savestates:
   Created a new [custom] savestate format; SNSS had too many problems
      and did not store certain vital pieces of information
   * CPU and PPU state information are stored in separate blocks CPUS
        and PPUS instead of a single block BASR
   * Added numerous fields to CPU and PPU data stored
   * Dropped mirroring status from PPU block - the mapper state stores
        this
   * CHR-RAM and PRG-RAM no longer store null data at the end, saving
        space
   * Dropped "SRAM Writable" field - the mapper state stores this
   * Mapper data no longer stores PRG/CHR banks separately - the custom
        data field stores this in a manner appropriate to the mapper
   * Custom mapper state data no longer has a size limitation
   * Added storage for Game Genie status and FDS disk write information
   When selecting a savestate, the titlebar now also indicates whether
      a savestate is present in the slot
- Movie support:
   Proper movie support has been added with the following features:
   * Record from reset OR from where you are currently playing
   * Use (almost) any controllers you want
   * Save your state while recording a movie, then load the state to
        'rewind' back to that point
   * Watch an existing movie, then save and load a savestate to resume
        recording at that point
- AVI output:
   With a sufficiently fast processor, you can create an AVI of your
      gameplay. On slower systems, it is best to record a movie and
      then create the AVI while playing back the movie.
   Notes:
   * Supports video compression using any codecs available.
   * Currently does not support audio compression.

0.900 - Initial release

-------------------
Supporting Features 
-------------------

- Supports files of types .NES, .FDS, .NSF, and .UNIF
- Save States (custom format .NS0-.NS9)
- Movie Recording (custom format .NMV) with savestates
- SRAM saving (standard .SAV files)
- FDS disk writing (stored as differences in custom .FSV file)
- 4 player support, and beyond
- Ultra-accurate pixel-based rendering
- Emulates all internal sound channels, as well as most of
     the external Japanese ones (FDS, MMC5, VRC6, VRC7, FME-7,
     Namco 106)
- Support for all DirectInput controllers
- Built-in REAL Game Genie support
- External mapper plugins

------------
Known Issues
------------

- "Non-standard" sprite overflow behavior is not yet emulated
- PAL APU timings have not yet been verified
- Certain system hardware properties may not be emulated 100%
     correctly.
- Not all mappers are emulated perfectly.
- When recording movies, it is not possible to assign text to the
     'Description' field.
- When recording a movie and stopping, excess data at the end of the
     movie file is not truncated, though it is ignored during playback.
- If your CPU speed is below 1000MHz, emulation will likely be slow.

-------------------
Using Nintendulator
-------------------

Minimum system requirements for Nintendulator are a 1000MHz or faster
processor. All commands & options are available through menus, many
with shortcut keys.

File:
- Open (Ctrl+O)
   Browse to a ROM's location and open it. 
- Close
   Unloads the currently opened ROM.
- Edit iNES Header
   Allows you to open an iNES ROM image and edit its header to change
     its PRG/CHR ROM sizes, mapper number, and flags.
- Auto-Run
   Causes the emulator to immediately start emulation after a ROM is
     loaded. Useful if you don't want to have to press F2 every time
     you load a game.
- Exit (Alt+F4)
   Closes the program.

CPU:
- Run (F2)
   Starts emulation. After loading a ROM, you must use this to start it
     running unless you have Auto-Run enabled.
- Step (Shift+F2)
   Runs the CPU for exactly one instruction and then stops.
   Only useful when the Debugger is open.
- Stop (F3)
   Temporarily stops emulation. Use 'Run' to resume.
- Soft Reset (F4)
   Resets the game as if you had just pushed the 'reset' button on the
     NES. Some games behave differently to hard vs. soft resets.
- Hard Reset (Shift+F4)
   Resets the game as if you had just power-cycled the NES.
- Save State (F5)
   Saves the game's state.
- Load State (F8)
   Loads the game's state.
- Prev State (F6)
   Changes the savestate slot to the previous slot.
- Next State (F7)
   Changes the savestate slot to the next slot.
- Game Genie (Ctrl+G)
   Enables Game Genie support. Perform a Reset to access the code entry
     screen, then enter the codes as you would on a real Game Genie.
     Press Ctrl+G while the game is running to enable/disable the
     currently entered codes.
- Frame Step
   Allows you to pause a game and advance it one frame at a time while
     updating your controls. Useful for recording movies.

PPU:
- Frameskip
   Allows the emulator to skip rendering frames to improve speed.
     If you have a slow computer and cannot attain full speed, turn up
     frameskip to attempt to speed up emulation. Use Auto-Frameskip to
     let the emulator automatically adjust frameskip for best speed.
- Size
   Allows you to set the window size between 1X and 4X stretch.
- Mode
   Allows you to switch between NTSC (American and Japanese) and PAL
     (European) timing. If a game plays at the wrong speed, its music
     is off-pitch, or has timing-related graphical errors (i.e. screen
     scrolls at the wrong location), try switching modes.
- Palette...
   Allows you to choose a different set of colors. In NTSC mode,
     specify the Hue and Saturation to your liking; other modes do not
     use these settings. If you do not like the builtin palettes,
     choose 'Custom' and load a .PAL file.
   Nintendulator allows you to specify different palettes for NTSC and
     PAL emulation (though the NTSC palette uses the same parameters
     in both modes).
- Slowdown
   Allows you to slow down emulation in order to better control
     the game you are playing.

Sound Menu:
- Enabled
   Enables sound playback AND speed throttling - when sound playback is
     disabled, emulation will be done as fast as possible.

Input Menu:
- Setup
   Opens the Input configuration dialog. Select a device for each
     controller port and press the corresponding "Config" button to
     configure it. Selecting "Four-Score" for one controller port will
     automatically select it for the second port.
   When configuring a controller, select a device for each button from
     the appropriate dropdown lists and click the corresponding button.
     When the "Press a key..." indicator appears, press AND release the
     key/button/axis you wish to use.
   Controllers cannot be changed while a movie is playing or
     recording, though you can still adjust the key bindings.
   The option "Allow simultaneous Left+Right and Up+Down" is available
     to enable the ability to press two opposing direction buttons at
     the same time. Be aware that this causes glitches in some games!

Debug:
- Level 1 (Ctrl+F1)
   Disables the debugger entirely. This should be used for all actual
     gameplay.
- Level 2 (Ctrl+F2)
   Displays the current contents of the pattern tables and the palette.
- Level 3 (Ctrl+F3)
   Displays the current contents of the nametables. (includes Level 2)
- Level 4 (Ctrl+F4)
   Displays the game's code where it is currently executing and the
     status of all CPU registers. Also indicates PPU timing information
     as well as currently mapped program and character ROM/RAM banks.
   Breakpoints can be placed by clicking on an instruction.
   (includes Level 3)
- Status Window
   Opens the Debug Information window, where the emulator can display
     various status messages without having to pop up a dialog box.

Game:
   If the game you have selected has any special options, this menu
     will be enabled. Clicking on it will open a dialog box through
     which you may configure your game more closely.
   This is used mainly for NSF playback (to choose which song to play)
     and FDS games (to eject/insert disks).

Misc:
- Start AVI Capture
   Allows you to capture video and audio data to an AVI file.
   Choose a location to save the AVI file in, then specify a video
     codec.
- Stop AVI Capture
   Stops the active AVI capture.
- Play Movie
   Allows you to play back a previously recorded movie.
   Browse to the location the movie is stored and open it.
- Record Movie from Reset
   Allows you to record a movie starting from system reset. These have
     the advantage of being significantly smaller than movies recorded
     from a savestate.
   Choose a location to save the movie file in, then choose the
     controllers you wish to use - once recording starts, they cannot
     be changed!
- Record Movie from Current State
   Allows you to record a movie starting from the current system state.
     These tend to be larger than movies recorded from reset, but they
     can be used to demonstrate a particular part of a game without
     having to waste time reaching that point.
   Choose a location to save the movie file in, then choose the
     controllers you wish to use - once recording starts, they cannot
     be changed!
- Resume Movie
   Allows you to watch a movie up to a certain point, and then save and
      load a savestate to continue recording it. Recording is enabled
      the instant you load a compatible savestate.
   Browse to the location the movie is stored and open it.
- Stop Movie
   Stops any movie currently playing or recording.

Help:
- About
   Displays the obligatory About box.

----------------
Acknowledgements
----------------

TNSe - For writing the emulator NESten, allowing me to implement the
mappers using a simple yet powerful interface.

Akilla - For writing the emulator NinthStar NES, upon which this
emulator was based.

Kevtris - For lots of help figuring out various aspects of the NES's
hardware using his CopyNES and other tools.

Everyone else from EFnet #nesdev and the NESdev board, who are too
numerous to name.

-------
Contact
-------

Homepage - http://qmt.ath.cx/~nes/nintendulator
           http://nintendulator.sourceforge.net/ (redirect)

----------
Disclaimer
----------

NES, Family Computer, and FDS are registered trademarks of Nintendo.
Game Genie is a registered trademark of Galoob.

The author is not affiliated with Nintendo of America or any other
company mentioned, and does not encourage the piracy of NES games.

Nintendulator may be distributed according to the terms of the
GNU General Public License.
It may NOT be distributed with copyrighted ROM images.

You use this software at your own risk. The author is not responsible
for any loss or damage resulting from the use of this software. If you
do not agree with these terms, do not use this software.
