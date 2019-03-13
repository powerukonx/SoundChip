/* =============================================================================

                  ██╗   ██╗██╗  ██╗ ██████╗ ███╗   ██╗██╗  ██╗
                  ██║   ██║██║ ██╔╝██╔═══██╗████╗  ██║╚██╗██╔╝
                  ██║   ██║█████╔╝ ██║   ██║██╔██╗ ██║ ╚███╔╝
                  ██║   ██║██╔═██╗ ██║   ██║██║╚██╗██║ ██╔██╗
                  ╚██████╔╝██║  ██╗╚██████╔╝██║ ╚████║██╔╝ ██╗
                   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝

  File name:    generator_ay.h
  Date:         23 07 2017
  Author:       Power.
  Description:  AY-3-8912 Sound generator - Header file.

============================================================================= */
#ifndef AYENGINE_H_INCLUDED
  #define AYENGINE_H_INCLUDED


  /* ===========================================================================
                                   DEBUG Section
  =========================================================================== */


  /* ===========================================================================
                            Public defines and typedefs
  =========================================================================== */
  #define AY_FALSE                  ((UINT8)0)
  #define AY_TRUE                   ((UINT8)1)

  typedef enum _AYERRORS_ {
    AY_ERROR_NO = 0u,
    AY_ERROR_PARAM
  } e_AyErrors_t;

  typedef enum _AYSIGNALVALUE_ {
    AY_LOW = 0u,
    AY_HIGH
  } e_AySignalValue_t;

  typedef enum _AYCHANNEL_ {
    AYCHANNEL_A = 0u,
    AYCHANNEL_B,
    AYCHANNEL_C,
    AYCHANNEL_MAX
  } e_AyChannel_t;

  typedef enum _AYREGS_ {
    AY_REG_0 = 0u,
    AY_REG_1,
    AY_REG_2,
    AY_REG_3,
    AY_REG_4,
    AY_REG_5,
    AY_REG_6,
    AY_REG_7,
    AY_REG_8,
    AY_REG_9,
    AY_REG_A,
    AY_REG_B,
    AY_REG_C,
    AY_REG_D,
    AY_REG_E,
    AY_REG_F,
    AY_REG_MAX
  } e_AyRegs_t;

  typedef struct _AYEXTSIG_ {         /* All AY-3-8912 external signals. */
    e_AySignalValue_t eRESETn_In;     /* /RESET */
    e_AySignalValue_t eCLOCK_In;      /* CLK */
    e_AySignalValue_t eTEST1_In;      /* TEST1 */
    UINT16 u16ChannelAOutput;       /* CHANNEL OUTPUT A */
    UINT16 u16ChannelBOutput;       /* CHANNEL OUTPUT B */
    UINT16 u16ChannelCOutput;       /* CHANNEL OUTPUT B */
    e_AySignalValue_t eBC1_In;        /* BUS CONTROL 1 */
    e_AySignalValue_t eBC2_In;        /* BUS CONTROL 2 */
    e_AySignalValue_t eBDIR_In;       /* BUS DIRECTION */
    e_AySignalValue_t eA8_In;         /* EXTRA ADDRESS BIT */
    UINT8 u8IOA;                    /* INPUT/OUTPUT PORT A*/
    UINT8 u8DataAddress;            /* DATA ADDRESS BUS */
    float fVcc_In;                    /* VCC */
    float fGnd_In;                    /* GND*/
  } s_AyExtSig_t;

  typedef struct _AYCONTEXT_ {
    s_AyExtSig_t sExternalSignals;     /* All AY-3-8912 external signals. */
    UINT16 u16ToneCounterChannelA;
    UINT8 u8ToneOutputChannelA;
    UINT16 u16ToneCounterChannelB;
    UINT8 u8ToneOutputChannelB;
    UINT16 u16ToneCounterChannelC;
    UINT8 u8ToneOutputChannelC;
    UINT32 u32NoiseCounter;
    UINT8 u8NoiseOutput;
    UINT32 u32NoiseShiftRegister;
    UINT32 u32EnvelopeCounter;
    UINT8 u8EnvelopeStep;
    UINT8 u8EnvelopeOutput;
    UINT8 u8AyCurrentRegister;
    UINT8 au8AyRegister[AY_REG_MAX];
    UINT8 u8ChannelAOutput;
    UINT8 u8ChannelBOutput;
    UINT8 u8ChannelCOutput;
    UINT8 u8ClockShiftRegister;
  } s_AyContext_t;


  /* ===========================================================================
                          Public constants and variables
  =========================================================================== */


  /* ===========================================================================
                          Public function declarations
  =========================================================================== */
  extern e_AyErrors_t eAYEngineUpdate (s_AyContext_t* p_psAyContext);
  extern VOID vAY_SetChannelASidState (UINT8 p_u8Activate);
  extern VOID vAY_SetChannelBSidState (UINT8 p_u8Activate);
  extern VOID vAY_SetChannelCSidState (UINT8 p_u8Activate);

#endif  /* AYENGINE_H_INCLUDED */

