/*
 * menu_video.c - SDL video settings menus.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"
#include "types.h"

#include "fullscreenarch.h"
#include "lib.h"
#include "interrupt.h"
#include "machine.h"
#include "menu_common.h"
#include "menu_video.h"
#include "palette.h"
#include "resources.h"
#include "ted.h"
#include "ui.h"
#include "uibottom.h"
#include "uifilereq.h"
#include "uimenu.h"
#include "vic.h"
#include "vicii.h"
#include "videoarch.h"
#include "vice3ds.h"
#include <3ds.h>

extern volatile bool app_pause; // this pauses the SDL update thread

static ui_menu_entry_t *palette_dyn_menu1 = NULL;
static ui_menu_entry_t *palette_dyn_menu2 = NULL;

static ui_menu_entry_t palette_menu1[4];
static ui_menu_entry_t palette_menu2[4];

#define UI_MENU_DEFINE_SLIDER_VIDEO(resource, min, max)                                  \
    static UI_MENU_CALLBACK(slider_##resource##_callback)                          \
    {                                                                              \
        return sdl_ui_menu_video_slider_helper(activated, param, #resource, min, max);   \
    }

/* Border mode menu */

UI_MENU_DEFINE_RADIO_TRAP(VICIIBorderMode)
UI_MENU_DEFINE_RADIO_TRAP(VICBorderMode)
UI_MENU_DEFINE_RADIO_TRAP(TEDBorderMode)

static const ui_menu_entry_t vicii_border_menu[] = {
    { "Normal",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICIIBorderMode_callback,
      (ui_callback_data_t)VICII_NORMAL_BORDERS },
    { "Full",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICIIBorderMode_callback,
      (ui_callback_data_t)VICII_FULL_BORDERS },
    { "Debug",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICIIBorderMode_callback,
      (ui_callback_data_t)VICII_DEBUG_BORDERS },
    { "None",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICIIBorderMode_callback,
      (ui_callback_data_t)VICII_NO_BORDERS },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vic_border_menu[] = {
    { "Normal",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICBorderMode_callback,
      (ui_callback_data_t)VIC_NORMAL_BORDERS },
    { "Full",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICBorderMode_callback,
      (ui_callback_data_t)VIC_FULL_BORDERS },
    { "Debug",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICBorderMode_callback,
      (ui_callback_data_t)VIC_DEBUG_BORDERS },
    { "None",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICBorderMode_callback,
      (ui_callback_data_t)VIC_NO_BORDERS },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t ted_border_menu[] = {
    { "Normal",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_TEDBorderMode_callback,
      (ui_callback_data_t)TED_NORMAL_BORDERS },
    { "Full",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_TEDBorderMode_callback,
      (ui_callback_data_t)TED_FULL_BORDERS },
    { "Debug",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_TEDBorderMode_callback,
      (ui_callback_data_t)TED_DEBUG_BORDERS },
    { "None",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_TEDBorderMode_callback,
      (ui_callback_data_t)TED_NO_BORDERS },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_TOGGLE(VICIIVSPBug)

/* audio leak */

UI_MENU_DEFINE_TOGGLE(VICIIAudioLeak)
UI_MENU_DEFINE_TOGGLE(VDCAudioLeak)
UI_MENU_DEFINE_TOGGLE(CrtcAudioLeak)
UI_MENU_DEFINE_TOGGLE(TEDAudioLeak)
UI_MENU_DEFINE_TOGGLE(VICAudioLeak)

/* CRT emulation menu */
/*
UI_MENU_DEFINE_SLIDER_VIDEO(VICPALScanLineShade, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICPALBlur, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICPALOddLinePhase, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICPALOddLineOffset, 0, 2000)

UI_MENU_DEFINE_SLIDER_VIDEO(VDCPALScanLineShade, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(VDCPALBlur, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(VDCPALOddLinePhase, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VDCPALOddLineOffset, 0, 2000)

UI_MENU_DEFINE_SLIDER_VIDEO(VICIIPALScanLineShade, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICIIPALBlur, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICIIPALOddLinePhase, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICIIPALOddLineOffset, 0, 2000)

UI_MENU_DEFINE_SLIDER_VIDEO(TEDPALScanLineShade, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(TEDPALBlur, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(TEDPALOddLinePhase, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(TEDPALOddLineOffset, 0, 2000)

UI_MENU_DEFINE_SLIDER_VIDEO(CrtcPALScanLineShade, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(CrtcPALBlur, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(CrtcPALOddLinePhase, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(CrtcPALOddLineOffset, 0, 2000)

#define VICE_SDL_CRTEMU_MENU_ITEMS(chip)                        \
    { "Scanline shade",                                         \
      MENU_ENTRY_RESOURCE_INT,                                  \
      slider_##chip##PALScanLineShade_callback,                 \
      (ui_callback_data_t)"Set PAL shade (0-1000)" },           \
    { "Blur",                                                   \
      MENU_ENTRY_RESOURCE_INT,                                  \
      slider_##chip##PALBlur_callback,                          \
      (ui_callback_data_t)"Set PAL blur (0-1000)" },            \
    { "Oddline phase",                                          \
      MENU_ENTRY_RESOURCE_INT,                                  \
      slider_##chip##PALOddLinePhase_callback,                  \
      (ui_callback_data_t)"Set PAL oddline phase (0-2000)" },   \
    { "Oddline offset",                                         \
      MENU_ENTRY_RESOURCE_INT,                                  \
      slider_##chip##PALOddLineOffset_callback,                 \
      (ui_callback_data_t)"Set PAL oddline offset (0-2000)" }

static const ui_menu_entry_t vic_crt_controls_menu[] = {
    VICE_SDL_CRTEMU_MENU_ITEMS(VIC),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vicii_crt_controls_menu[] = {
    VICE_SDL_CRTEMU_MENU_ITEMS(VICII),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vdc_crt_controls_menu[] = {
    VICE_SDL_CRTEMU_MENU_ITEMS(VDC),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t ted_crt_controls_menu[] = {
    VICE_SDL_CRTEMU_MENU_ITEMS(TED),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t crtc_crt_controls_menu[] = {
    VICE_SDL_CRTEMU_MENU_ITEMS(Crtc),
    SDL_MENU_LIST_END
};
*/

/* Color menu */

UI_MENU_DEFINE_SLIDER_VIDEO(VICColorGamma, 0, 4000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICColorTint, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICColorSaturation, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICColorContrast, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICColorBrightness, 0, 2000)

UI_MENU_DEFINE_SLIDER_VIDEO(VICIIColorGamma, 0, 4000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICIIColorTint, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICIIColorSaturation, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICIIColorContrast, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICIIColorBrightness, 0, 2000)

UI_MENU_DEFINE_SLIDER_VIDEO(VDCColorGamma, 0, 4000)
UI_MENU_DEFINE_SLIDER_VIDEO(VDCColorTint, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VDCColorSaturation, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VDCColorContrast, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VDCColorBrightness, 0, 2000)

UI_MENU_DEFINE_SLIDER_VIDEO(TEDColorGamma, 0, 4000)
UI_MENU_DEFINE_SLIDER_VIDEO(TEDColorTint, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(TEDColorSaturation, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(TEDColorContrast, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(TEDColorBrightness, 0, 2000)

UI_MENU_DEFINE_SLIDER_VIDEO(CrtcColorGamma, 0, 4000)
UI_MENU_DEFINE_SLIDER_VIDEO(CrtcColorTint, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(CrtcColorSaturation, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(CrtcColorContrast, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(CrtcColorBrightness, 0, 2000)

#define VICE_SDL_COLOR_MENU_ITEMS(chip)                        \
    { "Gamma",                                                 \
      MENU_ENTRY_RESOURCE_INT,                                 \
      slider_##chip##ColorGamma_callback,                      \
      (ui_callback_data_t)"Set gamma (0-4000)" },              \
    { "Tint",                                                  \
      MENU_ENTRY_RESOURCE_INT,                                 \
      slider_##chip##ColorTint_callback,                       \
      (ui_callback_data_t)"Set tint (0-2000)" },               \
    { "Saturation",                                            \
      MENU_ENTRY_RESOURCE_INT,                                 \
      slider_##chip##ColorSaturation_callback,                 \
      (ui_callback_data_t)"Set saturation (0-2000)" },         \
    { "Contrast",                                              \
      MENU_ENTRY_RESOURCE_INT,                                 \
      slider_##chip##ColorContrast_callback,                   \
      (ui_callback_data_t)"Set contrast (0-2000)" },           \
    { "Brightness",                                            \
      MENU_ENTRY_RESOURCE_INT,                                 \
      slider_##chip##ColorBrightness_callback,                 \
      (ui_callback_data_t)"Set brightness (0-2000)" }

static const ui_menu_entry_t vic_color_controls_menu[] = {
    VICE_SDL_COLOR_MENU_ITEMS(VIC),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vicii_color_controls_menu[] = {
    VICE_SDL_COLOR_MENU_ITEMS(VICII),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vdc_color_controls_menu[] = {
    VICE_SDL_COLOR_MENU_ITEMS(VDC),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t ted_color_controls_menu[] = {
    VICE_SDL_COLOR_MENU_ITEMS(TED),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t crtc_color_controls_menu[] = {
    VICE_SDL_COLOR_MENU_ITEMS(Crtc),
    SDL_MENU_LIST_END
};

/* Size menu template */

UI_MENU_DEFINE_INT(SDLCustomWidth)
UI_MENU_DEFINE_INT(SDLCustomHeight)
#ifndef USE_SDLUI2    
//UI_MENU_DEFINE_RADIO(SDLLimitMode)
#endif
//UI_MENU_DEFINE_INT(SDLWindowWidth)
//UI_MENU_DEFINE_INT(SDLWindowHeight)
    
UI_MENU_DEFINE_RADIO_TRAP(SDLFullscreenStretch)

#define VICE_SDL_SIZE_MENU_DOUBLESIZE(chip)

//	{ "Double size",                                
//      MENU_ENTRY_RESOURCE_TOGGLE,                   
//      toggle_##chip##DoubleSize_callback,           
//      NULL },

#define VICE_SDL_SIZE_MENU_STRETCHVERTICAL(chip)
//    { "Stretch vertically",                         
//      MENU_ENTRY_RESOURCE_TOGGLE,                   
//      toggle_##chip##StretchVertical_callback,      
//      NULL },

/* Removed entries
    { "Double scan",                                
      MENU_ENTRY_RESOURCE_TOGGLE,                   
      toggle_##chip##DoubleScan_callback,           
      NULL },                                       
*/

#define VICE_SDL_SIZE_MENU_ITEMS_SHARED(chip)       \
    { "Fullscreen",                                 \
      MENU_ENTRY_RESOURCE_TOGGLE,                   \
      toggle_##chip##Fullscreen_callback,           \
      NULL },                                       \
    SDL_MENU_ITEM_SEPARATOR,                        \
    SDL_MENU_ITEM_TITLE("Fullscreen mode"),         \
    { "Automatic",                                  \
      MENU_ENTRY_RESOURCE_RADIO,                    \
      radio_##chip##SDLFullscreenMode_callback,     \
      (ui_callback_data_t)FULLSCREEN_MODE_AUTO },   \
    { "Custom",                                     \
      MENU_ENTRY_RESOURCE_RADIO,                    \
      radio_##chip##SDLFullscreenMode_callback,     \
      (ui_callback_data_t)FULLSCREEN_MODE_CUSTOM }, \
    SDL_MENU_ITEM_SEPARATOR,                        \
    SDL_MENU_ITEM_TITLE("Fullscreen stretch mode"), \
    { "Full",                                       \
      MENU_ENTRY_RESOURCE_RADIO,                    \
      radio_SDLFullscreenStretch_callback,          \
      (ui_callback_data_t)SDL_FULLSCREEN },         \
    { "Height",                                     \
      MENU_ENTRY_RESOURCE_RADIO,                    \
      radio_SDLFullscreenStretch_callback,          \
      (ui_callback_data_t)SDL_FITHEIGHT },          \
    { "Width",                                      \
      MENU_ENTRY_RESOURCE_RADIO,                    \
      radio_SDLFullscreenStretch_callback,          \
      (ui_callback_data_t)SDL_FITWIDTH },           \
    SDL_MENU_ITEM_SEPARATOR,                        \
	SDL_MENU_ITEM_TITLE("Custom resolution"),       \
    { "Width",                                      \
      MENU_ENTRY_RESOURCE_INT,                      \
      int_SDLCustomWidth_callback,                  \
      (ui_callback_data_t)"Set width" },            \
    { "Height",                                     \
      MENU_ENTRY_RESOURCE_INT,                      \
      int_SDLCustomHeight_callback,                 \
      (ui_callback_data_t)"Set height" },
#ifndef USE_SDLUI2
#define VICE_SDL_SIZE_MENU_ITEMS_LIMIT(chip)
/*
	SDL_MENU_ITEM_SEPARATOR,                        \
    SDL_MENU_ITEM_TITLE("Resolution limit mode"),   \
    { "Off",                                        \
      MENU_ENTRY_RESOURCE_RADIO,                    \
      radio_SDLLimitMode_callback,                  \
      (ui_callback_data_t)SDL_LIMIT_MODE_OFF },     \
    { "Max",                                        \
      MENU_ENTRY_RESOURCE_RADIO,                    \
      radio_SDLLimitMode_callback,                  \
      (ui_callback_data_t)SDL_LIMIT_MODE_MAX },     \
    { "Fixed",                                      \
      MENU_ENTRY_RESOURCE_RADIO,                    \
      radio_SDLLimitMode_callback,                  \
      (ui_callback_data_t)SDL_LIMIT_MODE_FIXED },
*/
#else
#define VICE_SDL_SIZE_MENU_ITEMS_LIMIT(chip)
#endif
#define VICE_SDL_SIZE_MENU_ITEMS_LATER_SHARED(chip)
/*
	SDL_MENU_ITEM_SEPARATOR,                        \
    SDL_MENU_ITEM_TITLE("Initial resolution"),      \
    { "Width",                                      \
      MENU_ENTRY_RESOURCE_INT,                      \
      int_SDLWindowWidth_callback,                  \
      (ui_callback_data_t)"Set width" },            \
    { "Height",                                     \
      MENU_ENTRY_RESOURCE_INT,                      \
      int_SDLWindowHeight_callback,                 \
      (ui_callback_data_t)"Set height" },
*/
#define VICE_SDL_SIZE_MENU_ITEMS(chip)              \
VICE_SDL_SIZE_MENU_ITEMS_SHARED(chip)               \
VICE_SDL_SIZE_MENU_ITEMS_LIMIT(chip)                \
VICE_SDL_SIZE_MENU_ITEMS_LATER_SHARED(chip)

#if defined(HAVE_HWSCALE) || defined(USE_SDLUI2)

UI_MENU_DEFINE_RADIO(SDLGLAspectMode)

static const ui_menu_entry_t aspect_menu[] = {
    { "Off",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_SDLGLAspectMode_callback,
      (ui_callback_data_t)SDL_ASPECT_MODE_OFF },
    { "Custom",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_SDLGLAspectMode_callback,
      (ui_callback_data_t)SDL_ASPECT_MODE_CUSTOM },
    { "True",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_SDLGLAspectMode_callback,
      (ui_callback_data_t)SDL_ASPECT_MODE_TRUE },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_RADIO(SDLGLFilter)

static const ui_menu_entry_t filter_menu[] = {
    { "Nearest",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_SDLGLFilter_callback,
      (ui_callback_data_t)SDL_FILTER_NEAREST },
    { "Linear",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_SDLGLFilter_callback,
      (ui_callback_data_t)SDL_FILTER_LINEAR },
    SDL_MENU_LIST_END
};

#ifndef USE_SDLUI2
UI_MENU_DEFINE_TOGGLE(VICIIHwScale)
UI_MENU_DEFINE_TOGGLE(VDCHwScale)
UI_MENU_DEFINE_TOGGLE(CrtcHwScale)
UI_MENU_DEFINE_TOGGLE(TEDHwScale)
UI_MENU_DEFINE_TOGGLE(VICHwScale)
#endif
UI_MENU_DEFINE_STRING(AspectRatio)
UI_MENU_DEFINE_TOGGLE(SDLGLFlipX)
UI_MENU_DEFINE_TOGGLE(SDLGLFlipY)

#ifndef USE_SDLUI2
#define VICE_SDL_SIZE_MENU_OPENGL_ITEMS(chip)               \
    SDL_MENU_ITEM_SEPARATOR,                                \
    SDL_MENU_ITEM_TITLE("OpenGL"),                          \
    { "OpenGL free scaling",                                \
      MENU_ENTRY_RESOURCE_TOGGLE,                           \
      toggle_##chip##HwScale_callback,                      \
      NULL },                                               \
    { "Fixed aspect ratio",                                 \
      MENU_ENTRY_SUBMENU,                                   \
      submenu_radio_callback,                               \
      (ui_callback_data_t)aspect_menu },                    \
    { "Custom aspect ratio",                                \
      MENU_ENTRY_RESOURCE_STRING,                           \
      string_AspectRatio_callback,                          \
      (ui_callback_data_t)"Set aspect ratio (0.5 - 2.0)" }, \
    { "Filter",                                             \
      MENU_ENTRY_SUBMENU,                                   \
      submenu_radio_callback,                               \
      (ui_callback_data_t)filter_menu },                    \
    { "Flip X",                                             \
      MENU_ENTRY_RESOURCE_TOGGLE,                           \
      toggle_SDLGLFlipX_callback,                           \
      NULL },                                               \
    { "Flip Y",                                             \
      MENU_ENTRY_RESOURCE_TOGGLE,                           \
      toggle_SDLGLFlipY_callback,                           \
      NULL },
#else
#define VICE_SDL_SIZE_MENU_OPENGL_ITEMS(chip)               \
    SDL_MENU_ITEM_SEPARATOR,                                \
    SDL_MENU_ITEM_TITLE("OpenGL"),                          \
    { "Fixed aspect ratio",                                 \
      MENU_ENTRY_SUBMENU,                                   \
      submenu_radio_callback,                               \
      (ui_callback_data_t)aspect_menu },                    \
    { "Custom aspect ratio",                                \
      MENU_ENTRY_RESOURCE_STRING,                           \
      string_AspectRatio_callback,                          \
      (ui_callback_data_t)"Set aspect ratio (0.5 - 2.0)" }, \
    { "Filter",                                             \
      MENU_ENTRY_SUBMENU,                                   \
      submenu_radio_callback,                               \
      (ui_callback_data_t)filter_menu },                    \
    { "Flip X",                                             \
      MENU_ENTRY_RESOURCE_TOGGLE,                           \
      toggle_SDLGLFlipX_callback,                           \
      NULL },                                               \
    { "Flip Y",                                             \
      MENU_ENTRY_RESOURCE_TOGGLE,                           \
      toggle_SDLGLFlipY_callback,                           \
      NULL },
#endif
#endif

// VICII size menu

//UI_MENU_DEFINE_TOGGLE(VICIIDoubleSize)
//UI_MENU_DEFINE_TOGGLE(VICIIDoubleScan)
UI_MENU_DEFINE_TOGGLE_TRAP(VICIIFullscreen)
UI_MENU_DEFINE_RADIO(VICIISDLFullscreenMode)

static const ui_menu_entry_t vicii_size_menu[] = {
    VICE_SDL_SIZE_MENU_DOUBLESIZE(VICII)
    VICE_SDL_SIZE_MENU_ITEMS(VICII)
#if defined(HAVE_HWSCALE) || defined(USE_SDLUI2)
    VICE_SDL_SIZE_MENU_OPENGL_ITEMS(VICII)
#endif
    SDL_MENU_LIST_END
};

// VDC size menu

//UI_MENU_DEFINE_TOGGLE(VDCDoubleSize)
//UI_MENU_DEFINE_TOGGLE(VDCStretchVertical)
//UI_MENU_DEFINE_TOGGLE(VDCDoubleScan)
UI_MENU_DEFINE_TOGGLE_TRAP(VDCFullscreen)
UI_MENU_DEFINE_RADIO(VDCSDLFullscreenMode)

static const ui_menu_entry_t vdc_size_menu[] = {
    VICE_SDL_SIZE_MENU_DOUBLESIZE(VDC)
    VICE_SDL_SIZE_MENU_STRETCHVERTICAL(VDC)
    VICE_SDL_SIZE_MENU_ITEMS(VDC)
#if defined(HAVE_HWSCALE) || defined(USE_SDLUI2)
    VICE_SDL_SIZE_MENU_OPENGL_ITEMS(VDC)
#endif
    SDL_MENU_LIST_END
};

// Crtc size menu

UI_MENU_DEFINE_TOGGLE_TRAP(CrtcFullscreen)
UI_MENU_DEFINE_RADIO(CrtcSDLFullscreenMode)
//UI_MENU_DEFINE_TOGGLE(CrtcDoubleSize)
//UI_MENU_DEFINE_TOGGLE(CrtcStretchVertical)
//UI_MENU_DEFINE_TOGGLE(CrtcDoubleScan)

static const ui_menu_entry_t crtc_size_menu[] = {
    VICE_SDL_SIZE_MENU_DOUBLESIZE(Crtc)
    VICE_SDL_SIZE_MENU_STRETCHVERTICAL(Crtc)
    VICE_SDL_SIZE_MENU_ITEMS(Crtc)
#if defined(HAVE_HWSCALE) || defined(USE_SDLUI2)
    VICE_SDL_SIZE_MENU_OPENGL_ITEMS(Crtc)
#endif
    SDL_MENU_LIST_END
};

// TED size menu

//UI_MENU_DEFINE_TOGGLE(TEDDoubleSize)
//UI_MENU_DEFINE_TOGGLE(TEDDoubleScan)
UI_MENU_DEFINE_TOGGLE_TRAP(TEDFullscreen)
UI_MENU_DEFINE_RADIO(TEDSDLFullscreenMode)

static const ui_menu_entry_t ted_size_menu[] = {
    VICE_SDL_SIZE_MENU_DOUBLESIZE(TED)
    VICE_SDL_SIZE_MENU_ITEMS(TED)
#if defined(HAVE_HWSCALE) || defined(USE_SDLUI2)
    VICE_SDL_SIZE_MENU_OPENGL_ITEMS(TED)
#endif
    SDL_MENU_LIST_END
};

// VIC size menu

//UI_MENU_DEFINE_TOGGLE(VICDoubleSize)
//UI_MENU_DEFINE_TOGGLE(VICDoubleScan)
UI_MENU_DEFINE_TOGGLE_TRAP(VICFullscreen)
UI_MENU_DEFINE_RADIO(VICSDLFullscreenMode)

static const ui_menu_entry_t vic_size_menu[] = {
    VICE_SDL_SIZE_MENU_DOUBLESIZE(VIC)
    VICE_SDL_SIZE_MENU_ITEMS(VIC)
#if defined(HAVE_HWSCALE) || defined(USE_SDLUI2)
    VICE_SDL_SIZE_MENU_OPENGL_ITEMS(VIC)
#endif
    SDL_MENU_LIST_END
};

/* Output Rendering Filter */
/*
UI_MENU_DEFINE_RADIO(VICIIFilter)
UI_MENU_DEFINE_RADIO(TEDFilter)
UI_MENU_DEFINE_RADIO(VICFilter)
UI_MENU_DEFINE_RADIO(VDCFilter)
UI_MENU_DEFINE_RADIO(CrtcFilter)

#define VICE_SDL_FILTER_MENU_ITEMS(chip)       \
    { "None",                                  \
      MENU_ENTRY_RESOURCE_RADIO,               \
      radio_##chip##Filter_callback,           \
      (ui_callback_data_t)VIDEO_FILTER_NONE }, \
    { "CRT Emulation",                         \
      MENU_ENTRY_RESOURCE_RADIO,               \
      radio_##chip##Filter_callback,           \
      (ui_callback_data_t)VIDEO_FILTER_CRT }

static const ui_menu_entry_t vicii_filter_menu[] = {
    VICE_SDL_FILTER_MENU_ITEMS(VICII),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t ted_filter_menu[] = {
    VICE_SDL_FILTER_MENU_ITEMS(TED),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vic_filter_menu[] = {
    VICE_SDL_FILTER_MENU_ITEMS(VIC),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t crtc_filter_menu[] = {
    VICE_SDL_FILTER_MENU_ITEMS(Crtc),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vdc_filter_menu[] = {
    VICE_SDL_FILTER_MENU_ITEMS(VDC),
    SDL_MENU_LIST_END
};
*/
/* Misc. callbacks */
UI_MENU_DEFINE_TOGGLE(VICIIVideoCache)
UI_MENU_DEFINE_TOGGLE(VDCVideoCache)
UI_MENU_DEFINE_TOGGLE(CrtcVideoCache)
UI_MENU_DEFINE_TOGGLE(TEDVideoCache)
UI_MENU_DEFINE_TOGGLE(VICVideoCache)
UI_MENU_DEFINE_TOGGLE(VICIIExternalPalette)
UI_MENU_DEFINE_TOGGLE(VDCExternalPalette)
UI_MENU_DEFINE_TOGGLE(CrtcExternalPalette)
UI_MENU_DEFINE_TOGGLE(TEDExternalPalette)
UI_MENU_DEFINE_TOGGLE(VICExternalPalette)
UI_MENU_DEFINE_RADIO(MachineVideoStandard)
UI_MENU_DEFINE_TOGGLE(VICIICheckSsColl)
UI_MENU_DEFINE_TOGGLE(VICIICheckSbColl)

/*
static UI_MENU_CALLBACK(restore_size_callback)
{
    if (activated) {
        sdl_video_restore_size();
    }
    return NULL;
}
*/
UI_MENU_DEFINE_FILE_STRING(VICIIPaletteFile)
UI_MENU_DEFINE_FILE_STRING(VDCPaletteFile)
UI_MENU_DEFINE_FILE_STRING(CrtcPaletteFile)
UI_MENU_DEFINE_FILE_STRING(TEDPaletteFile)
UI_MENU_DEFINE_FILE_STRING(VICPaletteFile)

extern volatile bool app_pause; // this pauses the SDL update thread
static char c_fs[25],c_fsm[25];

static void set_resources_trap(void *data)
{
	int *rs=(int*)data;

	app_pause=true;
	for (int i=0;i<rs[0];i++)
		resources_set_int((char*)rs[i*2+1],rs[i*2+2]);
	app_pause=false;
	triggerSync(0);
}

static UI_MENU_CALLBACK(toggle_MaxScreen_callback)
{
	static int s_fs=0, s_fss=SDL_FULLSCREEN, s_fsm=FULLSCREEN_MODE_AUTO;
	int r=0, fs=0, fss=0, w=0, h=0, fsm=0;
	
	snprintf(c_fs,25,"%sFullscreen",sdl_active_canvas->videoconfig->chip_name);
	resources_get_int(c_fs, &fs); // 1?
	snprintf(c_fsm,25,"%sSDLFullscreenMode",sdl_active_canvas->videoconfig->chip_name);
	resources_get_int(c_fsm, &fsm); // FULLSCREEN_MODE_CUSTOM ?
	resources_get_int("SDLCustomWidth", &w); // 320?
	resources_get_int("SDLCustomHeight", &h); // 200?
	resources_get_int("SDLFullscreenStretch", &fss); // SDL_FULLSCREEN?

	if (fs==1 && w==320 && h==200 && fss==SDL_FULLSCREEN && fsm==FULLSCREEN_MODE_CUSTOM) r=1;

	if (activated) {
		u32 *rs = calloc(11,sizeof(u32));
		if (r==0) {
			r=1;
			s_fs=fs; s_fss=fss; s_fsm=fsm;
			rs[0]=5;
			rs[1]=(u32)"SDLCustomWidth"; rs[2]=320;
			rs[3]=(u32)"SDLCustomHeight"; rs[4]=200;
			rs[5]=(u32)"SDLFullscreenStretch"; rs[6]=SDL_FULLSCREEN;
			rs[7]=(u32)c_fsm; rs[8]=FULLSCREEN_MODE_CUSTOM;
			rs[9]=(u32)c_fs;rs[10]=1;
		} else {
			r=0;
			rs[0]=5;
			rs[1]=(u32)c_fs; rs[2]=s_fs;
			rs[3]=(u32)c_fsm; rs[4]=s_fsm;
			rs[5]=(u32)"SDLFullscreenStretch"; rs[6]=s_fss;
			rs[7]=(u32)"SDLCustomWidth";rs[8]=400;
			rs[9]=(u32)"SDLCustomHeight";rs[10]=240;
		}	
		ui_trigger_trap(set_resources_trap, rs);
		waitSync(0);
		free(rs);

		// update keypresses on bottom screen in case we have anything mapped
		uibottom_must_redraw |= UIB_RECALC_KEYPRESS;
	}
	return r ? sdl_menu_text_tick : NULL;
}

/* C128 video menu */
static UI_MENU_CALLBACK(toggle_VideoOutput_c128_callback)
{
    int value = sdl_active_canvas_num;

    if (activated) {
        value = value == 1 ? 0 : 1;
		app_pause=true;
		sdl_video_canvas_switch(value);
		app_pause=false;
    }
    return value == 1 ? sdl_menu_text_tick : NULL;
}

const ui_menu_entry_t c128_video_menu[] = {
	{ "Hide Border / Fullscreen",
		MENU_ENTRY_OTHER_TOGGLE,
		toggle_MaxScreen_callback,
		NULL},
	SDL_MENU_ITEM_TITLE("Video output"),
    { "Toggle VDC (80 cols)",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VideoOutput_c128_callback,
	  NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII Fullscreen",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIFullscreen_callback,
      NULL },
    { "VDC Fullscreen",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VDCFullscreen_callback,
      NULL },
    { "VICII size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_size_menu },
    { "VDC size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vdc_size_menu },
/*    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },*/
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIVideoCache_callback,
      NULL },
    { "VICII Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_color_controls_menu },
/*    { "VICII CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_crt_controls_menu },
    { "VICII render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_filter_menu },*/
    { "VICII colors",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)palette_menu1 },
    { "VICII Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIAudioLeak_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "VDC Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VDCVideoCache_callback,
      NULL },
    { "VDC Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vdc_color_controls_menu },
/*    { "VDC CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vdc_crt_controls_menu },
    { "VDC render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vdc_filter_menu },*/
    { "VDC colors",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)palette_menu2 },
    { "VDC Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VDCAudioLeak_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Video Standard"),
    { "PAL",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_PAL },
    { "NTSC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_NTSC },
    SDL_MENU_ITEM_SEPARATOR,
    { "Enable Sprite-Sprite collisions",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIICheckSsColl_callback,
      NULL },
    { "Enable Sprite-Background collisions",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIICheckSbColl_callback,
      NULL },
    SDL_MENU_LIST_END
};

/* C64 video menu */
const ui_menu_entry_t c64_video_menu[] = {
	{ "Hide Border / Fullscreen",
		MENU_ENTRY_OTHER_TOGGLE,
		toggle_MaxScreen_callback,
		NULL},
	SDL_MENU_ITEM_SEPARATOR,
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_size_menu },
/*    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },*/
    SDL_MENU_ITEM_SEPARATOR,
    { "Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIVideoCache_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII border mode",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_border_menu },
    SDL_MENU_ITEM_SEPARATOR,
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_color_controls_menu },
/*    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_crt_controls_menu },
    { "Render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_filter_menu },*/
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII colors",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)palette_menu1 },
    { "VICII Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIAudioLeak_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "Enable Sprite-Sprite collisions",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIICheckSsColl_callback,
      NULL },
    { "Enable Sprite-Background collisions",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIICheckSbColl_callback,
      NULL },
    SDL_MENU_LIST_END
};


/* C64SC video menu */

const ui_menu_entry_t c64sc_video_menu[] = {
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_size_menu },
/*    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },*/
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII border mode",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_border_menu },
    SDL_MENU_ITEM_SEPARATOR,
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_color_controls_menu },
/*    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_crt_controls_menu },
    { "Render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_filter_menu },*/
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII colors",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)palette_menu1 },
    { "VICII Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIAudioLeak_callback,
      NULL },
    { "VICII VSP-bug emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIVSPBug_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "Enable Sprite-Sprite collisions",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIICheckSsColl_callback,
      NULL },
    { "Enable Sprite-Background collisions",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIICheckSbColl_callback,
      NULL },
    SDL_MENU_LIST_END
};


/* C64DTV video menu */

const ui_menu_entry_t c64dtv_video_menu[] = {
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_size_menu },
/*    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },*/
    SDL_MENU_ITEM_SEPARATOR,
    { "Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIVideoCache_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII border mode",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_border_menu },
    SDL_MENU_ITEM_SEPARATOR,
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_color_controls_menu },
/*    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_crt_controls_menu },
    { "Render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_filter_menu },*/
#if 0   /* disabled until there are external DTV palette files available */
    { "External palette",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIExternalPalette_callback,
      NULL },
    { "External palette file",
      MENU_ENTRY_DIALOG,
      file_string_VICIIPaletteFile_callback,
      (ui_callback_data_t)"Choose palette file" },
#endif
    { "VICII Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIAudioLeak_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Video Standard"),
    { "PAL",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_PAL },
    { "NTSC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_NTSC },
    SDL_MENU_ITEM_SEPARATOR,
    { "Enable Sprite-Sprite collisions",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIICheckSsColl_callback,
      NULL },
    { "Enable Sprite-Background collisions",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIICheckSbColl_callback,
      NULL },
    SDL_MENU_LIST_END
};

/* CBM-II 5x0 video menu */

const ui_menu_entry_t cbm5x0_video_menu[] = {
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_size_menu },
/*    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },*/
    SDL_MENU_ITEM_SEPARATOR,
    { "Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIVideoCache_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII border mode",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_border_menu },
    SDL_MENU_ITEM_SEPARATOR,
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_color_controls_menu },
/*    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_crt_controls_menu },
    { "Render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_filter_menu },*/
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII colors",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)palette_menu1 },
    { "VICII Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIAudioLeak_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Video Standard"),
    { "PAL",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_PAL },
    { "NTSC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_NTSC },
    SDL_MENU_ITEM_SEPARATOR,
    { "Enable Sprite-Sprite collisions",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIICheckSsColl_callback,
      NULL },
    { "Enable Sprite-Background collisions",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIICheckSbColl_callback,
      NULL },
    SDL_MENU_LIST_END
};


/* CBM-II 6x0/7x0 video menu */

const ui_menu_entry_t cbm6x0_7x0_video_menu[] = {
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)crtc_size_menu },
/*    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },*/
    SDL_MENU_ITEM_SEPARATOR,
    { "Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_CrtcVideoCache_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)crtc_color_controls_menu },
/*    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)crtc_crt_controls_menu },
    { "Render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)crtc_filter_menu },*/
    { "CRTC colors",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)palette_menu1 },
    { "CRTC Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_CrtcAudioLeak_callback,
      NULL },
    SDL_MENU_LIST_END
};


/* PET video menu */

const ui_menu_entry_t pet_video_menu[] = {
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)crtc_size_menu },
/*    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },*/
    SDL_MENU_ITEM_SEPARATOR,
    { "Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_CrtcVideoCache_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)crtc_color_controls_menu },
/*    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)crtc_crt_controls_menu },
    { "Render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)crtc_filter_menu },*/
    { "CRTC colors",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)palette_menu1 },
    { "CRTC Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_CrtcAudioLeak_callback,
      NULL },
    SDL_MENU_LIST_END
};


/* PLUS4 video menu */

const ui_menu_entry_t plus4_video_menu[] = {
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)ted_size_menu },
/*    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },*/
    SDL_MENU_ITEM_SEPARATOR,
    { "Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_TEDVideoCache_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "TED border mode",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)ted_border_menu },
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)ted_color_controls_menu },
/*    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)ted_crt_controls_menu },
    { "Render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)ted_filter_menu },*/
    SDL_MENU_ITEM_SEPARATOR,
    { "TED colors",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)palette_menu1 },
    { "TED Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_TEDAudioLeak_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Video Standard"),
    { "PAL",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_PAL },
    { "NTSC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_NTSC },
    SDL_MENU_LIST_END
};


/* VIC-20 video menu */

static UI_MENU_CALLBACK(radio_MachineVideoStandard_vic20_callback)
{
    if (activated) {
        int value = vice_ptr_to_int(param);
        resources_set_int("MachineVideoStandard", value);
        machine_trigger_reset(MACHINE_RESET_MODE_HARD);
    }
    return sdl_ui_menu_radio_helper(activated, param, "MachineVideoStandard");
}

const ui_menu_entry_t vic20_video_menu[] = {
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vic_size_menu },
/*    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },*/
    SDL_MENU_ITEM_SEPARATOR,
    { "Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICVideoCache_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "VIC border mode",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vic_border_menu },
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vic_color_controls_menu },
/*    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vic_crt_controls_menu },
    { "Render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vic_filter_menu },*/
    SDL_MENU_ITEM_SEPARATOR,
    { "VIC colors",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)palette_menu1 },
    { "VIC Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICAudioLeak_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Video Standard"),
    { "PAL",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_vic20_callback,
      (ui_callback_data_t)MACHINE_SYNC_PAL },
    { "NTSC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_vic20_callback,
      (ui_callback_data_t)MACHINE_SYNC_NTSC },
    SDL_MENU_LIST_END
};

/* static int palette_dyn_menu_init = 0; */

static char *video_chip1_used = NULL;
static char *video_chip2_used = NULL;

static UI_MENU_CALLBACK(external_palette_file1_callback)
{
    const char *external_file_name;
    char *name = (char *)param;

    if (activated) {
        resources_set_string_sprintf("%sPaletteFile", name, video_chip1_used);
    } else {
        resources_get_string_sprintf("%sPaletteFile", &external_file_name, video_chip1_used);
        if (external_file_name) {
            if (!strcmp(external_file_name, name)) {
                return MENU_CHECKMARK_CHECKED_STRING;
            }
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(external_palette_file2_callback)
{
    const char *external_file_name;
    char *name = (char *)param;

    if (activated) {
        resources_set_string_sprintf("%sPaletteFile", name, video_chip2_used);
    } else {
        resources_get_string_sprintf("%sPaletteFile", &external_file_name, video_chip2_used);
        if (external_file_name) {
            if (!strcmp(external_file_name, name)) {
                return MENU_CHECKMARK_CHECKED_STRING;
            }
        }
    }
    return NULL;
}

static int countgroup(palette_info_t *palettelist, char *chip)
{
    int num = 0;

    while (palettelist->name) {
        if (palettelist->chip && !strcmp(palettelist->chip, chip)) {
            ++num;
        }
        ++palettelist;
    }
    return num;
}

typedef struct name2func_s {
    char *name;
    const char *(*toggle_func)(int activated, ui_callback_data_t param);
    const char *(*file_func)(int activated, ui_callback_data_t param);
} name2func_t;

static name2func_t name2func[] = {
    { "VICII", toggle_VICIIExternalPalette_callback, file_string_VICIIPaletteFile_callback },
    { "VDC", toggle_VDCExternalPalette_callback, file_string_VDCPaletteFile_callback },
    { "Crtc", toggle_CrtcExternalPalette_callback, file_string_CrtcPaletteFile_callback },
    { "TED", toggle_TEDExternalPalette_callback, file_string_TEDPaletteFile_callback },
    { "VIC", toggle_VICExternalPalette_callback, file_string_VICPaletteFile_callback },
    { NULL, NULL, NULL }
};

void uipalette_menu_create(char *chip1_name, char *chip2_name)
{
    int num;
    palette_info_t *palettelist = palette_get_info_list();
    int i;
    const char *(*toggle_func1)(int activated, ui_callback_data_t param) = NULL;
    const char *(*file_func1)(int activated, ui_callback_data_t param) = NULL;
    const char *(*toggle_func2)(int activated, ui_callback_data_t param) = NULL;
    const char *(*file_func2)(int activated, ui_callback_data_t param) = NULL;

    video_chip1_used = chip1_name;
    video_chip2_used = chip2_name;

    for (i = 0; name2func[i].name; ++i) {
        if (!strcmp(video_chip1_used, name2func[i].name)) {
            toggle_func1 = name2func[i].toggle_func;
            file_func1 = name2func[i].file_func;
        }
    }

    if (video_chip2_used) {
        for (i = 0; name2func[i].name; ++i) {
            if (!strcmp(video_chip2_used, name2func[i].name)) {
                toggle_func2 = name2func[i].toggle_func;
                file_func2 = name2func[i].file_func;
            }
        }
    }

    num = countgroup(palettelist, video_chip1_used);

    palette_dyn_menu1 = lib_malloc(sizeof(ui_menu_entry_t) * (num + 1));

    i = 0;

    while (palettelist->name) {
        if (palettelist->chip && !strcmp(palettelist->chip, video_chip1_used)) {
            palette_dyn_menu1[i].string = (char *)lib_stralloc(palettelist->name);
            palette_dyn_menu1[i].type = MENU_ENTRY_OTHER_TOGGLE;
            palette_dyn_menu1[i].callback = external_palette_file1_callback;
            palette_dyn_menu1[i].data = (ui_callback_data_t)lib_stralloc(palettelist->file);
            ++i;
        }
        ++palettelist;
    }

    palette_dyn_menu1[i].string = NULL;
    palette_dyn_menu1[i].type = 0;
    palette_dyn_menu1[i].callback = NULL;
    palette_dyn_menu1[i].data = NULL;

    if (video_chip2_used) {
        palettelist = palette_get_info_list();
        num = countgroup(palettelist, video_chip2_used);
        palette_dyn_menu2 = lib_malloc(sizeof(ui_menu_entry_t) * (num + 1));

        i = 0;

        while (palettelist->name) {
            if (palettelist->chip && !strcmp(palettelist->chip, video_chip2_used)) {
                palette_dyn_menu2[i].string = (char *)lib_stralloc(palettelist->name);
                palette_dyn_menu2[i].type = MENU_ENTRY_OTHER_TOGGLE;
                palette_dyn_menu2[i].callback = external_palette_file2_callback;
                palette_dyn_menu2[i].data = (ui_callback_data_t)lib_stralloc(palettelist->file);
                ++i;
            }
            ++palettelist;
        }
        palette_dyn_menu2[i].string = NULL;
        palette_dyn_menu2[i].type = 0;
        palette_dyn_menu2[i].callback = NULL;
        palette_dyn_menu2[i].data = NULL;
    }

    palette_menu1[0].string = "External palette";
    palette_menu1[0].type = MENU_ENTRY_RESOURCE_TOGGLE;
    palette_menu1[0].callback = toggle_func1;
    palette_menu1[0].data = NULL;

    palette_menu1[1].string = "Available palette files";
    palette_menu1[1].type = MENU_ENTRY_SUBMENU;
    palette_menu1[1].callback = submenu_callback;
    palette_menu1[1].data = (ui_callback_data_t)palette_dyn_menu1;

    palette_menu1[2].string = "Custom palette file";
    palette_menu1[2].type = MENU_ENTRY_DIALOG;
    palette_menu1[2].callback = file_func1;
    palette_menu1[2].data = (ui_callback_data_t)"Choose palette file";

    palette_menu1[3].string = NULL;
    palette_menu1[3].type = MENU_ENTRY_TEXT;
    palette_menu1[3].callback = NULL;
    palette_menu1[3].data = NULL;

    if (video_chip2_used) {
        palette_menu2[0].string = "External palette";
        palette_menu2[0].type = MENU_ENTRY_RESOURCE_TOGGLE;
        palette_menu2[0].callback = toggle_func2;
        palette_menu2[0].data = NULL;

        palette_menu2[1].string = "Available palette files";
        palette_menu2[1].type = MENU_ENTRY_SUBMENU;
        palette_menu2[1].callback = submenu_callback;
        palette_menu2[1].data = (ui_callback_data_t)palette_dyn_menu2;

        palette_menu2[2].string = "Custom palette file";
        palette_menu2[2].type = MENU_ENTRY_DIALOG;
        palette_menu2[2].callback = file_func2;
        palette_menu2[2].data = (ui_callback_data_t)"Choose palette file";

        palette_menu2[3].string = NULL;
        palette_menu2[3].type = MENU_ENTRY_TEXT;
        palette_menu2[3].callback = NULL;
        palette_menu2[3].data = NULL;
    }
}


/** \brief  Free memory used by the items of \a menu and \a menu itself
 *
 * \param[in,out]   menu    heap-allocated sub menu
 *
 * \note    the \a menu must be terminated with an empty entry
 */
static void palette_dyn_menu_free(ui_menu_entry_t *menu)
{
    ui_menu_entry_t *item = menu;
    while (item->string != NULL) {
        lib_free(item->string);
        lib_free(item->data);
        item++;
    }
    lib_free(menu);
}


/** \brief  Clean up memory used by the palette sub menu(s)
 */
void uipalette_menu_shutdown(void)
{
    if (palette_dyn_menu1 != NULL) {
        palette_dyn_menu_free(palette_dyn_menu1);
    }
    if (palette_dyn_menu2 != NULL) {
        palette_dyn_menu_free(palette_dyn_menu2);
    }
}
