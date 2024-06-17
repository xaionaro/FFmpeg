/*
 * Android MediaCodec encoders
 *
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

#ifndef AVCODEC_MEDIACODECENC_CTRL_H
#define AVCODEC_MEDIACODECENC_CTRL_H
#if __ANDROID_API__ >= 26

#include "avcodec.h"
#include "mediacodec_wrapper.h"

typedef enum mediacodecenc_ctrl_listener_status {
    mcls_stopped = 0,
    mcls_running = 1,
    mcls_stopping = 2,
} mediacodecenc_ctrl_listener_status_t;

typedef struct mediacodecenc_ctrl_ctx {
    char *server_sock_path;
    int server_sock_fd;
    pthread_t listener_thread_id;
    mediacodecenc_ctrl_listener_status_t listener_status;
    uint64_t bitrate_cur;
    uint64_t bitrate_next;
    FFAMediaFormat *out_format;

    // TODO: delete me
    void (*out_format_setInt32)(FFAMediaFormat* format, const char* name, int32_t value);
} MediaCodecEncControlContext;

extern int mediacodecenc_ctrl_init(AVCodecContext *avctx, FFAMediaFormat *format);
extern int mediacodecenc_ctrl_deinit(AVCodecContext *avctx);

#endif //__ANDROID_API__ >= 26
#endif /* AVCODEC_MEDIACODECENC_CTRL_H */
