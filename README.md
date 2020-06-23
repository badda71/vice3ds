![Screenshots](https://gbatemp.net/attachments/2-png.195186/ "vice3DS Screenshots")

# vice3ds
This is a port of the VICE C64 (x64) emulator v3.3 to Nintendo 3DS.
VICE - the Versatile Commodore Emulator - http://vice-emu.sourceforge.net/

## Installation
Install .cia file with FBI - or - copy .3dsx file to /3ds/vice3DS-C64 directory on your SD card and launch in HBL - or - launch .3ds file from flashcard
Apart from this, a DSP-dump is required for sound to work correctly in the CIA version.
https://gbatemp.net/threads/dsp1-a-new-dsp-dumper-cia-for-better-stability.469461/

## Download
https://github.com/badda71/vice3ds/releases

## Usage
Usage
Preconfigured button functions:

    Select: open Vice menu
    Start: open Gamebase64 launcher
    Directional Pad / A-Button: Joystick
    B-Button: Joystick Up
    R-button: Autofire. Within vice menu, R-button starts the mapping of a hotkey.
    C-Stick: cursor keys
    L/ZL-buttons: left/right mouse buttons if mouse is enabled.
    3D-slider: Emulation speed (0 = 100%, max = Warp-mode)
    Soft buttons functions (from left to right, top to bottom) :
        Autostart image
        Hit RUN/STOP
        Toggle joystick ports
        Toggle C-Pad / D-Pad for joystick
        Warp Mode

        Quickload snapshot
        Quicksave snapshot
        Toggle True Drive Emulation
        Hard Reset
        Pause emulation

        Attach disk image to drive 8
        Toggle Sprite-Sprite collisions
        Power off bottom screen backlight
        Type command: LOAD\"*\",8,1 - RUN
        Type command: LOAD\"$\",8 - LIST

        Hit RUN/STOP
        Hit SPACE
        Hit RETURN
        Enable Mouse
        Toggle No borders / Fullscreen

This (and a lot of other things) can be changed in Vice menu. To change the mapping of a soft button (or actually any other button incl. all the 3DS- and soft keyboard buttons) to a menu item in Vice menu, press the R-button when the menu item is selected, then touch/press the button. This button will now be mapped to the selected menu entry.

For further usage instructions, check here:
https://gbatemp.net/threads/release-vice3ds-c64-emulator.534830/

From v1.6 on, Vice3DS will run with acceptable framerates (~20 fps with fastSID emulation) on O3DS. However, a N3DS is recommended: Only here, you will get the full 50fps and incredible reSID sound.

## Compiling

You need devkitARM (provided by devkitPro) and the following packages: 3ds-sdl, 3ds-sdl_image, pkg-config, devkitpro-pkgbuild-helpers and graphicsmagick.

Install these packages with pacman / dkp-pacman:

Windows:
    
    pacman -Sy 3ds-sdl 3ds-sdl_image pkg-config devkitpro-pkgbuild-helpers graphicsmagick

Linux:
    
    [sudo] pacman -Sy 3ds-sdl 3ds-sdl_image devkitpro-pkgbuild-helpers
    [sudo] apt-get install pkg-config graphicsmagick

Apart from this, bannertool (https://github.com/Steveice10/bannertool/releases) and makerom (https://github.com/profi200/Project_CTR/releases) should be available in your path.

Afterwards, the emulator can be compiled via

    make
