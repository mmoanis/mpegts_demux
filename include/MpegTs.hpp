/// ////////////////////////////////////////////////////////////////////////////
/// Defines Transport Stream Structures.
/// /////////////////////////////////////////////////////////////////////////////
/// Transport Stream Structure
/// A packet is the basic unit of data. Transport stream is merely
/// a sequence of packets, without any global header.
/// Each packet starts with a sync byte and a header, the rest of
/// the packet consists of payload.
/// Packets are 188 bytes in length, but communication medium may
/// add additional information.
///
/// Reference: https://en.wikipedia.org/wiki/MPEG_transport_stream
///
///
///
#ifndef INCLUDED_MPEGTS_HPP
#define INCLUDED_MPEGTS_HPP
#include <cstdint>

/// Default Packet Size (I guess there are more variants)
#define PACKET_SIZE 188

/// Sync Byte
#define MPEGTS_SYNC_BYTE 0x47

/// Stuffing Byte
#define MPEGTS_STUFFING_BYTE 0xFF

/// Packet Identifier (Not Complete)
#define MPEGTS_PID_PAT 0x0000
#define MPEGTS_PID_CAT 0x0001
#define MPEGTS_PID_TSDT 0x0002
#define MPEGTS_PID_IPMP 0x0003

struct Program {
    uint16_t pid; /// Program ID
    uint8_t cc;   /// Number of last packet
    bool psi;     /// Is it a control program "PAT/PMT/CAT/...", else PES

    Program() : pid(0x1FFF), cc(0), psi(false) {
        // default to NULL packet
    }
};

#endif
