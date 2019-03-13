/* =============================================================================

                  ██╗   ██╗██╗  ██╗ ██████╗ ███╗   ██╗██╗  ██╗
                  ██║   ██║██║ ██╔╝██╔═══██╗████╗  ██║╚██╗██╔╝
                  ██║   ██║█████╔╝ ██║   ██║██╔██╗ ██║ ╚███╔╝
                  ██║   ██║██╔═██╗ ██║   ██║██║╚██╗██║ ██╔██╗
                  ╚██████╔╝██║  ██╗╚██████╔╝██║ ╚████║██╔╝ ██╗
                   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝

  File name:    parser_ym.h
  Date:         23 07 2017
  Author:       Power.
  Description:  YM file parser - Header file.

============================================================================= */
#ifndef YMPLAYER_H_INCLUDED
  #define YMPLAYER_H_INCLUDED

  /* ===========================================================================
                                   DEBUG Section
  =========================================================================== */

  /* ===========================================================================
                            Public defines and typedefs
  =========================================================================== */

  #define AY_ATARI_FREQUENCY      (2000000)
  #define AY_AMSTRAD_FREQUENCY    (1000000)
  #define AY_SINCLAIR_FREQUENCY   (1773400)
  #define AY_MSX_FREQUENCY        (1789772.5)
  #define AY_OVERCLOCK            (8000000)
  #define WAVFORM_SIZE            ((UINT16)2048)

  #define YM_NOLOOP               ((UINT8)0)
  #define YM_LOOP                 ((UINT8)1)

  typedef struct _YMGETDATA_ {
    UINT8 byYMRegData[16];
    UINT8 bYMReady;
  } s_YmGetData_t;

  typedef struct _YMINIT_ {
    UINT32 dwAYFrequency;
    UINT16 wPlayerFrameHz;
    TCHAR szFileName[MAX_PATH];
    TCHAR szSongName[MAX_PATH];
    TCHAR szAuthorName[MAX_PATH];
    TCHAR szSongComment[MAX_PATH];
  } s_YmInit_t;

  typedef struct _YMUPDATE_ {
    UINT16 fAnalogChannelA_Out;
    UINT16 fAnalogChannelB_Out;
    UINT16 fAnalogChannelC_Out;
  } s_YmUpdate_t;

  /* ===========================================================================
                          Public constants and variables
  =========================================================================== */

  /* ===========================================================================
                          Public function declarations
  =========================================================================== */
  extern BOOL bYm_Init (VOID* p_pvParameters);
  extern BOOL bYm_Update (VOID* p_pvParameters);
  extern VOID vYm_Quit (VOID);
  extern VOID vYm_SetChannelState (UINT8 p_u8ChannelState);
  extern BOOL bYm_GetWavForm (UINT16* p_pu16WavForm);
  
  extern BOOL bYm_GetData (VOID* p_pvParameters);
  
  extern BOOL bYm_CheckFileOK (TCHAR* p_pszFilename);
  extern UINT32 u32Ym_GetPosition (VOID);
  extern UINT32 u32Ym_GetSongLength (VOID);
  extern VOID vYm_SetPosition (UINT32 p_u32Position);
  extern VOID vYM_SetLoopMode (UINT8 p_u8LoopMode);

#endif  /* YMPLAYER_H_INCLUDED */

