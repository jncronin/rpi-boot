#ifndef FB_H
#define FB_H

#define FB_FAIL_GET_RESOLUTION		-1
#define FB_FAIL_INVALID_RESOLUTION	-2
#define FB_FAIL_SETUP_FB		-3
#define FB_FAIL_INVALID_TAGS		-4
#define FB_FAIL_INVALID_TAG_RESPONSE	-5
#define FB_FAIL_INVALID_TAG_DATA	-6
#define FB_FAIL_INVALID_PITCH_RESPONSE	-7
#define FB_FAIL_INVALID_PITCH_DATA	-8

uint8_t *fb_get_framebuffer();
int fb_init();
int fb_get_bpp();
int fb_get_byte_size();
int fb_get_width();
int fb_get_height();
int fb_get_pitch();

#endif

