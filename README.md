# vice3ds
Vice C64 emulator for Nintendo 3DS

For usage instructions, check here:
https://gbatemp.net/threads/release-vice3ds-c64-emulator.534830/

## Compiling

You need devkitARM (provided by devkitPro) and the following packages: 3ds-sdl, 3ds-sdl_gfx, 3ds-sdl_image, 3ds-sdl_mixer, p7zip, 3ds-sdl_ttf, pkg-config, devkitpro-pkgbuild-helpers and graphicsmagick.

Install these packages with pacman / dkp-pacman:

Windows:
    
    pacman -Sy 3ds-sdl 3ds-sdl_gfx 3ds-sdl_image 3ds-sdl_mixer 3ds-sdl_ttf p7zip pkg-config devkitpro-pkgbuild-helpers graphicsmagick

Linux:
    
    [sudo] pacman -Sy 3ds-sdl 3ds-sdl_gfx 3ds-sdl_image 3ds-sdl_mixer 3ds-sdl_ttf devkitpro-pkgbuild-helpers
    [sudo] apt-get install p7zip pkg-config graphicsmagick

Apart from this, bannertool (https://github.com/Steveice10/bannertool/releases) and makerom (https://github.com/profi200/Project_CTR/releases) should be available in your path.

Afterwards, the emulator can be compiled via

    make
