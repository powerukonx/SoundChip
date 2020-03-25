/* =============================================================================

                  ██╗   ██╗██╗  ██╗ ██████╗ ███╗   ██╗██╗  ██╗
                  ██║   ██║██║ ██╔╝██╔═══██╗████╗  ██║╚██╗██╔╝
                  ██║   ██║█████╔╝ ██║   ██║██╔██╗ ██║ ╚███╔╝
                  ██║   ██║██╔═██╗ ██║   ██║██║╚██╗██║ ██╔██╗
                  ╚██████╔╝██║  ██╗╚██████╔╝██║ ╚████║██╔╝ ██╗
                   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝

  File name:    main.cpp
  Date:         23 07 2017
  Author:       Power.
  Description:  Playkonx - Body file.

============================================================================= */


/* =============================================================================
                                 DEBUG Section
============================================================================= */
#define _WIN32_WINNT  0x0502
#define  WINVER       0x0502
#define _WIN32_IE     0x0600

#ifdef DBG_MAIN
  #warning "DBG_MAIN activated"
#endif  /* DBG_MAIN */


/* =============================================================================
                                 Include Files
============================================================================= */
#include <windows.h>
#include <conio.h>
#include <stdint.h>
#include <stdio.h>
#include "soundengine.h"
#include "emu_sn76489.h"
#include "parser_vgm.h"


/* =============================================================================
                          Private defines and typedefs
============================================================================= */

/* =============================================================================
                        Private constants and variables
============================================================================= */
static s_SoundEngine_t        g_sSoundEngine;
//static Ym                     g_clYm;

/* =============================================================================
                        Private function declarations
============================================================================= */


/* =============================================================================
                               Public functions
============================================================================= */

/* =============================================================================
                              Private functions
============================================================================= */


/*===================================================================================================
Function    : WinMain

Describe    : Main entry.

Parameters  : See MSDN.

Returns     : See MSDN.
===================================================================================================*/
INT APIENTRY WinMain (HINSTANCE p_hInstance, __attribute__ ((unused))HINSTANCE p_hPrevInstance, __attribute__ ((unused))LPSTR p_lpCmdLine, __attribute__ ((unused))INT p_nShowCmd)
{
  /* Initialize VGM player. */
//  g_clYm.bInit();

  /* Initialize g_sSoundEngine. */
  g_sSoundEngine.dwSampleRate = SOUNDENGINE_44100;
  g_sSoundEngine.byBitsPerSample = SOUNDENGINE_8BITS;
  g_sSoundEngine.byNbChannels = SOUNDENGINE_STEREO;
 // g_sSoundEngine.pbRenderingEnable = &g_bAudioRenderingStart;
 // g_sSoundEngine.bLeftChannelEnable =&g_bChannelABEnable;
 // g_sSoundEngine.bRightChannelEnable =&g_bChannelBCEnable;
 // g_sSoundEngine.bFilterEnable = &g_bFilterEnable;
 // g_sSoundEngine.pfFilterCutOff = &g_fFilterCutOff;
 // g_sSoundEngine.b_Render = &g_clYm;
  if (b_SoundEngineInit (&g_sSoundEngine) == FALSE)
  {
    MessageBox (NULL, "Error : b_SoundEngineInit", "DlgMain error", MB_ICONERROR | MB_OK);
  }

  /* Wait for key press. */
  while (!kbhit())
  {
    ;
  }
  return 0;
}
