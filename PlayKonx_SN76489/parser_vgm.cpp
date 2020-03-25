/* =============================================================================

                  ██╗   ██╗██╗  ██╗ ██████╗ ███╗   ██╗██╗  ██╗
                  ██║   ██║██║ ██╔╝██╔═══██╗████╗  ██║╚██╗██╔╝
                  ██║   ██║█████╔╝ ██║   ██║██╔██╗ ██║ ╚███╔╝
                  ██║   ██║██╔═██╗ ██║   ██║██║╚██╗██║ ██╔██╗
                  ╚██████╔╝██║  ██╗╚██████╔╝██║ ╚████║██╔╝ ██╗
                   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝

  File name:    parser_vgm.cpp
  Date:         23 07 2017
  Author:       Power.
  Description:  VGM file parser - Body file.

============================================================================= */

/* =============================================================================
                                 DEBUG Section
============================================================================= */

/* =============================================================================
                                 Include Files
============================================================================= */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>
#include "emu_sn76489.h"
#include "soundengine.h"
#include "parser_vgm.h"

/* =============================================================================
                               Private attributs
============================================================================= */

/* =============================================================================
                              Private methods
============================================================================= */

/* =============================================================================
                               Public methods
============================================================================= */
Vgm::Vgm() {

  // Init attributs values
  memset (&this->sFileHeader, 0u, sizeof (this->sFileHeader) );
  memset (&this->sGd3Tag, 0u, sizeof (this->sGd3Tag) );
  this->pau8Song = NULL;
  this->soundchip = NULL;
  this->pfDbg = NULL;
  this->u32FileLen = 0u;
  this->u32Index = 0u;
  this->dPlayerCounter = 0.0f;
  this->dPlayerTrigger = 0.0f;
}

Vgm::~Vgm() {

  if (NULL != this->pfDbg) {

    fclose (pfDbg);
  }

  if (NULL != this->soundchip) {

    delete this->soundchip;
  }

  if (NULL != this->pau8Song) {

    delete[] this->pau8Song;
  }

  if (NULL != this->sGd3Tag.achContent) {

    delete[] this->sGd3Tag.achContent;
  }
}

bool Vgm::bInit (void) {

  // Locals variables declaration.
  bool l_bReturn = FALSE;

  // New SN76489 instance
  this->soundchip = new Sn76489;
  if (NULL != this->soundchip) {

    // Open debug file
    this->pfDbg = fopen("VGM.txt","wt");
    if (NULL != this->pfDbg) {

      l_bReturn = true;
    }
  }
  else {

      printf ("bVgm_Init: instancing error soundchip !\r\n");
  }

  return (l_bReturn);
}

bool Vgm::bLoad (char p_achFilename[]) {

  // Locals variables declaration.
  bool l_bReturn = false;
  FILE *l_pFile = NULL;

  l_pFile = fopen (p_achFilename, "rb");
  if (NULL != l_pFile) {

    // Load first 0x40 and check version
    fread (&this->sFileHeader, sizeof (uint8_t), 0x40, l_pFile);
    if (   (this->sFileHeader.u32Version >= 0x00000150)
        && (this->sFileHeader.u32VGMdataoffset != 0x0000000C) ) {

      // Load remaining header bytes
      fread (((uint8_t *)&this->sFileHeader + 0x40), sizeof (uint8_t), 0xC0, l_pFile);
    }

    // VGM file ?
    if (0 == strncmp ("Vgm ", this->sFileHeader.achID, 4) ) {

        // Load GD3 tag if exist
        if (0UL != this->sFileHeader.u32GD3offset) {

          fseek (l_pFile, this->sFileHeader.u32GD3offset + 0x14, SEEK_SET);

          // Load 0x0c first byte
          fread (&this->sGd3Tag, sizeof (uint8_t), 0x0C, l_pFile);

          this->sGd3Tag.achContent = new TCHAR[this->sGd3Tag.u32Length];
          if (NULL != this->sGd3Tag.achContent) {

            // Load remaing Gd3 bytes
            fread (this->sGd3Tag.achContent, sizeof (uint8_t), this->sGd3Tag.u32Length, l_pFile);
          }
          else {

            // Debug message
            printf ("bVgm_Load : this->sGd3Tag.achContent => Can't allocate memory !");
          }
        }

        printf ("ID: %c%c%c%c\r\n", this->sFileHeader.achID[0]
                                  , this->sFileHeader.achID[1]
                                  , this->sFileHeader.achID[2]
                                  , this->sFileHeader.achID[3]);

        printf ("V: %x\r\n", this->sFileHeader.u32Version);

         if (0x80000000&this->sFileHeader.u32SN76489clock)
          printf ("T6W28 - Clock: %d\r\n",this->sFileHeader.u32SN76489clock);
        else
          printf ("SN76489 - Clock: %d\r\n",this->sFileHeader.u32SN76489clock);

        // Allocate memory for song
        this->u32FileLen = this->sFileHeader.u32GD3offset + 0x14 - 0x40;
        this->pau8Song = new uint8_t[this->u32FileLen];

        // Read song
        fseek (l_pFile, 0x40, SEEK_SET);
        fread (this->pau8Song, 1u, this->u32FileLen, l_pFile);
    }
    else {

       // Debug message
       printf ("bVgm_Load: %s not a VGM !", p_achFilename);
    }

    fclose (l_pFile);

    l_bReturn = true;
  }
  else {

    // Debug message
    printf ("bVgm_Load : Can't open %s !", p_achFilename);
  }

  return (l_bReturn);
}

bool Vgm::bUpdate (void* p_pvParameters) {

  /* Locals variables declaration. */
  bool l_bReturn                            = false;
  uint16_t *l_pu16AudioOutput               = NULL;
  static s_Sn76489ExtSig_t l_sSn76489ExtSig = {SN76489_LOW, SN76489_HIGH, SN76489_HIGH, SN76489_HIGH, 0, 0, 0, 0};
  static uint16_t l_u16Counter              = 0u;

  static double l_dCounter = 0.0f;
  static double l_dTrigger = 0.0f;

  /* Pointers checking. */
  if (NULL != p_pvParameters) {

    // Cast input pointer
    l_pu16AudioOutput = (uint16_t*) p_pvParameters;

    // If YM trig
    if (0u == l_u16Counter) {


      while (0x50 == this->pau8Song[this->u32Index])
      {
        this->u32Index++;
        l_sSn76489ExtSig.u8DataAddress = this->pau8Song[this->u32Index++];

        l_sSn76489ExtSig.eCEn_In = SN76489_LOW;
        l_sSn76489ExtSig.eWEn_In = SN76489_LOW;

        this->soundchip->u16UpdateIOs(&l_sSn76489ExtSig);

        l_sSn76489ExtSig.eCEn_In = SN76489_HIGH;
        l_sSn76489ExtSig.eWEn_In = SN76489_HIGH;
      }


      switch (this->pau8Song[this->u32Index]) {

        case 0x4F:
          this->u32Index++;
          this->u32Index++;
          break;

        case 0x50:
          this->u32Index++;
          l_sSn76489ExtSig.u8DataAddress = this->pau8Song[this->u32Index++];

          l_sSn76489ExtSig.eCEn_In = SN76489_LOW;
          l_sSn76489ExtSig.eWEn_In = SN76489_LOW;
          break;

        case 0x61:
          this->u32Index++;
          l_u16Counter  =  this->pau8Song[this->u32Index++];
          l_u16Counter |= (this->pau8Song[this->u32Index++]<<8u);
          break;

        case 0x62:
          this->u32Index++;
          l_u16Counter = 735u;
          break;

        case 0x63:
          this->u32Index++;
          l_u16Counter = 882u;
          break;

        case 0x66:
          this->u32Index = 0u;
          break;

        default:
          fprintf (pfDbg, "%02x\r\n", this->pau8Song[this->u32Index]);
          this->u32Index++;
          break;
      }
    }
    else {

      // Inc YM Player counter
      l_u16Counter--;
    }

    do {

        // Next clock state
        l_sSn76489ExtSig.eCLOCK_In = (l_sSn76489ExtSig.eCLOCK_In == SN76489_HIGH) ? SN76489_LOW : SN76489_HIGH;

        (*l_pu16AudioOutput) = this->soundchip->u16UpdateIOs(&l_sSn76489ExtSig);

        l_sSn76489ExtSig.eCEn_In = SN76489_HIGH;
        l_sSn76489ExtSig.eWEn_In = SN76489_HIGH;

        l_dCounter++;
    }
    while (l_dCounter < l_dTrigger);

    l_dTrigger += (double)this->sFileHeader.u32SN76489clock/16/(double)44100.0f;

    // No errors
    l_bReturn = true;
  }

  return (l_bReturn);
}

