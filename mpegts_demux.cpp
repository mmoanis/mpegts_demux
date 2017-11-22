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
FILE* V, A;
//map<uint16_t, FILE*> pid2f;

static bool decode_packet(uint8_t * p, uint8_t* endp) {
    if (!p) return false;

    if (*p++ != 0x47) {
        cerr << "decode_packet: Sync byte not found." << endl;
        return false;
    }

    //const bool tei = static_cast<bool> (*p & 0x80);
    const bool pusi = static_cast<bool> (*p & 0x40);
    //const bool tp = static_cast<bool> (*p&0x20);

    uint8_t *sp = p++;
    const uint16_t pid = ((*sp & 0x1F) << 8) | (*p);
    //cout << "pid: " << pid << endl;

    p++;

    if (pid == 33) { // TODO: Read all programs
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

    V = fopen("video", "wb");
    //A = fopen("audio")

    uint8_t packet[PACKET_SIZE];

    while (read_packet(F, &packet[0])
           && decode_packet(&packet[0], &packet[PACKET_SIZE-1]+1)) {

    }

    fclose(V);
    fclose(F);
}
