/*
 * audio.c
 *
 *  Created on: Aug 21, 2024
 *      Author: Sx107
 */

#include <uac2_audio.h>
#include "class/audio/audio.h"
#include "tusb.h"
#include <usb_descriptors.h>
#include "device/usbd.h"
#include "device/usbd_pvt.h"

uac2_handle hnd;

tu_static CFG_TUSB_MEM_ALIGN uint8_t audio_ep_out_sw_buf_1[AUDIO_FIFO_SIZE];
tu_static CFG_TUSB_MEM_ALIGN uint8_t audio_ep_in_sw_buf_1[AUDIO_FIFO_SIZE];

tu_static CFG_TUD_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t ctrl_buf_1[CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ];

static bool uac2_get_interface(uint8_t rhport, tusb_control_request_t const * p_request);
static bool uac2_set_interface(uint8_t rhport, tusb_control_request_t const * p_request);
static bool uac2_control_complete(uint8_t rhport, tusb_control_request_t const * p_request);
static bool uac2_control_request(uint8_t rhport, tusb_control_request_t const * p_request);

static bool uac2_get_request(uint8_t rhpoty, tusb_control_request_t const * p_request);
static bool uac2_get_range_request(uint8_t rhport, tusb_control_request_t const * p_request);
static bool uac2_set_request(uint8_t rhpoty, tusb_control_request_t const * p_request);

static bool uac2_send_fb(uint8_t rhport);

tu_fifo_t* uac2_get_out_fifo(void);
bool uac2_initialized(void);

void uac2_init(void) {
	tu_memclr(&hnd, sizeof(uac2_handle));
	hnd.ctrl_buf = ctrl_buf_1;
	hnd.ctrl_buf_size = CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ;
	hnd.alt_setting = 0;
	hnd.sample_rate = 48000;
	hnd.muted = 0;
	hnd.feedback_value = 0x60000;

	tu_fifo_config(&hnd.ep_out_ff, audio_ep_out_sw_buf_1, AUDIO_FIFO_SIZE, 1, true);
	tu_fifo_config(&hnd.ep_in_ff, audio_ep_in_sw_buf_1, AUDIO_FIFO_SIZE, 1, true);
}
bool uac2_deinit(void) {
	hnd.initialized = false;
	return true;
}
void uac2_reset(uint8_t rhport) {
	tu_memclr(&hnd, sizeof(uac2_handle));
	hnd.ctrl_buf = ctrl_buf_1;
	hnd.ctrl_buf_size = CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ;
	hnd.alt_setting = 0;
	hnd.sample_rate = 48000;
	hnd.muted = 0;
	hnd.feedback_value = 0x60000;

	tu_fifo_clear(&hnd.ep_out_ff);
	tu_fifo_clear(&hnd.ep_in_ff);
	tu_fifo_config(&hnd.ep_out_ff, audio_ep_out_sw_buf_1, AUDIO_FIFO_SIZE, 1, true);
	tu_fifo_config(&hnd.ep_in_ff, audio_ep_in_sw_buf_1, AUDIO_FIFO_SIZE, 1, true);
}
uint16_t uac2_open(uint8_t rhport, tusb_desc_interface_t const * itf_desc, uint16_t max_len) {
	TU_VERIFY ( TUSB_CLASS_AUDIO  == itf_desc->bInterfaceClass && 0x01    == itf_desc->bInterfaceSubClass);
	TU_VERIFY(itf_desc->bAlternateSetting == 0);
	if (!hnd.pdesc) {
		hnd.pdesc = (uint8_t const *)itf_desc;
		hnd.rhport = rhport;
		hnd.desc_length = UAC2_DESC_LEN;
	}

	usbd_edpt_iso_alloc(rhport, 1, 512);
	usbd_edpt_iso_alloc(rhport, 1, 4);

	return hnd.desc_length - TUD_AUDIO_DESC_IAD_LEN;
}
bool uac2_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
	if(stage == CONTROL_STAGE_SETUP) {
		return uac2_control_request(rhport, request);
	} else if (stage == CONTROL_STAGE_DATA) {
		return uac2_control_complete(rhport, request);
	}
	return true;
}
bool uac2_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {
	if(ep_addr == UAC2_STREAMING_OUT_EP_ADDR && hnd.alt_setting != 0) {
		TU_VERIFY(usbd_edpt_xfer_fifo(rhport, UAC2_STREAMING_OUT_EP_ADDR, &hnd.ep_out_ff, AUDIO_FIFO_SIZE), false);
		uac2_send_fb(rhport);
	}

	if(ep_addr == UAC2_FEEDBACK_EP_ADDR) {
		uac2_send_fb(rhport);
	}

	return true;
}
void uac2_sof_cb (uint8_t rhport, uint32_t frame_count) {

}

static bool uac2_control_request(uint8_t rhport, tusb_control_request_t const * p_request) {
	if (p_request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD) {
		switch(p_request->bRequest) {
			case TUSB_REQ_GET_INTERFACE:
				return uac2_get_interface(rhport, p_request);
			case TUSB_REQ_SET_INTERFACE:
				return uac2_set_interface(rhport, p_request);
			case TUSB_REQ_CLEAR_FEATURE:
				return true;

			default: TU_BREAKPOINT(); return false;
		}
	}

	if(p_request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS) {
		if (p_request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE && p_request->bmRequestType_bit.direction == TUSB_DIR_IN) {
			if (p_request->bRequest== AUDIO_CS_REQ_CUR) {
				return uac2_get_request(rhport, p_request);
			} else if (p_request->bRequest == AUDIO_CS_REQ_RANGE) {
				return uac2_get_range_request(rhport, p_request);
			}
		}
	}

	if(p_request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
		TU_VERIFY(tud_control_xfer(rhport, p_request, hnd.ctrl_buf, hnd.ctrl_buf_size));
		return true;
	}

	TU_BREAKPOINT();
	return false;
}
static bool uac2_control_complete(uint8_t rhport, tusb_control_request_t const * p_request) {
	if (p_request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS && p_request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
		if (p_request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE && p_request->bRequest == AUDIO_CS_REQ_CUR) {
			uac2_set_request(rhport, p_request);
		}
	}
	return true;
}

static bool uac2_get_request(uint8_t rhport, tusb_control_request_t const * p_request) {
    //uint8_t itf = TU_U16_LOW(p_request->wIndex);
    uint8_t entityID = TU_U16_HIGH(p_request->wIndex);
    if (entityID == 0) {return true;}

    audio_control_request_t const *req = (audio_control_request_t const *)p_request;

    if (req->bEntityID == UAC2_ENTITY_CLOCK) {
    	audio_control_cur_4_t curf = { (int32_t) tu_htole32(hnd.sample_rate) };
    	tu_memcpy_s(hnd.ctrl_buf, hnd.ctrl_buf_size, &curf, sizeof(curf));
    	tud_control_xfer(rhport, p_request, (void*)hnd.ctrl_buf, sizeof(curf));
    }
    else if (req->bEntityID == UAC2_ENTITY_FEATURE_UNIT) {
    	if (req->bControlSelector == AUDIO_FU_CTRL_MUTE) {
    		audio_control_cur_1_t mute = { .bCur = hnd.muted };
    		tu_memcpy_s(hnd.ctrl_buf, hnd.ctrl_buf_size, &mute, sizeof(mute));
    		tud_control_xfer(rhport, p_request, (void*)hnd.ctrl_buf, sizeof(mute));
    	}
    }

    return true;
}
static bool uac2_get_range_request(uint8_t rhport, tusb_control_request_t const * p_request) {
   // uint8_t itf = TU_U16_LOW(p_request->wIndex);
    uint8_t entityID = TU_U16_HIGH(p_request->wIndex);
    if (entityID == 0) {return true;}

    audio_control_request_t const *req = (audio_control_request_t const *)p_request;

    if(req->bEntityID == UAC2_ENTITY_CLOCK && req->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ) {
        audio_control_range_4_n_t(1) rangef =
        {
          .wNumSubRanges = tu_htole16(1)
        };
        rangef.subrange[0].bMin = 44100;
        rangef.subrange[0].bMax = 96000;
        rangef.subrange[0].bRes = 1;

		tu_memcpy_s(hnd.ctrl_buf, hnd.ctrl_buf_size, &rangef, sizeof(rangef));
		tud_control_xfer(rhport, p_request, (void*)hnd.ctrl_buf, sizeof(rangef));
    }

	return true;
}
static bool uac2_set_request(uint8_t rhport, tusb_control_request_t const * p_request) {
	// uint8_t itf = TU_U16_LOW(p_request->wIndex);
	uint8_t entityID = TU_U16_HIGH(p_request->wIndex);
	if (entityID == 0) {return true;}

	audio_control_request_t const *req = (audio_control_request_t const *)p_request;

	if (req->bEntityID == UAC2_ENTITY_CLOCK && req->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ) {
		TU_VERIFY(req->wLength == sizeof(audio_control_cur_4_t));
		hnd.sample_rate = (uint32_t) ((audio_control_cur_4_t const *)hnd.ctrl_buf)->bCur;
		return true;
	}
	else if (req->bEntityID == UAC2_ENTITY_FEATURE_UNIT && req->bControlSelector == AUDIO_FU_CTRL_MUTE) {
		TU_VERIFY(req->wLength == sizeof(audio_control_cur_1_t));
		hnd.muted = ((audio_control_cur_1_t const *)hnd.ctrl_buf)->bCur;
		return true;
	}

	return false;
}

static bool uac2_get_interface(uint8_t rhport, tusb_control_request_t const * p_request) {
	TU_VERIFY(tud_control_xfer(rhport, p_request, &hnd.alt_setting, 1));
	return true;
}
static bool uac2_set_interface(uint8_t rhport, tusb_control_request_t const * p_request) {
	if (hnd.initialized) {
		//tu_fifo_clear(&hnd.ep_in_ff);
		tu_fifo_clear(&hnd.ep_out_ff);

		//usbd_edpt_close(rhport, UAC2_STREAMING_IN_EP_ADDR);
		usbd_edpt_close(rhport, UAC2_STREAMING_OUT_EP_ADDR);
		usbd_edpt_close(rhport, UAC2_FEEDBACK_EP_ADDR);
	}
	//uint8_t const itf = tu_u16_low(p_request->wIndex);
	uint8_t const alt = tu_u16_low(p_request->wValue);

	hnd.alt_setting = alt;


	if(alt != 0) {
		tusb_desc_endpoint_t outep;
		outep.bDescriptorType = TUSB_DESC_ENDPOINT;
		outep.bEndpointAddress = UAC2_STREAMING_OUT_EP_ADDR;
		outep.bInterval = 1;
		outep.bLength = 7;
		outep.wMaxPacketSize = UAC2_AUDIO_MAX_PACKET;
		outep.bmAttributes.xfer = TUSB_XFER_ISOCHRONOUS;

		TU_VERIFY(usbd_edpt_open(rhport, &outep));
		usbd_edpt_clear_stall(rhport, UAC2_STREAMING_OUT_EP_ADDR);

		tusb_desc_endpoint_t fbep;
		fbep.bDescriptorType = TUSB_DESC_ENDPOINT;
		fbep.bEndpointAddress = UAC2_FEEDBACK_EP_ADDR;
		fbep.bInterval = 1;
		fbep.bLength = 7;
		fbep.wMaxPacketSize = UAC2_FB_PACKET_SIZE;
		fbep.bmAttributes.xfer = TUSB_XFER_ISOCHRONOUS;

		TU_VERIFY(usbd_edpt_open(rhport, &fbep));
		usbd_edpt_clear_stall(rhport, UAC2_FEEDBACK_EP_ADDR);

		TU_VERIFY(usbd_edpt_xfer_fifo(rhport, UAC2_STREAMING_OUT_EP_ADDR, &hnd.ep_out_ff, AUDIO_FIFO_SIZE), false);
		hnd.initialized = true;
	}

	usbd_sof_enable(rhport, SOF_CONSUMER_AUDIO, 1);

	tud_control_status(rhport, p_request);

	return true;
}

static bool uac2_send_fb(uint8_t rhport) {
	if (usbd_edpt_claim(rhport, UAC2_FEEDBACK_EP_ADDR))
	{
		// Needs a fix for the OSX - see original audio_device.c
		return usbd_edpt_xfer(rhport, UAC2_FEEDBACK_EP_ADDR, (uint8_t *) &hnd.feedback_value, 4);
	}
	return true;
}
tu_fifo_t* uac2_get_out_fifo(void) {
	return &hnd.ep_out_ff;
}
bool uac2_initialized() {
	return hnd.initialized;
}
