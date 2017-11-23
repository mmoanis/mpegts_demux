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
```
mpegts_demux -i elephants.ts
```

