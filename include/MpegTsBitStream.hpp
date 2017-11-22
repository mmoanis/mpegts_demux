/// /////////////////////////////////////////////////////////////////////////////
/// Defines Modules for Reading MPEG-TS Stream of data
/// ////////////////////////////////////////////////////////////////////////////
#ifndef INCLUDED_MPEGTSBITSTREAM_HPP
#define INCLUDED_MPEGTSBITSTREAM_HPP

#include <MpegTs.hpp>
#include <ByteStream_iterator.hpp>
#include <memory>
#include <cstdio>
#include <array>
#include <fstream>

/// ////////////////////////////////////////////////////////////////////////////
/// MPEG TS Packet
struct Packet {
    using value_type = uint8_t;
    //using iterator = ByteStream_Iterator;
    using iterator = uint8_t*;
    using const_iterator = const uint8_t*;

    iterator begin() { return &m_data[0]; }
    const_iterator begin() const { return &m_data[0]; }

    iterator end() { return &m_data[PACKET_SIZE-1] + 1; }
    const_iterator end() const { return &m_data[PACKET_SIZE-1] + 1; }

    uint8_t m_data[PACKET_SIZE];
};

/// ////////////////////////////////////////////////////////////////////////////
/// MPEG Bit Reader
class MpegTsBitStream {
    MpegTsBitStream(const char * filename);
    MpegTsBitStream(const MpegTsBitStream&) = delete;
    ~MpegTsBitStream();

    /// Read and fill data packet
    bool GetPacket(Packet& packet);

    bool bad() const { return m_bad; }
    bool eof() const { return m_eof; }
    bool good() const { return !m_eof && !m_bad; }

private:
    FILE* m_file;

    bool m_eof;
    bool m_bad;
};

#endif
