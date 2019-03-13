/* =============================================================================

                  ██╗   ██╗██╗  ██╗ ██████╗ ███╗   ██╗██╗  ██╗
                  ██║   ██║██║ ██╔╝██╔═══██╗████╗  ██║╚██╗██╔╝
                  ██║   ██║█████╔╝ ██║   ██║██╔██╗ ██║ ╚███╔╝
                  ██║   ██║██╔═██╗ ██║   ██║██║╚██╗██║ ██╔██╗
                  ╚██████╔╝██║  ██╗╚██████╔╝██║ ╚████║██╔╝ ██╗
                   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝

  File name:    parser_ym.cpp
  Date:         23 07 2017
  Author:       Power.
  Description:  YM file parser - Body file.

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
#include "generator_ay.h"
#include "soundengine.h"
#include "parser_ym.h"


/* =============================================================================
                          Private defines and typedefs
============================================================================= */
#define YM_LATCH_ADDRESS    (0)
#define YM_INACTIVE         (1)
#define YM_WRITE_TO_PSG     (2)
#define YM_INTERLEAVED      (1)
#define YM_VERSION_3        ('3')
#define YM_VERSION_4        ('4')
#define YM_VERSION_5        ('5')
#define YM_VERSION_6        ('6')
#define NULL_CHAR           ('\0')

typedef struct _YMFILEHEADER_ {
  TCHAR sID[4];
  TCHAR sCheckString[8];
  UINT32 dwNbOfFrame;
  UINT32 dwSongAttributes;
  UINT16 wNbOfDigidrum;
  UINT32 dwYmMasterClock;
  UINT16 wPlayerFrameHz;
  UINT32 dwLoopFrame;
  UINT16 wAddDataSize;
  UINT32* pdwDigidrumSize;
  UINT8** ppbyDigidrumData;
  TCHAR szSongName[MAX_PATH];
  TCHAR szAuthorName[MAX_PATH];
  TCHAR szSongComment[MAX_PATH];
  UINT8 *pbyYMDatas;
} s_YmFileHeader_t;


/* =============================================================================
                        Private constants and variables
============================================================================= */
static UINT8            g_u8YmLoopMode = YM_NOLOOP;
static UINT16           g_au16WavForm[WAVFORM_SIZE][3];
static BOOL             g_bChannelAEnable = TRUE;
static BOOL             g_bChannelBEnable = TRUE;
static BOOL             g_bChannelCEnable = TRUE;
static UINT8            g_bSamplingWavForm;
static UINT8            g_u8AYProgrammingStep;
static UINT8            g_u8YmRegIndex;
static UINT8            g_au8YmRegData[16];
static UINT16           g_u16WavFormTrigger;
static UINT16           g_u16WavFormIdx;
static UINT32           g_u32YMPlayerCounter;
static UINT32           g_u32YMPLayerTrigger;
static UINT32           g_u32YmFrameIndex;
static s_YmFileHeader_t g_sYmFileHeader;
static HANDLE           g_hYmFile = NULL;
static s_AyContext_t    g_sAyContext;

/* =============================================================================
                        Public constants and variables
============================================================================= */

/* =============================================================================
                        Private function declarations
============================================================================= */
static inline DWORD u32SwapDword (DWORD p_dwValueIn);
static inline WORD u16SwapWord (WORD p_wValueIn);


/* =============================================================================
                               Public functions
============================================================================= */

/*==============================================================================
Function    :   b_YmPlayerInit

Describe    :   Update AY-3-891x engine

Parameters  :   p_pvParameters => Pointer to init structure

Returns     :   TRUE if init ok
                else FALSE.
==============================================================================*/
BOOL bYm_Init (VOID* p_pvParameters)
{
  /* Locals variables declaration. */
  UINT8 l_bReturn = FALSE;
  s_YmInit_t* l_psYmInit;
  UINT32 l_u32YMFileSize;
  DWORD l_u32ByteRead;
  UINT16 l_u16TempData;
  UINT32 l_u32TempData;
  UINT16 l_u16Digidrums;
  TCHAR szYmPlayerInit[MAX_PATH];
  UINT32 l_u32Offset;
  UINT8 l_u8Reg;
  UINT32 l_u32Frame;
  UINT32 u32YmDataSize;
  UINT8 u8NbOfReg;

  // If parameters pointer not null
  if (NULL != p_pvParameters)
  {
    // Cast input pointer to local struct
    l_psYmInit = (s_YmInit_t*) p_pvParameters;

    // If valid parameters
    if ((l_psYmInit->dwAYFrequency > 0) && (l_psYmInit->szFileName != NULL))
    {
      // Try to open file
      g_hYmFile = CreateFile (l_psYmInit->szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
      if (g_hYmFile !=  INVALID_HANDLE_VALUE)
      {
        // Get the file size
        l_u32YMFileSize = GetFileSize (g_hYmFile, NULL);

        // Set file pointer to begin
        SetFilePointer (g_hYmFile, 0, NULL, FILE_BEGIN);

        // Try to load ym version (4 bytes)
        if (ReadFile (g_hYmFile, &g_sYmFileHeader, 4, &l_u32ByteRead, NULL) == TRUE)
        {
          // Check if YM file
          if ((g_sYmFileHeader.sID[0] == 'Y') &&
              (g_sYmFileHeader.sID[1] == 'M') &&
              (g_sYmFileHeader.sID[3] == '!'))
          {
            // Check version number
            if ((g_sYmFileHeader.sID[2] >= YM_VERSION_3) &&
                (g_sYmFileHeader.sID[2] <= YM_VERSION_6))
            {
              // Nb of music frame for YM > version 3
              if (g_sYmFileHeader.sID[2] != YM_VERSION_3)
              {
                // Read 8 bytes header (check string)
                ReadFile (g_hYmFile, &g_sYmFileHeader.sCheckString, 8, &l_u32ByteRead, NULL);

                // Read 4 bytes header (nb of frame in the file)
                ReadFile (g_hYmFile, &l_u32TempData, 4, &l_u32ByteRead, NULL);

                // Swap bytes and save
                g_sYmFileHeader.dwNbOfFrame = u32SwapDword(l_u32TempData);
              }
              else    // Version 3
              {
                // Compute number of frame
                g_sYmFileHeader.dwNbOfFrame = (l_u32YMFileSize - 4) / 14;
              }

              // Others infos are for YM > version 3
              if (g_sYmFileHeader.sID[2] != YM_VERSION_3)
              {
                // Read 4 bytes header (Song attribute)
                ReadFile (g_hYmFile, &l_u32TempData, 4, &l_u32ByteRead, NULL);

                // Swap bytes and save
                g_sYmFileHeader.dwSongAttributes = u32SwapDword(l_u32TempData);

                // Read 2 bytes header (Nb of digidrum)
                ReadFile (g_hYmFile, &l_u16TempData, 2, &l_u32ByteRead, NULL);

                // Swap bytes and save
                g_sYmFileHeader.wNbOfDigidrum = u16SwapWord(l_u16TempData);

                // Others infos are for YM > version 4
                if (g_sYmFileHeader.sID[2] > YM_VERSION_4)
                {
                  // Read 4 bytes header (YM Master clock)
                  ReadFile (g_hYmFile, &l_u32TempData, 4, &l_u32ByteRead, NULL);

                  // Swap bytes and save
                  g_sYmFileHeader.dwYmMasterClock = u32SwapDword(l_u32TempData);

                  // Update input buffer
                  l_psYmInit->dwAYFrequency = g_sYmFileHeader.dwYmMasterClock;

                  // Read 2 bytes header (Player frequency)
                  ReadFile (g_hYmFile, &l_u16TempData, 2, &l_u32ByteRead, NULL);

                  // Swap bytes and save
                  g_sYmFileHeader.wPlayerFrameHz = u16SwapWord(l_u16TempData);
                }
                else // YM version 4
                {
                  g_sYmFileHeader.dwYmMasterClock = l_psYmInit->dwAYFrequency;
                  g_sYmFileHeader.wPlayerFrameHz = 50;
                }

                l_psYmInit->wPlayerFrameHz = g_sYmFileHeader.wPlayerFrameHz;

                //  Read 4 bytes header (Loop frame)
                ReadFile (g_hYmFile, &l_u32TempData, 4, &l_u32ByteRead, NULL);

                // Swap bytes and save
                g_sYmFileHeader.dwLoopFrame = u32SwapDword(l_u32TempData);

                // Others infos are for YM > version 4
                if (g_sYmFileHeader.sID[2] >= YM_VERSION_4)
                {
                  // Read 2 bytes header (Future additional data)
                  ReadFile (g_hYmFile, &l_u16TempData, 2, &l_u32ByteRead, NULL);

                  // Swap bytes and save
                  g_sYmFileHeader.wAddDataSize = u16SwapWord(l_u16TempData);
                }
                else // YM version 4
                {
                  g_sYmFileHeader.wAddDataSize = 0;
                }
              }
              else // YM version 3
              {
                g_sYmFileHeader.dwSongAttributes = 1;
                g_sYmFileHeader.wNbOfDigidrum = 0;
                g_sYmFileHeader.dwYmMasterClock = l_psYmInit->dwAYFrequency;
                g_sYmFileHeader.wPlayerFrameHz = 50;
                g_sYmFileHeader.dwLoopFrame = 0;
                g_sYmFileHeader.wAddDataSize = 0;
              }

              // Create pointers to digidrum
              g_sYmFileHeader.pdwDigidrumSize = new UINT32[g_sYmFileHeader.wNbOfDigidrum];
              g_sYmFileHeader.ppbyDigidrumData = new UINT8*[g_sYmFileHeader.wNbOfDigidrum];

              // Manage digidrums if need
              for (l_u16Digidrums = 0; l_u16Digidrums < g_sYmFileHeader.wNbOfDigidrum; l_u16Digidrums++)
              {
                // Read 4 bytes header (Digidrum x size)
                ReadFile (g_hYmFile, &l_u32TempData, 4, &l_u32ByteRead, NULL);

                // Swap bytes
                g_sYmFileHeader.pdwDigidrumSize[g_sYmFileHeader.wNbOfDigidrum] = u32SwapDword(l_u32TempData);

                // Allocate memory for digidrums
                g_sYmFileHeader.ppbyDigidrumData[l_u16Digidrums] = new UINT8[g_sYmFileHeader.pdwDigidrumSize[g_sYmFileHeader.wNbOfDigidrum]];

                // Read dwDigidrumsSize bytes (Digidrum data)
                ReadFile (g_hYmFile, &g_sYmFileHeader.ppbyDigidrumData[l_u16Digidrums], g_sYmFileHeader.pdwDigidrumSize[g_sYmFileHeader.wNbOfDigidrum], &l_u32ByteRead, NULL);
              }

              // Others infos are for YM > version 3
              if (g_sYmFileHeader.sID[2] > YM_VERSION_3)
              {
                UINT16 wIndex;

                // Get song name
                wIndex = 0;
                do
                {
                  // Read 1 byte (Song name)
                  ReadFile (g_hYmFile, &g_sYmFileHeader.szSongName[wIndex], 1, &l_u32ByteRead, NULL);
                }
                while ((wIndex != MAX_PATH) && (g_sYmFileHeader.szSongName[wIndex++] != NULL_CHAR));

                // Get author name
                wIndex = 0;
                do
                {
                  // Read 1 byte (Song name)
                  ReadFile (g_hYmFile, &g_sYmFileHeader.szAuthorName[wIndex], 1, &l_u32ByteRead, NULL);
                }
                while ((wIndex != MAX_PATH) && (g_sYmFileHeader.szAuthorName[wIndex++] != NULL_CHAR));

                // Get song comment
                wIndex = 0;
                do
                {
                  // Read 1 byte (Song name)
                  ReadFile (g_hYmFile, &g_sYmFileHeader.szSongComment[wIndex], 1, &l_u32ByteRead, NULL);
                }
                while ((wIndex != MAX_PATH) && (g_sYmFileHeader.szSongComment[wIndex++] != NULL_CHAR));
              }
              else
              {
                g_sYmFileHeader.szSongName[0] = NULL_CHAR;
                g_sYmFileHeader.szAuthorName[0] = NULL_CHAR;
                g_sYmFileHeader.szSongComment[0] = NULL_CHAR;
              }

              // Update init structure
              sprintf (l_psYmInit->szSongName, "%s", g_sYmFileHeader.szSongName);
              sprintf (l_psYmInit->szAuthorName, "%s", g_sYmFileHeader.szAuthorName);
              sprintf (l_psYmInit->szSongComment, "%s", g_sYmFileHeader.szSongComment);

              // Compute necessary buffer size
              u32YmDataSize = g_sYmFileHeader.dwNbOfFrame * 16;

              // Allocate memory for YM datas
              g_sYmFileHeader.pbyYMDatas = new UINT8[u32YmDataSize];

              // If no-interleaved format
              if ((g_sYmFileHeader.dwSongAttributes & YM_INTERLEAVED) != YM_INTERLEAVED)
              {
                if (g_sYmFileHeader.sID[2] == '3')
                {
                  u8NbOfReg = 14;
                }
                else
                {
                  u8NbOfReg = 16;
                }

                l_u32Offset = 0;
                for (l_u32Frame = 0; l_u32Frame < g_sYmFileHeader.dwNbOfFrame; l_u32Frame++)
                {
                  // Read data
                  ReadFile (g_hYmFile, &g_sYmFileHeader.pbyYMDatas[l_u32Offset], 14, &l_u32ByteRead, NULL);

                  // If YM > version 3
                  if (g_sYmFileHeader.sID[2] > '3')
                  {
                    l_u32Offset+=14;
                    ReadFile (g_hYmFile, &g_sYmFileHeader.pbyYMDatas[l_u32Offset], 2, &l_u32ByteRead, NULL);
                    l_u32Offset+=2;
                  }
                  else
                  {
                    l_u32Offset+=14;
                    g_sYmFileHeader.pbyYMDatas[l_u32Offset++] = 0;
                    g_sYmFileHeader.pbyYMDatas[l_u32Offset++] = 0;
                  }
                }
              }
              else // Fichier entrelace
              {
                if (g_sYmFileHeader.sID[2] == '3')
                {
                  u8NbOfReg = 14;
                }
                else
                {
                  u8NbOfReg = 16;
                }

                // Init offset
                l_u32Offset = 0;

                // For each reg
                for (l_u8Reg = 0; l_u8Reg < u8NbOfReg; l_u8Reg++)
                {
                  // For each frame
                  for (l_u32Frame = 0; l_u32Frame < g_sYmFileHeader.dwNbOfFrame; l_u32Frame++)
                  {
                    // Compute buffer offset
                    l_u32Offset = l_u32Frame * 16 + l_u8Reg;

                    // Read one byte and save it in bufffer
                    ReadFile (g_hYmFile, &g_sYmFileHeader.pbyYMDatas[l_u32Offset], 1, &l_u32ByteRead, NULL);
                  }
                }
              }
#ifdef YMPLAYER_DBG
              // Check if all data read
              if ((l_u32Offset + 1) < u32YmDataSize)
              {
                // Debug message
                sprintf (szYmPlayerInit, "Warning => Wrong YM size : Need %ul, Read %ul", u32YmDataSize, l_u32Offset);
                // MessageBox (NULL, szYmPlayerInit, "b_YmPlayerUpdate", MB_ICONWARNING | MB_OK);
              }
#endif // YMPLAYER_DBG

              // Init for player update
              g_u8AYProgrammingStep = YM_LATCH_ADDRESS;
              g_u32YMPlayerCounter = 0;
              g_u32YMPLayerTrigger = (g_sYmFileHeader.dwYmMasterClock / g_sYmFileHeader.wPlayerFrameHz);
              g_u8YmRegIndex = 0;
              g_u32YmFrameIndex = 0;

              // Reset AY
              g_sAyContext.sExternalSignals.eRESETn_In = AY_LOW;
              eAYEngineUpdate (&g_sAyContext);

              // Set AY IO Line
              g_sAyContext.sExternalSignals.eRESETn_In = AY_HIGH;
              g_sAyContext.sExternalSignals.eCLOCK_In = AY_LOW;
              g_sAyContext.sExternalSignals.eTEST1_In = AY_LOW;
              g_sAyContext.sExternalSignals.eBC1_In = AY_LOW;
              g_sAyContext.sExternalSignals.eBC2_In = AY_LOW;
              g_sAyContext.sExternalSignals.eBDIR_In = AY_LOW;
              g_sAyContext.sExternalSignals.eA8_In = AY_HIGH;
              g_sAyContext.sExternalSignals.u8DataAddress = 0;
              g_u16WavFormIdx = 0;
              g_u16WavFormTrigger = 0;
              g_bSamplingWavForm = TRUE;


              // No errors
              l_bReturn = TRUE;
            }
            else
            {
              // Debug message
              sprintf (szYmPlayerInit, "Error : Unknown YM Version => %c", g_sYmFileHeader.sID[2]);
              //MessageBox (NULL, szYmPlayerInit, "b_YmPlayerUpdate", MB_ICONERROR | MB_OK);
            }
          }
          else
          {
            // Debug message
            // MessageBox (NULL, "Not an YM File !", "b_YmPlayerUpdate", MB_ICONERROR | MB_OK);
          }
        }
        else
        {
          // Debug message
          sprintf (szYmPlayerInit, "Error : ReadFile ID => %li", GetLastError());
          // MessageBox (NULL, szYmPlayerInit, "b_YmPlayerUpdate", MB_ICONERROR | MB_OK);
        }

        // Close file
        CloseHandle (g_hYmFile);
        g_hYmFile = NULL;
      }
      else
      {
        // Debug message
        sprintf (szYmPlayerInit, "Error : CreateFile => %li", GetLastError());
        // MessageBox (NULL, szYmPlayerInit, "b_YmPlayerUpdate", MB_ICONERROR | MB_OK);
      }
    }
    else
    {
      // Debug message
      //  MessageBox (NULL, "Error : Invalid parameters !", "b_YmPlayerInit", MB_ICONERROR | MB_OK);
    }
  }
  else
  {
    // Debug message
    // MessageBox (NULL, "Error : l_psYmInit => NULL pointer !", "b_YmPlayerInit", MB_ICONERROR | MB_OK);
  }

  return l_bReturn;
}


/*==============================================================================
Function    :   b_YmPlayerUpdate

Describe    :   Update AY-3-891x engine

Parameters  :   None

Returns     :   TRUE if init ok
                else FALSE.
==============================================================================*/
BOOL bYm_Update (VOID* p_pvParameters)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;
  s_YmUpdate_t* pymupdate;

  /* Pointers checking. */
  if (   (NULL != g_sYmFileHeader.pbyYMDatas)
      && (NULL != p_pvParameters) )
  {
    // Cast input pointer
    pymupdate = (s_YmUpdate_t*) p_pvParameters;

    if (AY_HIGH == g_sAyContext.sExternalSignals.eCLOCK_In)
    {
      // If YM trig
      if (g_u32YMPlayerCounter == g_u32YMPLayerTrigger)
      {
        switch (g_u8AYProgrammingStep)
        {
          case YM_LATCH_ADDRESS:
          {
            // Select AY_LATCH_ADDRESS (001)
            g_sAyContext.sExternalSignals.eBDIR_In = AY_LOW;
            g_sAyContext.sExternalSignals.eBC2_In = AY_LOW;
            g_sAyContext.sExternalSignals.eBC1_In = AY_HIGH;
            g_sAyContext.sExternalSignals.eA8_In = AY_HIGH;

            // Select register
            g_sAyContext.sExternalSignals.u8DataAddress = g_u8YmRegIndex;

            // Next programming step
            g_u8AYProgrammingStep++;
            break;
          }

          case YM_INACTIVE:
          {
            // Select AY_INACTIVE (000)
            g_sAyContext.sExternalSignals.eBDIR_In = AY_LOW;
            g_sAyContext.sExternalSignals.eBC2_In = AY_LOW;
            g_sAyContext.sExternalSignals.eBC1_In = AY_LOW;
            g_sAyContext.sExternalSignals.eA8_In = AY_HIGH;

            // Next programming step
            g_u8AYProgrammingStep++;
            break;
          }

          case YM_WRITE_TO_PSG:
          {
            // Select AY_WRITE_TO_PSG (110)
            g_sAyContext.sExternalSignals.eBDIR_In = AY_HIGH;
            g_sAyContext.sExternalSignals.eBC2_In = AY_HIGH;
            g_sAyContext.sExternalSignals.eBC1_In = AY_LOW;
            g_sAyContext.sExternalSignals.eA8_In = AY_HIGH;

            // Send register value
            g_sAyContext.sExternalSignals.u8DataAddress = g_sYmFileHeader.pbyYMDatas[g_u32YmFrameIndex*16 + g_u8YmRegIndex];

            // Save for stream
            g_au8YmRegData[g_u8YmRegIndex] = g_sAyContext.sExternalSignals.u8DataAddress;

            // Next programming step
            g_u8AYProgrammingStep++;
            break;
          }

          default:
          {
            // Select AY_INACTIVE (000)
            g_sAyContext.sExternalSignals.eBDIR_In = AY_LOW;
            g_sAyContext.sExternalSignals.eBC2_In = AY_LOW;
            g_sAyContext.sExternalSignals.eBC1_In = AY_LOW;
            g_sAyContext.sExternalSignals.eA8_In = AY_HIGH;

            // Reset step number for next time
            g_u8AYProgrammingStep = YM_LATCH_ADDRESS;

            // if last reg
            if (g_u8YmRegIndex == 15)
            {
              // Reset reg number
              g_u8YmRegIndex = 0;

              g_bSamplingWavForm = TRUE;


              // Compute next YM player trigger
              g_u32YMPLayerTrigger += g_dwSoundEngineAyFrequency / g_sYmFileHeader.wPlayerFrameHz / 2;
              if (g_u32YmFrameIndex == g_sYmFileHeader.dwNbOfFrame)
              {
                // If loop activated
                if (YM_LOOP == g_u8YmLoopMode)
                {
                  g_u32YmFrameIndex = g_sYmFileHeader.dwLoopFrame;
                }
              }
              else
              {
                g_u32YmFrameIndex++;
              }
            }
            else
            {
              // Select next reg
              g_u8YmRegIndex++;
            }
            break;
          }
        }
      }
      else
      {
        // Inc YM Player counter
        g_u32YMPlayerCounter++;
      }
    }
    else
    {
      g_sAyContext.sExternalSignals.eBDIR_In = AY_LOW;
      g_sAyContext.sExternalSignals.eBC2_In = AY_LOW;
      g_sAyContext.sExternalSignals.eBC1_In = AY_LOW;
    }

    // Update AY frequency
//  pymupdate->dwAYFrequency = g_sYmFileHeader.dwYmMasterClock;

    // Next clock state
    g_sAyContext.sExternalSignals.eCLOCK_In = (g_sAyContext.sExternalSignals.eCLOCK_In == AY_HIGH) ? AY_LOW : AY_HIGH;

    // Update AY engine
    eAYEngineUpdate (&g_sAyContext);

    // Update AY Analogs outputs
    if (g_bChannelAEnable == TRUE)
    {
      pymupdate->fAnalogChannelA_Out = g_sAyContext.sExternalSignals.u16ChannelAOutput;
    }
    else
    {
      pymupdate->fAnalogChannelA_Out = 0.0;
    }

    if (g_bChannelBEnable == TRUE)
    {
      pymupdate->fAnalogChannelB_Out = g_sAyContext.sExternalSignals.u16ChannelBOutput;
    }
    else
    {
      pymupdate->fAnalogChannelB_Out = 0.0;
    }

    if (g_bChannelCEnable == TRUE)
    {
      pymupdate->fAnalogChannelC_Out = g_sAyContext.sExternalSignals.u16ChannelCOutput;
    }
    else
    {
      pymupdate->fAnalogChannelC_Out = 0.0;
    }

    if (g_bSamplingWavForm == TRUE)
    {
      if (g_u16WavFormTrigger == 10)
      {
        g_u16WavFormTrigger = 0;

        // Save wavform
        g_au16WavForm[g_u16WavFormIdx][0] = g_sAyContext.sExternalSignals.u16ChannelAOutput;
        g_au16WavForm[g_u16WavFormIdx][1] = g_sAyContext.sExternalSignals.u16ChannelBOutput;
        g_au16WavForm[g_u16WavFormIdx][2] = g_sAyContext.sExternalSignals.u16ChannelCOutput;

        // Next wavform position
        g_u16WavFormIdx++;

        if (g_u16WavFormIdx == WAVFORM_SIZE)
        {
          g_bSamplingWavForm = FALSE;

          g_u16WavFormIdx = 0;
        }
      }
      else
      {
        g_u16WavFormTrigger++;
      }
    }

    // No errors
    l_bReturn = TRUE;
  }
  else
  {
      // Debug message
//    MessageBox (NULL, "Error : No YM file opened OR null pointer !", "b_YmPlayerUpdate", MB_ICONERROR | MB_OK);
  }

    return l_bReturn;
}


/*==============================================================================
Function    :   v_YmPlayerQuit

Describe    :

Parameters  :   None

Returns     :   None
==============================================================================*/
VOID vYm_Quit (VOID)
{
  /* Locals variables declaration. */
  UINT16 l_u16Digidrums;

  /* Delete digidrums buffer. */
  for (l_u16Digidrums = 0u; l_u16Digidrums < g_sYmFileHeader.wNbOfDigidrum; l_u16Digidrums++)
  {
    if (NULL != g_sYmFileHeader.ppbyDigidrumData[l_u16Digidrums])
    {
      delete[] g_sYmFileHeader.ppbyDigidrumData[l_u16Digidrums];
    }
  }

  if (g_sYmFileHeader.ppbyDigidrumData != NULL)
  {
    delete[] g_sYmFileHeader.ppbyDigidrumData;
  }

  if (g_sYmFileHeader.pdwDigidrumSize != NULL)
  {
    delete[] g_sYmFileHeader.pdwDigidrumSize;
  }

  // Delete YM datas
  if (g_sYmFileHeader.pbyYMDatas != NULL)
  {
    delete[] g_sYmFileHeader.pbyYMDatas;
  }

  // Delete YM file header structure
  memset(&g_sYmFileHeader, 0, sizeof (s_YmFileHeader_t));
}


/*==============================================================================
Function    :   vYm_SetChannelState

Describe    :   .

Parameters  :   .

Returns     :   None.
==============================================================================*/
VOID vYm_SetChannelState (UINT8 p_u8ChannelState)
{
  g_bChannelAEnable = (p_u8ChannelState & 0x01) ? TRUE : FALSE;
  g_bChannelBEnable = (p_u8ChannelState & 0x02) ? TRUE : FALSE;
  g_bChannelCEnable = (p_u8ChannelState & 0x04) ? TRUE : FALSE;
}


/*==============================================================================
Function    :   bYm_GetChannelState

Describe    :   Get state of selected channel.

Parameters  :   .

Returns     :   None.
==============================================================================*/
BOOL bYm_GetChannelState (UINT8 p_u8ChannelState)
{
  return ( (p_u8ChannelState & 0x01) ? g_bChannelAEnable : ((p_u8ChannelState & 0x02) ? g_bChannelBEnable : ((p_u8ChannelState & 0x04) ? g_bChannelCEnable : 0u)) );
}


/*==============================================================================
Function    :   bYm_GetWavForm

Describe    :

Parameters  :

Returns     :   TRUE or FALSE.
==============================================================================*/
BOOL bYm_GetWavForm (UINT16 *p_pu16WavForm)
{
  /* Locals variables declaration */
  BOOL l_bReturn = FALSE;

  /* Input parameters checking. */
  if (NULL != p_pu16WavForm)
  {
    /* Copy waveform. */
    memcpy (p_pu16WavForm, g_au16WavForm, sizeof (g_au16WavForm) );

    /* Copy done. */
    l_bReturn = TRUE;
  }
  else
  {
    MessageBox (NULL, "Error => Null pointer entry", "bYm_GetWavForm", MB_ICONERROR | MB_OK);
  }

  return (l_bReturn);
}


/*==============================================================================
Function    :   bYm_GetData

Describe    :   Retrieve YM data

Parameters  :   p_pvParameters => Pointer to init structure

Returns     :   TRUE or FALSE.
==============================================================================*/
BOOL bYm_GetData (VOID* p_pvParameters)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;
  s_YmGetData_t *l_psYmGetData;

  /* Input parameters checking. */
  if (NULL != p_pvParameters)
  {
    /* Cast pointer. */
    l_psYmGetData = (s_YmGetData_t*) p_pvParameters;

    /* Not ready. (TODO replace by MUTEX) */
    l_psYmGetData->bYMReady = FALSE;

    /* Wait AY inactivity. */
    if (g_u8AYProgrammingStep == YM_INACTIVE)
    {
      /* Copy AY register value. */
      memcpy (l_psYmGetData->byYMRegData, g_au8YmRegData, 16);

      /* Ready. (TODO replace by MUTEX) */
      l_psYmGetData->bYMReady = TRUE;
    }

    l_bReturn = TRUE;
  }
  else
  {
    MessageBox (NULL, "Error => Null pointer entry", "b_YmPlayerGetData", MB_ICONERROR | MB_OK);
  }

  return (l_bReturn);
}


/*==============================================================================
Function    :   bYm_CheckFileOK

Describe    :   Check if file is YM.

Parameters  :   p_pszFilename = Pointer to file name.

Returns     :   TRUE if OK, else FALSE.
==============================================================================*/
BOOL bYm_CheckFileOK (TCHAR* p_pszFilename) {

  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;
  HANDLE l_hYmFile;
  DWORD l_dwByteRead;
  TCHAR l_auchHeader[4];

  /* Input parameters checking. */
  if (NULL != p_pszFilename)
  {
    /* Open file in read mode. */
    l_hYmFile = CreateFile (p_pszFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE != l_hYmFile)
    {
      /* Load 4 first byte containing YM version. */
      if (FALSE != ReadFile (l_hYmFile, &l_auchHeader, sizeof (l_auchHeader), &l_dwByteRead, NULL) )
      {
        if (sizeof (l_auchHeader) == l_dwByteRead)
        {
          /*Check YM version. */
          if (   (0 == strncmp (l_auchHeader, "YM3!", sizeof (l_auchHeader)))
              || (0 == strncmp (l_auchHeader, "YM4!", sizeof (l_auchHeader)))
              || (0 == strncmp (l_auchHeader, "YM5!", sizeof (l_auchHeader)))
              || (0 == strncmp (l_auchHeader, "YM6!", sizeof (l_auchHeader))) )
          {
            l_bReturn = TRUE;
          }
        }
      }

      CloseHandle (l_hYmFile);
    }
  }

  return (l_bReturn);
}


/*==============================================================================
Function    :   u32Ym_GetPosition

Describe    :   Get song position.

Parameters  :   None.

Returns     :   Song position.
==============================================================================*/
UINT32 u32Ym_GetPosition (VOID) {
  return (g_u32YmFrameIndex);
}


/*==============================================================================
Function    :   u32Ym_GetPosition

Describe    :   Get song position.

Parameters  :   None.

Returns     :   Song position.
==============================================================================*/
UINT32 u32Ym_GetSongLength (VOID) {
  return (g_sYmFileHeader.dwNbOfFrame);
}


/*==============================================================================
Function    :   vYm_SetPosition

Describe    :   Set song position.

Parameters  :   p_u32Position = New song position.

Returns     :   Song position.
==============================================================================*/
VOID vYm_SetPosition (UINT32 p_u32Position) {
  g_u32YmFrameIndex = p_u32Position;
}


/*==============================================================================
Function    :   vYM_SetLoopMode

Describe    :   Set loop mode.

Parameters  :   None.

Returns     :   Song position.
==============================================================================*/
VOID vYM_SetLoopMode (UINT8 p_u8LoopMode) {
  g_u8YmLoopMode = p_u8LoopMode;
}


/* =============================================================================
                              Private functions
============================================================================= */

/*==============================================================================
Function    :   dw_SwapDword

Describe    :   Swap little endian <-> big endian DWORD

Parameters  :   p_u32ValueIn => DWORD to swap

Returns     :   swapped DWORD
==============================================================================*/
static inline DWORD u32SwapDword (DWORD p_u32ValueIn)
{
  /* Swap. */
  DWORD l_dwValueOut = ((p_u32ValueIn<<8) & 0xFF00FF00) | ((p_u32ValueIn>>8) & 0x00FF00FF);

  return ((l_dwValueOut<<16)|(l_dwValueOut>>16));
}


/*==============================================================================
Function    :   w_SwapWord

Describe    :   Swap little endian <-> big endian WORD

Parameters  :   p_u16ValueIn => WORD to swap

Returns     :   swapped WORD
==============================================================================*/
static inline WORD u16SwapWord (WORD p_u16ValueIn)
{
  return (((p_u16ValueIn<<8) & 0xFF00) | ((p_u16ValueIn>>8) & 0x00FF));
}

