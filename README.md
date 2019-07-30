# vice3ds
Vice C64 emulator for Nintendo 3DS

For usage instructions, check here:
https://gbatemp.net/threads/release-vice3ds-c64-emulator.534830/

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
