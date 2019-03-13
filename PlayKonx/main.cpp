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
#include <stdint.h>
#include <windows.h>
#include <winuser.h>
#include <stdio.h>
#include <math.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include "soundengine.h"
#include "parser_ym.h"
#include "generator_ay.h"
#include "resource.h"


/* =============================================================================
                          Private defines and typedefs
============================================================================= */
#define IDT_VIDEO         ((UINT_PTR)1)
#define VIDEO_REFRESH_MS  ((UINT32)30)


#define TEXTURE_SIZE  (256)
#define CHANNEL_LEFT  (0)
#define CHANNEL_RIGHT (CHANNEL_LEFT + 1)
#define NBR_CHANNEL   (CHANNEL_RIGHT + 1)

typedef enum _LOOPMODE_ {
  LOOP_NO = 0u,
  LOOP_SONG,
  LOOP_PLAYLIST
} eLoopMode_t;

typedef enum _UKXERROR_ {
  UKXERROR_NO = 0u,
  UKXERROR_PARAM,
  UKXERROR_UNKNOWN
} eUkxError_t;

typedef enum _UKXSTATE_ {
  UKXSTATE_OFF = 0u,
  UKXSTATE_ON,
  UKXSTATE_MAX
} eUkxState_t;

typedef enum _UKFREQ_ {
  UKFREQ_AMSTRAD = 0u,
  UKFREQ_ATARI,
  UKFREQ_SINCLAIR,
  UKFREQ_MSX,
  UKFREQ_EXPE
} eUkFreq_t;

typedef enum _UKCHANNEL_ {
  UKCHANNEL_RIGHT = 0u,
  UKCHANNEL_LEFT
} eUkChannel_t;

typedef enum _UKLOOPMODE_ {
  UKLOOPMODE_NO = 0u,
  UKLOOPMODE_SONG,
  UKLOOPMODE_PLAYLIST,
  UKLOOPMODE_MAX
} eUkLoopMode_t;


/* =============================================================================
                        Private constants and variables
============================================================================= */
static BOOL                   g_bFilterEnable = FALSE;
static BOOL                   g_bAudioRenderingStart;
static BOOL                   g_bChannelABEnable = TRUE;
static BOOL                   g_bChannelBCEnable = TRUE;
static UINT32                 g_ui32PlaylistIndex = 0;
static UINT32                 g_ui32PlaylistSize = 0;
static UINT8                  g_byChannelState;
static PIXELFORMATDESCRIPTOR  g_pfdWaveForm[NBR_CHANNEL];
static PIXELFORMATDESCRIPTOR  g_pfdChannel[AYCHANNEL_MAX];
static INT32                  g_i32FormatWaveForm[NBR_CHANNEL];
static INT32                  g_i32FormatChannel[AYCHANNEL_MAX];
static HBITMAP                g_hChannelOff;
static HBITMAP                g_hChannelOn;
static HINSTANCE              g_hInstance;
static s_YmInit_t             g_sYmInit;
static s_SoundEngine_t        g_sSoundEngine;
static HDC                    g_hDCWaveForm[NBR_CHANNEL];
static HDC                    g_hDCChannel[AYCHANNEL_MAX];
static HGLRC                  g_hRCWaveForm[NBR_CHANNEL];
static HGLRC                  g_hRCChannel[AYCHANNEL_MAX];
static eLoopMode_t            g_eLoopMode = LOOP_NO;
static eUkxState_t            g_eChannelAState;
static eUkxState_t            g_eChannelBState;
static eUkxState_t            g_eChannelCState;
static FLOAT                  g_fFilterCutOff;


/* =============================================================================
                        Private function declarations
============================================================================= */
static BOOL CALLBACK  bDlgMain (HWND p_hDialog, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam);
static eUkxError_t    eCB_InitDialog (HWND p_hDialog);
static BOOL           bCB_Command (HWND p_hDialog, WPARAM p_wParam, LPARAM p_lParam);
static BOOL           bCB_Close (HWND p_hDialog, WPARAM p_wParam, LPARAM p_lParam);
static BOOL           bCB_VScroll (HWND p_hDialog, WPARAM p_wParam, LPARAM p_lParam);
static BOOL           bCB_HScroll (HWND p_hDialog, WPARAM p_wParam, LPARAM p_lParam);
static VOID CALLBACK  vCB_Timer (HWND phWnd, UINT p_uMsg, UINT_PTR p_idEvent, DWORD p_dwTime);
static BOOL           bIsPlaying (HWND p_hDialog);
static VOID           vFileAdd (HWND p_hDialog);
static VOID           vFileSub (HWND p_hDialog);
static eUkxError_t    eSetLoopMode (HWND p_hDialog, eUkLoopMode_t p_eLoop);
static eUkxError_t    eSetChannelxState (HWND p_hDialog, e_AyChannel_t p_eChannel, eUkxState_t p_eState);
static VOID           vToggleStartButtonState (HWND p_hDialog);
static VOID           vEvent_Playlist (HWND p_hDialog);
static eUkxError_t    eSetAyFrequency (HWND p_hDialog, eUkFreq_t p_eFrequency);
static eUkxError_t    eWavFormInit (HWND p_hDialog);
static eUkxError_t    eSetFilterValue (HWND p_hDialog, FLOAT p_fValue);
static eUkxError_t    eSetVolumeValue (HWND p_hDialog, eUkChannel_t p_eChannel, UINT8 p_u8Value);
static VOID           vWaveFormRender (UCHAR p_uchChannel, BOOL p_bChannelStateOn);
static VOID           vChannelRender (e_AyChannel_t p_eChannel, eUkxState_t p_eState);
static VOID           vGetTimeElapsed (HWND p_hDialog);


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
  /* Save program instance. */
  g_hInstance = p_hInstance;

  /* Initialize common control. */
  InitCommonControls ();

  /* Start dialog box. */
  return DialogBox (g_hInstance, MAKEINTRESOURCE (IDD_DIALOG1), NULL, (DLGPROC)bDlgMain);
}


/*===================================================================================================
Function    : DlgMain

Describe    : Main windows application callback.

Parameters  : See MSDN.

Returns     : See MSDN.
===================================================================================================*/
static BOOL CALLBACK bDlgMain (HWND p_hDialog, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;

  /* Dispatch windows message. */
  switch (p_uMsg)
  {
    case WM_INITDIALOG:
    {
      /* Initialize dialog box resources. */
      if (UKXERROR_NO != eCB_InitDialog (p_hDialog) )
      {
        l_bReturn = bCB_Close (p_hDialog, p_wParam, p_lParam);
      }
      else
      {
        l_bReturn = TRUE;
      }
      break;
    }
    case WM_VSCROLL:
    {
      l_bReturn = bCB_VScroll (p_hDialog, p_wParam, p_lParam);
      break;
    }
    case WM_HSCROLL:
    {
      l_bReturn = bCB_HScroll (p_hDialog, p_wParam, p_lParam);
      break;
    }
    case WM_CLOSE:
    {
      l_bReturn = bCB_Close (p_hDialog, p_wParam, p_lParam);
      break;
    }
    case WM_COMMAND:
    {
      l_bReturn = bCB_Command (p_hDialog, p_wParam, p_lParam);
      break;
    }
    default:
    {
      /* Nothing to do. */
      break;
    }
  }

  return (l_bReturn);
}


/*==============================================================================
Function    : eDlgInit.

Describe    : Initialize dialog box.

Parameters  : p_hDialog = Handle to dialog box.

Returns     : eUkxError_t.
==============================================================================*/
static eUkxError_t eCB_InitDialog (HWND p_hDialog)
{
  /* Locals variables declaration. */
  eUkxError_t l_eReturn = UKXERROR_PARAM;
  HICON l_hIcon;
  HICON l_hIconSm;
  DWORD l_dwVolume;

  /* Inputs parameters checking. */
  if (NULL != p_hDialog)
  {
    /* Set application icon. */
    l_hIcon = (HICON)LoadImage (GetModuleHandle (NULL), MAKEINTRESOURCE (IDI_ICON1), IMAGE_ICON, 32, 32, 0);
    SendMessage (p_hDialog, WM_SETICON, ICON_BIG, (LPARAM)l_hIcon);
    l_hIconSm = (HICON)LoadImage (GetModuleHandle (NULL), MAKEINTRESOURCE (IDI_ICON1), IMAGE_ICON, 16, 16, 0);
    SendMessage (p_hDialog, WM_SETICON, ICON_SMALL, (LPARAM)l_hIconSm);

    /* Load bitmaps for channel on/off. */
    g_hChannelOff = LoadBitmap (g_hInstance, MAKEINTRESOURCE (IDB_CHANOFF));
    g_hChannelOn = LoadBitmap (g_hInstance, MAKEINTRESOURCE (IDB_CHANON));

    /* Enable all channels. */
    eSetChannelxState (p_hDialog, AYCHANNEL_A, UKXSTATE_ON);
    eSetChannelxState (p_hDialog, AYCHANNEL_B, UKXSTATE_ON);
    eSetChannelxState (p_hDialog, AYCHANNEL_C, UKXSTATE_ON);

    /* Don't start now. */
  //  eSetStartButtonState (p_hDialog, UKXSTATE_OFF);
    EnableWindow (GetDlgItem (p_hDialog, ID_START), FALSE);

    /* Initialize AY/YM frequency. */
    eSetAyFrequency (p_hDialog, UKFREQ_AMSTRAD);

    /* Initialize filter slider. */
    SendDlgItemMessage (p_hDialog, IDSLIDERCUTOFF, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG (30, 5700));
    eSetFilterValue (p_hDialog, 3000.0f);
    SendDlgItemMessage (p_hDialog, IDSLIDERCUTOFF, TBM_SETPOS, (WPARAM)1, (LPARAM)5700.0f - 3000.0f);

    /* Initialize g_sSoundEngine. */
    g_sSoundEngine.dwSampleRate = SOUNDENGINE_44100;
    g_sSoundEngine.byBitsPerSample = SOUNDENGINE_8BITS;
    g_sSoundEngine.byNbChannels = SOUNDENGINE_STEREO;
    g_sSoundEngine.pbRenderingEnable = &g_bAudioRenderingStart;
    g_sSoundEngine.bLeftChannelEnable =&g_bChannelABEnable;
    g_sSoundEngine.bRightChannelEnable =&g_bChannelBCEnable;
    g_sSoundEngine.bFilterEnable = &g_bFilterEnable;
    g_sSoundEngine.pfFilterCutOff = &g_fFilterCutOff;
    g_sSoundEngine.b_Render = &bYm_Update;
    if (b_SoundEngineInit (&g_sSoundEngine) == FALSE)
    {
      MessageBox (p_hDialog, "Error : b_SoundEngineInit", "DlgMain error", MB_ICONERROR | MB_OK);
    }

    /* Set current audio volume. */
    SendDlgItemMessage (p_hDialog, IDMASTER_R, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG (0,255));
    SendDlgItemMessage (p_hDialog, IDMASTER_L, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG (0,255));
    uiSoundEngineGetVolume (&l_dwVolume);
    eSetVolumeValue (p_hDialog, UKCHANNEL_LEFT, l_dwVolume>>8);
    eSetVolumeValue (p_hDialog, UKCHANNEL_RIGHT, l_dwVolume>>8);
    SendDlgItemMessage (p_hDialog, IDMASTER_R, TBM_SETPOS, TRUE, 255-((l_dwVolume>>24)&0xff));
    SendDlgItemMessage (p_hDialog, IDMASTER_L, TBM_SETPOS, TRUE, 255-((l_dwVolume>>8 )&0xff));
    SendDlgItemMessage (p_hDialog, ID_VOL_ASSOCIATE, BM_SETCHECK, BST_CHECKED, 0);

    /* Initialize wave shape viewer. */
    eWavFormInit (p_hDialog);

    /* Set no loop mode. */
    eSetLoopMode (p_hDialog, UKLOOPMODE_NO);

    /* Initialize SID. */
    vAY_SetChannelASidState (0);
    vAY_SetChannelBSidState (0);
    vAY_SetChannelCSidState (0);

    /* Set video timer. */
    SetTimer (p_hDialog, IDT_VIDEO, VIDEO_REFRESH_MS, vCB_Timer);

    /* Initialization done. */
    l_eReturn = UKXERROR_NO;
  }
  else
  {
    /* Nothing to do. */
  }

  return (l_eReturn);
}


/*===================================================================================================
Function    : bCB_Command

Describe    : .

Parameters  : .

Returns     : .
===================================================================================================*/
static BOOL bCB_Command (HWND p_hDialog, __attribute__ ((unused))WPARAM p_wParam, __attribute__ ((unused))LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;

  switch (LOWORD (p_wParam))
  {
    case ID_LOOP_NO:
    {
      eSetLoopMode (p_hDialog, UKLOOPMODE_NO);

      l_bReturn = TRUE;
      break;
    }

    case ID_LOOP_PLAYLIST:
    {
      eSetLoopMode (p_hDialog, UKLOOPMODE_PLAYLIST);

      l_bReturn = TRUE;
      break;
    }

    case ID_LOOP_SONG:
    {
      eSetLoopMode (p_hDialog, UKLOOPMODE_SONG);

      l_bReturn = TRUE;
      break;
    }

    case ID_FREQ_AMSTRAD:
    {
      eSetAyFrequency (p_hDialog, UKFREQ_AMSTRAD);

      l_bReturn = TRUE;
      break;
    }

    case ID_FREQ_SINCLAIR:
    {
      eSetAyFrequency (p_hDialog, UKFREQ_SINCLAIR);

      l_bReturn = TRUE;
      break;
    }

    case ID_FREQ_MSX:
    {
      eSetAyFrequency (p_hDialog, UKFREQ_MSX);

      l_bReturn = TRUE;
      break;
    }

    case ID_FREQ_ATARI:
    {
      eSetAyFrequency (p_hDialog, UKFREQ_ATARI);

      l_bReturn = TRUE;
      break;
    }

    case ID_FREQ_EXPE:
    {
      eSetAyFrequency (p_hDialog, UKFREQ_EXPE);

      l_bReturn = TRUE;
      break;
    }

    case IDCHECKFILTER:
    {
      if (SendDlgItemMessage (p_hDialog, IDCHECKFILTER, BM_GETCHECK, 0, 0) == BST_CHECKED)
      {
        vSoundEngineSetFilterOn ();
        g_bFilterEnable = TRUE;
      }
      else
      {
        vSoundEngineSetFilterOff ();
        g_bFilterEnable = FALSE;
      }

      l_bReturn = TRUE;
      break;
    }

    case ID_SID_CHANNELA:
    {
      if (SendDlgItemMessage (p_hDialog, ID_SID_CHANNELA, BM_GETCHECK, 0, 0) == BST_CHECKED)
      {
        vAY_SetChannelASidState (1);
      }
      else
      {
        vAY_SetChannelASidState (0);
      }

      l_bReturn = TRUE;
      break;
    }

    case ID_SID_CHANNELB:
    {
      if (SendDlgItemMessage (p_hDialog, ID_SID_CHANNELB, BM_GETCHECK, 0, 0) == BST_CHECKED)
      {
        vAY_SetChannelBSidState (1);
      }
      else
      {
        vAY_SetChannelBSidState (0);
      }

      l_bReturn = TRUE;
      break;
    }

    case ID_SID_CHANNELC:
    {
      if (SendDlgItemMessage (p_hDialog, ID_SID_CHANNELC, BM_GETCHECK, 0, 0) == BST_CHECKED)
      {
        vAY_SetChannelCSidState (1);
      }
      else
      {
        vAY_SetChannelCSidState (0);
      }

      l_bReturn = TRUE;
      break;
    }

    case ID_ACC_PROG:
    {
      if (MF_CHECKED == GetMenuState(GetMenu(p_hDialog), ID_ACC_PROG, MF_BYCOMMAND))
      {
        CheckMenuItem (GetMenu(p_hDialog), ID_ACC_PROG, MF_UNCHECKED);

        g_byChannelState |= 0x08;
      }
      else
      {
        CheckMenuItem (GetMenu(p_hDialog), ID_ACC_PROG, MF_CHECKED);

        g_byChannelState &= (0x08 ^ 0xff);
      }

      vYm_SetChannelState (g_byChannelState);

      l_bReturn = TRUE;
      break;
    }

    case IDCHANNELA:
    {
      if (HIWORD(p_wParam) == BN_CLICKED)
      {
        if (g_eChannelAState == UKXSTATE_ON)
        {
          eSetChannelxState (p_hDialog, AYCHANNEL_A, UKXSTATE_OFF);
        }
        else
        {
          eSetChannelxState (p_hDialog, AYCHANNEL_A, UKXSTATE_ON);
        }
      }

      l_bReturn = TRUE;
      break;
    }

    case IDCHANNELB:
    {
      if (HIWORD (p_wParam) == BN_CLICKED)
      {
        if (g_eChannelBState == UKXSTATE_ON)
        {
          eSetChannelxState (p_hDialog, AYCHANNEL_B, UKXSTATE_OFF);
        }
        else
        {
          eSetChannelxState (p_hDialog, AYCHANNEL_B, UKXSTATE_ON);
        }
      }

      l_bReturn = TRUE;
      break;
    }

    case IDCHANNELC:
    {
      if (HIWORD (p_wParam) == BN_CLICKED)
      {
        if (g_eChannelCState == UKXSTATE_ON)
        {
          eSetChannelxState (p_hDialog, AYCHANNEL_C, UKXSTATE_OFF);
        }
        else
        {
          eSetChannelxState (p_hDialog, AYCHANNEL_C, UKXSTATE_ON);
        }
      }

      l_bReturn = TRUE;
      break;
    }

    case ID_PLAYLIST:
    {
      /* Is User had double-clicked an item in a list box ? */
      if (LBN_DBLCLK == HIWORD (p_wParam))
      {
        vEvent_Playlist (p_hDialog);

      }

      l_bReturn = TRUE;
      break;
    }

    case ID_START: /* Play/Pause button. */
    {
      vToggleStartButtonState (p_hDialog);

      l_bReturn = TRUE;
      break;
    }

    case IDFILEADD: /* Add item to play-list. */
    {
      vFileAdd (p_hDialog);

      l_bReturn = TRUE;
      break;
    }

    case IDFILESUB: /* Remove item from play-list. */
    {
      vFileSub (p_hDialog);

      l_bReturn = TRUE;
      break;
    }

    default:
    {
      break;
    }
  }

  return (l_bReturn);
}


/*===================================================================================================
Function    : bCB_Close

Describe    : .

Parameters  : .

Returns     : .
===================================================================================================*/
static BOOL bCB_Close (HWND p_hDialog, __attribute__ ((unused))WPARAM p_wParam, __attribute__ ((unused))LPARAM p_lParam)
{
  /* Kill video timer. */
  KillTimer (p_hDialog, IDT_VIDEO);

  /* Close Sound engine. */
  if (b_SoundEngineQuit () == FALSE)
  {
    MessageBox (p_hDialog,"Error : b_SoundEngineQuit", "DlgMain error", MB_ICONERROR | MB_OK);
  }

  /* Close YM Player. */
  vYm_Quit();

  /* Close OpenGL resources. */
  wglMakeCurrent (NULL, NULL);
  wglDeleteContext (g_hRCWaveForm[CHANNEL_LEFT]);
  wglDeleteContext (g_hRCWaveForm[CHANNEL_RIGHT]);

  wglDeleteContext (g_hRCChannel[AYCHANNEL_A]);
  wglDeleteContext (g_hRCChannel[AYCHANNEL_B]);
  wglDeleteContext (g_hRCChannel[AYCHANNEL_C]);

  ReleaseDC (GetDlgItem (p_hDialog, IDWAVEAB), g_hDCWaveForm[CHANNEL_LEFT]);
  ReleaseDC (GetDlgItem (p_hDialog, IDWAVEBC), g_hDCWaveForm[CHANNEL_RIGHT]);

  ReleaseDC (GetDlgItem (p_hDialog, ID_VISU_CHANNELA), g_hDCChannel[AYCHANNEL_A]);
  ReleaseDC (GetDlgItem (p_hDialog, ID_VISU_CHANNELB), g_hDCChannel[AYCHANNEL_B]);
  ReleaseDC (GetDlgItem (p_hDialog, ID_VISU_CHANNELC), g_hDCChannel[AYCHANNEL_C]);

  /* Close dialog box. */
  EndDialog (p_hDialog, 0);

  return TRUE;
}


/*===================================================================================================
Function    : bCB_VScroll

Describe    : .

Parameters  : .

Returns     : .
===================================================================================================*/
static BOOL bCB_VScroll (HWND p_hDialog, __attribute__ ((unused))WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;

  /* If master left volume change. */
  if ((LPARAM) GetDlgItem (p_hDialog, IDMASTER_L) == p_lParam)
  {
    eSetVolumeValue (p_hDialog, UKCHANNEL_LEFT, 255-SendDlgItemMessage (p_hDialog, IDMASTER_L, TBM_GETPOS, 0, 0));

    if (SendDlgItemMessage (p_hDialog, ID_VOL_ASSOCIATE, BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
      SendDlgItemMessage (p_hDialog, IDMASTER_R, TBM_SETPOS, TRUE, SendDlgItemMessage (p_hDialog, IDMASTER_L, TBM_GETPOS, 0, 0));
      eSetVolumeValue (p_hDialog, UKCHANNEL_RIGHT, 255-SendDlgItemMessage (p_hDialog, IDMASTER_L, TBM_GETPOS, 0, 0));
    }

    l_bReturn = TRUE;
  }
  else if ((LPARAM) GetDlgItem (p_hDialog, IDMASTER_R) == p_lParam) /* If master right volume change. */
  {
    eSetVolumeValue (p_hDialog, UKCHANNEL_RIGHT, 255-SendDlgItemMessage (p_hDialog, IDMASTER_R, TBM_GETPOS, 0, 0));

    if (SendDlgItemMessage (p_hDialog, ID_VOL_ASSOCIATE, BM_GETCHECK, 0, 0) == BST_CHECKED) {

      SendDlgItemMessage (p_hDialog, IDMASTER_L, TBM_SETPOS, TRUE, SendDlgItemMessage (p_hDialog, IDMASTER_R, TBM_GETPOS, 0, 0));
      eSetVolumeValue (p_hDialog, UKCHANNEL_LEFT, 255-SendDlgItemMessage (p_hDialog, IDMASTER_R, TBM_GETPOS, 0, 0));
    }

    l_bReturn = TRUE;
  }
  else if ((LPARAM) GetDlgItem (p_hDialog, IDSLIDERCUTOFF) == p_lParam) /* If Cutoff frequency change. */
  {
    eSetFilterValue (p_hDialog, 5700.0f - SendDlgItemMessage (p_hDialog, IDSLIDERCUTOFF, TBM_GETPOS, 0, 0));

    l_bReturn = TRUE;
  }

  return (l_bReturn);
}


/*===================================================================================================
Function    : bCB_HScroll

Describe    : .

Parameters  : .

Returns     : .
===================================================================================================*/
static BOOL bCB_HScroll (HWND p_hDialog, __attribute__ ((unused))WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;

  /* Time line ? */
  if ((LPARAM) GetDlgItem (p_hDialog, IDTIMELINE) == p_lParam)
  {
    vYm_SetPosition (SendDlgItemMessage (p_hDialog, IDTIMELINE, TBM_GETPOS, 0, 0));

    l_bReturn = TRUE;
  }

  return (l_bReturn);
}


/*===================================================================================================
Function    : vCB_Timer

Describe    : .

Parameters  : .

Returns     : .
===================================================================================================*/
static VOID CALLBACK vCB_Timer (HWND p_hDialog, __attribute__ ((unused))UINT p_uMsg, UINT_PTR p_idEvent, __attribute__ ((unused))DWORD p_dwTime)
{
  /* Video refresh timer ? */
  if (IDT_VIDEO == p_idEvent)
  {
    /* Update wave form render. */
    vWaveFormRender (CHANNEL_LEFT, g_bChannelABEnable);
    vWaveFormRender (CHANNEL_RIGHT, g_bChannelBCEnable);

    vChannelRender (AYCHANNEL_A, UKXSTATE_ON);
    vChannelRender (AYCHANNEL_B, UKXSTATE_ON);
    vChannelRender (AYCHANNEL_C, UKXSTATE_ON);

    /* Update time-line et time elapsed. */
    vGetTimeElapsed (p_hDialog);
  }
}


/*==============================================================================
Function    :   bIsPlaying

Describe    :   Check if player is currently playing song.

Parameters  :   p_hDialog = .

Returns     :   TRUE or FALSE.
==============================================================================*/
static BOOL bIsPlaying (HWND p_hDialog)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;
  TCHAR l_szState[MAX_PATH];
  UINT l_uiLength;

  /* Get current state. */
  l_uiLength = GetDlgItemText (p_hDialog, ID_START, l_szState, MAX_PATH);
  if (0u < l_uiLength)
  {
    /* Is current state is STOP ?*/
    if (0 == strcmp (l_szState, "PLAY") )
    {
      /* Nothing to do. */
    }
    else
    {
      l_bReturn = TRUE;
    }
  }
  else
  {
    /* Nothing to do. */
  }

  return (l_bReturn);
}


/*==============================================================================
Function    :   vFileAdd

Describe    :   Add item to play-list.

Parameters  :   p_hDialog = .

Returns     :   None.
==============================================================================*/
static VOID vFileAdd (HWND p_hDialog)
{
  /* Locals variables declaration. */
  OPENFILENAME l_sOpenfilename;
  TCHAR l_szFilePath[MAX_PATH];
  TCHAR l_szFileName[MAX_PATH];

  /* Fill OPENFILENAME structure. */
  ZeroMemory (&l_sOpenfilename, sizeof (OPENFILENAME) );
  l_sOpenfilename.lStructSize = sizeof (OPENFILENAME);
  l_sOpenfilename.lpstrFile = l_szFilePath;
  l_sOpenfilename.lpstrFile[0] = '\0';
  l_sOpenfilename.hwndOwner = p_hDialog;
  l_sOpenfilename.nMaxFile = sizeof (l_szFilePath);
  l_sOpenfilename.lpstrFilter = TEXT ("YM files(*.*)\0*.YM\0");
  l_sOpenfilename.nFilterIndex = 1;
  l_sOpenfilename.lpstrInitialDir = NULL;
  l_sOpenfilename.lpstrFileTitle = l_szFileName;
  l_sOpenfilename.nMaxFileTitle = sizeof (l_szFileName);
  l_sOpenfilename.Flags = OFN_FILEMUSTEXIST;

  /* Open common dialog box and if user select a file. */
  if(FALSE != GetOpenFileName (&l_sOpenfilename))
  {
    /* If true YM. */
    if (TRUE == bYm_CheckFileOK (l_szFilePath))
    {
      /* Add to play-list. */
      SendDlgItemMessage (p_hDialog, ID_PLAYLIST, LB_ADDSTRING, 0, (LPARAM)l_szFilePath);

      /* Get play-list length. */
      g_ui32PlaylistSize = SendDlgItemMessage (p_hDialog, ID_PLAYLIST, LB_GETCOUNT, 0, 0);

      /* Enable PLAY button. */
      EnableWindow (GetDlgItem (p_hDialog, ID_START), TRUE);
    }
  }
}


/*==============================================================================
Function    :   vFileSub

Describe    :   Remove item from play-list.

Parameters  :   p_hDialog = .

Returns     :   None.
==============================================================================*/
static VOID vFileSub (HWND p_hDialog)
{
  /* Locals variables declaration. */
  INT l_iCurrentSel;

  /* If not currently playing. */
  if (FALSE == bIsPlaying (p_hDialog) )
  {
    /* Get current selection. */
    l_iCurrentSel = SendDlgItemMessage (p_hDialog, ID_PLAYLIST, LB_GETCURSEL, 0, 0);
    if (LB_ERR != l_iCurrentSel)
    {
      /* Remove from play-list and get new length. */
      g_ui32PlaylistSize = SendDlgItemMessage (p_hDialog, ID_PLAYLIST, LB_DELETESTRING, l_iCurrentSel, 0);

      /* If play-list empty. */
      if (0u == g_ui32PlaylistSize)
      {
        /* Disable PLAY button. */
        EnableWindow (GetDlgItem (p_hDialog, ID_START), FALSE);
      }
    }
  }
}


/*==============================================================================
Function    :   eSetLoopMode

Describe    :   .

Parameters  :   p_hDialog =
                p_eLoop = .

Returns     :   eUkxError_t.
==============================================================================*/
static eUkxError_t eSetLoopMode (HWND p_hDialog, eUkLoopMode_t p_eLoop)
{
  /* Locals variables declaration. */
  eUkxError_t l_eReturn = UKXERROR_PARAM;

  /* Inputs parameters checking. */
  if (   (NULL != p_hDialog)
      && (UKLOOPMODE_MAX > p_eLoop) )
  {
    switch (p_eLoop)
    {
      case UKLOOPMODE_NO:
        CheckMenuItem (GetMenu(p_hDialog), ID_LOOP_NO,        MF_CHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_LOOP_PLAYLIST,  MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_LOOP_SONG,      MF_UNCHECKED);
        vYM_SetLoopMode (YM_NOLOOP);
        break;

      case UKLOOPMODE_SONG:
        CheckMenuItem (GetMenu(p_hDialog), ID_LOOP_NO,        MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_LOOP_PLAYLIST,  MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_LOOP_SONG,      MF_CHECKED);
        vYM_SetLoopMode (YM_LOOP);
        break;

      case UKLOOPMODE_PLAYLIST:
        CheckMenuItem (GetMenu(p_hDialog), ID_LOOP_NO,        MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_LOOP_PLAYLIST,  MF_CHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_LOOP_SONG,      MF_UNCHECKED);
        vYM_SetLoopMode (YM_NOLOOP);
        break;

      case UKLOOPMODE_MAX:
      default:
        break;
    }

    /* Setting done. */
    l_eReturn = UKXERROR_NO;
  }
  else
  {
    /* Nothing to do. */
  }

  return (l_eReturn);
}


/*==============================================================================
Function    :   eSetChannelxState

Describe    :   .

Parameters  :   .

Returns     :   eUkxError_t.
==============================================================================*/
static eUkxError_t eSetChannelxState (  HWND p_hDialog
                                      , e_AyChannel_t p_eChannel
                                      , eUkxState_t p_eState)
{
  /* Locals variables declaration. */
  eUkxError_t l_eReturn = UKXERROR_PARAM;

  /* Inputs parameters checking. */
  if (   (NULL != p_hDialog)
      && (AYCHANNEL_MAX > p_eChannel)
      && (UKXSTATE_MAX > p_eState) )
  {
    switch (p_eChannel)
    {
      case AYCHANNEL_A:
        if (UKXSTATE_ON == p_eState)
        {
          SendDlgItemMessage (p_hDialog, IDCHANNELA, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)g_hChannelOn);
          g_byChannelState ^= 0x01;
        }
        else
        {
          SendDlgItemMessage (p_hDialog, IDCHANNELA, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)g_hChannelOff);
          g_byChannelState &= (~0x01);
        }
        g_eChannelAState = p_eState;
        break;

      case AYCHANNEL_B:
        if (UKXSTATE_ON == p_eState)
        {
          SendDlgItemMessage (p_hDialog, IDCHANNELB, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)g_hChannelOn);
          g_byChannelState ^= 0x02;
        }
        else
        {
          SendDlgItemMessage (p_hDialog, IDCHANNELB, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)g_hChannelOff);
          g_byChannelState &= (~0x02);
        }
        g_eChannelBState = p_eState;
        break;

      case AYCHANNEL_C:
        if (UKXSTATE_ON == p_eState)
        {
          SendDlgItemMessage (p_hDialog, IDCHANNELC, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)g_hChannelOn);
          g_byChannelState ^= 0x04;
        }
        else
        {
          SendDlgItemMessage (p_hDialog, IDCHANNELC, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)g_hChannelOff);
          g_byChannelState &= (~0x04);
        }
        g_eChannelCState = p_eState;
        break;

      case AYCHANNEL_MAX:
      default:
        break;
    }

    /* Save new channel state. */
    vYm_SetChannelState (g_byChannelState);

    /* Setting done. */
    l_eReturn = UKXERROR_NO;
  }
  else
  {
    /* Nothing to do. */
  }

  return (l_eReturn);
}


/*==============================================================================
Function    :   vToggleStartButtonState

Describe    :   .

Parameters  :   p_hDialog = .

Returns     :   None.
==============================================================================*/
static VOID vToggleStartButtonState (HWND p_hDialog)
{
  /* Is current state is STOP ?*/
  if (FALSE == bIsPlaying (p_hDialog))
  {
    /* Set current selection. */
    SendDlgItemMessage (p_hDialog, ID_PLAYLIST, LB_SETCURSEL, (WPARAM)g_ui32PlaylistIndex, 0);

    /* Get current file path. */
    SendDlgItemMessage (p_hDialog, ID_PLAYLIST, LB_GETTEXT, g_ui32PlaylistIndex, (LPARAM)g_sYmInit.szFileName);

    /* Set AY frequency. */
    g_sYmInit.dwAYFrequency = g_dwSoundEngineAyFrequency;

    /* Try to read YM file. */
    if (TRUE == bYm_Init (&g_sYmInit)) {

      /* Print song name, author name, song comment and filename. */
      SendDlgItemMessage (p_hDialog, IDSONGNAME, WM_SETTEXT, 0, (LPARAM)g_sYmInit.szSongName);
      SendDlgItemMessage (p_hDialog, IDAUTHORNAME, WM_SETTEXT, 0, (LPARAM)g_sYmInit.szAuthorName);
      SendDlgItemMessage (p_hDialog, IDSONGCOMMENT, WM_SETTEXT, 0, (LPARAM)g_sYmInit.szSongComment);

      /* Set Time line range and position. */
      SendDlgItemMessage (p_hDialog, IDTIMELINE, TBM_SETRANGE, FALSE, (LPARAM)MAKELONG(u32Ym_GetSongLength()>>16, u32Ym_GetSongLength()&0xFFFF));
      SendDlgItemMessage (p_hDialog, IDTIMELINE, TBM_SETPOS, (WPARAM)1, (LPARAM)u32Ym_GetPosition());

      /* Update AY frequency. */
      g_dwSoundEngineAyFrequency = g_sYmInit.dwAYFrequency;

      /* All unchecked. */
      CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_AMSTRAD,   MF_UNCHECKED);
      CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_ATARI,     MF_UNCHECKED);
      CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_SINCLAIR,  MF_UNCHECKED);
      CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_MSX,       MF_UNCHECKED);
      CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_EXPE,      MF_UNCHECKED);

      /* Witch target ? (Update combo box target). */
      switch (g_sYmInit.dwAYFrequency) {
        case AY_AMSTRAD_FREQUENCY: {
          CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_AMSTRAD, MF_CHECKED);
          break;
        }

        case AY_ATARI_FREQUENCY: {
          CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_ATARI, MF_CHECKED);
          break;
        }

        case AY_SINCLAIR_FREQUENCY: {
          CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_SINCLAIR, MF_CHECKED);
          break;
        }

        default: {
          break;
        }
      }

      /* Enable rendering start. */
      g_bAudioRenderingStart = TRUE;
    }

    /* Change button text. */
    SendDlgItemMessage (p_hDialog, ID_START, WM_SETTEXT, 0, (LPARAM)"STOP");
  }
  else /* PLAY state. */
  {
    /* Quit player. */
    vYm_Quit ();

    /* End rendering. */
    g_bAudioRenderingStart = FALSE;

    /* Wait end rendering. */
    while (FALSE != g_bRenderingInProgress);

    /* Change button text. */
    SendDlgItemMessage (p_hDialog, ID_START, WM_SETTEXT, 0, (LPARAM)"PLAY");
  }
}


/*==============================================================================
Function    :   vEvent_Playlist

Describe    :   .

Parameters  :   p_hDialog = .

Returns     :   None.
==============================================================================*/
static VOID vEvent_Playlist (HWND p_hDialog)
{
  /* If current playing song. */
  if (TRUE == bIsPlaying (p_hDialog) )
  {
    /* Toggle button state. */
    vToggleStartButtonState (p_hDialog);
  }

  /* Get current selection. */
  g_ui32PlaylistIndex = SendDlgItemMessage (p_hDialog, ID_PLAYLIST, LB_GETCURSEL, 0, 0);

  /* Send a START button clicked notification to window. */
  SendMessage (p_hDialog, WM_COMMAND, (WPARAM)MAKELONG (ID_START, BN_CLICKED),(LPARAM)p_hDialog);
}


/*==============================================================================
Function    :

Describe    :   .

Parameters  :   .

Returns     :   .
==============================================================================*/
static eUkxError_t eSetAyFrequency (HWND p_hDialog, eUkFreq_t p_eFrequency)
{
  /* Locals variables declaration. */
  eUkxError_t l_eErr = UKXERROR_PARAM;

  if (NULL != p_hDialog)
  {
    switch (p_eFrequency)
    {
      case UKFREQ_ATARI:
      {
        g_dwSoundEngineAyFrequency = AY_ATARI_FREQUENCY;
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_AMSTRAD,   MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_ATARI,     MF_CHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_SINCLAIR,  MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_MSX,       MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_EXPE,      MF_UNCHECKED);
        break;
      }

      case UKFREQ_SINCLAIR:
      {
        g_dwSoundEngineAyFrequency = AY_SINCLAIR_FREQUENCY;
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_AMSTRAD,   MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_ATARI,     MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_SINCLAIR,  MF_CHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_MSX,       MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_EXPE,      MF_UNCHECKED);

        break;
      }

      case UKFREQ_MSX:
      {
        g_dwSoundEngineAyFrequency = AY_MSX_FREQUENCY;
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_AMSTRAD,   MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_ATARI,     MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_SINCLAIR,  MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_MSX,       MF_CHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_EXPE,      MF_UNCHECKED);

        break;
      }

      case UKFREQ_EXPE:
      {
        g_dwSoundEngineAyFrequency = AY_OVERCLOCK;
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_AMSTRAD,   MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_ATARI,     MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_SINCLAIR,  MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_MSX,       MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_EXPE,      MF_CHECKED);

        break;
      }

      case UKFREQ_AMSTRAD:
      default:
      {
        g_dwSoundEngineAyFrequency = AY_AMSTRAD_FREQUENCY;
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_AMSTRAD,   MF_CHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_ATARI,     MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_SINCLAIR,  MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_MSX,       MF_UNCHECKED);
        CheckMenuItem (GetMenu(p_hDialog), ID_FREQ_EXPE,      MF_UNCHECKED);

        break;
      }
    }
  }
  else
  {
    /* Nothing to do. */
  }

  return (l_eErr);
}


/*==============================================================================
Function    :

Describe    :   .

Parameters  :   .

Returns     :   .
==============================================================================*/
static eUkxError_t eWavFormInit (HWND p_hDialog)
{
  /* Locals variables declaration. */
  eUkxError_t l_eErr = UKXERROR_PARAM;

  if (NULL != p_hDialog)
  {
    // Init Waveform channel left (A+B) OpenGL window
    g_hDCWaveForm[CHANNEL_LEFT] = GetDC (GetDlgItem (p_hDialog, IDWAVEAB));
    ZeroMemory (&g_pfdWaveForm[CHANNEL_LEFT], sizeof (g_pfdWaveForm[CHANNEL_LEFT]));
    g_pfdWaveForm[CHANNEL_LEFT].nSize = sizeof (g_pfdWaveForm[CHANNEL_LEFT]);
    g_pfdWaveForm[CHANNEL_LEFT].nVersion = 1;
    g_pfdWaveForm[CHANNEL_LEFT].dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    g_pfdWaveForm[CHANNEL_LEFT].iPixelType = PFD_TYPE_RGBA;
    g_pfdWaveForm[CHANNEL_LEFT].cColorBits = 32;
    g_pfdWaveForm[CHANNEL_LEFT].cRedBits = 8;
    g_pfdWaveForm[CHANNEL_LEFT].cRedShift = 16;
    g_pfdWaveForm[CHANNEL_LEFT].cGreenBits = 8;
    g_pfdWaveForm[CHANNEL_LEFT].cGreenShift = 8;
    g_pfdWaveForm[CHANNEL_LEFT].cBlueBits = 8;
    g_pfdWaveForm[CHANNEL_LEFT].cBlueShift = 0;
    g_pfdWaveForm[CHANNEL_LEFT].cAlphaBits = 0;
    g_pfdWaveForm[CHANNEL_LEFT].cAlphaShift = 0;
    g_pfdWaveForm[CHANNEL_LEFT].cAccumBits = 64;
    g_pfdWaveForm[CHANNEL_LEFT].cAccumRedBits = 16;
    g_pfdWaveForm[CHANNEL_LEFT].cAccumGreenBits = 16;
    g_pfdWaveForm[CHANNEL_LEFT].cAccumBlueBits = 16;
    g_pfdWaveForm[CHANNEL_LEFT].cAccumAlphaBits = 0;
    g_pfdWaveForm[CHANNEL_LEFT].cDepthBits = 32;
    g_pfdWaveForm[CHANNEL_LEFT].cStencilBits = 8;
    g_pfdWaveForm[CHANNEL_LEFT].cAuxBuffers = 0;
    g_pfdWaveForm[CHANNEL_LEFT].iLayerType = PFD_MAIN_PLANE;
    g_pfdWaveForm[CHANNEL_LEFT].bReserved = 0;
    g_pfdWaveForm[CHANNEL_LEFT].dwLayerMask = 0;
    g_pfdWaveForm[CHANNEL_LEFT].dwVisibleMask = 0;
    g_pfdWaveForm[CHANNEL_LEFT].dwDamageMask = 0;
    g_i32FormatWaveForm[CHANNEL_LEFT] = ChoosePixelFormat (g_hDCWaveForm[CHANNEL_LEFT], &g_pfdWaveForm[CHANNEL_LEFT]);
    SetPixelFormat (g_hDCWaveForm[CHANNEL_LEFT], g_i32FormatWaveForm[CHANNEL_LEFT], &g_pfdWaveForm[CHANNEL_LEFT]);
    g_hRCWaveForm[CHANNEL_LEFT] = wglCreateContext (g_hDCWaveForm[CHANNEL_LEFT]);

    // Init Waveform channel right (B+C) OpenGL window
    g_hDCWaveForm[CHANNEL_RIGHT] = GetDC (GetDlgItem (p_hDialog, IDWAVEBC));
    ZeroMemory ( &g_pfdWaveForm[CHANNEL_RIGHT], sizeof (g_pfdWaveForm[CHANNEL_RIGHT]));
    g_pfdWaveForm[CHANNEL_RIGHT].nSize = sizeof (g_pfdWaveForm[CHANNEL_RIGHT]);
    g_pfdWaveForm[CHANNEL_RIGHT].nVersion = 1;
    g_pfdWaveForm[CHANNEL_RIGHT].dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    g_pfdWaveForm[CHANNEL_RIGHT].iPixelType = PFD_TYPE_RGBA;
    g_pfdWaveForm[CHANNEL_RIGHT].cColorBits = 32;
    g_pfdWaveForm[CHANNEL_RIGHT].cRedBits = 8;
    g_pfdWaveForm[CHANNEL_RIGHT].cRedShift = 16;
    g_pfdWaveForm[CHANNEL_RIGHT].cGreenBits = 8;
    g_pfdWaveForm[CHANNEL_RIGHT].cGreenShift = 8;
    g_pfdWaveForm[CHANNEL_RIGHT].cBlueBits = 8;
    g_pfdWaveForm[CHANNEL_RIGHT].cBlueShift = 0;
    g_pfdWaveForm[CHANNEL_RIGHT].cAlphaBits = 0;
    g_pfdWaveForm[CHANNEL_RIGHT].cAlphaShift = 0;
    g_pfdWaveForm[CHANNEL_RIGHT].cAccumBits = 64;
    g_pfdWaveForm[CHANNEL_RIGHT].cAccumRedBits = 16;
    g_pfdWaveForm[CHANNEL_RIGHT].cAccumGreenBits = 16;
    g_pfdWaveForm[CHANNEL_RIGHT].cAccumBlueBits = 16;
    g_pfdWaveForm[CHANNEL_RIGHT].cAccumAlphaBits = 0;
    g_pfdWaveForm[CHANNEL_RIGHT].cDepthBits = 32;
    g_pfdWaveForm[CHANNEL_RIGHT].cStencilBits = 8;
    g_pfdWaveForm[CHANNEL_RIGHT].cAuxBuffers = 0;
    g_pfdWaveForm[CHANNEL_RIGHT].iLayerType = PFD_MAIN_PLANE;
    g_pfdWaveForm[CHANNEL_RIGHT].bReserved = 0;
    g_pfdWaveForm[CHANNEL_RIGHT].dwLayerMask = 0;
    g_pfdWaveForm[CHANNEL_RIGHT].dwVisibleMask = 0;
    g_pfdWaveForm[CHANNEL_RIGHT].dwDamageMask = 0;
    g_i32FormatWaveForm[CHANNEL_RIGHT] = ChoosePixelFormat (g_hDCWaveForm[CHANNEL_RIGHT], &g_pfdWaveForm[CHANNEL_RIGHT]);
    SetPixelFormat ( g_hDCWaveForm[CHANNEL_RIGHT], g_i32FormatWaveForm[CHANNEL_RIGHT], &g_pfdWaveForm[CHANNEL_RIGHT]);
    g_hRCWaveForm[CHANNEL_RIGHT] = wglCreateContext (g_hDCWaveForm[CHANNEL_RIGHT]);


    // Init Waveform channel right (B+C) OpenGL window
    g_hDCChannel[AYCHANNEL_A] = GetDC (GetDlgItem (p_hDialog, ID_VISU_CHANNELA));
    ZeroMemory ( &g_pfdChannel[AYCHANNEL_A], sizeof (g_pfdChannel[AYCHANNEL_A]));
    g_pfdChannel[AYCHANNEL_A].nSize = sizeof (g_pfdChannel[AYCHANNEL_A]);
    g_pfdChannel[AYCHANNEL_A].nVersion = 1;
    g_pfdChannel[AYCHANNEL_A].dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    g_pfdChannel[AYCHANNEL_A].iPixelType = PFD_TYPE_RGBA;
    g_pfdChannel[AYCHANNEL_A].cColorBits = 32;
    g_pfdChannel[AYCHANNEL_A].cRedBits = 8;
    g_pfdChannel[AYCHANNEL_A].cRedShift = 16;
    g_pfdChannel[AYCHANNEL_A].cGreenBits = 8;
    g_pfdChannel[AYCHANNEL_A].cGreenShift = 8;
    g_pfdChannel[AYCHANNEL_A].cBlueBits = 8;
    g_pfdChannel[AYCHANNEL_A].cBlueShift = 0;
    g_pfdChannel[AYCHANNEL_A].cAlphaBits = 0;
    g_pfdChannel[AYCHANNEL_A].cAlphaShift = 0;
    g_pfdChannel[AYCHANNEL_A].cAccumBits = 64;
    g_pfdChannel[AYCHANNEL_A].cAccumRedBits = 16;
    g_pfdChannel[AYCHANNEL_A].cAccumGreenBits = 16;
    g_pfdChannel[AYCHANNEL_A].cAccumBlueBits = 16;
    g_pfdChannel[AYCHANNEL_A].cAccumAlphaBits = 0;
    g_pfdChannel[AYCHANNEL_A].cDepthBits = 32;
    g_pfdChannel[AYCHANNEL_A].cStencilBits = 8;
    g_pfdChannel[AYCHANNEL_A].cAuxBuffers = 0;
    g_pfdChannel[AYCHANNEL_A].iLayerType = PFD_MAIN_PLANE;
    g_pfdChannel[AYCHANNEL_A].bReserved = 0;
    g_pfdChannel[AYCHANNEL_A].dwLayerMask = 0;
    g_pfdChannel[AYCHANNEL_A].dwVisibleMask = 0;
    g_pfdChannel[AYCHANNEL_A].dwDamageMask = 0;
    g_i32FormatChannel[AYCHANNEL_A] = ChoosePixelFormat (g_hDCChannel[AYCHANNEL_A], &g_pfdChannel[AYCHANNEL_A]);
    SetPixelFormat ( g_hDCChannel[AYCHANNEL_A], g_i32FormatChannel[AYCHANNEL_A], &g_pfdChannel[AYCHANNEL_A]);
    g_hRCChannel[AYCHANNEL_A] = wglCreateContext (g_hDCChannel[AYCHANNEL_A]);

    // Init Waveform channel right (B+C) OpenGL window
    g_hDCChannel[AYCHANNEL_B] = GetDC (GetDlgItem (p_hDialog, ID_VISU_CHANNELB));
    ZeroMemory ( &g_pfdChannel[AYCHANNEL_B], sizeof (g_pfdChannel[AYCHANNEL_B]));
    g_pfdChannel[AYCHANNEL_B].nSize = sizeof (g_pfdChannel[AYCHANNEL_B]);
    g_pfdChannel[AYCHANNEL_B].nVersion = 1;
    g_pfdChannel[AYCHANNEL_B].dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    g_pfdChannel[AYCHANNEL_B].iPixelType = PFD_TYPE_RGBA;
    g_pfdChannel[AYCHANNEL_B].cColorBits = 32;
    g_pfdChannel[AYCHANNEL_B].cRedBits = 8;
    g_pfdChannel[AYCHANNEL_B].cRedShift = 16;
    g_pfdChannel[AYCHANNEL_B].cGreenBits = 8;
    g_pfdChannel[AYCHANNEL_B].cGreenShift = 8;
    g_pfdChannel[AYCHANNEL_B].cBlueBits = 8;
    g_pfdChannel[AYCHANNEL_B].cBlueShift = 0;
    g_pfdChannel[AYCHANNEL_B].cAlphaBits = 0;
    g_pfdChannel[AYCHANNEL_B].cAlphaShift = 0;
    g_pfdChannel[AYCHANNEL_B].cAccumBits = 64;
    g_pfdChannel[AYCHANNEL_B].cAccumRedBits = 16;
    g_pfdChannel[AYCHANNEL_B].cAccumGreenBits = 16;
    g_pfdChannel[AYCHANNEL_B].cAccumBlueBits = 16;
    g_pfdChannel[AYCHANNEL_B].cAccumAlphaBits = 0;
    g_pfdChannel[AYCHANNEL_B].cDepthBits = 32;
    g_pfdChannel[AYCHANNEL_B].cStencilBits = 8;
    g_pfdChannel[AYCHANNEL_B].cAuxBuffers = 0;
    g_pfdChannel[AYCHANNEL_B].iLayerType = PFD_MAIN_PLANE;
    g_pfdChannel[AYCHANNEL_B].bReserved = 0;
    g_pfdChannel[AYCHANNEL_B].dwLayerMask = 0;
    g_pfdChannel[AYCHANNEL_B].dwVisibleMask = 0;
    g_pfdChannel[AYCHANNEL_B].dwDamageMask = 0;
    g_i32FormatChannel[AYCHANNEL_B] = ChoosePixelFormat (g_hDCChannel[AYCHANNEL_B], &g_pfdChannel[AYCHANNEL_B]);
    SetPixelFormat ( g_hDCChannel[AYCHANNEL_B], g_i32FormatChannel[AYCHANNEL_B], &g_pfdChannel[AYCHANNEL_B]);
    g_hRCChannel[AYCHANNEL_B] = wglCreateContext (g_hDCChannel[AYCHANNEL_B]);

    // Init Waveform channel right (B+C) OpenGL windowAYCHANNEL_C
    g_hDCChannel[AYCHANNEL_C] = GetDC (GetDlgItem (p_hDialog, ID_VISU_CHANNELC));
    ZeroMemory ( &g_pfdChannel[AYCHANNEL_C], sizeof (g_pfdChannel[AYCHANNEL_C]));
    g_pfdChannel[AYCHANNEL_C].nSize = sizeof (g_pfdChannel[AYCHANNEL_C]);
    g_pfdChannel[AYCHANNEL_C].nVersion = 1;
    g_pfdChannel[AYCHANNEL_C].dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    g_pfdChannel[AYCHANNEL_C].iPixelType = PFD_TYPE_RGBA;
    g_pfdChannel[AYCHANNEL_C].cColorBits = 32;
    g_pfdChannel[AYCHANNEL_C].cRedBits = 8;
    g_pfdChannel[AYCHANNEL_C].cRedShift = 16;
    g_pfdChannel[AYCHANNEL_C].cGreenBits = 8;
    g_pfdChannel[AYCHANNEL_C].cGreenShift = 8;
    g_pfdChannel[AYCHANNEL_C].cBlueBits = 8;
    g_pfdChannel[AYCHANNEL_C].cBlueShift = 0;
    g_pfdChannel[AYCHANNEL_C].cAlphaBits = 0;
    g_pfdChannel[AYCHANNEL_C].cAlphaShift = 0;
    g_pfdChannel[AYCHANNEL_C].cAccumBits = 64;
    g_pfdChannel[AYCHANNEL_C].cAccumRedBits = 16;
    g_pfdChannel[AYCHANNEL_C].cAccumGreenBits = 16;
    g_pfdChannel[AYCHANNEL_C].cAccumBlueBits = 16;
    g_pfdChannel[AYCHANNEL_C].cAccumAlphaBits = 0;
    g_pfdChannel[AYCHANNEL_C].cDepthBits = 32;
    g_pfdChannel[AYCHANNEL_C].cStencilBits = 8;
    g_pfdChannel[AYCHANNEL_C].cAuxBuffers = 0;
    g_pfdChannel[AYCHANNEL_C].iLayerType = PFD_MAIN_PLANE;
    g_pfdChannel[AYCHANNEL_C].bReserved = 0;
    g_pfdChannel[AYCHANNEL_C].dwLayerMask = 0;
    g_pfdChannel[AYCHANNEL_C].dwVisibleMask = 0;
    g_pfdChannel[AYCHANNEL_C].dwDamageMask = 0;
    g_i32FormatChannel[AYCHANNEL_C] = ChoosePixelFormat (g_hDCChannel[AYCHANNEL_C], &g_pfdChannel[AYCHANNEL_C]);
    SetPixelFormat ( g_hDCChannel[AYCHANNEL_C], g_i32FormatChannel[AYCHANNEL_C], &g_pfdChannel[AYCHANNEL_C]);
    g_hRCChannel[AYCHANNEL_C] = wglCreateContext (g_hDCChannel[AYCHANNEL_C]);

    // Correct texture distortion in perpective projection
    glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  }
  else
  {
    /* Nothing to do .*/
  }

  return (l_eErr);
}


/*==============================================================================
Function    :

Describe    :   .

Parameters  :   .

Returns     :   .
==============================================================================*/
static eUkxError_t eSetFilterValue (HWND p_hDialog, FLOAT p_fValue)
{
  /* Locals variables declaration. */
  eUkxError_t l_eErr = UKXERROR_PARAM;
  TCHAR l_szTempString[MAX_PATH];

  if (NULL != p_hDialog)
  {
    sprintf (l_szTempString, "%i Hz", (WORD) p_fValue);
    SendDlgItemMessage (p_hDialog, IDSTATICSCUTOFF, WM_SETTEXT, 0, (LPARAM)l_szTempString);
    g_fFilterCutOff = p_fValue;
  }
  else
  {
    /* Nothing to do. */
  }

  return (l_eErr);
}


/*==============================================================================
Function    :

Describe    :   .

Parameters  :   .

Returns     :   .
==============================================================================*/
static eUkxError_t eSetVolumeValue (HWND p_hDialog, eUkChannel_t p_eChannel, UINT8 p_u8Value)
{
  /* Locals variables declaration. */
  eUkxError_t l_eErr = UKXERROR_PARAM;
  DWORD l_dwVolume;

  if (NULL != p_hDialog)
  {
    if (p_eChannel == UKCHANNEL_LEFT)
    {
      uiSoundEngineGetVolume (&l_dwVolume);
      l_dwVolume &= 0x0000FFFFUL;
      l_dwVolume |= (p_u8Value<<24);

    //  SendDlgItemMessage (p_hDialog, IDMASTER_L, TBM_SETPOS, (WPARAM)1, (LPARAM)255 - p_u8Value);
      uiSoundEngineSetVolume (l_dwVolume);
    }
    else /* Channel right */
    {
      uiSoundEngineGetVolume (&l_dwVolume);
      l_dwVolume &= 0xFFFF0000UL;
      l_dwVolume |= (p_u8Value<<8);

   //   SendDlgItemMessage (p_hDialog, IDMASTER_R, TBM_SETPOS, (WPARAM)1, (LPARAM)255 - p_u8Value);
    }

    /* Update volume. */
    uiSoundEngineSetVolume (l_dwVolume);
  }
  else
  {
    /* Nothing to do. */
  }

  return (l_eErr);
}


/*===================================================================================================
Function    : vWaveFormRender.

Describe    : Render Wave form  for left channel (A+B) or right channel (B+C).

Parameters  : p_uchChannel = CHANNEL_LEFT or CHANNEL_RIGHT.

Returns     : None.
===================================================================================================*/
static VOID vWaveFormRender (UCHAR p_uchChannel, BOOL p_bChannelStateOn)
{
  // Locals variables declaration
  WORD l_wAYWavForm[WAVFORM_SIZE][3];
  WORD l_wIndex;
  FLOAT l_fTime;
  FLOAT l_fValue;

  // Select Spectrum render GL context
  wglMakeCurrent (g_hDCWaveForm[p_uchChannel], g_hRCWaveForm[p_uchChannel]);

  // Clear GL buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (TRUE == p_bChannelStateOn)
  {
    // Get AY data
    bYm_GetWavForm (&l_wAYWavForm[0][0]);

    l_wIndex = 0;
    for (l_fTime = -1.0f; l_fTime < 1.0f; l_fTime += (2.0f / WAVFORM_SIZE))
    {
      if (CHANNEL_LEFT == p_uchChannel)
      {
        if ((g_byChannelState&0x01) && (g_byChannelState&0x02))
        {
          // Compute Left
          l_fValue = (2.0f * (((l_wAYWavForm[l_wIndex][0] * 0.66f) + (l_wAYWavForm[l_wIndex][1] * 0.33f)) / 65535.0f)) - 1.0f;
        }
        else if (g_byChannelState&0x01)
        {
          l_fValue = (2.0f * (((l_wAYWavForm[l_wIndex][0] * 0.66f)) / 65535.0f)) - 1.0f;
        }
        else if (g_byChannelState&0x02)
        {
          l_fValue = (2.0f * (((l_wAYWavForm[l_wIndex][1] * 0.33f)) / 65535.0f)) - 1.0f;
        }
        else
        {
          l_fValue = 0.0f;
        }
      }
      else if (CHANNEL_RIGHT == p_uchChannel)
      {
        // Compute Right
        if ((g_byChannelState&0x04) && (g_byChannelState&0x02))
        {
          // Compute Left
          l_fValue = (2.0f * (((l_wAYWavForm[l_wIndex][2] * 0.66f) + (l_wAYWavForm[l_wIndex][1] * 0.33f)) / 65535.0f)) - 1.0f;
        }
        else if (g_byChannelState&0x04)
        {
          l_fValue = (2.0f * (((l_wAYWavForm[l_wIndex][2] * 0.66f)) / 65535.0f)) - 1.0f;
        }
        else if (g_byChannelState&0x02)
        {
          l_fValue = (2.0f * (((l_wAYWavForm[l_wIndex][1] * 0.33f)) / 65535.0f)) - 1.0f;
        }
        else
        {
          l_fValue = 0.0f;
        }
      }
      else
      {
        // No value
        l_fValue = 0.0f;
      }

      glBegin(GL_QUADS);
      if (CHANNEL_LEFT == p_uchChannel)
      {
        glColor3f (1.0f, 0.0f, 0.0f);
      }
      else
      {
        glColor3f (0.0f, 1.0f, 0.0f);
      }
      glVertex3d (l_fTime        , l_fValue - 0.01f, 0.0f);
      glVertex3d (l_fTime + 0.01f, l_fValue - 0.01f, 0.0f);
      glVertex3d (l_fTime + 0.01f, l_fValue + 0.01f, 0.0f);
      glVertex3d (l_fTime        , l_fValue + 0.01f, 0.0f);
      glEnd();

      l_wIndex++;
    }
  }
  else
  {
    for (l_fTime = -1.0f; l_fTime < 1.0f; l_fTime += 0.01f)
    {
      glBegin(GL_QUADS);
      if (CHANNEL_LEFT == p_uchChannel)
      {
        glColor3f (1.0f, 0.0f, 0.0f);
      }
      else
      {
        glColor3f (0.0f, 1.0f, 0.0f);
      }
      glVertex3d (l_fTime      , l_fTime - 0.01f, 0.0f);
      glVertex3d (l_fTime+0.01f, l_fTime - 0.01f, 0.0f);
      glVertex3d (l_fTime+0.01f, l_fTime + 0.01f, 0.0f);
      glVertex3d (l_fTime      , l_fTime + 0.01f, 0.0f);

      glVertex3d (l_fTime      , -l_fTime - 0.01f, 0.0f);
      glVertex3d (l_fTime+0.01f, -l_fTime - 0.01f, 0.0f);
      glVertex3d (l_fTime+0.01f, -l_fTime + 0.01f, 0.0f);
      glVertex3d (l_fTime      , -l_fTime + 0.01f, 0.0f);

      glEnd();
    }
  }

  // Swap buffer
  SwapBuffers (g_hDCWaveForm[p_uchChannel]);
}


/*===================================================================================================
Function    : vWaveFormRender.

Describe    : .

Parameters  : .

Returns     : None.
===================================================================================================*/
static VOID vChannelRender (e_AyChannel_t p_eChannel, eUkxState_t p_eState)
{
  // Locals variables declaration
  WORD l_wAYWavForm[WAVFORM_SIZE][3];
  WORD l_wIndex;
  FLOAT l_fTime;
  FLOAT l_fValue;

  // Select Spectrum render GL context
  wglMakeCurrent (g_hDCChannel[p_eChannel], g_hRCChannel[p_eChannel]);

  // Clear GL buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (TRUE == p_eState)
  {
    // Get AY data
    bYm_GetWavForm (&l_wAYWavForm[0][0]);

    l_wIndex = 0;
    for (l_fTime = -1.0f; l_fTime < 1.0f; l_fTime += (2.0f / WAVFORM_SIZE))
    {
      if (p_eChannel == AYCHANNEL_A)
      {
        l_fValue = (2.0f * (l_wAYWavForm[l_wIndex][0] / 65535.0f)) - 1.0f;
      }
      else if (p_eChannel == AYCHANNEL_B)
      {
        l_fValue = (2.0f * (l_wAYWavForm[l_wIndex][1] / 65535.0f)) - 1.0f;
      }
      else /* C */
      {
        l_fValue = (2.0f * (l_wAYWavForm[l_wIndex][2] / 65535.0f)) - 1.0f;
      }

      glBegin(GL_QUADS);

      if (p_eChannel == AYCHANNEL_A)
      {
        glColor3f (1.0f, 0.0f, 0.0f);
      }
      else if (p_eChannel == AYCHANNEL_B)
      {
        glColor3f (0.0f, 1.0f, 0.0f);
      }
      else
      {
        glColor3f (0.0f, 1.0f, 1.0f);
      }

      glVertex3d (l_fTime        , l_fValue - 0.01f, 0.0f);
      glVertex3d (l_fTime + 0.01f, l_fValue - 0.01f, 0.0f);
      glVertex3d (l_fTime + 0.01f, l_fValue + 0.01f, 0.0f);
      glVertex3d (l_fTime        , l_fValue + 0.01f, 0.0f);
      glEnd();

      l_wIndex++;
    }
  }
  else
  {
    for (l_fTime = -1.0f; l_fTime < 1.0f; l_fTime += 0.01f)
    {
      glBegin(GL_QUADS);

      if (p_eChannel == AYCHANNEL_A)
      {
        glColor3f (1.0f, 0.0f, 0.0f);
      }
      else if (p_eChannel == AYCHANNEL_B)
      {
        glColor3f (0.0f, 1.0f, 0.0f);
      }
      else
      {
        glColor3f (0.0f, 0.0f, 1.0f);
      }

      glVertex3d (l_fTime      , l_fTime - 0.01f, 0.0f);
      glVertex3d (l_fTime+0.01f, l_fTime - 0.01f, 0.0f);
      glVertex3d (l_fTime+0.01f, l_fTime + 0.01f, 0.0f);
      glVertex3d (l_fTime      , l_fTime + 0.01f, 0.0f);

      glVertex3d (l_fTime      , -l_fTime - 0.01f, 0.0f);
      glVertex3d (l_fTime+0.01f, -l_fTime - 0.01f, 0.0f);
      glVertex3d (l_fTime+0.01f, -l_fTime + 0.01f, 0.0f);
      glVertex3d (l_fTime      , -l_fTime + 0.01f, 0.0f);

      glEnd();
    }
  }

  // Swap buffer
  SwapBuffers (g_hDCChannel[p_eChannel]);
}


/*===================================================================================================
Function    : vGetTimeElapsed

Describe    : TODO.

Parameters  : TODO.

Returns     : TODO.
-----------------------------------------------------------------------------------------------------
Revision A  : Creation
===================================================================================================*/
static VOID vGetTimeElapsed (HWND p_hDialog)
{
  // Local variables definitions
  UINT l_uiTimeMs;
  UINT l_uiTimeS;
  UINT l_uiTimeMn;
  UINT l_uiTimeHr;
  TCHAR l_szTimeElapsed[30];

  if (TRUE == bIsPlaying (p_hDialog))
  {
    // Get position
    l_uiTimeMs = u32Ym_GetPosition ();

    // If end of song
    if (u32Ym_GetSongLength() == l_uiTimeMs)
    {
      /* Loop mode manager. */
      switch (g_eLoopMode)
      {
        case LOOP_SONG:
        {
          /* Nothing to do. */
          break;
        }

        case LOOP_PLAYLIST:
        {
          // Quit player
          vYm_Quit ();

          // End rendering
          g_bAudioRenderingStart = FALSE;

          // Wait end rendering
          while (FALSE != g_bRenderingInProgress);

          // Increment current selection
          g_ui32PlaylistIndex++;

          // If end of playlist
          if (g_ui32PlaylistSize == g_ui32PlaylistIndex)
          {
            // Reset playlist position
            g_ui32PlaylistIndex = 0;
          }

          // Send a START button clicked notification to window
          SendMessage (p_hDialog, WM_COMMAND, (WPARAM)MAKELONG (ID_START, BN_CLICKED),(LPARAM)p_hDialog);

          break;
        }

        case LOOP_NO:
        default:
        {
          // Quit player
          vYm_Quit ();

          // End rendering
          g_bAudioRenderingStart = FALSE;

          // Wait end rendering
          while (FALSE != g_bRenderingInProgress);

          // Increment current selection
          g_ui32PlaylistIndex++;

          // If not end of playlist
          if (g_ui32PlaylistSize > g_ui32PlaylistIndex)
          {
            // Send a START button clicked notification to window
            SendMessage (p_hDialog, WM_COMMAND, (WPARAM)MAKELONG (ID_START, BN_CLICKED),(LPARAM)p_hDialog);
          }
          else
          {
            // Write start button
            SendDlgItemMessage (p_hDialog, ID_START, WM_SETTEXT, 0, (LPARAM)"PLAY");
          }
          break;
        }
      }
    }
    /*else
    {*/
      // Set time line
      SendDlgItemMessage (p_hDialog, IDTIMELINE, TBM_SETPOS, (WPARAM)1, (LPARAM)l_uiTimeMs);

      // switch player frequency
      switch (g_sYmInit.wPlayerFrameHz)
      {
        case 50:  // 50Hz => 1 frame / 20 ms
        {
          l_uiTimeMs *= 20;
          break;
        }

        case 60:  // 60Hz => 1 frame / 16.6 ms
        {
          l_uiTimeMs *= 16.6f;
          break;
        }

        case 100:
        {
          l_uiTimeMs *= 10;
          break;
        }

        case 200:
        {
          l_uiTimeMs *= 5;
          break;
        }

        case 300:
        {
          l_uiTimeMs *= 3.33f;
          break;
        }

        default:
        {
          break;
        }
      }

      // Get hour
      l_uiTimeHr = l_uiTimeMs / 3600000;
      l_uiTimeMs -= (l_uiTimeHr * 3600000);

      // Get minute
      l_uiTimeMn = l_uiTimeMs / 60000;
      l_uiTimeMs -= (l_uiTimeMn * 60000);

      // Get seconds
      l_uiTimeS = l_uiTimeMs / 1000;
      l_uiTimeMs -= (l_uiTimeS * 1000);

      // Print hour only if necessary
      if (0 != l_uiTimeHr)
      {
        sprintf (l_szTimeElapsed,"%02d:%02d:%02d", l_uiTimeHr, l_uiTimeMn, l_uiTimeS);
      }
      else
      {
        sprintf (l_szTimeElapsed,"%02d:%02d", l_uiTimeMn, l_uiTimeS);
      }

      // Update control
      SetDlgItemText (p_hDialog, IDTIMEELAPSED, l_szTimeElapsed);
   // }
  }
}


