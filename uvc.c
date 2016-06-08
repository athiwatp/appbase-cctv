/*
 * uvc.c
 *
 *  Created on: 3 Jun 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <stdlib.h>
#include <string.h>
#include <linux/videodev2.h>
#include "uvc.h"
#include "utils.h"

struct camera *uvc_open()
{
	char default_dev_path[] = "/dev/video0";
	struct camera *c = ec_malloc(sizeof(struct camera));

	c->dev_path = ec_malloc(sizeof(default_dev_path));
	memcpy(c->dev_path, default_dev_path, sizeof(default_dev_path));

	c->frame = NULL;
	c->frame_size = 0;

	return c;
}

bool uvc_alloc_frame(struct camera *c, int format,
		size_t width,
		size_t height)
{
	/*
	 * We only allow V4L2_PIX_FMT_YUYV for now.
	 * It's easy to extend this API in the future, but
	 * keep in mind the calculations that follow are all performed
	 * assuming YUYV format is being used.
	 */
	if (!c || format != V4L2_PIX_FMT_YUYV)
		goto fail;

	c->frame_size = width * height * 2;
	c->frame = ec_malloc(c->frame_size);

	return true;

fail:
	return false;
}

bool uvc_capture_frame(struct camera *c)
{
	char test_data[] = "foo";
	size_t size = 0;

	if (!c || !c->frame || c->frame_size <= 0)
		return false;

	size = (c->frame_size < sizeof(test_data) ?
			c->frame_size :
			sizeof(test_data));
	memcpy(c->frame, test_data, size);

	return true;
}

void uvc_close(struct camera *c)
{
	if (c) {
		if (c->dev_path)
			free(c->dev_path);
		if (c->frame)
			free(c->frame);
		free(c);
	}
}
