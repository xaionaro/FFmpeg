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

#if __ANDROID_API__ >= 26

#include <endian.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>

#include "mediacodecenc_ctrl.h"
#include "mediacodecenc.h"

typedef struct mediacodecenc_ctrl_handle_connection_context {
    int client_sock_fd;
    AVCodecContext *avctx;
} mediacodecenc_ctrl_handle_connection_context_t;

static void *mediacodecenc_ctrl_handle_connection(void *arg) {
    mediacodecenc_ctrl_handle_connection_context_t *hctx = arg;
    AVCodecContext *avctx = hctx->avctx;
    MediaCodecEncContext *s = avctx->priv_data;

    while (1) {
        uint64_t received_value, bitrate_old, bitrate_new;
        ssize_t num_bytes = recv(hctx->client_sock_fd, &received_value, sizeof(received_value), MSG_CMSG_CLOEXEC);
        if (num_bytes <= 0) {
            switch (errno) {
            case 0:
            case ECONNRESET:
            case EPIPE:
            case EBADF:
                av_log(avctx, AV_LOG_INFO, "The MediaCodec control connection was closed: errno:%d: %s\n", errno, strerror(errno));
                break;
            default:
                av_log(avctx, AV_LOG_ERROR, "A failure of a MediaCodec control connection: errno:%d: %s\n", errno, strerror(errno));
            }
            break;
        }

        bitrate_new = be64toh(received_value);

        av_log(avctx, AV_LOG_TRACE, "Received a message to change the bitrate to %lu.\n", bitrate_new);

        bitrate_old = __atomic_load_n(&s->ctrl_ctx.bitrate_cur, __ATOMIC_SEQ_CST);
        if (bitrate_new == bitrate_old) {
            av_log(avctx, AV_LOG_INFO, "Received a message to change the bitrate, but the bitrate is already %lu, not changing.\n", bitrate_old);
            continue;
        }

        av_log(avctx, AV_LOG_INFO, "Received a message to change the bitrate from %lu to %lu.\n", bitrate_old, bitrate_new);
        __atomic_store_n(&s->ctrl_ctx.bitrate_next, bitrate_new, __ATOMIC_SEQ_CST);
    }

    close(hctx->client_sock_fd);
    free(hctx);
    pthread_exit(NULL);
}

static void *mediacodecenc_ctrl_handle_listener(void *_avctx) {
    AVCodecContext *avctx = _avctx;
    MediaCodecEncContext *s = avctx->priv_data;

    if (listen(s->ctrl_ctx.server_sock_fd, 5) < 0) {
        av_log(avctx, AV_LOG_ERROR, "Failed to start listening socket '%s': errno:%d: %s\n", s->ctrl_ctx.server_sock_path, errno, strerror(errno));
        goto mediacodecenc_ctrl_handle_listener_end;
    }

    av_log(avctx, AV_LOG_TRACE, "Listening socket '%s'\n", s->ctrl_ctx.server_sock_path);

    s->ctrl_ctx.listener_status = mcls_running;
    while (s->ctrl_ctx.listener_status == mcls_running) {
        pthread_t client_thread_id;
        int pthread_error;
        mediacodecenc_ctrl_handle_connection_context_t *hctx;

        int client_sock_fd = accept(s->ctrl_ctx.server_sock_fd, NULL, NULL);
        if (client_sock_fd < 0) {
            av_log(avctx, AV_LOG_ERROR, "Failed to accept a connection to socket '%s': code:%d: description:%s\n", s->ctrl_ctx.server_sock_path, errno, strerror(errno));
            continue;
        }

        av_log(avctx, AV_LOG_TRACE, "Accepted a connection to '%s'\n", s->ctrl_ctx.server_sock_path);

        hctx = malloc(sizeof(mediacodecenc_ctrl_handle_connection_context_t));
        hctx->client_sock_fd = client_sock_fd;
        hctx->avctx = avctx;

        pthread_error = pthread_create(&client_thread_id, NULL, mediacodecenc_ctrl_handle_connection, hctx);
        if (pthread_error != 0) {
            av_log(avctx, AV_LOG_ERROR, "Failed to accept a connection to socket '%s': code:%d: description:%s\n", s->ctrl_ctx.server_sock_path, pthread_error, strerror(pthread_error));
            free(hctx);
            close(client_sock_fd);
            continue;
        }

        // mediacodecenc_ctrl_handle_connection takes ownership of hctx and frees it when finished,
        // so we are only detaching here and that's it.
        pthread_error = pthread_detach(client_thread_id);
        if (pthread_error != 0) {
            av_log(avctx, AV_LOG_ERROR, "Unable to detach the client handler thread: code:%d: description:%s\n", pthread_error, strerror(pthread_error));
        }
    }

mediacodecenc_ctrl_handle_listener_end:
    close(s->ctrl_ctx.server_sock_fd);
    unlink(s->ctrl_ctx.server_sock_path);
    s->ctrl_ctx.listener_status = mcls_stopped;
    pthread_exit(NULL);
}

int mediacodecenc_ctrl_init(AVCodecContext *avctx, FFAMediaFormat *format) {
    MediaCodecEncContext *s = avctx->priv_data;
    char *socket_path = s->bitrate_ctrl_socket;
    int server_sock_fd;
    struct sockaddr_un server_addr;
    int pthread_error;

    s->ctrl_ctx.bitrate_cur = avctx->bit_rate;
    s->ctrl_ctx.bitrate_next = s->ctrl_ctx.bitrate_cur;
    s->ctrl_ctx.out_format = format;
    s->ctrl_ctx.out_format_setInt32 = format->setInt32;
    av_log(avctx, AV_LOG_TRACE, "setInt32:%p\n", format->setInt32);

    server_sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock_fd < 0) {
        av_log(avctx, AV_LOG_ERROR, "Failed to create socket '%s': errno:%d: %s\n", socket_path, errno, strerror(errno));
        return errno;
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_path, sizeof(server_addr.sun_path) - 1);

    if (bind(server_sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) < 0) {
        av_log(avctx, AV_LOG_ERROR, "Failed to bind to the socket '%s': errno:%d: %s\n", socket_path, errno, strerror(errno));
        return errno;
    }
    s->ctrl_ctx.server_sock_path = socket_path;
    s->ctrl_ctx.server_sock_fd = server_sock_fd;

    pthread_error = pthread_create(&s->ctrl_ctx.listener_thread_id, NULL, mediacodecenc_ctrl_handle_listener, avctx);
    if (pthread_error != 0) {
        av_log(avctx, AV_LOG_ERROR, "Failed to create a thread to listen the socket '%s': errno:%d: %s\n", socket_path, pthread_error, strerror(pthread_error));
        return pthread_error;
    }

    return 0;
}

int mediacodecenc_ctrl_deinit(AVCodecContext *avctx) {
    MediaCodecEncContext *s = avctx->priv_data;
    void *retval;
    int pthread_error;
    if (s->ctrl_ctx.listener_thread_id != 0) {
        return 0;
    }

    s->ctrl_ctx.listener_status = mcls_stopping;
    close(s->ctrl_ctx.server_sock_fd);

    pthread_error = pthread_join(s->ctrl_ctx.listener_thread_id, &retval);
    if (pthread_error != 0) {
        av_log(avctx, AV_LOG_ERROR, "Failed to detach from the listener thread of the socket '%s': errno:%d: %s\n", s->ctrl_ctx.server_sock_path, pthread_error, strerror(pthread_error));
        return pthread_error;
    }

    ff_AMediaFormat_delete(s->ctrl_ctx.out_format);
    return 0;
}

#endif //__ANDROID_API__ >= 26
