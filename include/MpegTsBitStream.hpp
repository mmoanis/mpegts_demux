/// /////////////////////////////////////////////////////////////////////////////
/// Defines Modules for Reading MPEG-TS Stream of data
/// ////////////////////////////////////////////////////////////////////////////
#ifndef INCLUDED_MPEGTSBITSTREAM_HPP
#define INCLUDED_MPEGTSBITSTREAM_HPP

#include <MpegTs.hpp>
#include <ByteStream_iterator.hpp>
#include <cstdio>

/// ////////////////////////////////////////////////////////////////////////////
/// MPEG Bit Reader
class MpegTsBitStream {
public:
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
