/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 HiFiPhile
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_

// Defined in TUD_AUDIO_SPEAKER_STEREO_FB_DESCRIPTOR
#define UAC2_ENTITY_CLOCK               0x04
#define UAC2_ENTITY_INPUT_TERMINAL      0x01
#define UAC2_ENTITY_FEATURE_UNIT        0x02
#define UAC2_ENTITY_OUTPUT_TERMINAL     0x03

#define UAC2_STREAMING_IN_EP_ADDR 0x02
#define UAC2_STREAMING_OUT_EP_ADDR 0x01
#define UAC2_FEEDBACK_EP_ADDR 0x81

#define UAC2_STREAMING_IN_ATT	0x05
#define UAC2_STREAMING_OUT_ATT	0x05
#define UAC2_FEEDBACK_ATT		0b010001

#define UAC2_DESC_LEN 192
#define UAC2_AUDIO_DESC_LENGTH

#define UAC2_AUDIO_MAX_PACKET 512
#define UAC2_FB_PACKET_SIZE 4

// Interface definitions
#define AC_INTERFACE_NUM															0x00
#define AS_INTERFACE_NUM															0x01
#define ASIN_INTERFACE_NUM															0x02

#define UAC2_CLOCK_CTRL AUDIO_TERM_TYPE_USB_STREAMING

#endif
