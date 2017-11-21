#ifndef INCLUDED_MPEGTS_HPP
#define INCLUDED_MPEGTS_HPP
#include <iostream>
////////////////////////////////////////////////
// Transport Stream Structure
// A packet is the basic unit of data. Transport stream is merely
// a sequence of packets, without any global header.
// Each packet starts with a sync byte and a header, the rest of
// the packet consists of payload.
// Packets are 188 bytes in length, but communication medium may
// add additional information.

/// Partial Transport Stream Packet Format
struct PTS
{
    bool m_tei; /// Transport Error Indicator
    bool m_pusi; /// Payload Unit Start Indicator
    bool m_tp; /// Transport Priority
    uint32_t m_pid; /// Packet Identifier
    enum {
        TRANSPORT_NOT_SCRAMBLED = 0,
        TRANSPORT_RESERVED = 1,
        TRANSPORT_SCRAMBLED_EVEN,
        TRANSPORT_SCRAMBLED_ODD
    } m_tsc; /// Transport Scrambling Control

    enum {
        ADAPTION_RESERVED = 0,
        ADAPTION_NO_ADAPTION_FIELD = 1,
        ADAPTION_FIELD_ONLY,
        ADAPTION_FIELD_PAYLOAD
    } m_afc; /// Adaption Field Control

    uint32_t m_cc; /// Continuity Counter

    friend std::ostream& operator<<(std::ostream &out, const PTS& pts);
};

struct PAT {
    uint16_t pid; /// Program Number
    uint16_t pmid; /// Program Map PID

    friend std::ostream& operator<<(std::ostream &out, const PAT& pat);
};
inline std::ostream& operator<<(std::ostream &out, const PAT& pat)
{
    out << "**** PAT DUMP ****" << std::endl;
    out << "Program Number: " << pat.pid << std::endl;
    out << "Program Map PID: " << std::dec << pat.pmid << std::endl;
    out << "**** DUMP END ****" << std::endl;
    return out;
}

inline std::ostream& operator<<(std::ostream &out, const PTS& pts)
{
    out << "**** PTS Dump ****" << std::endl;
    out << "Transport Error Indicator: " << pts.m_tei << std::endl;
    out << "Payload Unit Start Indicator: " << pts.m_pusi << std::endl;
    out << "Transport Priority: " << pts.m_tp << std::endl;
    out << "PID: " << pts.m_pid << std::endl;
    out << "Transport Scrambling Control: " << pts.m_tsc << std::endl;
    out << "Adaption Field Control: " << pts.m_afc << std::endl;
    out << "Continuity Counter: " << std::hex << pts.m_cc << std::endl;
    out << "**** DUMP END ****" << std::endl;
    return out;
}


#endif // MPEGTS_HPP_INCLUDED
