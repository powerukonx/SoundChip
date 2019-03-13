/*==============================================================================

                  ██╗   ██╗██╗  ██╗ ██████╗ ███╗   ██╗██╗  ██╗
                  ██║   ██║██║ ██╔╝██╔═══██╗████╗  ██║╚██╗██╔╝
                  ██║   ██║█████╔╝ ██║   ██║██╔██╗ ██║ ╚███╔╝
                  ██║   ██║██╔═██╗ ██║   ██║██║╚██╗██║ ██╔██╗
                  ╚██████╔╝██║  ██╗╚██████╔╝██║ ╚████║██╔╝ ██╗
                   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝

  File name:    soundengine.h
  Date:         23 09 2017
  Author:       Power
  Description:  Win32 sound engine - Header file.
============================================================================= */

/* =============================================================================
                                 DEBUG Section
============================================================================= */

#ifndef SOUNDENGINE_H_INCLUDED
  #define SOUNDENGINE_H_INCLUDED

  /* ===========================================================================
                            Public defines and typedefs
  =========================================================================== */
  #define SOUNDENGINE_MONO            (1)
  #define SOUNDENGINE_STEREO          (2)
  #define SOUNDENGINE_8BITS           (8)
  #define SOUNDENGINE_16BITS          (16)
  #define SOUNDENGINE_11025           (11025)
  #define SOUNDENGINE_22050           (22050)
  #define SOUNDENGINE_44100           (44100)
  #define SPEAKERCUTOFF               (3000.0)

  typedef BOOL (*RenderFunction)(VOID* pvParameters);

  typedef struct _SOUNDENGINE_ {
    DWORD dwSampleRate;                        /* Sample rate, ex: SOUNDENGINE_44100 */
    UINT8 byBitsPerSample;                      /* Bits per sample, ex: SOUNDENGINE_8BITS */
    UINT8 byNbChannels;                         /* Number of channel, ex: SOUNDENGINE_MONO */
    volatile BOOL* pbRenderingEnable;          /* Pointer to state of sound rendering */
    volatile DWORD* pdwAyFrequency;            /* Pointer to AY frequency */
    volatile BOOL* bLeftChannelEnable;         /* State of Left channel */
    volatile BOOL* bRightChannelEnable;        /* State of Right channel */
    volatile BOOL* bFilterEnable;              /* Sate of low-filter */
    volatile FLOAT* pfFilterCutOff;            /* Low pass filter cutoff pointer */
    RenderFunction b_Render;                   /* Pointer to audio rendering function */
  } s_SoundEngine_t;

  /* ===========================================================================
                        Public constants and variables
  =========================================================================== */
  extern volatile BOOL g_bRenderingInProgress;
  extern volatile DWORD g_dwSoundEngineAyFrequency;
  extern volatile BOOL g_bGetData;

  /* ===========================================================================
                        Public function declarations
  =========================================================================== */
  extern BOOL b_SoundEngineInit (s_SoundEngine_t* psoundengine);
  extern BOOL b_SoundEngineQuit (VOID);
  extern WORD w_SoundEngineGetVolume (VOID);
  extern MMRESULT uiSoundEngineSetVolume (DWORD p_dwVolume);
  extern MMRESULT uiSoundEngineGetVolume (PDWORD p_pdwVolume);
  extern VOID vSoundEngineSetFilterOn (VOID);
  extern VOID vSoundEngineSetFilterOff (VOID);
  extern VOID vSoundEngineSetFilterCutOffValue (FLOAT p_fFilterCutOffValue);
#endif  /* SOUNDENGINE_H_INCLUDED */

