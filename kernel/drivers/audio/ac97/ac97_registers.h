#ifndef AC97_REGISTERS_H
#define AC97_REGISTERS_H

// AC97 NAM (Native Audio Mixer) Registers
#define AC97_REG_RESET               0x00
#define AC97_REG_MASTER_VOLUME       0x02
#define AC97_REG_HEADPHONE_VOLUME    0x04
#define AC97_REG_MASTER_VOLUME_MONO  0x06
#define AC97_REG_PC_BEEP_VOLUME      0x0A
#define AC97_REG_PHONE_VOLUME        0x0C
#define AC97_REG_MIC_VOLUME          0x0E
#define AC97_REG_LINE_IN_VOLUME      0x10
#define AC97_REG_CD_VOLUME           0x12
#define AC97_REG_VIDEO_VOLUME        0x14
#define AC97_REG_AUX_IN_VOLUME       0x16
#define AC97_REG_PCM_OUT_VOLUME      0x18
#define AC97_REG_RECORD_SELECT       0x1A
#define AC97_REG_RECORD_GAIN         0x1C
#define AC97_REG_GENERAL_PURPOSE     0x20
#define AC97_REG_3D_CONTROL          0x22
#define AC97_REG_POWER_CONTROL       0x26
#define AC97_REG_EXT_AUDIO_ID        0x28
#define AC97_REG_EXT_AUDIO_STAT      0x2A
#define AC97_REG_PCM_FRONT_DAC_RATE  0x2C
#define AC97_REG_PCM_ADC_RATE        0x32

// AC97 NABM (Native Audio Bus Master) Registers (For later DMA use)
#define AC97_NABM_PI_BDBAR           0x00
#define AC97_NABM_PI_CIV             0x04
#define AC97_NABM_PI_LVI             0x05
#define AC97_NABM_PI_SR              0x06
#define AC97_NABM_PI_PICB            0x08
#define AC97_NABM_PI_PIV             0x0A
#define AC97_NABM_PI_CR              0x0B

#define AC97_NABM_PO_BDBAR           0x10
#define AC97_NABM_PO_CIV             0x14
#define AC97_NABM_PO_LVI             0x15
#define AC97_NABM_PO_SR              0x16
#define AC97_NABM_PO_PICB            0x18
#define AC97_NABM_PO_PIV             0x1A
#define AC97_NABM_PO_CR              0x1B

#define AC97_NABM_GLOB_CNT           0x2C
#define AC97_NABM_GLOB_STA           0x30
#define AC97_NABM_CAS                0x34

#endif // AC97_REGISTERS_H
