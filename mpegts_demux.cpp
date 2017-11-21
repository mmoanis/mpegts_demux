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
/// Buffer Reading Utilities
#define MPEG_TS_PACKET_SIZE 188 // For now ignore other packet sizes
#define MPEG_TS_PACKET_MAX_SIZE 256
using PacketContainer = array<uint8_t, MPEG_TS_PACKET_MAX_SIZE>;
class MpegTsPacket {
    friend MpegTsPacket GetPacket(FILE * F);
public:
    using value_type = uint8_t;
    using pointer = value_type*;
    using size_type = size_t;

    MpegTsPacket(){}
    MpegTsPacket( MpegTsPacket && p) {
        m_data = std::move(p.m_data);
        cout << "Move Ctor" << endl;
    }

    // Not Optimum
    struct iterator {
        friend class MpegTsPacket;
        /// Advance the iterator and return a fixed number of bytes from the underlying packet.
        template <typename T>
        const T& Read() { T*c = reinterpret_cast<T*>(m_ptr); m_ptr += sizeof(T); return *c; }

        //size_type GetBytesLeft() const { std::distance(m_ptr, m_ptrEnd) + 1; }

        /// Move the iterator back with num bytes
        void Discard(size_type num) { m_ptr -= num; };
    private:
        iterator(pointer p) : m_ptr(p) {}
        pointer m_ptr;
    };

    /// Tells the number of bytes left in this iterator
    size_type GetBytesLeft(const iterator &i) { return std::distance(i.m_ptr, m_data.data() + MPEG_TS_PACKET_SIZE-1); }

    /// Return an iterator that can be used to read data from the packet.
    iterator GetIterator() { return iterator(m_data.data()); }

private:
    PacketContainer m_data;
};

MpegTsPacket GetPacket(FILE * F) {
    MpegTsPacket packet;
    int C = fgetc(F);
    if (C != 0x47) {
        cerr << "GetPacket: Failed to read new packet." << endl;
    }

    int count = 0;
    packet.m_data[count++] = C;

    while (count < MPEG_TS_PACKET_SIZE) {
        C = fgetc(F);
        if (C == EOF) {
            cerr << "GetPacket: Premature end of file while reading a packet." << endl;
        }
        packet.m_data[count++] = C;
    }
    cout << "Last Byte= " << hex << int(packet.m_data[count-1]) << endl;
    return packet;
}
///////////////////////////////////////////////////////////////////////////////
/// Parse Header
PTS ParsePTS(uint32_t H)
{
    // TODO: Check Sync Byte
    PTS pts;
    pts.m_tei = H & (uint32_t)0x800000;
    pts.m_pusi = H & (uint32_t)0x400000;
    pts.m_tp = H & (uint32_t)0x200000;
    pts.m_pid = H & (uint32_t)0x1FFF00;
    uint8_t tsc = (H&(uint32_t)0xC0) >> 6;
    switch(tsc) {
    case 0:
        pts.m_tsc = PTS::TRANSPORT_NOT_SCRAMBLED;
        break;
    case 1:
        pts.m_tsc = PTS::TRANSPORT_RESERVED;
        break;
    case 2:
        pts.m_tsc = PTS::TRANSPORT_SCRAMBLED_EVEN;
        break;
    case 3:
        pts.m_tsc = PTS::TRANSPORT_SCRAMBLED_ODD;
        break;
    default:
        break;
    }
    uint8_t afc = (H & (uint32_t)0x30) >> 4;
    switch (afc) {
    case 0:
        pts.m_afc = PTS::ADAPTION_RESERVED;
        break;
    case 1:
        pts.m_afc = PTS::ADAPTION_NO_ADAPTION_FIELD;
        break;
    case 2:
        pts.m_afc = PTS::ADAPTION_FIELD_ONLY;
        break;
    case 3:
        pts.m_afc = PTS::ADAPTION_FIELD_PAYLOAD;
        break;
    default:
        break;
    }
    pts.m_cc = H & (uint32_t)0xF;

    return pts;
}
////////////////////////////////////////////////////
PAT ParsePAT(uint32_t H)
{
    PAT pat;
    uint16_t pid = H >> 16;
    //H = H >> 16;
    pat.pid = pid;

    uint8_t MAGIC = H & 0x7;
    if (MAGIC != 0x07)
        cerr << "Error in PAT: Reserved Bits are not set." << endl;
    //H = H>>3;
    pat.pmid = H & 0xFFFF;
    return pat;
}
////////////////////////////////////////////////////
uint32_t Read4Bytes(FILE* F)
{
    uint32_t H = 0;

    // 1st
    int C = H = fgetc(F);
    H = H << 8;

    // 2nd
    C = fgetc(F);
    H |= C;
    H = H << 8;

    // 3rd
    C = fgetc(F);
    H |= C;
    H = H << 8;

    // 4th
    C = fgetc(F);
    H |= C;

    return H;
}
uint16_t Read2Bytes(FILE * F)
{
    uint16_t H = 0;

    // 1st
    int C = H = fgetc(F);
    H = H << 8;

    // 2nd
    C = fgetc(F);
    H |= C;

    return H;
}
uint8_t Read1Byte(FILE * F)
{
    uint8_t H = 0;
    int C = fgetc(F);
    H = C;
    //if (H == EOF) throw out_of_range("No More Data in Stream");
    return H;
}
////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    print_banner(cout);
    if (argc < 2) {
        print_usage(cerr);
        return -1;
    }

    FILE* F = fopen(argv[1], "rb");

    if (!F) {
        cerr << "Can't Open File " << argv[1] << endl;
        return -1;
    }

    //uint8_t * BUF = new uint8_t[1024]; // BUFFER
    int C;
    /*
    uint32_t H32 = Read4Bytes(F);
    cout << ParsePTS(H32) << endl;
    //H = Read4Bytes(F);
    //bitset<32> B(H);
    //cout << B << endl;
    //cout << ParsePAT(H) << endl;

    uint8_t H8 = Read1Byte(F);
    if (H8) {
        cout << "Pointer = " << int(H8) << endl;
    } else {
        cout << "Pointer = 0" << endl;
    }

    H8 = Read1Byte(F);
    cout << "Table ID: " << int(H8) << endl;


    uint16_t H16 = Read2Bytes(F);
    cout << "Section Syntax Indicator: ";
    if (H16 & 0x8000)
        cout << 1;
    else cout << 0;
    cout << endl;

    bool B = (H16 & 0x4000) == 0;
    cout << "PAT/PMT/CAT: " << !B << endl;
    if (((H16 & 0x3000)>>12) != 0x03) cerr << "Reserved Bits not set." << endl;

    cout << "Unused: " << int(H16 & 0x0C00) << endl;
    cout << "Section length: " << dec << (H16 & 0x03FF) << endl;

    H16 = Read2Bytes(F);
    cout << "Table ID extension: " << dec << int(H16) << endl;
    H8 = Read1Byte(F);

    if (((H8 & 0xC0)>>6) != 0x03) cerr << "Reserved Bits not set." << endl;
    cout << "Version Number: " << int(H8 & 0x3E) << endl;
    cout << "Current/next Indicator: " << int (H8 &0x1) << endl;

    H8 = Read1Byte(F);
    cout << "Section Number: " << int(H8) << endl;

    H8 = Read1Byte(F);
    cout << "Last Section Number: " << int(H8) << endl;

    H16 = Read2Bytes(F);
    cout << "Program Number: " << int(H16) << endl;

    H16 = Read2Bytes(F);
    if (((H16&0xE000)>>13) != 0x07) cerr << "Reserved Bits not set." << endl;
    cout << "Program map PID: " << int (H16&0x1000) << endl;


    H32 = Read4Bytes(F);
    cout << "CRC: " << hex << H32 << endl;

    int count = 21;
    while ((C=fgetc(F)) != 0x47) {
        ++count;
    }
    cout << "Packet Size:= " << dec << count << " Bytes." << endl;
    */

    /// GetPacket
    while (1) {
        MpegTsPacket packet = GetPacket(F);
    MpegTsPacket::iterator packetIter = packet.GetIterator();
    while (packet.GetBytesLeft(packetIter) > 0) {
        cout << hex << int(packetIter.Read<uint8_t>()) << ' ';
    }
    cout << endl;
    }


    //do {
      //  C = fgetc(F);
      //  bitset<8> B(C);
    //cout << B << ' ';
    //    cout << std::hex << C << ' ';
    //} while (C != EOF);
    //cout << endl;

    fclose(F);
    //FileReader fileReader(argv[1]);
    //MpegTsDemuxer demuxer();
    //if (!demuxer.DemuxStreams(&fileReader)) {
    //    return -1;
    //}
}
