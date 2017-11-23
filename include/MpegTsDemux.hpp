/// /////////////////////////////////////////////////////////////////////////////
/// Define Module for MPEG-TS Demuxing
/// /////////////////////////////////////////////////////////////////////////////
#ifndef INCLUDED_MPEGTS_DEMUX_HPP
#define INCLUDED_MPEGTS_DEMUX_HPP
#include <MpegTs.hpp>
#include <cstdio>
#include <map>
#include <set>

/// /////////////////////////////////////////////////////////////////////////////
/// Stream
/// Video/Audio stream data extracted from MPEG-TS.
/// Demuxer uses stream objects to feed them with new data as they arrive.
/// For the sake of now, this stream dumps data to disk.
class Stream {
public:
    Stream(const char * filename, PID id, PID pid);
    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;
    ~Stream();

    PID GetId() const { return m_id; }
    PID GetPId() const { return m_pid; }
    void SetPId(PID id) { m_pid = id; }

    /// Write data from packet to sink
    void Write(Packet::const_iterator b, Packet::const_iterator e);
private:
    FILE* m_file;
    PID m_id; /// Packet identifier for this stream
    PID m_pid; /// Program id for this stream
};

/// Type of packets Demuxer is listening to
enum DemuxerEvents {
    DEMUXER_EVENT_PCR, /// FIXME: what is PCR
    DEMUXER_EVENT_PMT,
    DEMUXER_EVENT_PES,
    DEMUXER_EVENT_PAT,
    DEMUXER_EVENT_NIL
};
/// /////////////////////////////////////////////////////////////////////////////
/// Demuxer Interface.
/// Demuxer assumes all packets are related, it will keep list of streams
/// as they appear in TS.
class MpegTsDemuxer {
public:
    using Programs = std::map<PID, Program>;
    using Streams = std::map<PID, Stream*>;
    using Filters = std::map<PID, DemuxerEvents>;

    MpegTsDemuxer(bool info);
    ~MpegTsDemuxer();

    bool DecodePacket(const Packet& packet);

    void Reset();

protected:
    /// Fills a header with data
    bool ReadHeader(Packet::const_iterator &p, Packet::const_iterator e, PacketHeader& header);

    /// Reads PAT packet
    bool ReadPAT(Packet::const_iterator &p, Packet::const_iterator e);
    /// Reads PMT packet
    bool ReadPMT(Packet::const_iterator &p, Packet::const_iterator e);
    /// Reads PES packet
    bool ReadPES(Packet::const_iterator &p, Packet::const_iterator e, PacketHeader& header, PID id);
    bool CheckPES(Packet::const_iterator &p, Packet::const_iterator e, PacketHeader& header);

    /// Fills a section header info
    bool ReadSection(Packet::const_iterator &p, Packet::const_iterator e, PacketSection& section);

    /// Read Elementary stream data
    bool ReadESD(Packet::const_iterator &p, Packet::const_iterator e, const Program& prog);

    /// Add program if new
    void RegisterProgram(PID id, PID pid);
    bool RegisterStream(PID id, const Program& prog, bool video);

private:
    Programs m_programs; /// List of programs
    Streams m_streams; /// List of streams
    Filters m_filters;

    uint64_t m_pnum; /// Packet number
};


#endif
