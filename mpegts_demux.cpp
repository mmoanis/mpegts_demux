/***
** MPEG-TS Demuxer.
** Author: Mohamed Moanis
***/

#include <MpegTsBitStream.hpp>
#include <MpegTsDemux.hpp>
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
    out << "mpegts_demux v1.0" << endl;
}
static void print_usage(ostream& out)
{
    out << "usage: mpegts_demux input_file" << endl;
}
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    print_banner(cout);

    /// CMD LINE OPTIONS
    const char  *filename = NULL;
    switch(argc) {
        case 2:
            filename = argv[1];
            break;
        default:
            print_usage(cerr);
            return -1;
    }

    MpegTsBitStream bitStream(filename);
    MpegTsDemuxer demuxer;
    Packet packet;

    while (bitStream.good() && bitStream.GetPacket(packet)) {
        if (!demuxer.DecodePacket(packet))
            break;
    }
}
