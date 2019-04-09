# vice3ds
Vice C64 emulator for Nintendo 3DS

## Compiling

You need devkitpro w/ devkitARM and the following packages: 3ds-sdl, 3ds-sdl_gfx, 3ds-sdl_image, 3ds-sdl_mixer, p7zip, 3ds-sdl_ttf, pkg-config and devkitpro-pkgbuild-helpers.

Install these packages with pacman / dkp-pacman:

devkitpro on windows:
    
    pacman -Sy 3ds-sdl 3ds-sdl_gfx 3ds-sdl_image 3ds-sdl_mixer 3ds-sdl_ttf p7zip pkg-config devkitpro-pkgbuild-helpers

devkitpro on Linux:
    
    [sudo] pacman -Sy 3ds-sdl 3ds-sdl_gfx 3ds-sdl_image 3ds-sdl_mixer 3ds-sdl_ttf devkitpro-pkgbuild-helpers
    [sudo] apt-get install p7zip pkg-config

Afterwards, the emulator can be compiled via

    make
