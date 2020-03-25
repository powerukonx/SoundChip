/* =============================================================================

                  ██╗   ██╗██╗  ██╗ ██████╗ ███╗   ██╗██╗  ██╗
                  ██║   ██║██║ ██╔╝██╔═══██╗████╗  ██║╚██╗██╔╝
                  ██║   ██║█████╔╝ ██║   ██║██╔██╗ ██║ ╚███╔╝
                  ██║   ██║██╔═██╗ ██║   ██║██║╚██╗██║ ██╔██╗
                  ╚██████╔╝██║  ██╗╚██████╔╝██║ ╚████║██╔╝ ██╗
                   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝

  File name:    emu_sn74489.h
  Date:         23 07 2017
  Author:       Power.
  Description:  SN76486 Digital Complex Sound generator - Header file.

============================================================================= */
#ifndef EMU_SN76489_H_INCLUDED
  #define EMU_SN76489_H_INCLUDED

  /* ===========================================================================
                                   DEBUG Section
  =========================================================================== */

  /* ===========================================================================
                            Public defines and typedefs
  =========================================================================== */
  #define VOLUME_NB                   ((uint8_t)16)

  typedef enum _SN76489SIGNALVALUE_ {
    SN76489_LOW = (uint8_t)0u,
    SN76489_HIGH
  } e_Sn76489SignalValue_t;

  typedef struct _SN76489EXTSIG_ { // All SN76489 external signals.
    e_Sn76489SignalValue_t eCLOCK_In;   // Output clock
    e_Sn76489SignalValue_t eREADY_In;   // Ready
    e_Sn76489SignalValue_t eWEn_In;     // Write Enable
    e_Sn76489SignalValue_t eCEn_In;     // Chip Enable
    UINT16 u16AUDIOOUT_Out;             // Audio output
    UINT8 u8DataAddress;                // DATA ADDRESS BUS
    float fVcc_In;                      // VCC
    float fGnd_In;                      // GND
  } s_Sn76489ExtSig_t;

  typedef enum _SN76489REGS_ {
    SN76489_REG_TONE1_FREQ = (uint8_t)0u,
    SN76489_REG_TONE1_VOL,
    SN76489_REG_TONE2_FREQ,
    SN76489_REG_TONE2_VOL,
    SN76489_REG_TONE3_FREQ,
    SN76489_REG_TONE3_VOL,
    SN76489_REG_NOISE_CTRL,
    SN76489_REG_NOISE_VOL,
    SN76489_REG_MAX
  } e_Sn76489Regs_t;

  /* ===========================================================================
                            Class
  =========================================================================== */
  class Sn76489 {

    public:
      Sn76489 ();
      ~Sn76489 ();
      uint16_t u16UpdateIOs (s_Sn76489ExtSig_t *p_psExtSig);

    private:
      // methods
      void vUpdateCounters (void);
      void vUpdateNoiseGenerator (void);
      void vUpdateChannels (void);
      int iParity (int val);

      // attributs
      uint16_t  u16Counter[SN76489_REG_MAX>>1u];
      uint8_t   u8Clock;
      uint8_t   u8CurrentChannel;
      uint8_t   u8ToggleOutput[SN76489_REG_MAX>>1u];
      uint16_t  u16AudioOutput;
      uint16_t u16LFSR;
      uint16_t au16Regs[SN76489_REG_MAX];
      FILE *pfDbg = NULL;
      s_Sn76489ExtSig_t sExternalSignals;
      static const uint16_t cau16Amplitude[VOLUME_NB];
  };

#endif  /* EMU_SN76489_H_INCLUDED */

