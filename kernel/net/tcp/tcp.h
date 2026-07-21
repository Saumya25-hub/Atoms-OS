#ifndef SIGNATURES_TCP_H
#define SIGNATURES_TCP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TCP_MIN_HLEN        20
#define MAX_TCP_CONNECTIONS 16
#define TCP_RX_STREAM_SIZE  8192

#define TCP_FLAG_FIN        0x01
#define TCP_FLAG_SYN        0x02
#define TCP_FLAG_RST        0x04
#define TCP_FLAG_PSH        0x08
#define TCP_FLAG_ACK        0x10
#define TCP_FLAG_URG        0x20

typedef enum {
    TCP_STATE_CLOSED = 0,
    TCP_STATE_LISTEN,
    TCP_STATE_SYN_SENT,
    TCP_STATE_SYN_RECEIVED,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN_WAIT_1,
    TCP_STATE_FIN_WAIT_2,
    TCP_STATE_CLOSE_WAIT,
    TCP_STATE_CLOSING,
    TCP_STATE_LAST_ACK,
    TCP_STATE_TIME_WAIT
} TcpState;

// 20-byte packed standard TCP Header (RFC 793)
struct tcp_hdr {
    uint16_t src_port;             // Big Endian
    uint16_t dest_port;            // Big Endian
    uint32_t seq_num;              // Big Endian
    uint32_t ack_num;              // Big Endian
    uint8_t  data_offset_reserved; // Data offset (high 4 bits) + Reserved (low 4 bits)
    uint8_t  flags;                // Control Flags
    uint16_t window;               // Window Size (Big Endian)
    uint16_t checksum;             // Checksum (Big Endian)
    uint16_t urgent_ptr;           // Urgent Pointer (Big Endian)
} __attribute__((packed));

struct tcp_pseudo_hdr {
    uint32_t src_ip;      // Big Endian
    uint32_t dest_ip;     // Big Endian
    uint8_t  zero;        // 0
    uint8_t  protocol;    // 6 (IP_PROTO_TCP)
    uint16_t tcp_len;     // Big Endian (Header + Options + Payload)
} __attribute__((packed));

typedef struct {
    uint32_t local_ip;        // Big Endian
    uint32_t remote_ip;       // Big Endian
    uint16_t local_port;      // Host Byte Order
    uint16_t remote_port;     // Host Byte Order

    TcpState state;

    uint32_t isn;             // Initial Send Sequence Number (Host Byte Order)
    uint32_t snd_una;         // Send Unacknowledged (Host Byte Order)
    uint32_t snd_nxt;         // Send Next (Host Byte Order)
    uint32_t rcv_nxt;         // Receive Next (Host Byte Order)

    uint16_t snd_wnd;         // Remote Advertised Window (Host Byte Order)
    uint16_t rcv_wnd;         // Local Receive Window (Host Byte Order)

    bool     syn_ack_received;
    bool     rst_received;
    bool     fin_received;
    bool     fin_sent;

    uint32_t rx_seq;          // Last received sequence number
    uint32_t rx_ack;          // Last received ack number
    uint8_t  rx_flags;        // Last received TCP flags
    uint32_t syn_retries;

    // Stream Rx Ring Buffer
    uint8_t  rx_stream[TCP_RX_STREAM_SIZE];
    size_t   rx_head;
    size_t   rx_tail;
    size_t   rx_count;

    bool     in_use;
} TcpConnection;

void tcp_init(void);
uint16_t tcp_calc_checksum(uint32_t src_ip, uint32_t dest_ip, const void* tcp_data, uint16_t tcp_len);

bool tcp_connect(uint32_t remote_ip, uint16_t remote_port, TcpConnection** conn_out);
int  tcp_send(TcpConnection* conn, const void* data, size_t length);
bool tcp_send_segment_ex(TcpConnection* conn, uint8_t flags, const void* payload, uint16_t payload_len);

size_t tcp_available(TcpConnection* conn);
int    tcp_recv(TcpConnection* conn, void* buffer, size_t max_len);

void tcp_process_packet(uint32_t src_ip, uint32_t dest_ip, const uint8_t* payload, uint16_t length);
const TcpConnection* tcp_get_last_connection(void);

#endif // SIGNATURES_TCP_H
