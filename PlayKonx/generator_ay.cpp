/* =============================================================================

                  ██╗   ██╗██╗  ██╗ ██████╗ ███╗   ██╗██╗  ██╗
                  ██║   ██║██║ ██╔╝██╔═══██╗████╗  ██║╚██╗██╔╝
                  ██║   ██║█████╔╝ ██║   ██║██╔██╗ ██║ ╚███╔╝
                  ██║   ██║██╔═██╗ ██║   ██║██║╚██╗██║ ██╔██╗
                  ╚██████╔╝██║  ██╗╚██████╔╝██║ ╚████║██╔╝ ██╗
                   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝

  File name:    generator_ay.cpp
  Date:         23 07 2017
  Author:       Power.
  Description:  AY-3-8912 Sound generator - Body file.

============================================================================= */


/* =============================================================================
                                 DEBUG Section
============================================================================= */
#define DBG_CHANNELA_ENABLE
#define DBG_CHANNELB_ENABLE
#define DBG_CHANNELC_ENABLE
#define DBG_NOISE_ENABLE
#define DBG_ENVELOPE_ENABLE


/* =============================================================================
                                 Include Files
============================================================================= */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "generator_ay.h"


/* =============================================================================
                          Private defines and typedefs
============================================================================= */
#define TONE_CHANNEL_A_ENABLE       (0x01)
#define TONE_CHANNEL_B_ENABLE       (0x02)
#define TONE_CHANNEL_C_ENABLE       (0x04)
#define TONE_PERIOD_MIN             (0)
#define NOISE_CHANNEL_A_ENABLE      (0x08)
#define NOISE_CHANNEL_B_ENABLE      (0x10)
#define NOISE_CHANNEL_C_ENABLE      (0x20)
#define NOISE_PERIOD_MIN            (0)
#define NOISE_REGISTER_INIT         (0x24000)
#define ENVELOPE_SHAPE_INIT         (0)
#define ENVELOPE_ENABLE             (0x10)
#define ENVELOPE_STEP_0             (0)
#define ENVELOPE_OFF                (16)
#define ENVELOPE_ON                 (17)
#define ENVELOPE_LOOP               (18)
#define ENVELOPE_PERIOD_MIN         (0)
#define MIXER_INIT                  0x3F
#define AY_MAX_AMPLITUDE            (15)
#define AY_MIN_AMPLITUDE            (0)
#define MASK_16BITS                 (0xFFFF)
#define BIT0                        (1<<0)
#define BIT1                        (1<<1)
#define BIT2                        (1<<2)
#define CLOCK_SAMPLING_MASK         (0x03)
#define CLOCK_RISING_EDGE           (1)
#define CLOCK_FALLING_EDGE          (2)

typedef enum _AYBCxBDIR_ {
  AY_INACTIVE_000 = 0u,
  AY_LATCH_ADDRESS_001,
  AY_INACTIVE_010,
  AY_READ_FROM_PSG,
  AY_LATCH_ADDRESS_100,
  AY_INACTIVE_101,
  AY_WRITE_TO_PSG ,
  AY_LATCH_ADDRESS_111
} e_AyBcxBdir_t;


/* =============================================================================
                        Private constants and variables
============================================================================= */
static const UINT8 g_cau8EnvelopeShap[16][33] = { /* AY-3-8912 Envelope shape .*/
  {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,ENVELOPE_OFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,ENVELOPE_OFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,ENVELOPE_OFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,ENVELOPE_OFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,ENVELOPE_OFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,ENVELOPE_OFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,ENVELOPE_OFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,ENVELOPE_OFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,ENVELOPE_LOOP,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,ENVELOPE_OFF},
  {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,ENVELOPE_LOOP},
  {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,ENVELOPE_ON,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,ENVELOPE_LOOP,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,ENVELOPE_ON,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,ENVELOPE_LOOP},
  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,ENVELOPE_OFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

static const UINT16 g_cau16Amplitude[16] = {
  0,231,695,1158,2084,2779,4168,6716,8105,13200,18294,24315,32189,40757,52799,65535
};

static UINT8 g_u8ChannelASidState = 0u;
static UINT8 g_u8ChannelBSidState = 0u;
static UINT8 g_u8ChannelCSidState = 0u;


/* =============================================================================
                        Public constants and variables
============================================================================= */


/* =============================================================================
                        Private function declarations
============================================================================= */
static VOID vAYEngineReset (s_AyContext_t* p_psAyContext);
static VOID vReadBusDirControl (s_AyContext_t* p_psAyContext);

#if defined(DBG_CHANNELA_ENABLE) || defined(DBG_CHANNELB_ENABLE) || defined(DBG_CHANNELC_ENABLE)
static VOID vUpdateToneGenerator (s_AyContext_t* p_psAyContext);
#endif /* DBG_CHANNELA_ENABLE || DBG_CHANNELB_ENABLE || DBG_CHANNELC_ENABLE*/

#ifdef DBG_NOISE_ENABLE
static VOID vUpdateNoiseGenerator (s_AyContext_t* p_psAyContext);
#endif /* DBG_NOISE_ENABLE */

#ifdef DBG_ENVELOPE_ENABLE
static VOID vUpdateEnvelopeGenerator (s_AyContext_t* p_psAyContext);
#endif /* DBG_ENVELOPE_ENABLE */

#ifdef DBG_CHANNELA_ENABLE
static VOID vUpdateChannelA (s_AyContext_t* p_psAyContext);
#endif /* DBG_CHANNELA_ENABLE */

#ifdef DBG_CHANNELB_ENABLE
static VOID vUpdateChannelB (s_AyContext_t* p_psAyContext);
#endif /* DBG_CHANNELB_ENABLE */

#ifdef DBG_CHANNELC_ENABLE
static VOID vUpdateChannelC (s_AyContext_t* p_psAyContext);
#endif /* DBG_CHANNELC_ENABLE */


/* =============================================================================
                               Public functions
============================================================================= */

/*==============================================================================
Function    :   vAY_SetChannelASidState

Describe    :   .

Parameters  :   .

Returns     :   None.
==============================================================================*/
VOID vAY_SetChannelASidState (UINT8 p_u8Activate) {
  g_u8ChannelASidState = p_u8Activate;
}


/*==============================================================================
Function    :   vAY_SetChannelBSidState

Describe    :   .

Parameters  :   .

Returns     :   None.
==============================================================================*/
VOID vAY_SetChannelBSidState (UINT8 p_u8Activate) {
  g_u8ChannelBSidState = p_u8Activate;
}


/*==============================================================================
Function    :   vAY_SetChannelCSidState

Describe    :   .

Parameters  :   .

Returns     :   None.
==============================================================================*/
VOID vAY_SetChannelCSidState (UINT8 p_u8Activate) {
  g_u8ChannelCSidState = p_u8Activate;
}


/*==============================================================================
Function    :   eAYEngineUpdate

Describe    :   Update AY-3-891x engine

Parameters  :   p_psAyContext = Pointer to Ay context structure.

Returns     :   AY_ERROR_PARAM or AY_ERROR_NO
==============================================================================*/
e_AyErrors_t eAYEngineUpdate (s_AyContext_t* p_psAyContext) {

  /* Locals variables declaration. */
  e_AyErrors_t l_eReturn = AY_ERROR_PARAM;

  /* Input parameters checking. */
  if (NULL != p_psAyContext) {

    /* Reset active ? */
    if (AY_LOW == p_psAyContext->sExternalSignals.eRESETn_In) {

      /* Reset engine. */
      vAYEngineReset (p_psAyContext);
    }
    else {

      /* Sample clock input. */
      p_psAyContext->u8ClockShiftRegister = ( (p_psAyContext->u8ClockShiftRegister<<1)
                                            | p_psAyContext->sExternalSignals.eCLOCK_In);

      /* On Falling edge. */
      if (CLOCK_FALLING_EDGE == (p_psAyContext->u8ClockShiftRegister & CLOCK_SAMPLING_MASK)) {

        vReadBusDirControl (p_psAyContext);
      }
      else {

        /* Nothing to do. */
      }

      /* On rising and falling edge. */
      if (   ((p_psAyContext->u8ClockShiftRegister & CLOCK_SAMPLING_MASK) == CLOCK_RISING_EDGE)
          || ((p_psAyContext->u8ClockShiftRegister & CLOCK_SAMPLING_MASK) == CLOCK_FALLING_EDGE)) {

#if defined(DBG_CHANNELA_ENABLE) || defined(DBG_CHANNELB_ENABLE) || defined(DBG_CHANNELC_ENABLE)
        /* Update tone generators. */
        vUpdateToneGenerator (p_psAyContext);
#endif /* CHANNELA_ENABLE || CHANNELB_ENABLE || CHANNELC_ENABLE */

#ifdef DBG_NOISE_ENABLE
        /* Update noise generator. */
        vUpdateNoiseGenerator (p_psAyContext);
#endif /* NOISE_ENABLE */

#ifdef DBG_ENVELOPE_ENABLE
        /* Update envelope generator. */
        vUpdateEnvelopeGenerator (p_psAyContext);
#endif /* ENVELOPE_ENABLE */

#ifdef DBG_CHANNELA_ENABLE
        /* Update Channel A. */
        vUpdateChannelA (p_psAyContext);
#endif /* CHANNELA_ENABLE */

#ifdef DBG_CHANNELB_ENABLE
        /* Update Channel B. */
        vUpdateChannelB (p_psAyContext);
#endif /* CHANNELB_ENABLE */

#ifdef DBG_CHANNELC_ENABLE
        /* Update Channel C. */
        vUpdateChannelC (p_psAyContext);
#endif /* CHANNELC_ENABLE */
      }
      else {

        /* Nothing to do. */
      }
    }

    l_eReturn = AY_ERROR_NO;
  }
  else {

    /* Nothing to do. */
  }

   return (l_eReturn);
}


/* =============================================================================
                              Private functions
============================================================================= */

/*==============================================================================
Function    :   vAYEngineReset

Describe    :   Reset AY-3-891x engine

Parameters  :   p_psAyContext = Pointer to Ay context structure.

Returns     :   None.
==============================================================================*/
static VOID vAYEngineReset (s_AyContext_t* p_psAyContext) {

  /* Initialize registers. */
  memset (p_psAyContext->au8AyRegister, 0u, sizeof (p_psAyContext->au8AyRegister));

  /* Tone generator init. */
  p_psAyContext->u16ToneCounterChannelA = 0u;
  p_psAyContext->u16ToneCounterChannelB = 0u;
  p_psAyContext->u16ToneCounterChannelC = 0u;
  p_psAyContext->u8ToneOutputChannelA = 0x00u;
  p_psAyContext->u8ToneOutputChannelB = 0x00u;
  p_psAyContext->u8ToneOutputChannelC = 0x00u;

  /* Noise generator init. */
  p_psAyContext->u32NoiseCounter = 0u;
  p_psAyContext->u32NoiseShiftRegister = NOISE_REGISTER_INIT;
  p_psAyContext->u8NoiseOutput = 0x00u;

  /* Envelope generator init. */
  p_psAyContext->u32EnvelopeCounter = 0u;
  p_psAyContext->u8EnvelopeStep = ENVELOPE_STEP_0;

  /* Mixer/IO init. */
  p_psAyContext->au8AyRegister[AY_REG_7] = 0xFFu;

  /* Set current latch register. */
  p_psAyContext->u8AyCurrentRegister = AY_REG_0;

  /* Initialize clock shift register. */
  p_psAyContext->u8ClockShiftRegister = 0x00u;
}


/*==============================================================================
Function    :   vReadBusDirControl

Describe    :   Read bus and direction control, update engine.

Parameters  :   p_psAyContext = Pointer to Ay context structure.

Returns     :   None.
==============================================================================*/
static VOID vReadBusDirControl (s_AyContext_t* p_psAyContext) {

  /* Locals variables declaration. */
  UINT8 l_u8Address;

  /* Read bus constrol and direction. */
  l_u8Address =   (p_psAyContext->sExternalSignals.eBDIR_In<<2)
                | (p_psAyContext->sExternalSignals.eBC2_In<<1)
                | (p_psAyContext->sExternalSignals.eBC1_In);

  /* Bus control and direction manager. */
  switch (l_u8Address) {

    case AY_LATCH_ADDRESS_001:
    case AY_LATCH_ADDRESS_100:
    case AY_LATCH_ADDRESS_111: {

      /* If A8=1 and D7=D6=D5=D4=0. */
      if (   (0x00u == (p_psAyContext->sExternalSignals.u8DataAddress & 0xF0u))
          && (AY_HIGH == p_psAyContext->sExternalSignals.eA8_In)) {

        /* Set current register. */
        p_psAyContext->u8AyCurrentRegister = p_psAyContext->sExternalSignals.u8DataAddress;
      }
      break;
    }

    case AY_READ_FROM_PSG: {

      /* Current Reg 16 ? */
      if (AY_REG_E == p_psAyContext->u8AyCurrentRegister) {

        /* Read Port A and put result to Data bus. */
        p_psAyContext->sExternalSignals.u8DataAddress = p_psAyContext->sExternalSignals.u8IOA;
      }
      else {

        /* Put register contains to data bus. */
        p_psAyContext->sExternalSignals.u8DataAddress = p_psAyContext->au8AyRegister[p_psAyContext->u8AyCurrentRegister];
      }
      break;
    }

    case AY_WRITE_TO_PSG: {

      /* Current Reg 14 ? */
      if (AY_REG_E == p_psAyContext->u8AyCurrentRegister) {

        /* Read Data bus and put result to Port A. */
        p_psAyContext->sExternalSignals.u8IOA = p_psAyContext->sExternalSignals.u8DataAddress;
      }
      else {

        /* Enveloppe Shape register selected */
        if (AY_REG_D == p_psAyContext->u8AyCurrentRegister) {

          /* new value ? */
          if (0xffu !=  p_psAyContext->sExternalSignals.u8DataAddress) {

            /* Save enveloppe shape. */
            p_psAyContext->au8AyRegister[p_psAyContext->u8AyCurrentRegister] = p_psAyContext->sExternalSignals.u8DataAddress;

            /* Initialize envelope generator step. */
            p_psAyContext->u8EnvelopeStep = ENVELOPE_STEP_0;
          }
        }
        else {

          /* Save data. */
          p_psAyContext->au8AyRegister[p_psAyContext->u8AyCurrentRegister] = p_psAyContext->sExternalSignals.u8DataAddress;
        }
      }
      break;
    }

    case AY_INACTIVE_000:
    case AY_INACTIVE_010:
    case AY_INACTIVE_101:
    default: {

      break;
    }
  }
}


#if defined(DBG_CHANNELA_ENABLE) || defined(DBG_CHANNELB_ENABLE) || defined(DBG_CHANNELC_ENABLE)
/*==============================================================================
Function    :   eUpdateToneGenerator

Describe    :   Update tone generator.

Parameters  :   p_psAyContext = Pointer to Ay context structure.

Returns     :   None.
==============================================================================*/
static VOID vUpdateToneGenerator (s_AyContext_t* p_psAyContext) {

  static UINT8 l_u8ChannelAPwm = 1u;
  static UINT8 l_u8ChannelBPwm = 1u;
  static UINT8 l_u8ChannelCPwm = 1u;
  static UINT16 l_u16ChannelATon = 0u;
  static UINT16 l_u16ChannelBTon = 0u;
  static UINT16 l_u16ChannelCTon    = 0u;
  static UINT16 l_u16ChannelAPeriod = ((p_psAyContext->au8AyRegister[AY_REG_1]<<8)
                                        | p_psAyContext->au8AyRegister[AY_REG_0])<<4;
  static UINT16 l_u16ChannelBPeriod = ((p_psAyContext->au8AyRegister[AY_REG_3]<<8)
                                        | p_psAyContext->au8AyRegister[AY_REG_2])<<4;
  static UINT16 l_u16ChannelCPeriod = ((p_psAyContext->au8AyRegister[AY_REG_5]<<8)
                                        | p_psAyContext->au8AyRegister[AY_REG_4])<<4;
#ifdef DBG_CHANNELA_ENABLE
  if (g_u8ChannelASidState == 1u)
  {
    /* Channel A update */
    if (p_psAyContext->u16ToneCounterChannelA < l_u16ChannelATon)
    {
      p_psAyContext->u8ToneOutputChannelA = 15;
      p_psAyContext->u16ToneCounterChannelA++;
    }
    else if (p_psAyContext->u16ToneCounterChannelA < l_u16ChannelAPeriod)
    {
      p_psAyContext->u8ToneOutputChannelA = 0;
      p_psAyContext->u16ToneCounterChannelA++;
    }
    else
    {
      l_u16ChannelAPeriod = ((p_psAyContext->au8AyRegister[AY_REG_1]<<8)
                            | p_psAyContext->au8AyRegister[AY_REG_0])<<4;

      l_u16ChannelATon = l_u8ChannelAPwm * l_u16ChannelAPeriod/100;

      l_u8ChannelAPwm = (l_u8ChannelAPwm + 1)%100;

      p_psAyContext->u16ToneCounterChannelA = 0u;
    }
  }
  else
  {
    /* If Channel A Tone generator need to be updated .*/
    if (TONE_PERIOD_MIN == p_psAyContext->u16ToneCounterChannelA) {

      /* Reload counter with Reg 1 and 0 registers. Period = 16TP/F => divide by 2 because half-period working ! */
       p_psAyContext->u16ToneCounterChannelA  = (  (p_psAyContext->au8AyRegister[AY_REG_1]<<8)
                                                | p_psAyContext->au8AyRegister[AY_REG_0])<<3;

      /* Output toggle .*/
      p_psAyContext->u8ToneOutputChannelA ^= 0xFFu;
    }
    else {

      /* Wait before update .*/
      p_psAyContext->u16ToneCounterChannelA--;
    }
  }
#endif /* CHANNELA_ENABLE */

#ifdef DBG_CHANNELB_ENABLE
  if (g_u8ChannelBSidState == 1u)
  {
    /* Channel B update */
    if (p_psAyContext->u16ToneCounterChannelB < l_u16ChannelBTon)
    {
      p_psAyContext->u8ToneOutputChannelB = 15;
      p_psAyContext->u16ToneCounterChannelB++;
    }
    else if (p_psAyContext->u16ToneCounterChannelB < l_u16ChannelBPeriod)
    {
      p_psAyContext->u8ToneOutputChannelB = 0;
      p_psAyContext->u16ToneCounterChannelB++;
    }
    else
    {
      l_u16ChannelBPeriod = ((p_psAyContext->au8AyRegister[AY_REG_3]<<8)
                            | p_psAyContext->au8AyRegister[AY_REG_2])<<4;

      l_u16ChannelBTon = l_u8ChannelBPwm * l_u16ChannelBPeriod/100;

      l_u8ChannelBPwm = (l_u8ChannelBPwm + 1)%100;

      p_psAyContext->u16ToneCounterChannelB = 0u;
    }
  }
  else
  {
    /* If Channel B Tone generator need to be updated. */
    if (TONE_PERIOD_MIN == p_psAyContext->u16ToneCounterChannelB) {

      /* Reload counter with Reg 3 and 2 registers. Period = 16TP/F => divide by 2 because half-period working ! */
      p_psAyContext->u16ToneCounterChannelB  = (  (p_psAyContext->au8AyRegister[AY_REG_3]<<8)
                                                | p_psAyContext->au8AyRegister[AY_REG_2])<<3;

      /* Output toggle .*/
      p_psAyContext->u8ToneOutputChannelB ^= 0xFFu;
    }
    else {

      /* Wait before update .*/
      p_psAyContext->u16ToneCounterChannelB--;
    }
  }
#endif /* CHANNELB_ENABLE */

#ifdef DBG_CHANNELC_ENABLE
  if (g_u8ChannelCSidState == 1u)
  {
    /* Channel C update */
    if (p_psAyContext->u16ToneCounterChannelC < l_u16ChannelCTon)
    {
      p_psAyContext->u8ToneOutputChannelC = 15;
      p_psAyContext->u16ToneCounterChannelC++;
    }
    else if (p_psAyContext->u16ToneCounterChannelC < l_u16ChannelCPeriod)
    {
      p_psAyContext->u8ToneOutputChannelC = 0;
      p_psAyContext->u16ToneCounterChannelC++;
    }
    else
    {
      l_u16ChannelCPeriod = ((p_psAyContext->au8AyRegister[AY_REG_5]<<8)
                            | p_psAyContext->au8AyRegister[AY_REG_4])<<4;

      l_u16ChannelCTon = l_u8ChannelCPwm * l_u16ChannelCPeriod/100;

      l_u8ChannelCPwm++;
      if (l_u8ChannelCPwm > 90) l_u8ChannelCPwm = 10;// = (l_u8ChannelCPwm + 1)%100;

      p_psAyContext->u16ToneCounterChannelC = 0u;
    }
  }
  else
  {
    /* If Channel C Tone generator need to be updated.*/
    if (TONE_PERIOD_MIN == p_psAyContext->u16ToneCounterChannelC) {

      /* Reload counter with Reg 3 and 2 registers. Period = 16TP/F => divide by 2 because half-period working ! */
      p_psAyContext->u16ToneCounterChannelC  = (  (p_psAyContext->au8AyRegister[AY_REG_5]<<8)
                                                | p_psAyContext->au8AyRegister[AY_REG_4])<<3;

      /* Output toggle .*/
      p_psAyContext->u8ToneOutputChannelC ^= 0xFFu;
    }
    else {

      /* Wait before update .*/
      p_psAyContext->u16ToneCounterChannelC--;
    }
  }
#endif /* CHANNELC_ENABLE */
}
#endif /* CHANNELA_ENABLE || CHANNELB_ENABLE || CHANNELC_ENABLE */


#ifdef DBG_NOISE_ENABLE
/*==============================================================================
Function    :   vUpdateNoiseGenerator

Describe    :   Update noise generator.

Parameters  :   p_psAyContext = Pointer to Ay context structure.

Returns     :   None.
==============================================================================*/
static VOID vUpdateNoiseGenerator (s_AyContext_t* p_psAyContext) {

  /* Locals variables declaration. */
  UINT8 l_u8Noise;

  /* If noise generator need to be updated .*/
  if (NOISE_PERIOD_MIN == p_psAyContext->u32NoiseCounter) {

    /* Reload counter with Reg 6 register. Period = 16TP/F => divide by 2 because half-period working ! */
    p_psAyContext->u32NoiseCounter  = (p_psAyContext->au8AyRegister[AY_REG_6])<<3;

    /* Do some pseudo-random stuffs... */
    l_u8Noise = ((((p_psAyContext->u32NoiseShiftRegister & BIT2)>>2) ^ (p_psAyContext->u32NoiseShiftRegister & BIT0)) & BIT0);
    p_psAyContext->u32NoiseShiftRegister  = ((p_psAyContext->u32NoiseShiftRegister >> 1) & MASK_16BITS) + (l_u8Noise << 16);
    if (p_psAyContext->u32NoiseShiftRegister & BIT1) {

      /* Nothing to do. */
    }
    else {

      p_psAyContext->u8NoiseOutput ^= 0xFFu;
    }
  }
  else {

    /* Wait before update .*/
    p_psAyContext->u32NoiseCounter--;
  }
}
#endif /* NOISE_ENABLE */


#ifdef DBG_ENVELOPE_ENABLE
/*==============================================================================
Function    :   vUpdateEnvelopeGenerator

Describe    :   Update envelope generator.

Parameters  :   p_psAyContext = Pointer to Ay context structure.

Returns     :   None.
==============================================================================*/
static VOID vUpdateEnvelopeGenerator (s_AyContext_t* p_psAyContext) {

  /* If envelope generator need to be updated. */
  if (ENVELOPE_PERIOD_MIN == p_psAyContext->u32EnvelopeCounter) {

    /* Reload counter with Reg 12 and 11 registers. Period = 32TP/F => divide by 2 because half-period working ! */
    p_psAyContext->u32EnvelopeCounter  = (  (p_psAyContext->au8AyRegister[AY_REG_C]<<8)
                                          | p_psAyContext->au8AyRegister[AY_REG_B])<<4;

    /* Switch envelopp step. */
    switch (g_cau8EnvelopeShap[p_psAyContext->au8AyRegister[AY_REG_D]][p_psAyContext->u8EnvelopeStep]) {

      case ENVELOPE_LOOP: {

        p_psAyContext->u8EnvelopeStep = ENVELOPE_STEP_0;
        p_psAyContext->u8EnvelopeOutput = g_cau8EnvelopeShap[p_psAyContext->au8AyRegister[AY_REG_D]][p_psAyContext->u8EnvelopeStep];
        p_psAyContext->u8EnvelopeStep++;
        break;
      }

      case ENVELOPE_OFF: {

        p_psAyContext->u8EnvelopeOutput = AY_MIN_AMPLITUDE;
        break;
      }

      case ENVELOPE_ON: {

        p_psAyContext->u8EnvelopeOutput = AY_MAX_AMPLITUDE;
        break;
      }

      default: {

        p_psAyContext->u8EnvelopeOutput = g_cau8EnvelopeShap[p_psAyContext->au8AyRegister[AY_REG_D]][p_psAyContext->u8EnvelopeStep];
        p_psAyContext->u8EnvelopeStep++;
        break;
      }
    }
  }
  else {

    /* Wait before update. */
    p_psAyContext->u32EnvelopeCounter--;
  }
}
#endif /* ENVELOPE_ENABLE */


#ifdef DBG_CHANNELA_ENABLE
/*==============================================================================
Function    :   vUpdateChannelA

Describe    :   Update channel A.

Parameters  :   p_psAyContext = Pointer to Ay context structure.

Returns     :   None.
==============================================================================*/
static VOID vUpdateChannelA (s_AyContext_t* p_psAyContext) {

  /* If envelope on channel A is active. */
  if (p_psAyContext->au8AyRegister[AY_REG_8] & ENVELOPE_ENABLE) {

    /* Set channel A output. */
    p_psAyContext->u8ChannelAOutput = p_psAyContext->u8EnvelopeOutput;
  }
  else {

    /* Else get amplitude from reg 8. */
    p_psAyContext->u8ChannelAOutput = p_psAyContext->au8AyRegister[AY_REG_8];
  }

  /* If noise on channel A is active. */
  if (!(p_psAyContext->au8AyRegister[AY_REG_7] & NOISE_CHANNEL_A_ENABLE)) {

    /* And output with noise. */
    p_psAyContext->u8ChannelAOutput &= p_psAyContext->u8NoiseOutput;
  }

  /* If tone on channel A is active. */
  if (!(p_psAyContext->au8AyRegister[AY_REG_7] & TONE_CHANNEL_A_ENABLE)) {

    /* And output with tone. */
    p_psAyContext->u8ChannelAOutput &= p_psAyContext->u8ToneOutputChannelA;
  }

  p_psAyContext->sExternalSignals.u16ChannelAOutput = g_cau16Amplitude[p_psAyContext->u8ChannelAOutput];
}
#endif /* CHANNELA_ENABLE */


#ifdef DBG_CHANNELB_ENABLE
/*==============================================================================
Function    :   vUpdateChannelB

Describe    :   Update channel B.

Parameters  :   p_psAyContext = Pointer to Ay context structure.

Returns     :   None.
==============================================================================*/
static VOID vUpdateChannelB (s_AyContext_t* p_psAyContext) {

  /* If envelope on channel B is active .*/
  if (p_psAyContext->au8AyRegister[AY_REG_9] & ENVELOPE_ENABLE) {

    /* Set channel B output .*/
    p_psAyContext->u8ChannelBOutput = p_psAyContext->u8EnvelopeOutput;
  }
  else {

    /* Else get amplitude from reg 9 .*/
    p_psAyContext->u8ChannelBOutput = p_psAyContext->au8AyRegister[AY_REG_9];
  }

  /* If noise on channel B is active .*/
  if (!(p_psAyContext->au8AyRegister[AY_REG_7] & NOISE_CHANNEL_B_ENABLE)) {

    /* And output with noise .*/
    p_psAyContext->u8ChannelBOutput &= p_psAyContext->u8NoiseOutput;
  }

  /* If tone on channel B is active .*/
  if (!(p_psAyContext->au8AyRegister[AY_REG_7] & TONE_CHANNEL_B_ENABLE)) {

    /* And output with tone .*/
    p_psAyContext->u8ChannelBOutput &= p_psAyContext->u8ToneOutputChannelB;
  }

  p_psAyContext->sExternalSignals.u16ChannelBOutput = g_cau16Amplitude[p_psAyContext->u8ChannelBOutput];
}
#endif /* CHANNELB_ENABLE */


#ifdef DBG_CHANNELC_ENABLE
/*==============================================================================
Function    :   vUpdateChannelC

Describe    :   Update channel C.

Parameters  :   p_psAyContext = Pointer to Ay context structure.

Returns     :   None.
==============================================================================*/
static VOID vUpdateChannelC (s_AyContext_t* p_psAyContext) {

  /* If envelope on channel C is active .*/
  if (p_psAyContext->au8AyRegister[AY_REG_A] & ENVELOPE_ENABLE) {

    /* Set channel C output .*/
    p_psAyContext->u8ChannelCOutput = p_psAyContext->u8EnvelopeOutput;
  }
  else {

    /* Else get amplitude from reg 10 .*/
    p_psAyContext->u8ChannelCOutput = p_psAyContext->au8AyRegister[AY_REG_A];
  }

  /* If noise on channel C is active .*/
  if (!(p_psAyContext->au8AyRegister[AY_REG_7] & NOISE_CHANNEL_C_ENABLE))  {

    /* And output with noise .*/
    p_psAyContext->u8ChannelCOutput &= p_psAyContext->u8NoiseOutput;
  }

  /* If tone on channel C is active .*/
  if (!(p_psAyContext->au8AyRegister[AY_REG_7] & TONE_CHANNEL_C_ENABLE)) {

    /* And output with tone .*/
    p_psAyContext->u8ChannelCOutput &= p_psAyContext->u8ToneOutputChannelC;
  }

  p_psAyContext->sExternalSignals.u16ChannelCOutput = g_cau16Amplitude[p_psAyContext->u8ChannelCOutput];
}
#endif /* CHANNELC_ENABLE */

