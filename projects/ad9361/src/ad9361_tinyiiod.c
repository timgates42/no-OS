/***************************************************************************//**
 *   @file   ad9361_tinyiiod.c
 *   @brief  Implementation of ad9361_tinyiiod
 *   @author Cristian Pop (cristian.pop@analog.com)
********************************************************************************
 * Copyright 2019(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/
#include "ad9361_tinyiiod.h"
#include "ad9361_parameters.h"
#include "tinyiiod.h"
#include "config.h"
#include "tinyiiod_util.h"
#include "tinyiiod_phy.h"
#include "tinyiiod_dac.h"
#include "tinyiiod_adc.h"

#ifdef UART_INTERFACE
#include "serial.h"
#endif /* UART_INTERFACE */

static uint32_t request_mask;
/* mask for cf-ad9361-lpc 0x0F, it has 4 channels */
static uint32_t input_channel_mask = 0x0F;
extern struct ad9361_rf_phy *ad9361_phy;

/**
 * Check device
 * @param *device
 * @return TRUE if valid device is found
 */
static bool supporter_dev(const char *device)
{
	return strequal(device, "ad9361-phy")
	       || strequal(device, "cf-ad9361-lpc")
	       || strequal(device, "cf-ad9361-dds-core-lpc");
}

/**
 * read global attribute
 * @param *device name
 * @param *attr name
 * @param *buff where value is stored
 * @param len maximum length of value to be stored in buf
 * @param debug
 * @return length of chars written in buf
 */
static ssize_t read_attr(const char *device, const char *attr,
			 char *buf, size_t len, bool debug)
{
	if (!supporter_dev(device))
		return -ENODEV;
	if(strequal(device, "ad9361-phy")) {
		return read_phy_attr(attr, buf, len, debug);
	} else if(strequal(device, "cf-ad9361-dds-core-lpc")) {
		return read_dac_attr(attr, buf, len, debug);
	} else if(strequal(device, "cf-ad9361-lpc")) {
		return read_adc_attr(attr, buf, len, debug);
	}

	return -ENODEV;
}

/**
 * write global attribute
 * @param *device name
 * @param *attr name
 * @param *buff where value is stored
 * @param len length of the value
 * @debug
 * @return length of chars written to attribute, negative value in case of failure
 */
static ssize_t write_attr(const char *device, const char *attr,
			  const char *buf, size_t len, bool debug)
{
	if (!supporter_dev(device))
		return -ENODEV;
	if(strequal(device, "ad9361-phy")) {
		return write_phy_attr(attr, buf, len, debug);
	} else if(strequal(device, "cf-ad9361-dds-core-lpc")) {
		return write_dac_attr(attr, buf, len, debug);
	} else if(strequal(device, "cf-ad9361-lpc")) {
		return write_adc_attr(attr, buf, len, debug);
	}

	return -ENODEV;
}

/**
 * read channel attribute
 * @param *device name
 * @param *channel name
 * @param *ch_out type: input/output
 * @param *attr name
 * @param *buff where value is stored
 * @param len maximum length of value to be stored in buf
 * @return length of chars written in buf
 */
static ssize_t ch_read_attr(const char *device, const char *channel,
			    bool ch_out, const char *attr, char *buf, size_t len)
{
	if (!supporter_dev(device))
		return -ENODEV;

	if(strequal(device, "ad9361-phy")) {
		return ch_read_phy_attr(channel, ch_out, attr, buf, len);
	} else if(strequal(device, "cf-ad9361-dds-core-lpc")) {
		return ch_read_dac_attr(channel, ch_out, attr, buf, len);
	} else if(strequal(device, "cf-ad9361-lpc")) {
		return ch_read_adc_attr(channel, ch_out, attr, buf, len);
	}

	return -ENOENT;
}

/**
 * write channel attribute
 * @param *device name
 * @param *channel name
 * @param *ch_out type: input/output
 * @param *attr name
 * @param *buff where value is stored
 * @param len length of the value
 * @return length of chars written to attribute, negative value in case of failure
 */
static ssize_t ch_write_attr(const char *device, const char *channel,
			     bool ch_out, const char *attr, const char *buf, size_t len)
{
	if (!supporter_dev(device))
		return -ENODEV;
	if(strequal(device, "ad9361-phy")) {
		return ch_write_phy_attr(channel, ch_out, attr, buf, len);
	} else if(strequal(device, "cf-ad9361-dds-core-lpc")) {
		return ch_write_dac_attr(channel, ch_out, attr, buf, len);
	} else if(strequal(device, "cf-ad9361-lpc")) {
		return ch_write_adc_attr(channel, ch_out, attr, buf, len);
	}

	return -ENOENT;
}

/**
 * open device
 * @param *device name
 * @param sample_size
 * @param mask
 * @return 0 on success, otherwise negative value
 */
static int32_t open_dev(const char *device, size_t sample_size, uint32_t mask)
{
	if (!supporter_dev(device))
		return -ENODEV;

	if (mask & ~input_channel_mask)
		return -ENOENT;

	request_mask = mask;

	return 0;
}

/**
 * close device
 * @param *device name
 * @return 0 on success, otherwise negative value
 */
static int32_t close_dev(const char *device)
{
	return supporter_dev(device) ? 0 : -ENODEV;
}

/**
 * get device mask, this specifies the channels that are used
 * @param *device name
 * @param *device mask
 * @return 0 on success, otherwise negative value
 */
static int32_t get_mask(const char *device, uint32_t *mask)
{
	if (!supporter_dev(device))
		return -ENODEV;
	*mask = input_channel_mask; /*  this way client has to do demux of data */

	return 0;
}

const struct tinyiiod_ops ops = {
	/* communication */
#ifdef UART_INTERFACE
	.read = serial_read,
	.read_line = serial_read_line,
	.write = serial_write_data,
#endif /* UART_INTERFACE */

	/* device operations */
	.read_attr = read_attr,
	.write_attr = write_attr,
	.ch_read_attr = ch_read_attr,
	.ch_write_attr = ch_write_attr,
	.transfer_dev_to_mem = transfer_dev_to_mem,
	.read_data = read_dev,
	.transfer_mem_to_dev = transfer_mem_to_dev,
	.write_data = write_dev,

	.open = open_dev,
	.close = close_dev,
	.get_mask = get_mask,
};

struct tinyiiod * ad9361_tinyiiod_create()
{
	tinyiiod_adc_configure(ADC_DDR_BASEADDR);
	tinyiiod_dac_configure(DAC_DDR_BASEADDR);
	return tinyiiod_create(xml, &ops);
}