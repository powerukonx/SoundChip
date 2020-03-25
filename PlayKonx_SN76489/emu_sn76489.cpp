/* =============================================================================

                  ██╗   ██╗██╗  ██╗ ██████╗ ███╗   ██╗██╗  ██╗
                  ██║   ██║██║ ██╔╝██╔═══██╗████╗  ██║╚██╗██╔╝
                  ██║   ██║█████╔╝ ██║   ██║██╔██╗ ██║ ╚███╔╝
                  ██║   ██║██╔═██╗ ██║   ██║██║╚██╗██║ ██╔██╗
                  ╚██████╔╝██║  ██╗╚██████╔╝██║ ╚████║██╔╝ ██╗
                   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝

  File name:    emu_sn74489.cpp
  Date:         23 07 2017
  Author:       Power.
  Description:  SN76486 Digital Complex Sound generator - Body file.

============================================================================= */


/* =============================================================================
                                 DEBUG Section
============================================================================= */

/* =============================================================================
                                 Include Files
============================================================================= */
#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "emu_sn76489.h"

/* =============================================================================
                               Defines and constants
============================================================================= */

#define UINT8_ZERO              ((uint8_t)0)
#define UINT16_ZERO             ((uint16_t)0)
#define UINT32_ZERO             ((uint32_t)0)

#define BIT0_POS                ((uint8_t)0)
#define BIT1_POS                ((uint8_t)1)
#define BIT2_POS                ((uint8_t)2)
#define BIT3_POS                ((uint8_t)3)
#define BIT4_POS                ((uint8_t)4)
#define BIT5_POS                ((uint8_t)5)
#define BIT6_POS                ((uint8_t)6)
#define BIT7_POS                ((uint8_t)7)
#define BIT8_POS                ((uint8_t)8)
#define BIT9_POS                ((uint8_t)9)
#define BIT10_POS               ((uint8_t)10)
#define BIT11_POS               ((uint8_t)11)
#define BIT12_POS               ((uint8_t)12)
#define BIT13_POS               ((uint8_t)13)
#define BIT14_POS               ((uint8_t)14)
#define BIT15_POS               ((uint8_t)15)

#define BIT0_MSK                ((uint8_t)(1 << BIT0_POS))
#define BIT1_MSK                ((uint8_t)(1 << BIT1_POS))
#define BIT2_MSK                ((uint8_t)(1 << BIT2_POS))
#define BIT3_MSK                ((uint8_t)(1 << BIT3_POS))
#define BIT4_MSK                ((uint8_t)(1 << BIT4_POS))

#define SAMPLING_MASK           ((uint8_t)0x03)
#define SAMPLING_LOW            ((uint8_t)0)
#define RISING_EDGE             ((uint8_t)1)
#define FALLING_EDGE            ((uint8_t)2)
#define SAMPLING_HIGH           ((uint8_t)3)

#define CLOCK_DIVIDER           16u
#define NOISE_TAPPED            9u

#define LFSR_INIT_VALUE         ((uint16_t)0x8000)

#define TOGGLE_INIT_VALUE       1

#define SN76489_VOLUME_MAX      ((uint8_t)0)
#define SN76489_VOLUME_MIN      ((uint8_t)0x0F)

#define LATCHDATA_POS           ((uint8_t)7)
#define LATCHDATA_MSK           ((uint8_t)(1<<LATCHDATA_POS))

// Specific latch
#define LATCH_CHANNEL_POS       ((uint8_t)5)
#define LATCH_CHANNEL_MSK       ((uint8_t)(3<<LATCH_CHANNEL_POS))
#define LATCH_CHANNEL0          ((uint8_t)(0<<LATCH_CHANNEL_POS))
#define LATCH_CHANNEL1          ((uint8_t)(1<<LATCH_CHANNEL_POS))
#define LATCH_CHANNEL2          ((uint8_t)(2<<LATCH_CHANNEL_POS))
#define LATCH_CHANNEL3          ((uint8_t)(3<<LATCH_CHANNEL_POS))

#define LATCH_TYPE_POS          ((uint8_t)4)
#define LATCH_TYPE_MSK          ((uint8_t)(1<<LATCH_TYPE_POS))
#define LATCH_TYPE_VOLUME       ((uint8_t)(1<<LATCH_TYPE_POS))
#define LATCH_TYPE_TONENOISE    ((uint8_t)(0<<LATCH_TYPE_POS))

#define LATCH_NOISE_POS         ((uint8_t)0)
#define LATCH_NOISE_MSK         ((uint8_t)(0x07<<LATCH_DATA_POS))
#define LATCH_NOISE_MODE_POS    ((uint8_t)2)
#define LATCH_NOISE_MODE_MSK    ((uint8_t)(0x01<<LATCH_NOISE_MODE_POS))
#define LATCH_NOISE_PERIODIC    ((uint8_t)(0<<LATCH_NOISE_MODE_POS))
#define LATCH_NOISE_WHITE       ((uint8_t)(1<<LATCH_NOISE_MODE_POS))

#define LATCH_DATA_POS          ((uint8_t)0)
#define LATCH_DATA_MSK          ((uint8_t)(0x0F<<LATCH_DATA_POS))

// Specific data
#define DATA_POS                ((uint8_t)0)
#define DATA_MSK                ((uint8_t)(0x3F<<LATCH_DATA_POS))

#define TONE_REG_LEN            ((uint8_t)10)
#define VOL_REG_LEN             ((uint8_t)4)
#define NOISE_REG_LEN           ((uint8_t)4)

typedef enum _SN76489OUTS_ {
  SN76489_OUT_TONE1 = (uint8_t)0u,
  SN76489_OUT_TONE2,
  SN76489_OUT_TONE3,
  SN76489_OUT_NOISE,
  SN76489_OUT_MAX
} e_Sn76489Outs_t;

typedef enum _NOISE_COUNTER_ {
    NOISE_COUNTER_0 = 0x10u,
    NOISE_COUNTER_1 = 0x20u,
    NOISE_COUNTER_2 = 0x30u,
} e_NoiseCounter_t;

/* =============================================================================
                               Private attributs
============================================================================= */
const uint16_t Sn76489::cau16Amplitude[VOLUME_NB] = {
  32767, 26028, 20675, 16422, 13045, 10362,  8231,  6568,
   5193,  4125,  3277,  2603,  2067,  1642,  1304,     0
};

/* =============================================================================
                              Private methods
============================================================================= */
void Sn76489::vUpdateCounters (void) {

  for (uint8_t l_u8Chan = SN76489_REG_TONE1_FREQ; l_u8Chan < SN76489_REG_NOISE_VOL; l_u8Chan += 2u)
  {
    uint8_t l_u8Tmp = l_u8Chan >> BIT1_POS;

    // Each clock cycle, the counter is decremented (if it is non-zero)
    if (UINT16_ZERO < this->u16Counter[l_u8Tmp]) {

      this->u16Counter[l_u8Tmp]--;
    }
    else {

      switch (l_u8Chan) {

        case (SN76489_REG_TONE3_FREQ):
          this->u16Counter[l_u8Tmp] = this->au16Regs[SN76489_REG_TONE3_FREQ];
          this->u8ToggleOutput[l_u8Tmp] ^= TOGGLE_INIT_VALUE;
          break;
        case (SN76489_REG_TONE2_FREQ):
          this->u16Counter[l_u8Tmp] = this->au16Regs[SN76489_REG_TONE2_FREQ];
          this->u8ToggleOutput[l_u8Tmp] ^= TOGGLE_INIT_VALUE;
          break;
        case (SN76489_REG_TONE1_FREQ):
          this->u16Counter[l_u8Tmp] = this->au16Regs[SN76489_REG_TONE1_FREQ];
          this->u8ToggleOutput[l_u8Tmp] ^= TOGGLE_INIT_VALUE;
          break;
        case (SN76489_REG_NOISE_CTRL):

          switch (this->au16Regs[SN76489_REG_NOISE_CTRL] & 0x03u) {

            case 0u:
                this->u16Counter[l_u8Tmp] = NOISE_COUNTER_0;
                break;
            case 1u:
                this->u16Counter[l_u8Tmp] = NOISE_COUNTER_1;
                break;
            case 2u:
                this->u16Counter[l_u8Tmp] = NOISE_COUNTER_2;
                break;
            default:
            case 3u:
                this->u16Counter[l_u8Tmp] = this->au16Regs[SN76489_REG_TONE2_FREQ];
                break;
          }
          break;
        default:
          break;
      }

    }
  }
}

int Sn76489::iParity (int val) {
  val^=val>>8;
  val^=val>>4;
  val^=val>>2;
  val^=val>>1;
  return val&1;
}

void Sn76489::vUpdateNoiseGenerator (void) {

  static uint8_t l_u8Clock = 0;

  if (l_u8Clock == 64) // TODO Pourquoi 64 ?
  {
      uint8_t u8NoiseFB = (this->au16Regs[SN76489_REG_NOISE_CTRL] >> BIT2_POS) & BIT0_MSK;
      this->u8ToggleOutput[SN76489_OUT_NOISE] = this->u16LFSR & BIT0_MSK;
      this->u16LFSR = (this->u16LFSR >> BIT1_POS) | ( (u8NoiseFB ? this->iParity(this->u16LFSR & NOISE_TAPPED) : this->u16LFSR & BIT0_MSK) << BIT15_POS);

      l_u8Clock = 0u;


  }
  else
  {
      l_u8Clock++;


  }
}

void Sn76489::vUpdateChannels (void) {

  uint32_t u32AudioOutput = UINT32_ZERO;

  u32AudioOutput =  cau16Amplitude[this->au16Regs[SN76489_REG_TONE1_VOL]]*this->u8ToggleOutput[SN76489_OUT_TONE1]
                  + cau16Amplitude[this->au16Regs[SN76489_REG_TONE2_VOL]]*this->u8ToggleOutput[SN76489_OUT_TONE2]
                  + cau16Amplitude[this->au16Regs[SN76489_REG_TONE3_VOL]]*this->u8ToggleOutput[SN76489_OUT_TONE3]
                  + cau16Amplitude[this->au16Regs[SN76489_REG_NOISE_VOL]]*this->u8ToggleOutput[SN76489_OUT_NOISE];

  this->u16AudioOutput = (uint16_t)(u32AudioOutput / (SN76489_REG_MAX >> BIT1_POS))*2;
}

/* =============================================================================
                               Public methods
============================================================================= */
Sn76489::Sn76489() {

  // Initialize all registers to default values.
  this->u8CurrentChannel  = UINT8_ZERO;
  this->u8Clock           = UINT8_ZERO;

  this->pfDbg = fopen("SN76489.txt","wt");

  // Initilize external signals to default value.
  memset ((void *)&this->au16Regs, UINT8_ZERO, sizeof (this->au16Regs) );
  memset ((void *)&this->u16Counter, UINT8_ZERO, sizeof (this->u16Counter) );
  memset ((void *)&this->sExternalSignals, 0, sizeof (this->sExternalSignals) );
  memset ((void *)&this->u8ToggleOutput, UINT8_ZERO, sizeof (this->u8ToggleOutput) );

  this->u8ToggleOutput[SN76489_OUT_NOISE] = 0u;
}

Sn76489::~Sn76489() {

  fclose (this->pfDbg);

  // Nothing to do.
}

uint16_t Sn76489::u16UpdateIOs (s_Sn76489ExtSig_t *p_psExtSig) {

  // Locals variables d  this->u8ToggleOutput[SN76489_OUT_NOISE] = this->u16LFSR & BIT0_MSK;eclaration.
  uint16_t l_u16Return          = UINT8_ZERO;
  static uint8_t l_u8CurrentReg = UINT8_ZERO;

  if (NULL != p_psExtSig) {

    // Copy input signal into context
    this->sExternalSignals.eCLOCK_In     = p_psExtSig->eCLOCK_In;
    this->sExternalSignals.eWEn_In       = p_psExtSig->eWEn_In;
    this->sExternalSignals.eCEn_In       = p_psExtSig->eCEn_In;
    this->sExternalSignals.fVcc_In       = p_psExtSig->fVcc_In;
    this->sExternalSignals.fGnd_In       = p_psExtSig->fGnd_In;
    this->sExternalSignals.u8DataAddress = p_psExtSig->u8DataAddress;

    // Signal sampling
    this->u8Clock = ( (this->u8Clock << BIT1_POS) | this->sExternalSignals.eCLOCK_In);

    if (SN76489_LOW == this->sExternalSignals.eCEn_In) {

      if (SN76489_LOW == this->sExternalSignals.eWEn_In) {

        // Latch/Data ?
        if (LATCHDATA_MSK == (LATCHDATA_MSK & this->sExternalSignals.u8DataAddress) ) {

          l_u8CurrentReg = (this->sExternalSignals.u8DataAddress >> BIT4_POS) & 0x7u;
          switch (l_u8CurrentReg) {

            case SN76489_REG_TONE1_FREQ:
            case SN76489_REG_TONE2_FREQ:
            case SN76489_REG_TONE3_FREQ:
              au16Regs[l_u8CurrentReg]  &= 0x03F0u;
              au16Regs[l_u8CurrentReg]  |= (this->sExternalSignals.u8DataAddress & 0x0fu);
              break;
            case SN76489_REG_TONE1_VOL:
            case SN76489_REG_TONE2_VOL:
            case SN76489_REG_TONE3_VOL:
            case SN76489_REG_NOISE_VOL:
              au16Regs[l_u8CurrentReg]  = (this->sExternalSignals.u8DataAddress & 0x0fu);
              break;
            case SN76489_REG_NOISE_CTRL:
              au16Regs[l_u8CurrentReg]  = (this->sExternalSignals.u8DataAddress & 0x07u);
              this->u16LFSR             = LFSR_INIT_VALUE;
              break;
            default:
              break;
          }
        }
        else {

          au16Regs[l_u8CurrentReg] &= 0x000Fu;
          au16Regs[l_u8CurrentReg] |= (this->sExternalSignals.u8DataAddress << BIT4_POS);
        }
      }
    }

    // On rising and falling edge.
    if (   ( RISING_EDGE == (this->u8Clock & SAMPLING_MASK) )
        || (FALLING_EDGE == (this->u8Clock & SAMPLING_MASK) ) ) {

      // Update tone generato              au16Regs[l_u8CurrentReg]  = (this->sExternalSignals.u8DataAddress & 0x0fu);
             // break;rs.
      this->vUpdateCounters ();

      // Update noise generator.
      this->vUpdateNoiseGenerator ();

      // Update Channels.
      this->vUpdateChannels ();
    }

    // Update analog output.
    p_psExtSig->u16AUDIOOUT_Out = this->u16AudioOutput;

    l_u16Return = this->u16AudioOutput;
  }

  return (l_u16Return);
}

