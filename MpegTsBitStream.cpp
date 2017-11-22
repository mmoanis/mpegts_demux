#include <MpegTsBitStream.hpp>
#include <cassert>
#include <cstdlib>
#include <iostream>
using namespace std;

MpegTsBitStream::MpegTsBitStream(const char * filename) : m_file(NULL), m_eof(false), m_bad(true)
{
    m_file = fopen(filename, "rb");
    if (!m_file) throw logic_error("File doesn't exists.");

    m_bad = false;
}

MpegTsBitStream::~MpegTsBitStream() {
    if (m_file) fclose(m_file);
}

bool MpegTsBitStream::GetPacket(Packet& packet)
{
    if (!m_file) return false;

    Packet::iterator p = packet.begin();

    size_t count = 1;
    int C = fgetc(m_file);

    if (C == EOF) {
        /// No More packets in stream
        m_eof = true;
        return false;
    }

    uint8_t c = static_cast<uint8_t> (C);
    if (c != 0x47) {
        cerr << "GetPacket: Sync byte not found." << endl;
        m_bad = true;
        return false;
    }

    *p++ = c;
    while (p != packet.end()) {
        C = fgetc(m_file);
        if (C == EOF) {
            m_eof = true;
            break;
        }
        //++count;
        c = static_cast<uint8_t> (C);
        *p++ = c;
        ++count;
    }

    if (count != PACKET_SIZE) {
        cerr << "GetPacket: Expected " << PACKET_SIZE << " Got " << count << endl;
        m_bad = true;
        return false;
    }

    return true;
}

