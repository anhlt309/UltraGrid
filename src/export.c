/**
 * @file   export.c
 * @author Martin Pulec     <pulec@cesnet.cz>
 */
/*
 * Copyright (c) 2017 CESNET z.s.p.o.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of CESNET nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "config_unix.h"
#include "config_win32.h"
#endif /* HAVE_CONFIG_H */

#include "export.h"

#include "audio/export.h"
#include "video_export.h"

struct exporter {
        bool should_export;
        char *dir;
        bool dir_auto;
        struct video_export *video_export;
        struct audio_export *audio_export;
};

static bool create_dir(struct exporter *s);
static bool enable_export(struct exporter *s);
static void disable_export(struct exporter *s);

struct exporter *export_init(const char *path, bool should_export)
{
        struct exporter *s = calloc(1, sizeof(struct exporter));

        s->should_export = should_export;
        if (path) {
                s->dir = strdup(path);
        } else {
                s->dir_auto = true;
        }

        if (should_export) {
                if (!enable_export(s)) {
                        goto error;
                }
        }

        return s;

error:
        free(s->dir);
        free(s);
        return NULL;
}

static bool enable_export(struct exporter *s)
{
        if (!create_dir(s)) {
                goto error;
        }

        s->video_export = video_export_init(s->dir);
        if (!s->video_export) {
                goto error;
        }

        char name[512];
        snprintf(name, 512, "%s/sound.wav", s->dir);
        s->audio_export = audio_export_init(name);
        if (!s->audio_export) {
                goto error;
        }

        return true;

error:
        video_export_destroy(s->video_export);
        s->video_export = NULL;
        return false;
}

static bool create_dir(struct exporter *s)
{
        if (!s->dir) {
                for (int i = 1; i <= 9999; i++) {
                        char name[16];
                        snprintf(name, 16, "export.%04d", i);
                        int ret = platform_mkdir(name);
                        if(ret == -1) {
                                if(errno == EEXIST) {
                                        continue;
                                } else {
                                        fprintf(stderr, "[Export] Directory creation failed: %s\n",
                                                        strerror(errno));
                                        return false;
                                }
                        } else {
                                s->dir = strdup(name);
                                break;
                        }
                }
        } else {
                int ret = platform_mkdir(s->dir);
                if(ret == -1) {
                        if(errno == EEXIST) {
                                fprintf(stderr, "[Export] Warning: directory %s exists!\n", s->dir);
                                return false;
                        } else {
                                perror("[Export] Directory creation failed");
                                return false;
                        }
                }
        }

        if (s->dir) {
                printf("Using export directory: %s\n", s->dir);
                return true;
        } else {
                return false;
        }
}

static void disable_export(struct exporter *s) {
        audio_export_destroy(s->audio_export);
        video_export_destroy(s->video_export);
        s->audio_export = NULL;
        s->video_export = NULL;
        if (s->dir_auto) {
                free(s->dir);
                s->dir = NULL;
        }
}

void export_destroy(struct exporter *s) {
        disable_export(s);

        free(s->dir);
        free(s);
}

void export_audio(struct exporter *s, struct audio_frame *frame)
{
        if (s->should_export) {
                audio_export(s->audio_export, frame);
        }
}

void export_video(struct exporter *s, struct video_frame *frame)
{
        if (s->should_export) {
                video_export(s->video_export, frame);
        }
}
