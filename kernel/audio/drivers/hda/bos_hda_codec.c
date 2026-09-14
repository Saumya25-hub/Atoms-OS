/*
 * ATOMS OS — Intel High Definition Audio (HDA) Driver
 * Codec Enumeration, Realtek ALC Driver & Widget Routing
 *
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Hardware Interface Constants Adapted from FreeBSD snd_hda (BSD-2-Clause)
 */

#include "bos_hda.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static void hda_codec_delay(uint32_t count) {
    for (volatile uint32_t i = 0; i < count * 20; i++) {
        __asm__ __volatile__("pause");
    }
}

/*
 * Resolve Human-Readable Codec Model Name
 */
static const char* hda_resolve_codec_name(uint32_t vendor_id, uint32_t device_id, bool* is_realtek) {
    *is_realtek = false;
    if (vendor_id == HDA_VENDOR_REALTEK) {
        *is_realtek = true;
        switch (device_id) {
            case 0x0887: return "Realtek ALC887 High Definition Audio";
            case 0x0892: return "Realtek ALC892 High Definition Audio";
            case 0x0662: return "Realtek ALC662 High Definition Audio";
            case 0x0283: return "Realtek ALC283 High Definition Audio";
            case 0x0269: return "Realtek ALC269 High Definition Audio";
            case 0x0888: return "Realtek ALC888 High Definition Audio";
            case 0x0898: return "Realtek ALC898 High Definition Audio";
            default:     return "Realtek ALC Generic Codec";
        }
    } else if (vendor_id == HDA_VENDOR_INTEL) {
        return "Intel Integrated Display Audio Codec";
    } else if (vendor_id == 0x1013) {
        return "Cirrus Logic CS420x (QEMU HDA Duplex)";
    } else if (vendor_id == HDA_VENDOR_SIGMATEL) {
        return "SigmaTel / IDT High Definition Audio";
    }
    return "Generic High Definition Audio Codec";
}

/*
 * Discover and Configure Codec Widgets (DAC & Pin Complex)
 */
bool bos_hda_codec_discover_and_configure(bos_hda_controller_t* ctrl, bos_hda_codec_t* out_codec) {
    if (!ctrl || !out_codec) return false;
    memset(out_codec, 0, sizeof(bos_hda_codec_t));

    uint8_t target_cad = 0xFF;
    uint32_t vendor_resp = 0;

    /* Scan Codec Addresses 0 through 3 */
    for (uint8_t cad = 0; cad < BOS_HDA_MAX_CODECS; cad++) {
        vendor_resp = bos_hda_send_verb(cad, 0, 0xF00, HDA_PARAM_VENDOR_ID);
        if (vendor_resp != 0 && vendor_resp != 0xFFFFFFFFU && (vendor_resp >> 16) != 0) {
            target_cad = cad;
            break;
        }
    }

    if (target_cad == 0xFF) {
        /* Fallback check on CAD 0 */
        target_cad = 0;
        vendor_resp = bos_hda_send_verb(0, 0, 0xF00, HDA_PARAM_VENDOR_ID);
        if (vendor_resp == 0 || vendor_resp == 0xFFFFFFFFU) {
            return false;
        }
    }

    out_codec->cad = target_cad;
    out_codec->vendor_id = (uint16_t)(vendor_resp >> 16);
    out_codec->device_id = (uint16_t)(vendor_resp & 0xFFFF);

    display_print("[BOS-AUDIO] Codec #");
    display_print_dec(out_codec->cad);
    display_print(": vendor ");
    display_print_hex(out_codec->vendor_id);
    display_print(" device ");
    display_print_hex(out_codec->device_id);
    display_print("\n");

    const char* codec_name = hda_resolve_codec_name(out_codec->vendor_id, out_codec->device_id, &out_codec->is_realtek);
    size_t i = 0;
    while (codec_name[i] && i < sizeof(out_codec->name) - 1) {
        out_codec->name[i] = codec_name[i];
        i++;
    }
    out_codec->name[i] = '\0';

    display_print("[BOS-AUDIO] Codec backend: ");
    display_print(out_codec->name);
    display_print("\n");

    /* Step 1: Discover Audio Function Group (AFG) */
    uint32_t sub_node_resp = bos_hda_send_verb(out_codec->cad, 0, 0xF00, HDA_PARAM_SUB_NODE_COUNT);
    uint8_t start_fct = (uint8_t)((sub_node_resp >> 16) & 0xFF);
    uint8_t num_fct = (uint8_t)(sub_node_resp & 0xFF);

    uint8_t afg_node = 1; /* Standard default AFG Node */
    for (uint8_t f = 0; f < num_fct; f++) {
        uint8_t node = start_fct + f;
        uint32_t grp_type = bos_hda_send_verb(out_codec->cad, node, 0xF00, HDA_PARAM_FCT_GROUP_TYPE);
        if ((grp_type & 0xFF) == HDA_FCT_AUDIO_GROUP) {
            afg_node = node;
            break;
        }
    }
    out_codec->afg_node = afg_node;

    /* Power up AFG to State D0 */
    bos_hda_send_verb(out_codec->cad, afg_node, HDA_VERB_SET_POWER_STATE, HDA_POWER_STATE_D0);
    hda_codec_delay(500);

    /* Step 2: Enumerate Child Widgets within AFG */
    uint32_t widget_node_resp = bos_hda_send_verb(out_codec->cad, afg_node, 0xF00, HDA_PARAM_SUB_NODE_COUNT);
    uint8_t start_widget = (uint8_t)((widget_node_resp >> 16) & 0xFF);
    uint8_t total_widgets = (uint8_t)(widget_node_resp & 0xFF);

    uint8_t primary_dac = 0;
    uint8_t primary_pin = 0;

    for (uint8_t w = 0; w < total_widgets; w++) {
        uint8_t wid = start_widget + w;
        uint32_t cap = bos_hda_send_verb(out_codec->cad, wid, 0xF00, HDA_PARAM_AUDIO_WIDGET_CAP);
        uint8_t wtype = HDA_WIDGET_TYPE(cap);

        /* Audio Output Converter (DAC) */
        if (wtype == HDA_WIDGET_AUDIO_OUTPUT) {
            if (primary_dac == 0) {
                primary_dac = wid;
            }
            /* Set DAC to Power D0 */
            bos_hda_send_verb(out_codec->cad, wid, HDA_VERB_SET_POWER_STATE, HDA_POWER_STATE_D0);
            /* Assign Stream 1, Channel 0 */
            bos_hda_send_verb(out_codec->cad, wid, HDA_VERB_SET_STREAM_CHANNEL, (BOS_HDA_DEFAULT_STREAM_ID << 4) | 0);
            /* Set Converter Format: 48kHz 16-bit Stereo (0x0011) */
            bos_hda_send_verb(out_codec->cad, wid, HDA_VERB_SET_CONV_FMT, HDA_FORMAT_48K_16BIT_STEREO);
            /* Unmute Output Converter Amplifier Gain (0 dB) */
            bos_hda_send_verb(out_codec->cad, wid, HDA_VERB_SET_AMP_GAIN_MUTE, 0xB000 | 0x00);
        }
        /* Pin Complex */
        else if (wtype == HDA_WIDGET_PIN_COMPLEX) {
            uint32_t pin_cap = bos_hda_send_verb(out_codec->cad, wid, 0xF00, HDA_PARAM_PIN_CAP);
            if (pin_cap & (HDA_PIN_CAP_OUTPUT | HDA_PIN_CAP_HEADPHONE)) {
                if (primary_pin == 0) {
                    primary_pin = wid;
                }
                /* Set Pin to Power D0 */
                bos_hda_send_verb(out_codec->cad, wid, HDA_VERB_SET_POWER_STATE, HDA_POWER_STATE_D0);
                /* Enable Pin Output & Headphone Driver (0xC0) */
                bos_hda_send_verb(out_codec->cad, wid, HDA_VERB_SET_PIN_CTRL, HDA_PIN_CTRL_OUT_ENABLE | HDA_PIN_CTRL_HP_ENABLE);
                /* Enable External Amplifier (EAPD) if supported */
                if (pin_cap & HDA_PIN_CAP_EAPD) {
                    bos_hda_send_verb(out_codec->cad, wid, HDA_VERB_SET_EAPD, 0x02);
                }
                /* Unmute Pin Amplifier Gain */
                bos_hda_send_verb(out_codec->cad, wid, HDA_VERB_SET_AMP_GAIN_MUTE, 0xB000 | 0x00);
                /* Route DAC Connection (Select first input node) */
                bos_hda_send_verb(out_codec->cad, wid, HDA_VERB_SET_CONN_SELECT, 0x00);
            }
        }
    }

    /* Fallback default nodes if enumeration returned zero */
    if (primary_dac == 0) primary_dac = 0x02;
    if (primary_pin == 0) primary_pin = (out_codec->is_realtek) ? 0x14 : 0x04;

    out_codec->dac_node = primary_dac;
    out_codec->pin_node = primary_pin;

    display_print("[BOS-AUDIO] Output path: DAC [");
    display_print_hex(out_codec->dac_node);
    display_print("] -> Pin [");
    display_print_hex(out_codec->pin_node);
    display_print("]\n");

    display_print("[BOS-AUDIO] PCM capability: 48000Hz 16-bit 2-channel Stereo\n");
    return true;
}

/*
 * Adjust Codec Hardware Master Volume (DAC & Pin Complexes)
 */
void bos_hda_codec_set_volume(bos_hda_controller_t* ctrl, uint8_t left, uint8_t right) {
    if (!ctrl || !ctrl->codec_ready) return;

    bos_hda_codec_t* codec = &ctrl->active_codec;
    /* Scale 0..100 to HDA Gain 0..0x7F (0 dB to maximum) */
    uint8_t left_gain = (uint8_t)((left * 0x7F) / 100);
    uint8_t right_gain = (uint8_t)((right * 0x7F) / 100);

    /* Set DAC Output Gain */
    if (codec->dac_node) {
        /* Left Channel */
        bos_hda_send_verb(codec->cad, codec->dac_node, HDA_VERB_SET_AMP_GAIN_MUTE, 0x9000 | (left_gain & 0x7F));
        /* Right Channel */
        bos_hda_send_verb(codec->cad, codec->dac_node, HDA_VERB_SET_AMP_GAIN_MUTE, 0x5000 | (right_gain & 0x7F));
    }

    /* Set Pin Complex Gain */
    if (codec->pin_node) {
        bos_hda_send_verb(codec->cad, codec->pin_node, HDA_VERB_SET_AMP_GAIN_MUTE, 0x9000 | (left_gain & 0x7F));
        bos_hda_send_verb(codec->cad, codec->pin_node, HDA_VERB_SET_AMP_GAIN_MUTE, 0x5000 | (right_gain & 0x7F));
    }
}
