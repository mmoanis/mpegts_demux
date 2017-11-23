[![Travis Build Status](https://travis-ci.org/mohamed-moanis/mpegts_demux.svg?branch=master)](https://travis-ci.org/mohamed-moanis/mpegts_demux)

# About
"mpegts_demux" is a tool that reads a [TS](https://en.wikipedia.org/wiki/MPEG_transport_stream) file and dump the video/audio streams into disk.
For each stream a file named ```audio_id_xxx``` or ```video_id_xxx``` is generated with the raw stream data.
The dumped data can be viewed by ffmpeg.

# Build Instructions
Make like build
```
make
```

# Comand line options
```
mpegts_demux [-i] input_file.ts
```

Default invokation will only read [PES](https://en.wikipedia.org/wiki/Packetized_elementary_stream) packets that contain audio/video streams and dump them to disk.
While the ```-i``` switch will read the [program information](https://en.wikipedia.org/wiki/Program-specific_information) to identify all programs and their associated streams.

## Examples
```
mpegts_demux elephants.ts
```
Read program information
```
mpegts_demux -i elephants.ts
```

# Code Walk through
List of files:
* MpegTs.hpp: Definitions of structs and special numbers
* MpegTsBitStream.hpp/.cpp: Class for reading TS bit stream.
* MpegTsDemux.hpp/.cpp: Class for parsing and demuxing, contain definitions for streams.
* mpegts_demux.cpp: main function for the program.

The code base is organized into 2 modules:
1. Bit stream reader.
   This handles reading bit stream from input file and converting it into packets of fixed 188 bytes. 188 bytes is only supported for now. Source files for this module in ```MpegTsBitStream.cpp``` and ```include/MpegTsBitStream.hpp```.
   Method ```MpegTsBitStream::GetPacket()``` handles reading bitstream and packing it into TS packets ```Packet```.
2. TS packet reader and demuxer.
   This module operates on fixed size packets - which make running out of data easier to check and handle.
   ```MpegTsDemuxer::DecodePacket()``` is the entry point for handling new packets. Firstly, packet header is parsed to get the packet identifier ```PID```. The demuxer uses the concept of filters/actions; for each ```PID``` of interest, its checked against a list of know identifiers of interest saved in ```MpegTsDemuxer::m_filters```. When the command line switch "-i" is set, the PAT/PMT identifies the ```PID``` of streams, which in turn will be registered in the filters to pick them up. In the absence of the program data - no "-i" switch or packets that appears before PAT/PMT, the ```MpegTsDemuxer::CheckPES``` method tries to probe the packet if it is a PES packet and if so, the packet will be parsed. one thing to note here is that changes/updates to stream program number is allowed, and a warning messege will be issued for that.
   Finally, with stream registered, a ```Stream``` class object is instantiated which hold a ```FILE*``` to the output file to dump the stream into.

