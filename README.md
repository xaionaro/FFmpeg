# The fork of `ffmpeg`

This fork adds option `-bitrate_ctrl_socket` to MediaCodec encoders, which enables
the user to change the bitrate on fly.

An example of ffmpeg execution:
```
ffmpeg -listen 1 -i rtmp://0.0.0.0:1935/live/myStream -c:v hevc_mediacodec -bitrate_ctrl_socket /run/bitrate.sock -b:v 8M -f rtsp rtsp://127.0.0.1:1935/live/reEncoded
```
An example of changing the bitrate to 1 Mbps:
```
printf '%016X' 1000000 | xxd -r -p | socat -u STDIN UNIX:/run/bitrate.sock
```

## Building

```
cd /tmp
git clone https://github.com/xaionaro/termux-packages
cd termux-packages
./scripts/run-docker.sh ./scripts/setup-android-sdk.sh
curl https://raw.githubusercontent.com/gouravkhunger/termux-packages/20070/fix-glib/packages/glib/build.sh > packages/glib/build.sh
./scripts/run-docker.sh env TERMUX_FORCE_BUILD_DEPENDENCIES=true ./build-package.sh glib
./scripts/run-docker.sh rm -f /data/data/.built-packages/glib
./scripts/run-docker.sh ./build-package.sh glib
./scripts/run-docker.sh ./build-package.sh ffmpeg
ls -ld ../output/ffmpeg*deb
```

## Merging to the upstream

The patch was suggested here: https://ffmpeg.org/pipermail/ffmpeg-devel/2024-May/328280.html

# Original `README.md`

FFmpeg is a collection of libraries and tools to process multimedia content
such as audio, video, subtitles and related metadata.

## Libraries

* `libavcodec` provides implementation of a wider range of codecs.
* `libavformat` implements streaming protocols, container formats and basic I/O access.
* `libavutil` includes hashers, decompressors and miscellaneous utility functions.
* `libavfilter` provides means to alter decoded audio and video through a directed graph of connected filters.
* `libavdevice` provides an abstraction to access capture and playback devices.
* `libswresample` implements audio mixing and resampling routines.
* `libswscale` implements color conversion and scaling routines.

## Tools

* [ffmpeg](https://ffmpeg.org/ffmpeg.html) is a command line toolbox to
  manipulate, convert and stream multimedia content.
* [ffplay](https://ffmpeg.org/ffplay.html) is a minimalistic multimedia player.
* [ffprobe](https://ffmpeg.org/ffprobe.html) is a simple analysis tool to inspect
  multimedia content.
* Additional small tools such as `aviocat`, `ismindex` and `qt-faststart`.

## Documentation

The offline documentation is available in the **doc/** directory.

The online documentation is available in the main [website](https://ffmpeg.org)
and in the [wiki](https://trac.ffmpeg.org).

### Examples

Coding examples are available in the **doc/examples** directory.

## License

FFmpeg codebase is mainly LGPL-licensed with optional components licensed under
GPL. Please refer to the LICENSE file for detailed information.

## Contributing

Patches should be submitted to the ffmpeg-devel mailing list using
`git format-patch` or `git send-email`. Github pull requests should be
avoided because they are not part of our review process and will be ignored.
