/*
 * audio.h
 *
 *  Created on: Aug 21, 2024
 *      Author: Sx107
 */

#ifndef INC_UAC2_AUDIO_H_
#define INC_UAC2_AUDIO_H_

#include "tusb.h"
#include <stdbool.h>

typedef struct {
	uint8_t rhport;

	uint8_t const * pdesc;
	uint32_t desc_length;

	uint8_t* ctrl_buf;
	uint16_t ctrl_buf_size;
	uint8_t alt_setting;
	tu_fifo_t ep_out_ff;
	tu_fifo_t ep_in_ff;

	uint8_t initialized;

	uint32_t sample_rate;
	int8_t muted;

	uint32_t feedback_value;

} __attribute__((aligned(32)))  uac2_handle;

#define AUDIO_FIFO_SIZE 256

	void uac2_init(void);
	bool uac2_deinit(void);
	void uac2_reset(uint8_t rhport);
	uint16_t uac2_open(uint8_t rhport, tusb_desc_interface_t const * itf_desc, uint16_t max_len);
	bool uac2_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request);
	bool uac2_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
	void uac2_sof_cb (uint8_t rhport, uint32_t frame_count);

	tu_fifo_t* uac2_get_out_fifo(void);
	bool uac2_initialized(void);

#endif /* INC_UAC2_AUDIO_H_ */
