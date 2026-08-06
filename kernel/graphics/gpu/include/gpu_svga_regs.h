#ifndef BOS_GPU_SVGA_REGS_H
#define BOS_GPU_SVGA_REGS_H

#include <stdint.h>

/* VMware PCI Identifiers */
#define SVGA_PCI_VENDOR_ID          0x15AD
#define SVGA_PCI_DEVICE_ID_II       0x0405
#define SVGA_PCI_DEVICE_ID_EX       0x0406

/* VMware SVGA I/O Port Offsets relative to BAR0 Base */
#define SVGA_INDEX_PORT             0x00
#define SVGA_VALUE_PORT             0x01
#define SVGA_BIOS_PORT              0x02
#define SVGA_IRQSTATUS_PORT         0x08

/* VMware Magic ID Versions */
#define SVGA_MAGIC_NUMBER           0x90000000U
#define SVGA_ID_0                   (SVGA_MAGIC_NUMBER | 0)
#define SVGA_ID_1                   (SVGA_MAGIC_NUMBER | 1)
#define SVGA_ID_2                   (SVGA_MAGIC_NUMBER | 2)

/* VMware SVGA Registers */
#define SVGA_REG_ID                 0
#define SVGA_REG_ENABLE             1
#define SVGA_REG_WIDTH              2
#define SVGA_REG_HEIGHT             3
#define SVGA_REG_MAX_WIDTH          4
#define SVGA_REG_MAX_HEIGHT         5
#define SVGA_REG_DEPTH              6
#define SVGA_REG_BITS_PER_PIXEL     7
#define SVGA_REG_PSEUDOCOLOR        8
#define SVGA_REG_RED_MASK           9
#define SVGA_REG_GREEN_MASK         10
#define SVGA_REG_BLUE_MASK          11
#define SVGA_REG_BYTES_PER_LINE     12
#define SVGA_REG_FB_START           13
#define SVGA_REG_FB_OFFSET          14
#define SVGA_REG_FB_SIZE            15
#define SVGA_REG_FB_MAX_FLUSH_SIZE  16
#define SVGA_REG_FIFO_START         17
#define SVGA_REG_FIFO_SIZE          18
#define SVGA_REG_CONFIG_DONE        19
#define SVGA_REG_SYNC               20
#define SVGA_REG_BUSY               21
#define SVGA_REG_GUEST_ID           22
#define SVGA_REG_CURSOR_ID          23
#define SVGA_REG_CURSOR_X           24
#define SVGA_REG_CURSOR_Y           25
#define SVGA_REG_CURSOR_ON          26
#define SVGA_REG_HOST_BITS_PER_PIXEL 27
#define SVGA_REG_SCRATCH_SIZE       28
#define SVGA_REG_MEM_REGS           29
#define SVGA_REG_NUM_DISPLAYS       30
#define SVGA_REG_PITCHLOCK          31

/* FIFO Header Register Offsets (in uint32_t indices at FIFO Base) */
#define SVGA_FIFO_MIN               0   /* Min offset of command area */
#define SVGA_FIFO_MAX               1   /* Max offset / size of FIFO */
#define SVGA_FIFO_NEXT_CMD          2   /* Next command write offset */
#define SVGA_FIFO_STOP              3   /* Command execute stop offset */
#define SVGA_FIFO_CAPABILITIES      4   /* FIFO Capabilities bitfield */
#define SVGA_FIFO_FLAGS             5   /* FIFO flags */

/* FIFO Capabilities Bitmask */
#define SVGA_FIFO_CAP_NONE          0x00000000
#define SVGA_FIFO_CAP_RECT_COPY     0x00000002
#define SVGA_FIFO_CAP_RECT_FILL     0x00000008
#define SVGA_FIFO_CAP_PITCHLOCK     0x00000010
#define SVGA_FIFO_CAP_TRANS_BLT     0x00000020
#define SVGA_FIFO_CAP_3D            0x00000040
#define SVGA_FIFO_CAP_EXTENDED_FIFO 0x00000080

/* SVGA 2D Command Packet IDs */
#define SVGA_CMD_INVALID            0
#define SVGA_CMD_UPDATE             1
#define SVGA_CMD_RECT_FILL          2
#define SVGA_CMD_RECT_COPY          3
#define SVGA_CMD_DEFINE_BITMAP      4
#define SVGA_CMD_DEFINE_BITMAP_SCANLINE 5
#define SVGA_CMD_DEFINE_PIXMAP      6
#define SVGA_CMD_DEFINE_PIXMAP_SCANLINE 7
#define SVGA_CMD_RECT_BITMAP_FILL   8
#define SVGA_CMD_RECT_PIXMAP_FILL   9
#define SVGA_CMD_RECT_BITMAP_COPY   10
#define SVGA_CMD_RECT_PIXMAP_COPY   11
#define SVGA_CMD_FREE_PIXMAP        12
#define SVGA_CMD_RECT_ROP_FILL      13
#define SVGA_CMD_RECT_ROP_COPY      14
#define SVGA_CMD_DEFINE_CURSOR      19
#define SVGA_CMD_DISPLAY_CURSOR     20
#define SVGA_CMD_MOVE_CURSOR        21

#endif /* BOS_GPU_SVGA_REGS_H */
