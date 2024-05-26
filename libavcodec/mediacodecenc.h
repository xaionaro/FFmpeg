/*
 * Android MediaCodec encoders
 *
 * Copyright (c) 2022 Zhao Zhili <zhilizhao@tencent.com>
 * Copyright (c) 2024 Dmitrii Okunev <xaionaro@gmail.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVCODEC_MEDIACODECENC_H
#define AVCODEC_MEDIACODECENC_H

#include "avcodec.h"
#include "bsf.h"
#include "mediacodec_wrapper.h"
#include "mediacodecenc_ctrl.h"

#define INPUT_DEQUEUE_TIMEOUT_US 8000
#define OUTPUT_DEQUEUE_TIMEOUT_US 8000

enum BitrateMode {
    /* Constant quality mode */
    BITRATE_MODE_CQ = 0,
    /* Variable bitrate mode */
    BITRATE_MODE_VBR = 1,
    /* Constant bitrate mode */
    BITRATE_MODE_CBR = 2,
    /* Constant bitrate mode with frame drops */
    BITRATE_MODE_CBR_FD = 3,
};

typedef struct MediaCodecEncContext {
    AVClass *avclass;
    FFAMediaCodec *codec;
    int use_ndk_codec;
    const char *name;
    FFANativeWindow *window;

    int fps;
    int width;
    int height;

    uint8_t *extradata;
    int extradata_size;
    int eof_sent;

    AVFrame *frame;
    AVBSFContext *bsf;

    int bitrate_mode;
    char *bitrate_ctrl_socket;
    int level;
    int pts_as_dts;
    int extract_extradata;

#if __ANDROID_API__ >= 26
    MediaCodecEncControlContext ctrl_ctx;
#endif //__ANDROID_API__ >= 26
} MediaCodecEncContext;

enum {
    COLOR_FormatYUV420Planar                              = 0x13,
    COLOR_FormatYUV420SemiPlanar                          = 0x15,
    COLOR_FormatSurface                                   = 0x7F000789,
};

static const struct {
    int color_format;
    enum AVPixelFormat pix_fmt;
} color_formats[] = {
    { COLOR_FormatYUV420Planar,         AV_PIX_FMT_YUV420P },
    { COLOR_FormatYUV420SemiPlanar,     AV_PIX_FMT_NV12    },
    { COLOR_FormatSurface,              AV_PIX_FMT_MEDIACODEC },
};

static const enum AVPixelFormat avc_pix_fmts[] = {
    AV_PIX_FMT_MEDIACODEC,
    AV_PIX_FMT_YUV420P,
    AV_PIX_FMT_NV12,
    AV_PIX_FMT_NONE
};

#endif /* AVCODEC_MEDIACODECENC_H */
