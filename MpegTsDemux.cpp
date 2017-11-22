#include <MpegTsDemux.hpp>
#include <iostream>
#include <cassert>
#include <sstream>
#include <string>
using namespace std;

/// ////////////////////////////////////////////////////////////////////////////
/// Create Unique file name
static string GetFileName(bool video, PID id, PID prog)
{
    stringstream ss;
    ss << (video? "video" : "audio") << "_id_";
    ss << id;
    ss << "_prog_";
    ss << prog;
    return ss.str();
}

/// Log Error
#define LOGE cerr << "[ERROR]: "

/// Log Info
#define LOGI cout << "[INFO]: "
/// ////////////////////////////////////////////////////////////////////////////
Stream::Stream(const char *filename, PID id, PID pid)
    : m_file(NULL), m_id(id), m_pid(pid)
{
    assert(filename != NULL);
    m_file = fopen(filename, "wb");
    if (!m_file) {
        cerr << "STREAM " << m_id << " PID " << m_pid << " : can't open file \'" << filename << "\'" << endl;
    }
}
Stream::~Stream()
{
    if (m_file) fclose(m_file);
}
/// ////////////////////////////////////////////////////////////////////////////
void Stream::Write(Packet::const_iterator b, Packet::const_iterator e)
{
    if (!m_file) return; // Failed to create stream. Exit quietly

    while (b != e) {
        int r = fputc(*b++, m_file);
        if (r == EOF) {
            cerr << "STREAM " << m_id << " PID " << m_pid << " Write Failed." << endl;
            fclose(m_file);
            m_file = NULL;
        }
    }
}
/// ////////////////////////////////////////////////////////////////////////////

/// ////////////////////////////////////////////////////////////////////////////
MpegTsDemuxer::MpegTsDemuxer()
    : m_pnum(0)
{
}

MpegTsDemuxer::~MpegTsDemuxer()
{
    Reset();
}
/// ////////////////////////////////////////////////////////////////////////////
void MpegTsDemuxer::Reset()
{
    m_programs.clear();
    for (Streams::iterator i = m_streams.begin(), e = m_streams.end(); i != e; ++i) {
        // clean up
        delete (*i).second;
    }
    m_streams.clear();
    m_filters.clear();
    m_pnum = 0;
}
/// ////////////////////////////////////////////////////////////////////////////
bool MpegTsDemuxer::DecodePacket(const Packet& packet)
{
    Packet::const_iterator p = packet.begin(), e = packet.end();
    PacketHeader header;
    if (!ReadHeader(p, e, header))
        return false;

    PID &id = header.id;
    //cout << "DecodePacket: PID=" << id << endl;
    switch (id) {
        case MPEGTS_PID_NULL: {
            cout << "DecodePacket: Null packet." << endl;
            return true;
        }
        case MPEGTS_PID_PAT: {
            if (!ReadPAT(p, e))
                return false;
            break;
        }
        case MPEGTS_PID_CAT:
        case MPEGTS_PID_TSDT:
        case MPEGTS_PID_IPMP:
            cout << "DecodePacket: Discard packet PID=" << id << endl;
            return true;
        default: {
            Filters::iterator f = m_filters.find(id);
            if (f != m_filters.end()) {
                if ((*f).second == DEMUXER_EVENT_PMT) {
                    if (!ReadPMT(p, e))
                    return false;
                } else if ((*f).second == DEMUXER_EVENT_PES) {
                    if (!ReadPES(p, e, header, id))
                        return false;
                } else {
                    // clock frequency packet
                }
            } else {
if (id == 33) cout << "Here" << endl;
            }
            break;
        }
    }

    ++m_pnum;
    return true;
}
/// ////////////////////////////////////////////////////////////////////////////
bool MpegTsDemuxer::ReadHeader(Packet::const_iterator &p, Packet::const_iterator e, PacketHeader& header)
{
    if (*p != MPEGTS_SYNC_BYTE) {
        cerr << "DecodePacket: Sync byte not found." << endl;
        return false;
    }
    p++;

    header.tei = static_cast<bool> (*p&0x80);
    header.pusi = static_cast<bool> (*p&0x40);
    header.tp = static_cast<bool> (*p&0x20);

    header.id = PID((p[0]&0x1F)<<8) | PID(p[1]);
    p += 2;

    header.afc = (((*p&0x30)>>4) == 0x2) || (((*p&0x30)>>4) == 0x3);
    header.payload = (((*p&0x30)>>4) == 0x1) || (((*p&0x30)>>4) == 0x3);
    header.cc = *p&0xF;
    p++;

    // Greedy, just escape the adaptation control field
    if (header.afc) {
        const uint8_t l = *p++;
        p += l;
    }

    return true;
}
/// ////////////////////////////////////////////////////////////////////////////
bool MpegTsDemuxer::ReadPAT(Packet::const_iterator &p, Packet::const_iterator e)
{
    const uint8_t pointer = *p++;
    p += pointer; // Jump to table at pointer offset

    bool done = false; // Must get atleast 1 Program

    // Tables can be repeated.
    // TODO: But not sure this is the case for PAT packets
    while (p < e) {
        const uint8_t id = *p++;
        if (id == MPEGTS_TABLE_NIL) {
            if (!done) {
                cerr << "PAT packet has no associated PSI." << endl;
                return false;
            }
            return true;
        } else if (id != MPEGTS_TABLE_PAS) {
            cerr << "[ERROR]: EXPECTED PAT SECTION=" << MPEGTS_TABLE_PAS << " FOUND " << id << endl;
            return false;
        }

        const bool ssi = static_cast<bool> (*p&0x80); // PAT/PMT/CAT == 1
        if (!ssi) {
            cerr << "[ERROR]: PAT Section syntax indicator not set." << endl;
            return false;
        }

        const bool pb = static_cast<bool> (*p&0x40); // PAT/PMT/CAT == 0
        if (pb) {
            cerr << "[ERROR]: PAT Private bit is set." << endl;
            return false;
        }

        if (((*p&0x30)>>4) != 0x03 ) {
            cerr << "[ERROR]: PAT PSI reserved bits not set." << endl;
            return false;
        }

        const uint16_t slength = uint16_t((p[0]&0x03)<<8) | uint16_t(p[1]);
        p += 2;
        if (p + slength >= e) {
            cerr << "[ERROR]: PAT Bad section length." << endl;
            return false;
        }

        PacketSection section;
        if (!ReadSection(p, e, section))
            return false;

        // TODO: PAT can be repeated for multiple programs.
        uint16_t pnum = uint16_t(p[0]<<8) | uint16_t(p[1]);
        p+=2;

        if (((*p&0xE0)>>5) != 0x07) {
            cerr << "[ERROR]: PAT Reserved bits not set." << endl;
            return false;
        }
        uint16_t pmid = uint16_t((p[0]&0x1F)<<8) | uint16_t(p[1]);
        p+=2;

        RegisterProgram(pnum, pmid);
        //cout << "PNUM: " << pnum << " PMAP PID: " << pmid << endl;

        // End of the section
        //uint32_t crc = uint32_t(p[0]<<24) | uint32_t(p[1]<<16) | uint32_t(p[2]<<8) | uint32_t(p[3]);
        //cout << "CRC: " << hex << crc << dec << endl;
        p += 4;
        done = true; /// TODO: FIXME: XXX:
    }

    return done;
}
/// ////////////////////////////////////////////////////////////////////////////
bool MpegTsDemuxer::ReadPMT(Packet::const_iterator &p, Packet::const_iterator e)
{
    /// FIXME: Similar to ReadPAT, can it be grouped together.

    const uint8_t pointer = *p++;
    p += pointer; // Jump to table at pointer offset

    bool done = false; // Must get atleast 1 Program

    // Tables can be repeated.
    // TODO: But not sure this is the case for PAT packets
    while (p < e) {
        const uint8_t id = *p++;
        if (id == MPEGTS_TABLE_NIL) {
            if (!done) {
                cerr << "[ERROR]: PAT packet has no associated PSI." << endl;
                return false;
            }
            return true;
        } else if (id != MPEGTS_TABLE_PMS) {
            // can happen! Just ignore.
            cout << "[WARNING]: EXPECTED PMT SECTION=" << MPEGTS_TABLE_PMS << " FOUND " << id << endl;
            return true;
        }

        const bool ssi = static_cast<bool> (*p&0x80); // PAT/PMT/CAT == 1
        if (!ssi) {
            cerr << "[ERROR]: PMT Section syntax indicator not set." << endl;
            return false;
        }

        const bool pb = static_cast<bool> (*p&0x40); // PAT/PMT/CAT == 0
        if (pb) {
            cerr << "[ERROR]: PMT Private bit is set." << endl;
            return false;
        }

        if (((*p&0x30)>>4) != 0x03 ) {
            cerr << "[ERROR]: PMT PSI Reserved bits not set." << endl;
            return false;
        }

        const uint16_t slength = uint16_t((p[0]&0x03)<<10) | uint16_t(p[1]);
        p += 2;
        if (p + slength >= e) {
            cerr << "[ERROR]: PMT Bad section length." << endl;
            return false;
        }

        PacketSection section;
        if (!ReadSection(p, e, section))
            return false;

        // Get the program this PMT referring to
        Programs::const_iterator prog = m_programs.find(section.id);
        if (prog == m_programs.end()) {
            cerr << "[ERROR]: PMT references non existing Program " << section.id << endl;
            return false;
        }

        if (((*p&0xE0)>>5) != 0x07) {
            cerr << "[ERROR]: PMT Reserved bits not set." << endl;
            return false;
        }

        //PID pcrid = uint16_t((p[0]&0x1F)<<8) | uint16_t(p[1]); // clock frequency
        p += 2;

        // expect clock frequency packet
        //m_filters.insert(make_pair(pcrid, DEMUXER_EVENT_PCR));

        if (((*p&0xF0)>>4) != 0x0F) {
            cerr << "[ERROR]: PMT Reserved bits not set." << endl;
            return false;
        }

        // program descriptors
        const uint16_t pinfol = uint16_t((p[0]&0x03)<<8) | uint16_t(p[1]);
        p += 2;

        if (p + pinfol >= e) {
            cerr << "[ERROR]: PMT Bad program info length." << endl;
            return false;
        }
        p += pinfol;

        // read data stream info
        if (!ReadESD(p, e, (*prog).second))
            return false;

        // End of the section
        //uint32_t crc = uint32_t(p[0]<<24) | uint32_t(p[1]<<16) | uint32_t(p[2]<<8) | uint32_t(p[3]);
        p += 4;
        //cout << "CRC: " << hex << crc << dec << endl;
        done = true; /// TODO: FIXME: XXX:

        //cout << "PAT: " << int(tableID) << ' ' << slength << endl;
        //return true;

    }

    return done;
}
/// ////////////////////////////////////////////////////////////////////////////
/// Reads PES packet
bool MpegTsDemuxer::ReadPES(Packet::const_iterator &p, Packet::const_iterator e, PacketHeader& header, PID id)
{
    Stream* S = NULL;
    {
        Streams::iterator s = m_streams.find(id);
        if (s == m_streams.end()) {
            cerr << "[ERROR]: INVALID STREAM=" << id << endl;
            return false;
        }
        S = (*s).second;
    }

    if (header.pusi) {
        /// PES PSI
        if (p[0] == 0x0 && p[1] == 0x0 && p[2] == 0x01) {
            p += 3;
            const uint8_t sidx = *p++;
            const bool expected = MPEGTS_AUDIO_STREAM(sidx) || MPEGTS_VIDEO_STREAM(sidx);
            if (!expected) {
                cerr << "[ERROR]: EXPECTED AUDIO/VIDEO PACKETS ONLY. FOUND=" << hex << uint16_t(sidx) << dec << endl;
                return false;
            }

            // escape packet length
            p += 2;

            /// Greedy, just escape more data
            p += 2;

            const uint8_t pl = *p++;
            if (p + pl >= e) {
                cerr << "[ERROR]: PES Invalid length." << m_pnum << endl;
                return false;
            }
            p += pl;

            // write to stream
            S->Write(p, e);
        } else {
            cerr << "[ERROR]: EXPECTED PES START SEQUENCE." << endl;
            return false;
        }
    } else {
        /// TODO: continuity

        S->Write(p, e);
    }
    return true;
}
/// ////////////////////////////////////////////////////////////////////////////
bool MpegTsDemuxer::ReadSection(Packet::const_iterator &p, Packet::const_iterator e, PacketSection& section)
{
    // TODO: Check we have enough buffer ahead
    section.id = uint16_t(p[0]<<8) | uint16_t(p[1]);
    p+= 2;

    if (((*p&0xC0)>>6) != 0x03) {
        cerr << "[ERROR]: Section Reserved bits not set." << endl;
        return false;
    }
    section.version = (*p&0x3E)>>1;
    section.cni = static_cast<bool> (*p++&0x1);

    section.sn = *p++;
    section.lsn = *p++;
    return true;
}
/// ////////////////////////////////////////////////////////////////////////////
void MpegTsDemuxer::RegisterProgram(PID id, PID pid)
{
    assert(id != MPEGTS_PID_NULL);

    Programs::iterator p = m_programs.find(id);
    if (p == m_programs.end()) {
        cout << "Found program: " << id << " PMT=" << pid << endl;

        Program P;
        P.id = id;
        P.pid = pid;
        m_programs.insert(make_pair(id, P));

        // expect packet for PMT
        m_filters.insert( make_pair(pid, DEMUXER_EVENT_PMT) );
    } else {
        if (p->second.pid != pid) {
            // FIXME: update/or error?
            cout << "[WARNING]: PROGRAM=" << id << " PMT=" << p->second.pid << " APPEARED WITH PMT=" << pid <<  endl;
        }
    }
}
/// ////////////////////////////////////////////////////////////////////////////
bool MpegTsDemuxer::ReadESD(Packet::const_iterator &p, Packet::const_iterator e, const Program& prog)
{
    bool found = false;
    while (p + 3 < e) {
        // watch out not to read CRC
        if (p[4] == MPEGTS_STUFFING_BYTE) {
            // Oh, we hit it. Okay happy return.
            if (!found) {

            }
            return true;
        }

        const uint8_t st = *p++;
        if (((*p&0xE0)>>5) != 0x07) {
            cerr << "[ERROR]: ESD Reserved bits not set." << endl;
            return false;
        }

        // See what is the stream type
        const bool video = MPEGTS_VIDEO_STREAM_TYPE(st);
        const bool needed = MPEGTS_AUDIO_STREAM_TYPE(st) || video;

        PID id = PID((p[0]&0x1F)<<8) | PID(p[1]);
        p += 2;

        if (((*p&0xF0)>>4) != 0x0F) {
            cerr << "ESD Reserved bits not set." << endl;
            return false;
        }

        const uint16_t esil = uint16_t((p[0]&0x03)<<8) | uint16_t(p[1]);
        p += 2;

        if (p + esil >= e) {
            cerr << "[ERROR]: ESD INVALID ES info length." << endl;
            return false;
        }
        p += esil;

        // Add video/audio streams only
        if (needed) {
            if (!RegisterStream(id, prog, video))
                return false;
            else
                found = true;// we read a stream
        }
    }
    return true;
}
/// ////////////////////////////////////////////////////////////////////////////
bool MpegTsDemuxer::RegisterStream(PID id, const Program& prog, bool video)
{
    Streams::iterator s = m_streams.find(id);
    if (s == m_streams.end()) {
        string filename = GetFileName(video, id, prog.id);
        Stream *S = new Stream(filename.c_str(), id, prog.id);
        m_streams.insert( make_pair(id, S) );
        cout << "INFO: STREAM=" << id << " PROGRAM=" << prog.id << " TYPE=" << (video? "VIDEO" : "AUDIO") << endl;
        // expect packet for stream
        m_filters.insert( make_pair(id, DEMUXER_EVENT_PES) );
    }  else {
        if ((*s).second->GetPId() != prog.id) {
            cout << "WARNING: STREAM=" << id << " PROGRAM=" << (*s).second->GetPId() << " APPEARED IN PROGRAM=" << prog.id << endl;
            return false;
        }
    }
    return true;
}
