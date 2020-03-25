/* =============================================================================

                  ██╗   ██╗██╗  ██╗ ██████╗ ███╗   ██╗██╗  ██╗
                  ██║   ██║██║ ██╔╝██╔═══██╗████╗  ██║╚██╗██╔╝
                  ██║   ██║█████╔╝ ██║   ██║██╔██╗ ██║ ╚███╔╝
                  ██║   ██║██╔═██╗ ██║   ██║██║╚██╗██║ ██╔██╗
                  ╚██████╔╝██║  ██╗╚██████╔╝██║ ╚████║██╔╝ ██╗
                   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝

  File name:    parser_vgm.h
  Date:         23 07 2017
  Author:       Power.
  Description:  VGM file parser - Header file.

============================================================================= */
#ifndef VGMPLAYER_H_INCLUDED
  #define VGMPLAYER_H_INCLUDED

  /* ===========================================================================
                                   DEBUG Section
  =========================================================================== */

  /* ===========================================================================
                            Public defines and typedefs
  =========================================================================== */

  typedef struct _VGMFILEHEADER_ { /* 256 bytes. */
    char      achID[4];           // file identification "Vgm "
    uint32_t  u32EoFoffset;       // Relative offset to end of file (i.e. file length - 4).
    uint32_t  u32Version;         // Version number in BCD-Code. e.g. Version 1.70 is stored as 0x00000171
    uint32_t  u32SN76489clock;    // Input clock rate in Hz for the SN76489 PSG chip. A typical value is 3579545
                                  // Bit 31 (0x80000000) is used on combination with the dual-chip-bit to indicate that this is a T6W28.
    uint32_t  u32YM2413clock;     // Input clock rate in Hz for the YM2413 chip
    uint32_t  u32GD3offset;       // Relative offset to GD3 tag
    uint32_t  u32Totalsamples;    // Total of all wait values in the file
    uint32_t  u32Loopoffset;      // Relative offset to loop point
    uint32_t  u32Loopsamples;     // Number of samples in one loop
    uint32_t  u32Rate;            // "Rate" of recording in Hz, used for rate scaling on playback
    uint16_t  u16SNFB;            // The white noise feedback pattern for the SN76489 PSG
    uint8_t   u8SNW;              // The noise feedback shift register width, in bits
    uint8_t   u8SF;               // Misc flags for the SN76489
    uint32_t  u32YM2612clock;     // Input clock rate in Hz for the YM2612 chip. A typical value is 7670454.
    uint32_t  u32YM2151clock;     // Input clock rate in Hz for the YM2151 chip. A typical value is 3579545
    uint32_t  u32VGMdataoffset;   // Relative offset to VGM data stream
    uint32_t  u32SegaPCMclock;    // Input clock rate in Hz for the Sega PCM chip. A typical value is 4000000
    uint32_t  u32SPCMinterface;   // The interface register for the Sega PCM chip
    uint32_t  u32RF5C68clock;     // Input clock rate in Hz for the RF5C68 PCM chip. A typical value is 12500000
    uint32_t  u32YM2203clock;     // Input clock rate in Hz for the YM2203 chip. A typical value is 3000000
    uint32_t  u32YM2608clock;     // Input clock rate in Hz for the YM2608 chip. A typical value is 8000000
    uint32_t  u32YM2610Bclock;    // Input clock rate in Hz for the YM2610/B chip. A typical value is 8000000
    uint32_t  u32YM3812clock;     // Input clock rate in Hz for the YM3812 chip. A typical value is 3579545
    uint32_t  u32YM3526clock;     // Input clock rate in Hz for the YM3526 chip. A typical value is 3579545
    uint32_t  u32Y8950clock;      // Input clock rate in Hz for the Y8950 chip. A typical value is 3579545
    uint32_t  u32YMF262clock;     // Input clock rate in Hz for the YMF262 chip. A typical value is 14318180
    uint32_t  u32YMF278Bclock;    // Input clock rate in Hz for the YMF278B chip. A typical value is 33868800
    uint32_t  u32YMF271clock;     // Input clock rate in Hz for the YMF271 chip. A typical value is 16934400
    uint32_t  u32YMZ280Bclock;    // Input clock rate in Hz for the YMZ280B chip. A typical value is 16934400
    uint32_t  u32RF5C164clock;    // Input clock rate in Hz for the RF5C164 PCM chip. A typical value is 12500000
    uint32_t  u32PWMclock;        // Input clock rate in Hz for the PWM chip. A typical value is 23011361
    uint32_t  u32AY8910clock;     // Input clock rate in Hz for the AY8910 chip. A typical value is 1789750
    uint8_t   u8AYT;              // Defines the exact type of AY8910
    uint8_t   au8AYFlags[3];      // Misc flags for the AY8910
    uint8_t   u8VM;               // Volume = 2 ^ (VolumeModifier / 0x20) where VolumeModifier is a number from -63 to 192 (-63 = 0xC1, 0 = 0x00, 192 = 0xC0)
    uint8_t   u8Reserved7D;       //
    uint8_t   u8LB;               // Modifies the number of loops that are played before the playback ends
    uint8_t   u8LM;               // Modifies the number of loops that are played before the playback ends
    uint32_t  u32GBDMGclock;      // Input clock rate in Hz for the GameBoy DMG chip, LR35902. A typical value is 4194304
    uint32_t  u32NESAPUGclock;    // Input clock rate in Hz for the NES APU chip, N2A03. A typical value is 1789772.
    uint32_t  u32MultiPCMclock;   // Input clock rate in Hz for the MultiPCM chip. A typical value is 8053975
    uint32_t  u32uPD7759clock;    // Input clock rate in Hz for the uPD7759 chip. A typical value is 640000
    uint32_t  u32OKIM6258clock;   // Input clock rate in Hz for the OKIM6258 chip. A typical value is 4000000
    uint8_t   u8OF;               // Misc flags for the OKIM6258
    uint8_t   u8KF;               // Misc flags for the K054539
    uint8_t   u8CF;               // Defines the exact type of C140 and its banking method
    uint8_t   u8Reserved97;       //
    uint32_t  u32OKIM6295clock;   // Input clock rate in Hz for the OKIM6295 chip. A typical value is 8000000
    uint32_t  u32K051649clock;    // Input clock rate in Hz for the K051649 chip. A typical value is 1500000
    uint32_t  u32K054539clock;    // Input clock rate in Hz for the K054539 chip. A typical value is 18432000
    uint32_t  u32HuC6280clock;    // Input clock rate in Hz for the HuC6280 chip. A typical value is 3579545
    uint32_t  u32C140clock;       // Input clock rate in Hz for the C140 chip. A typical value is 21390
    uint32_t  u32K053260clock;    // Input clock rate in Hz for the K053260 chip. A typical value is 3579545
    uint32_t  u32Pokeyclock;      // Input clock rate in Hz for the Pokey chip. A typical value is 1789772
    uint32_t  u32QSoundclock;     // Input clock rate in Hz for the QSound chip. A typical value is 4000000
    uint32_t  u32SCSPclock;       // Input clock rate in Hz for the SCSP chip. A typical value is 22579200
    uint32_t  u32ExtraHdrofsclock;// Relative offset to the extra header or 0 if no extra header is present
    uint32_t  u32WonderSwanclock; // Input clock rate in Hz for the WonderSwan chip. A typical value is 3072000
    uint32_t  u32VSUclock;        // Input clock rate in Hz for the VSU chip. A typical value is 5000000
    uint32_t  u32SAA1099clock;    // Input clock rate in Hz for the SAA1099 chip. A typical value is 8000000 (or 7159000/7159090)
    uint32_t  u32ES5503clock;     // Input clock rate in Hz for the ES5503 chip. A typical value is 7159090
    uint32_t  u32ES5506clock;     // Input clock rate in Hz for the ES5505/ES5506 chip
    uint16_t  u16ESchns;          // Defines the internal number of output channels for the ES5503
    uint8_t   u8CD;               // Defines the clock divider for the C352 chip, divided by 4 in order to achieve a divider range of 0 to 1020. A typical value is 288
    uint8_t   u8ReservedD7;       //
    uint32_t  u32X1010clock;      // Input clock rate in Hz for the X1-010 chip. A typical value is 16000000
    uint32_t  u32C352clock;       // Input clock rate in Hz for the C352 chip. A typical value is 24192000
    uint32_t  u32GA20clock;       // Input clock rate in Hz for the GA20 chip. A typical value is 3579545
    uint8_t   au8ReservedE4[28];  //
  } s_VgmFileHeader_t;

  typedef struct _GD3TAG_ {
    char achID[4];             // file identification "Gd3 "
    uint32_t u32Version;         // Version number in BCD-Code. e.g. Version 1.70 is stored as 0x00000171
    uint32_t u32Length;          // 32-bit length of the following data in bytes
    char *achContent;        // series of null-terminated strings
  } s_Gd3Tag_t;

  class Vgm {

    public:
      // methods
      Vgm ();
      ~Vgm ();
      bool bInit (void);
      bool bLoad (char p_achFilename[]);
      bool bUpdate (void* p_pvParameters);

    private:
      // Attributs
      s_VgmFileHeader_t sFileHeader;
      s_Gd3Tag_t sGd3Tag;
      uint8_t *pau8Song;
      uint32_t u32FileLen;
      uint32_t u32Index;
      double dPlayerCounter;
      double dPlayerTrigger;
      Sn76489 *soundchip;

      FILE *pfDbg;
  };




#endif  /* VGMPLAYER_H_INCLUDED */

