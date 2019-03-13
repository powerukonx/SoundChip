/*==============================================================================

                  ██╗   ██╗██╗  ██╗ ██████╗ ███╗   ██╗██╗  ██╗
                  ██║   ██║██║ ██╔╝██╔═══██╗████╗  ██║╚██╗██╔╝
                  ██║   ██║█████╔╝ ██║   ██║██╔██╗ ██║ ╚███╔╝
                  ██║   ██║██╔═██╗ ██║   ██║██║╚██╗██║ ██╔██╗
                  ╚██████╔╝██║  ██╗╚██████╔╝██║ ╚████║██╔╝ ██╗
                   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝

  File name:    soundengine.cpp
  Date:         23 09 2017
  Author:       Power
  Description:  Win32 sound engine - Main file.
============================================================================= */


/* =============================================================================
                                 DEBUG Section
============================================================================= */


/* =============================================================================
                                 Include Files
============================================================================= */
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <math.h>
#include "parser_ym.h"
#include "soundengine.h"


/* =============================================================================
                          Private defines and typedefs
============================================================================= */
#define WAVEOUT_BUFFER_SIZE     (4096)

typedef enum _THREAD_AUDIO_ {
    THREAD_AUDIO_RUN = 0u
  , THREAD_AUDIO_STOPPING
  , THREAD_AUDIO_END
} e_ThreadAudio_t;

typedef enum _WAVEOUT_SELECTBUFFER_ {
    WAVEOUT_SELECTBUFFER1 = 0u
  , WAVEOUT_SELECTBUFFER2
  , WAVEOUT_SELECTBUFFER3
  , WAVEOUT_SELECTBUFFER4
  , WAVEOUT_SELECTBUFFER5
  , WAVEOUT_SELECTBUFFER6
  , WAVEOUT_SELECTBUFFER7
  , WAVEOUT_SELECTBUFFER8
  , WAVEOUT_SELECTBUFFER9
  , WAVEOUT_SELECTBUFFER10
  , WAVEOUT_SELECTBUFFER11
  , WAVEOUT_SELECTBUFFER12
  , WAVEOUT_SELECTBUFFER13
  , WAVEOUT_SELECTBUFFER14
  , WAVEOUT_SELECTBUFFER15
  , WAVEOUT_SELECTBUFFER16
  , WAVEOUT_NBBUFFER
} e_WaveOut_SelectBuffer_t;

typedef struct _SOUNDENGINEEX_ {
  s_SoundEngine_t soundengine;      // Client structure of sound engine
  HWAVEOUT hWaveOut;            /* Handle to an open waveform-audio output device. */
  WAVEFORMATEX wfx;             /* format of waveform-audio data. */
  WAVEHDR waveheader;           /* header used to identify a waveform-audio buffer. */
} s_SoundEngineEx_t;


/* =============================================================================
                        Private constants and variables
============================================================================= */
static BOOL g_bSoundEngineFilterStateOn = FALSE;
static FLOAT g_fSoundEngineFilterCutOffValue = 3000.0f;

static BOOL g_bSoundEngineLeftChannelStateOn = TRUE;
static BOOL g_bSoundEngineRightChannelStateOn = TRUE;

static UINT8 g_abyWaveOutput[WAVEOUT_NBBUFFER][WAVEOUT_BUFFER_SIZE];
static UINT8 g_byBufferIdxRead = WAVEOUT_SELECTBUFFER1;
static UINT8 g_byBufferIdxWrite = WAVEOUT_SELECTBUFFER4;

/* =============================================================================
                        Public constants and variables
============================================================================= */
volatile BOOL g_bFillNextAudioBuffer;
volatile BOOL g_bRenderingInProgress;
volatile BOOL g_bGetData;

// volatile UINT8
volatile UINT8 g_bySoundEngineThreadState;           // Audio thread 3-States

//UINT8 byNextBuffer;

// volatile DWORD
volatile DWORD g_dwSoundEngineAyFrequency;

// DWORD
DWORD g_dwSoundEngineThreadID;

// SOUNDENGINEEX
s_SoundEngineEx_t g_sSoundengineex;

// HANDLE
HANDLE g_hSoundEngineThread;

// YMUPDATE
s_YmUpdate_t g_sYMUpdate;

// Fc : filter's center frequency (4000)
// Fs : sampling frequency (44100)
// 0.998;362222226663;	// 2 * sin(pi() * Fc / Fs)
double Q1	= 1.7; 			// [0.5, 2]
double D1	= 0.0;
double D2	= 0.0;


/* =============================================================================
                        Private function declarations
============================================================================= */
static FLOAT fFilter (FLOAT p_fInput);
static VOID CALLBACK waveOutProc(HWAVEOUT p_hWaveOut, UINT p_uMsg, DWORD p_dwInstance, DWORD p_dwParam1, DWORD p_dwParam2);
static DWORD WINAPI SoundEngineThread(LPVOID p_LpParameter);


/* =============================================================================
                               Public functions
============================================================================= */

/*==============================================================================
Function    :   vSoundEngineSetFilterOn

Describe    :   Set filter on.

Parameters  :   None.

Returns     :   None.
==============================================================================*/
VOID vSoundEngineSetFilterOn (VOID)
{
  g_bSoundEngineFilterStateOn = TRUE;
}


/*==============================================================================
Function    :   vSoundEngineSetFilterOff

Describe    :   Set filter off.

Parameters  :   None.

Returns     :   None.
==============================================================================*/
VOID vSoundEngineSetFilterOff (VOID)
{
  g_bSoundEngineFilterStateOn = FALSE;
}


/*==============================================================================
Function    :   w_SoundEngineSetFilterOff

Describe    :   Set filter cut off value.

Parameters  :   p_fFilterCutOffValue = Cutoff value.

Returns     :   None.
==============================================================================*/
VOID vSoundEngineSetFilterCutOffValue (FLOAT p_fFilterCutOffValue)
{
  g_fSoundEngineFilterCutOffValue = p_fFilterCutOffValue;
}


/*==============================================================================
Function    :   uiSoundEngineGetVolume

Describe    :   Retrieves the current volume level of the specified
                waveform-audio output device.

Parameters  :   p_pdwVolume = Pointer to DWORD for save read volume.

Returns     :   MMSYSERR_NOERROR if successful.
==============================================================================*/
MMRESULT uiSoundEngineGetVolume (PDWORD p_pdwVolume)
{
  return (waveOutGetVolume (g_sSoundengineex.hWaveOut, p_pdwVolume));
}


/*==============================================================================
Function    :   v_SoundEngineSetVolume

Describe    :   Sets the volume level of the specified waveform-audio output
                device.

Parameters  :   p_dwVolume = The low-order word contains the left-channel volume
                             setting, and the high-order word contains the
                             right-channel setting.

Returns     :   MMSYSERR_NOERROR if successful.
==============================================================================*/
MMRESULT uiSoundEngineSetVolume (DWORD p_dwVolume)
{
  return (waveOutSetVolume (g_sSoundengineex.hWaveOut, p_dwVolume));
}


/*==============================================================================
Function    :   v_SoundEngineInit

Describe    :   Initialize the sound engine.

Parameters  :   psoundengine = pointer to sound engine structure.

Returns     :   TRUE if initialization OK else FALSE.
==============================================================================*/
BOOL b_SoundEngineInit (s_SoundEngine_t* psoundengine)
{
  // Locals variables declaration
  BOOL l_bReturn;
  MMRESULT mmresult;
  TCHAR szSoundEngineInit[MAX_PATH];

  // Init variables
  l_bReturn = FALSE;

  // If entries parameters are valid
  if ( (psoundengine != NULL) &&
      ((psoundengine->dwSampleRate == SOUNDENGINE_22050) ||
       (psoundengine->dwSampleRate == SOUNDENGINE_44100)) &&
      ((psoundengine->byBitsPerSample == SOUNDENGINE_8BITS) ||
       (psoundengine->byBitsPerSample == SOUNDENGINE_16BITS)) &&
      ((psoundengine->byNbChannels == SOUNDENGINE_MONO) ||
       (psoundengine->byNbChannels == SOUNDENGINE_STEREO)) &&
      (psoundengine->pbRenderingEnable != NULL))
  {
    // Initial state of audio thread
    g_bySoundEngineThreadState = THREAD_AUDIO_END;

    // Initial handle of audio thread
    g_hSoundEngineThread = NULL;

    // Local copy of parameters
    memcpy (&g_sSoundengineex.soundengine, psoundengine, sizeof (s_SoundEngine_t));

    // Set up the WAVEFORMATEX structure (format of the audio)
    g_sSoundengineex.wfx.nSamplesPerSec = g_sSoundengineex.soundengine.dwSampleRate;
    g_sSoundengineex.wfx.wBitsPerSample = g_sSoundengineex.soundengine.byBitsPerSample;
    g_sSoundengineex.wfx.nChannels = g_sSoundengineex.soundengine.byNbChannels;
    g_sSoundengineex.wfx.cbSize = 0;                                             // size of _extra_ info
    g_sSoundengineex.wfx.wFormatTag = WAVE_FORMAT_PCM;                           // audio format
    g_sSoundengineex.wfx.nBlockAlign = (g_sSoundengineex.wfx.wBitsPerSample * g_sSoundengineex.wfx.nChannels)>>3;
    g_sSoundengineex.wfx.nAvgBytesPerSec = g_sSoundengineex.wfx.nBlockAlign * g_sSoundengineex.wfx.nSamplesPerSec;

    /* Open the given waveform-audio output device for playback. */
    mmresult = waveOutOpen (&g_sSoundengineex.hWaveOut, WAVE_MAPPER, &g_sSoundengineex.wfx, (DWORD_PTR)waveOutProc, 0, CALLBACK_FUNCTION);
    if(mmresult == MMSYSERR_NOERROR)
    {
      /* Create and run sound engine Thread. */
      g_bySoundEngineThreadState = THREAD_AUDIO_RUN;
      g_hSoundEngineThread = CreateThread (NULL, 0, SoundEngineThread, NULL, 0, &g_dwSoundEngineThreadID);

      // Check for error
      if (g_hSoundEngineThread == NULL)
      {
        // Debug message
        sprintf (szSoundEngineInit, "Error : CreateThread => %li", GetLastError());
        MessageBox (NULL, szSoundEngineInit, "b_SoundEngineInit", MB_ICONERROR | MB_OK);
      }
      else
      {
        /* Set highest priority for audio thread. */
        if (SetThreadPriority (g_hSoundEngineThread, THREAD_PRIORITY_HIGHEST) == FALSE)
        {
          // Debug message
          sprintf (szSoundEngineInit, "Warning : SetThreadPriority => %li", GetLastError());
          MessageBox (NULL, szSoundEngineInit, "b_SoundEngineInit", MB_ICONWARNING | MB_OK);
        }

        // No errors
        l_bReturn = TRUE;
      }
    }
    else
    {
      // Debug message
      sprintf (szSoundEngineInit, "Error : waveOutOpen => %i", mmresult);
      MessageBox (NULL, szSoundEngineInit, "b_SoundEngineInit", MB_ICONERROR | MB_OK);
    }
  }
  else
  {
    // Debug message
    MessageBox (NULL, "Error : wrong parameters", "b_SoundEngineInit", MB_ICONERROR | MB_OK);
  }

  return l_bReturn;
}


/*==============================================================================
Function    :   b_SoundEngineQuit.

Describe    :   Quit the sound engine.

Parameters  :   None.

Returns     :   None.
==============================================================================*/
BOOL b_SoundEngineQuit (VOID)
{
  // Locals variables declaration
  BOOL l_bReturn;
  MMRESULT mmresult;
  TCHAR szSoundEngineQuit[MAX_PATH];

  // Initialize variables
  l_bReturn = FALSE;

  // If thread is running
  if (g_bySoundEngineThreadState == THREAD_AUDIO_RUN)
  {
    // Thread need to stop
    g_bySoundEngineThreadState = THREAD_AUDIO_STOPPING;

    // wait for thread end
    while (g_bySoundEngineThreadState != THREAD_AUDIO_END);

    /* Close the given waveform-audio output device. */
    mmresult = waveOutClose (g_sSoundengineex.hWaveOut);
    if (mmresult != MMSYSERR_NOERROR)
    {
      // Debug message
      sprintf (szSoundEngineQuit, "Error : waveOutClose => %i", mmresult);
      MessageBox (NULL, szSoundEngineQuit, "b_SoundEngineQuit", MB_ICONERROR | MB_OK);
    }
    else
    {
      // No error
      l_bReturn = TRUE;
    }
  }
  else
  {
    // Debug message
    MessageBox (NULL, "Info : Sound engine not running !", "b_SoundEngineQuit", MB_ICONINFORMATION | MB_OK);
  }

  return l_bReturn;
}


/* =============================================================================
                              Private functions
============================================================================= */

/*==============================================================================
Function    :   fFilter

Describe    :   Low pass filter (Hal Chamberlin's state-variable)
                http://www.musicdsp.org/archive.php?classid=3#142.

Parameters  :   fInput = input flow to filter.

Returns     :   input flow filtered.
==============================================================================*/
static FLOAT fFilter (FLOAT fInput)
{
  // Locals variables declaration
	double l,h,b;//,n;
	double F1	= 2 * sin(M_PI * (*g_sSoundengineex.soundengine.pfFilterCutOff) / g_sSoundengineex.soundengine.dwSampleRate);

	// Band pass filter ~[400Hz - 8000Hz]
  l = D2 + (F1 * D1);
  h = fInput - l - (Q1 * D1);
  b = (F1 * h) + D1;
  //n = h + l;

  D1 = b;
  D2 = l;

  return (l * 1.0);
}


static float outputs[2];
float hipass (float input);
float hipass (float input) {

 float a = (*g_sSoundengineex.soundengine.pfFilterCutOff)/5700.0f;

 outputs[0] += a * (input - outputs[0]);

/* outputs[1] = (a * outputs[0]) + (input - (a * input));
 outputs[0] = outputs[1];*/

 return (outputs[0]);
}


/*==============================================================================
Function    :   waveOutProc

Describe    :   Callback to play streaming sound.

Parameters  :   See MSDN.

Returns     :   None.
==============================================================================*/
static VOID CALLBACK waveOutProc (__attribute__ ((unused))HWAVEOUT p_hWaveOut
                           , UINT p_uMsg
                           ,__attribute__ ((unused))DWORD p_dwInstance
                           ,__attribute__ ((unused))DWORD p_dwParam1
                           ,__attribute__ ((unused))DWORD p_dwParam2)
{
  // Message management
  switch (p_uMsg)
  {
    case WOM_CLOSE: // Sent when the device is closed using the waveOutClose function
    {
      break;
    }

    case WOM_DONE: // Sent when the device driver is finished with a data block sent using the waveOutWrite function
    {
      g_bFillNextAudioBuffer = TRUE;
      break;
    }

    case WOM_OPEN:  // Sent when the device is opened using the waveOutOpen function
    {
      break;
    }

    default:
    {
      break;      // Impossible
    }
  }
}


/*==============================================================================
Function    :   ThreadAudio

Describe    :   Thread for audio playback.

Parameters  :   Some parameters....

Returns     :   Some returns...
==============================================================================*/
static DWORD WINAPI SoundEngineThread (__attribute__ ((unused))LPVOID LpParameter)
{
  /* Locals variables declaration */
  MMRESULT l_eResult;
  WORD wBufferIndex;
  DOUBLE dResamplingTrigger;
  DOUBLE dResamplingCounter;

  // Init indexs to buffers
  g_byBufferIdxRead = WAVEOUT_SELECTBUFFER1;
  g_byBufferIdxWrite = WAVEOUT_SELECTBUFFER4;

  // Init audio structure
  ZeroMemory (&g_sSoundengineex.waveheader, sizeof(WAVEHDR));
  g_sSoundengineex.waveheader.dwBufferLength = WAVEOUT_BUFFER_SIZE;
  g_sSoundengineex.waveheader.lpData = (CHAR *)g_abyWaveOutput[g_byBufferIdxRead];

  // Clear all buffers
  wBufferIndex = 0;
  do
  {
    memset (g_abyWaveOutput[wBufferIndex++], 0x7F, WAVEOUT_BUFFER_SIZE);
  }
  while (wBufferIndex < WAVEOUT_NBBUFFER);

  /* Prepare a waveform-audio data block for playback. */
  l_eResult = waveOutPrepareHeader (g_sSoundengineex.hWaveOut, &g_sSoundengineex.waveheader, sizeof(WAVEHDR));
  if (l_eResult == MMSYSERR_NOERROR)
  {
    /* Send a data block to the given waveform-audio output device. */
    l_eResult = waveOutWrite (g_sSoundengineex.hWaveOut, &g_sSoundengineex.waveheader, sizeof(WAVEHDR));
    if (l_eResult == MMSYSERR_NOERROR)
    {
      /* Computer next buffer to play. */
      g_byBufferIdxRead = (g_byBufferIdxRead + 1u) % WAVEOUT_NBBUFFER;

      /* Write data to audio buffer. */
      ZeroMemory (&g_sSoundengineex.waveheader, sizeof(WAVEHDR));
      g_sSoundengineex.waveheader.dwBufferLength = WAVEOUT_BUFFER_SIZE;
      g_sSoundengineex.waveheader.lpData = (CHAR *)g_abyWaveOutput[g_byBufferIdxRead];

      /* Prepare a waveform-audio data block for playback. */
      l_eResult = waveOutPrepareHeader (g_sSoundengineex.hWaveOut, &g_sSoundengineex.waveheader, sizeof(WAVEHDR));
      if (l_eResult == MMSYSERR_NOERROR)
      {
        /* Send a data block to the given waveform-audio output device. */
        l_eResult = waveOutWrite (g_sSoundengineex.hWaveOut, &g_sSoundengineex.waveheader, sizeof(WAVEHDR));
        if (l_eResult == MMSYSERR_NOERROR)
        {
          /* Computer next buffer to play. */
          g_byBufferIdxRead = (g_byBufferIdxRead + 1u) % WAVEOUT_NBBUFFER;

          /* Write data to audio buffer. */
          ZeroMemory (&g_sSoundengineex.waveheader, sizeof(WAVEHDR));
          g_sSoundengineex.waveheader.dwBufferLength = WAVEOUT_BUFFER_SIZE;
          g_sSoundengineex.waveheader.lpData = (CHAR *)g_abyWaveOutput[g_byBufferIdxRead];
          l_eResult = waveOutPrepareHeader (g_sSoundengineex.hWaveOut, &g_sSoundengineex.waveheader, sizeof(WAVEHDR));
          l_eResult = waveOutWrite(g_sSoundengineex.hWaveOut, &g_sSoundengineex.waveheader, sizeof(WAVEHDR));

          /* Select next buffer to play. */
          g_byBufferIdxRead = (g_byBufferIdxRead + 1) & (WAVEOUT_NBBUFFER - 1);

          /* Resampling counter/trigger. */
          dResamplingTrigger = 0.0f;
          dResamplingCounter = 0.0f;

          /* Loop. */
          while (g_bySoundEngineThreadState == THREAD_AUDIO_RUN)
          {
            /* No rendering. */
            g_bRenderingInProgress = FALSE;

            /* If driver need buffer. */
            if (g_bFillNextAudioBuffer == TRUE)
            {
              /* Reset flags. */
              g_bFillNextAudioBuffer = FALSE;

              /* Clean up the preparation performed by the waveOutPrepareHeader function. */
              l_eResult = waveOutUnprepareHeader (g_sSoundengineex.hWaveOut, &g_sSoundengineex.waveheader, sizeof(WAVEHDR));

              /* Select next buffer as current buffer. */
              ZeroMemory (&g_sSoundengineex.waveheader, sizeof(WAVEHDR));
              g_sSoundengineex.waveheader.dwBufferLength = WAVEOUT_BUFFER_SIZE;
              g_sSoundengineex.waveheader.lpData = (CHAR *)g_abyWaveOutput[g_byBufferIdxRead];

              /* Prepare a waveform-audio data block for playback. */
              l_eResult = waveOutPrepareHeader (g_sSoundengineex.hWaveOut, &g_sSoundengineex.waveheader, sizeof(WAVEHDR));

              /* Send a data block to the given waveform-audio output device. */
              l_eResult = waveOutWrite(g_sSoundengineex.hWaveOut, &g_sSoundengineex.waveheader, sizeof(WAVEHDR));

              /* Select next buffer to play. */
              g_byBufferIdxRead = (g_byBufferIdxRead + 1) & (WAVEOUT_NBBUFFER - 1);
            }

            /* Need to prepare buffer ? */
            if (g_byBufferIdxWrite != g_byBufferIdxRead)
            {
              /* If rendering enable. */
              if ((*g_sSoundengineex.soundengine.pbRenderingEnable) == TRUE)
              {
                /* Rendering in progress. */
                g_bRenderingInProgress = TRUE;

                /* Compute next buffer. */
                for (wBufferIndex = 0; wBufferIndex < WAVEOUT_BUFFER_SIZE; wBufferIndex += 2)
                {
                  g_bGetData = TRUE;
                  do
                  {
                    /* Get AY datas. */
                    g_sSoundengineex.soundengine.b_Render (&g_sYMUpdate);

                    g_bGetData = FALSE;

                    /* Inc counter. */
                    dResamplingCounter++;
                  }
                  while (dResamplingCounter < dResamplingTrigger);

                  dResamplingTrigger += (double) g_dwSoundEngineAyFrequency / (double) g_sSoundengineex.soundengine.dwSampleRate;

                  /* If left channel activated. */
                  if (TRUE == g_bSoundEngineLeftChannelStateOn)
                  {
                    /* If filter activated. */
                    if (TRUE == g_bSoundEngineFilterStateOn)
                    {
                      g_abyWaveOutput[g_byBufferIdxWrite][wBufferIndex] = fFilter (((g_sYMUpdate.fAnalogChannelB_Out + (g_sYMUpdate.fAnalogChannelA_Out<<1)) >> 8) / 3);
                    }
                    else
                    {
                      g_abyWaveOutput[g_byBufferIdxWrite][wBufferIndex] = ((g_sYMUpdate.fAnalogChannelB_Out + (g_sYMUpdate.fAnalogChannelA_Out<<1)) >> 8) / 3;
                    }
                  }
                  else
                  {
                    g_abyWaveOutput[g_byBufferIdxWrite][wBufferIndex] = 0x7F;
                  }

                  /* If right channel activated. */
                  if (TRUE == g_bSoundEngineRightChannelStateOn)
                  {
                    /* If filter activated. */
                    if (TRUE == g_bSoundEngineFilterStateOn)
                    {
                      g_abyWaveOutput[g_byBufferIdxWrite][wBufferIndex+1] = fFilter(((g_sYMUpdate.fAnalogChannelB_Out + (g_sYMUpdate.fAnalogChannelC_Out<<1)) >> 8) / 3);
                    }
                    else
                    {
                      g_abyWaveOutput[g_byBufferIdxWrite][wBufferIndex+1] = ((g_sYMUpdate.fAnalogChannelB_Out + (g_sYMUpdate.fAnalogChannelC_Out<<1)) >> 8) / 3;
                    }
                  }
                  else
                  {
                    g_abyWaveOutput[g_byBufferIdxWrite][wBufferIndex + 1] = 0x7F;
                  }
                }
              }
              else
              {
                memset (&g_abyWaveOutput[g_byBufferIdxWrite], 0x7F, WAVEOUT_BUFFER_SIZE);
              }
              g_byBufferIdxWrite = (g_byBufferIdxWrite + 1) & (WAVEOUT_NBBUFFER - 1);
            }
          }
        }
        else
        {
          /* Debug message. */
          printf ("\r\nError : waveOutWrite => %i", l_eResult);
        }
      }
      else
      {
        /* Debug message. */
        printf ("\r\nError : waveOutPrepareHeader => %i", l_eResult);
      }
    }
     else
    {
      /* Debug message. */
      printf ("\r\nError : waveOutWrite => %i", l_eResult);
    }
  }
   else
  {
    /* Debug message. */
    printf ("\r\nError : waveOutPrepareHeader => %i", l_eResult);
  }

  /* Stop playback. */
  l_eResult = waveOutReset (g_sSoundengineex.hWaveOut);

  /* Unprepare current buffer. */
  l_eResult = waveOutUnprepareHeader (g_sSoundengineex.hWaveOut, &g_sSoundengineex.waveheader, sizeof(WAVEHDR));

  /* End of thread. */
  g_bySoundEngineThreadState = THREAD_AUDIO_END;

  /* End thread. */
  ExitThread(0);

  return 0;
}



