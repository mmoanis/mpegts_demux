/***
** MPEG-TS Demuxer.
** Author: Mohamed Moanis
***/

#include <MpegTs.hpp>
#include <memory>
#include <iostream>
#include <cstdio>
#include <array>
#include <bitset>
#include <cassert>
#include <map>
using namespace std;

///////////////////////////////////////////////////////////////////////////////
static void print_banner(ostream& out)
{
    out << "mpegts_demux V0.1" << endl;
}
static void print_usage(ostream& out)
{
    out << "usage: mpegts_demux input_file" << endl;
}
///////////////////////////////////////////////////////////////////////////////

/// Programs and Streams
map<uint16_t, FILE*> pid2Stream;
map<uint16_t, Program> pid2Prog;

bool add_pid_stream(uint16_t pid, bool v) {
    string fn = (v? "video" : "audio");
    FILE* F = fopen(fn.c_str(), "wb");
    if (!F) {
            cerr << "add_pid_stream: Failed to allocate new stream." << endl;
        return false;
    }
    assert(pid2Stream.find(pid) == pid2Stream.end());
    pid2Stream[pid] = F;
    return true;
}
void close_streams() {
    for (map<uint16_t, FILE*>::iterator i = pid2Stream.begin(), e = pid2Stream.end(); i != e; ++i)
    {
        assert((*i).second);
        fclose((*i).second);
        //*i == NULL;
    }
}
///////////////////////////////////////////////////////////////////////////////
static bool read_packet(FILE*F, uint8_t * p) {
    size_t count = 1;
    int C = fgetc(F);
    if (C == EOF) {
            //cerr << "read_packet: Premature EOF." << endl;
            return false;
    }

    uint8_t c = static_cast<uint8_t> (C);
    if (c != 0x47) {
        cerr << "read_packet: Sync byte not found." << endl;
        return false;
    }

    *p++ = c;

    while (count < PACKET_SIZE) {
        C = fgetc(F);
        if (C == EOF) break;
        //++count;
        c = static_cast<uint8_t> (C);
        *p++ = c;
        ++count;
    }

    if (count != PACKET_SIZE) {
            cerr << "read_packet: Expected " << PACKET_SIZE << " Got " << count << endl;
            return false;
    }
    return true;
}

static bool decode_packet(uint8_t * p, uint8_t* endp) {
    if (!p) return false;

    if (*p++ != 0x47) {
        cerr << "decode_packet: Sync byte not found." << endl;
        return false;
    }

/// Parse Header
    //const bool tei = static_cast<bool> (*p & 0x80);
    const bool pusi = static_cast<bool> (*p & 0x40);
    //const bool tp = static_cast<bool> (*p&0x20);

    uint8_t *sp = p++;
    const uint16_t pid = ((*sp & 0x1F) << 8) | (*p);
    //cout << "pid: " << pid << endl;

    p++;
    const bool afc = (((*p&0x30)>>4) == 0x2)
            || (((*p&0x30)>>4) == 0x3);
    //const bool payload = (((*p&0x30)>>4) == 0x1)
    //    || (((*p&0x30)>>4) == 0x3);
    //const uint8_t cc = *p & 0xF;

    p++;
    if (afc) {
        // escape it
        const uint8_t afcl = *p++;
        p += afcl;
    }
/// Parse Header

    if (pid == MPEGTS_PID_PAT) {
        cout << "decode_packet: Found PAT Section." << endl;
        uint8_t pointer = *p++;
        p += pointer; // Jump to table at pointer offset
/// Parse Table
        uint8_t tableID = *p++;
        const bool ssi = static_cast<bool> (*p&0x80); // PAT/PMT/CAT == 1
        const bool pb = static_cast<bool> (*p&0x40); // PAT/PMT/CAT == 0
        if (((*p&0x30)>>4) != 0x03 ) {
            cerr << "decode_packet: Table reserved bits not set." << endl;
            return false;
        }
        sp = p++;
        uint16_t slength = uint16_t((*sp&0x03)<<10) | uint16_t(*p++);
        if (p + slength >= endp) {
            cerr << "decode_packet: bad PAT section length." << endl;
            return false;
        }

        /// SECTION
        sp = p++;
        uint16_t tid = uint16_t(*sp<<8) | uint16_t(*p++);
        if (((*p&0xC0)>>6) != 0x03) {
            cerr << "decode_packet: Section reserved bits not set." << endl;
            return false;
        }
        uint8_t version = (*p&0x3E)>>1;
        const  bool sni = static_cast<bool> (*p++&0x1);

        uint8_t sno = *p++;
        uint8_t lsno = *p++;

        /// PAT
        uint16_t pnum = uint16_t(p[0]<<8) | uint16_t(p[1]);
        p+=2;

        if (((*p&0xE0)>>5) != 0x07) {
            cerr << "decode_packet: PAT reserved bits not set." << endl;
            return false;
        }
        uint16_t pmid = uint16_t((p[0]&0x1F)<<8) | uint16_t(p[1]);
        p+=2;

        cout << "PNUM: " << pnum << " PMAP PID: " << pmid << endl;
        /// PAT

        uint32_t crc = uint32_t(p[0]<<24) | uint32_t(p[1]<<16) | uint32_t(p[2]<<8) | uint32_t(p[3]);
        p += 4;
        cout << "CRC: " << hex << crc << dec << endl;
        /// SECTION

        cout << "PAT: " << int(tableID) << ' ' << slength << endl;
        return true;
/// Parse Table
    }

    if (pid == 33) { // TODO: Read all programs
        if (pid2Stream.find(pid) == pid2Stream.end()) add_pid_stream(pid, true);
        FILE* V = pid2Stream[pid];
        /// Read the payload
        if (pusi) {
            // new packet

            // check PES header
            if (p[0] == 0x0 && p[1] == 0x0 && p[2] == 0x1) {
                    p += 3;
                    const uint8_t sidx = *p++;
                    if (sidx < 0xE0 || sidx > 0xEF) {
                        cerr << "decode_packet: should expect video only." << endl;
                    }

                    sp = p++;
                    uint16_t pl = uint16_t(*sp<<8) | uint16_t(*p++);

                    p += 2;

                    pl = *p++;
                    // skip rest of header
                    p += pl;

                    // put all to output
                    while (p != endp) {
                        fputc(*p++, V);
                    }

            } else {
                cerr << "decode_packet: No PES start." << endl;
            }
        } else {
            //if (!cc) {
           //     cerr << "decode_packet: Discontinuity." << endl;
           //     return false;
            //}

            // put all to output
            while (p != endp) {
                fputc(*p++, V);
            }
        }
    }

    return true;
}

int main(int argc, char *argv[])
{
    print_banner(cout);
    /// CMD LINE OPTIONS
    const char  *filename = NULL;
    FILE * F = NULL;
    switch(argc) {
        case 2:
            filename = argv[1];
            break;
        default:
            print_usage(cerr);
            return -1;
    }
    F = fopen(filename, "rb");
    if (!F) return 1;

    //V = fopen("video", "wb");
    //A = fopen("audio")

    uint8_t packet[PACKET_SIZE];

    while (read_packet(F, &packet[0])
           && decode_packet(&packet[0], &packet[PACKET_SIZE-1]+1)) {

    }

    close_streams();
    //fclose(V);
    fclose(F);

}
