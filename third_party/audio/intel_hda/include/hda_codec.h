/*-
 * Copyright (c) 2006 Stephane E. Fabie
 * Copyright (c) 2008-2012 Alexander Motin <mav@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _THIRD_PARTY_HDA_CODEC_H_
#define _THIRD_PARTY_HDA_CODEC_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Standard HDA Codec Parameters (Queried via Verb 0xF00) */
#define HDA_PARAM_VENDOR_ID         0x00U
#define HDA_PARAM_REVISION_ID       0x02U
#define HDA_PARAM_SUB_NODE_COUNT    0x04U
#define HDA_PARAM_FCT_GROUP_TYPE    0x05U
#define HDA_PARAM_AUDIO_WIDGET_CAP  0x09U
#define HDA_PARAM_PCM_SIZE_RATE     0x0AU
#define HDA_PARAM_STREAM_FORMATS    0x0BU
#define HDA_PARAM_PIN_CAP           0x0CU
#define HDA_PARAM_INPUT_AMP_CAP     0x0DU
#define HDA_PARAM_CONN_LIST_LEN     0x0EU
#define HDA_PARAM_POWER_STATES      0x0FU
#define HDA_PARAM_PROCESSING_CAP    0x10U
#define HDA_PARAM_GPIO_COUNT        0x11U
#define HDA_PARAM_OUTPUT_AMP_CAP    0x12U

/* Audio Function Group Types */
#define HDA_FCT_AUDIO_GROUP         0x01U
#define HDA_FCT_MODEM_GROUP         0x02U

/* Audio Widget Types (Audio Widget Capabilities Bits 20-23) */
#define HDA_WIDGET_AUDIO_OUTPUT     0x00U /* DAC */
#define HDA_WIDGET_AUDIO_INPUT      0x01U /* ADC */
#define HDA_WIDGET_AUDIO_MIXER      0x02U
#define HDA_WIDGET_AUDIO_SELECTOR   0x03U
#define HDA_WIDGET_PIN_COMPLEX      0x04U
#define HDA_WIDGET_POWER_WIDGET     0x05U
#define HDA_WIDGET_VOLUME_KNOB      0x06U
#define HDA_WIDGET_BEEP_GEN         0x07U
#define HDA_WIDGET_VENDOR_DEFINED   0x0FU

#define HDA_WIDGET_TYPE(cap)        (((cap) >> 20) & 0x0FU)

/* Pin Capabilities (HDA_PARAM_PIN_CAP) Bits */
#define HDA_PIN_CAP_IMPEDANCE       (1U << 0)
#define HDA_PIN_CAP_TRIGGER         (1U << 1)
#define HDA_PIN_CAP_HEADPHONE       (1U << 2)
#define HDA_PIN_CAP_OUTPUT          (1U << 4)
#define HDA_PIN_CAP_INPUT           (1U << 5)
#define HDA_PIN_CAP_BALANCED        (1U << 6)
#define HDA_PIN_CAP_HDMI            (1U << 7)
#define HDA_PIN_CAP_EAPD            (1U << 16)

/* Pin Widget Control Verbs (0x707 / 0xF07) */
#define HDA_PIN_CTRL_HP_ENABLE      0x80U
#define HDA_PIN_CTRL_OUT_ENABLE     0x40U
#define HDA_PIN_CTRL_IN_ENABLE      0x20U
#define HDA_PIN_CTRL_VREF_HIZ       0x00U
#define HDA_PIN_CTRL_VREF_50        0x01U
#define HDA_PIN_CTRL_VREF_GROUND    0x02U
#define HDA_PIN_CTRL_VREF_80        0x04U
#define HDA_PIN_CTRL_VREF_100       0x05U

/* Power States (Verb 0x705 / 0xF05) */
#define HDA_POWER_STATE_D0          0x00U /* Full On */
#define HDA_POWER_STATE_D1          0x01U
#define HDA_POWER_STATE_D2          0x02U
#define HDA_POWER_STATE_D3          0x03U /* Powered Off */

/* Amplifier Gain / Mute Control (Verb 0x300) */
#define HDA_AMP_SET_OUTPUT          (1U << 15)
#define HDA_AMP_SET_INPUT           (1U << 14)
#define HDA_AMP_SET_LEFT            (1U << 13)
#define HDA_AMP_SET_RIGHT           (1U << 12)
#define HDA_AMP_MUTE                (1U << 7)
#define HDA_AMP_GAIN_MASK           0x7FU

/* Standard Codec Verbs (12-bit payload commands) */
#define HDA_VERB_GET_CONV_FMT       0xAU
#define HDA_VERB_SET_CONV_FMT       0x2U
#define HDA_VERB_SET_AMP_GAIN_MUTE  0x3U
#define HDA_VERB_SET_CONN_SELECT    0x701U
#define HDA_VERB_GET_CONN_SELECT    0xF01U
#define HDA_VERB_GET_CONN_LIST      0xF02U
#define HDA_VERB_SET_PIN_CTRL       0x707U
#define HDA_VERB_GET_PIN_CTRL       0xF07U
#define HDA_VERB_SET_STREAM_CHANNEL 0x706U
#define HDA_VERB_GET_STREAM_CHANNEL 0xF06U
#define HDA_VERB_SET_POWER_STATE    0x705U
#define HDA_VERB_GET_POWER_STATE    0xF05U
#define HDA_VERB_GET_EAPD           0xF0CU
#define HDA_VERB_SET_EAPD           0x70CU

/* Well-Known Codec Vendor IDs */
#define HDA_VENDOR_REALTEK          0x10ECU
#define HDA_VENDOR_ANALOG_DEVICES   0x11D4U
#define HDA_VENDOR_CONEXANT         0x14F1U
#define HDA_VENDOR_INTEL            0x8086U
#define HDA_VENDOR_SIGMATEL         0x8384U

/* Common Realtek ALC Codec Device IDs */
#define HDA_CODEC_ALC887            0x10EC0887U
#define HDA_CODEC_ALC892            0x10EC0892U
#define HDA_CODEC_ALC662            0x10EC0662U
#define HDA_CODEC_ALC283            0x10EC0283U
#define HDA_CODEC_ALC269            0x10EC0269U
#define HDA_CODEC_ALC888            0x10EC0888U
#define HDA_CODEC_ALC898            0x10EC0898U

#ifdef __cplusplus
}
#endif

#endif /* _THIRD_PARTY_HDA_CODEC_H_ */
