#include "decoder_tests.h"
#include "../common/bitstream_reader.h"
#include "../common/idct.h"
#include "../include/bospectra_decoder.h"
#include "../../packet/bospectra_packet.h"
#include "../../debug/bospectra_debug.h"
#include "../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

bospectra_error_t bospectra_decoder_test_bitstream_reader(void) {
    bospectra_log("TEST", "Running Bitstream Reader Test...");

    uint8_t buf[4] = {0xAB, 0xCD, 0xEF, 0x12};
    BitstreamReader bs;
    bitstream_init(&bs, buf, sizeof(buf));

    uint32_t b1 = bitstream_read_bits(&bs, 8); // 0xAB
    if (b1 != 0xAB) {
        bospectra_log("TEST_FAIL", "Bitstream read 8 bits failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    uint32_t b2 = bitstream_read_bits(&bs, 4); // 0xC
    if (b2 != 0xC) {
        bospectra_log("TEST_FAIL", "Bitstream read 4 bits failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_log("TEST_PASS", "Bitstream Reader Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_decoder_test_8x8_idct(void) {
    bospectra_log("TEST", "Running 8x8 IDCT Transform Reference Test...");

    int32_t in_block[64];
    uint8_t out_plane[64];
    memset(in_block, 0, sizeof(in_block));
    memset(out_plane, 0, sizeof(out_plane));

    in_block[0] = 0; // DC = 0 -> +128 level shift output should be 128

    idct_8x8(in_block, out_plane, 8, 0, 0, 8);

    if (out_plane[0] != 128 || out_plane[63] != 128) {
        bospectra_log("TEST_FAIL", "8x8 IDCT transform DC test failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_log("TEST_PASS", "8x8 IDCT Transform Reference Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_decoder_test_mjpeg(void) {
    bospectra_log("TEST", "Running MJPEG Decoder Test...");

    BOSPECTRA_StreamDescriptor stream;
    memset(&stream, 0, sizeof(stream));
    stream.id = 1;
    stream.type = BOSPECTRA_STREAM_VIDEO;
    stream.codec_id = BOSPECTRA_CODEC_MJPEG;
    stream.width = 64;
    stream.height = 64;

    bospectra_decoder_id_t dec_id = 0;
    if (BOSPECTRA_Decoder_Open(&stream, &dec_id) != BOSPECTRA_SUCCESS || dec_id == 0) {
        bospectra_log("TEST_FAIL", "Failed to open MJPEG decoder session!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    // Dummy JPEG packet (SOI + 64 bytes payload)
    uint8_t jpeg_pkt_data[68] = { 0xFF, 0xD8, 0x00, 0x00 };
    BOSPacket* pkt = bospectra_packet_alloc(sizeof(jpeg_pkt_data));
    memcpy(pkt->data, jpeg_pkt_data, sizeof(jpeg_pkt_data));

    BOSFrame* out_frame = NULL;
    bospectra_error_t err = BOSPECTRA_Decoder_DecodePacket(dec_id, pkt, &out_frame);
    bospectra_packet_free(pkt);

    if (err != BOSPECTRA_SUCCESS || !out_frame) {
        BOSPECTRA_Decoder_Close(dec_id);
        bospectra_log("TEST_FAIL", "MJPEG packet decode failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    if (out_frame->format != BOSPECTRA_PIXEL_FORMAT_YUV420P) {
        bospectra_frame_unref(out_frame);
        BOSPECTRA_Decoder_Close(dec_id);
        bospectra_log("TEST_FAIL", "Decoded output frame format mismatch!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_frame_unref(out_frame);
    BOSPECTRA_Decoder_Close(dec_id);

    bospectra_log("TEST_PASS", "MJPEG Decoder Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_decoder_test_1k_decode_stress(void) {
    bospectra_log("TEST", "Running 1,000 Frame Decoder Stress Test...");

    BOSPECTRA_StreamDescriptor stream;
    memset(&stream, 0, sizeof(stream));
    stream.id = 1;
    stream.type = BOSPECTRA_STREAM_VIDEO;
    stream.codec_id = BOSPECTRA_CODEC_MJPEG;
    stream.width = 64;
    stream.height = 64;

    bospectra_decoder_id_t dec_id = 0;
    BOSPECTRA_Decoder_Open(&stream, &dec_id);

    uint8_t jpeg_pkt_data[68] = { 0xFF, 0xD8, 0x00, 0x00 };
    BOSPacket* pkt = bospectra_packet_alloc(sizeof(jpeg_pkt_data));
    memcpy(pkt->data, jpeg_pkt_data, sizeof(jpeg_pkt_data));

    for (int i = 0; i < 1000; i++) {
        BOSFrame* frame = NULL;
        if (BOSPECTRA_Decoder_DecodePacket(dec_id, pkt, &frame) != BOSPECTRA_SUCCESS) {
            bospectra_packet_free(pkt);
            BOSPECTRA_Decoder_Close(dec_id);
            bospectra_log("TEST_FAIL", "1K Decode stress failed!");
            return BOSPECTRA_ERR_SELF_TEST_FAILED;
        }
        bospectra_frame_unref(frame);
    }

    bospectra_packet_free(pkt);
    BOSPECTRA_Decoder_Close(dec_id);

    bospectra_log("TEST_PASS", "1,000 Frame Decoder Stress Test PASSED.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_decoder_tests_run_all(void) {
    display_print("========= BOSPECTRA VIDEO DECODER ENGINE TESTS =========\n");

    bospectra_error_t res = bospectra_decoder_test_bitstream_reader();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_decoder_test_8x8_idct();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_decoder_test_mjpeg();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_decoder_test_1k_decode_stress();
    if (res != BOSPECTRA_SUCCESS) return res;

    display_print("[BOSPECTRA:DECODER_TEST] All Video Decoder Self-Tests PASSED!\n");
    display_print("=========================================================\n");

    return BOSPECTRA_SUCCESS;
}
