/***
** MPEG-TS Demuxer.
** Author: Mohamed Moanis
***/

#include <MpegTsBitStream.hpp>
#include <MpegTsDemux.hpp>
#include <iostream>
#include <cstring>
using namespace std;

///////////////////////////////////////////////////////////////////////////////
static void print_banner(ostream& out)
{
    out << "mpegts_demux v1.0" << endl;
}
static void print_usage(ostream& out)
{
    out << "usage: mpegts_demux [-i] input_file" << endl;
    out << "    Read MPEG-TS files and dump all video/audio streams to disk." << endl;
    out << "    Each stream dumped to disk, will be named [video|audio]_streamid" << endl;
    out << "Options:" << endl;
    out << " -i Read PAT/PMT data to identify programs." << endl;
    out << "    Default is to just read PES." << endl;
}
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    print_banner(cout);

    /// CMD LINE OPTIONS
    const char  *filename = NULL;
    bool info = false;
    switch(argc) {
        case 2:
            filename = argv[1];
            break;
        case 3:
            if (strncmp(argv[1], "-i", 3) == 0) {
                info = true;
                filename = argv[2];
                break;
            }
            // fall through
        default:
            print_usage(cerr);
            return -1;
    }

    MpegTsBitStream bitStream(filename);
    MpegTsDemuxer demuxer(info);
    Packet packet;

    while (bitStream.good() && bitStream.GetPacket(packet)) {
        if (!demuxer.DecodePacket(packet))
            break;
    }
}
