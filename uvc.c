/*
 * uvc.c
 *
 *  Created on: 3 Jun 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include "uvc.h"
#include "utils.h"

#define NUM_REQUESTED_BUFS	16
#define NUM_MIN_BUFS		2

struct camera_internal {
	int fd;
	struct v4l2_capability cap;
	struct v4l2_format format;
	struct v4l2_requestbuffers reqbufs;
	char **buffers;
	size_t *buflens;
};

static bool uvc_setup_format(struct camera_internal *c,
		size_t *width, size_t *height,
		int format)
{
	struct v4l2_format *fmt = &c->format;

	memset(fmt, 0, sizeof(struct v4l2_format));

	fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt->fmt.pix.width = (*width);
	fmt->fmt.pix.height = (*height);
	fmt->fmt.pix.pixelformat = format;
	fmt->fmt.pix.field = V4L2_FIELD_ANY;
	if (ioctl(c->fd, VIDIOC_S_FMT, fmt) < 0)
		return false;

	/* Replace with actual width and height */
	*width = fmt->fmt.pix.width;
	*height = fmt->fmt.pix.height;

	return true;
}

static void uvc_unmap_buffers(struct camera_internal *c)
{
	struct v4l2_requestbuffers *rb = &c->reqbufs;

	for (int i = 0; i < rb->count; i++)
		munmap(c->buffers[i], c->buflens[i]);

	free(c->buffers);
	free(c->buflens);

	c->buffers = NULL;
	c->buflens = NULL;
}

static bool uvc_map_buffers(struct camera_internal *c)
{
	struct v4l2_requestbuffers *rb = &c->reqbufs;
	struct v4l2_buffer buf;

	memset(rb, 0, sizeof(struct v4l2_requestbuffers));
	memset(&buf, 0, sizeof(buf));

	c->buffers = ec_malloc(rb->count * sizeof(char *));
	c->buflens = ec_malloc(rb->count * sizeof(size_t));

	/*
	 * Request NUM_REQUESTED_BUFS buffers, and accept
	 * a minimun of NUM_MIN_BUFS.
	 */
	rb->count = NUM_REQUESTED_BUFS;
	rb->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	rb->memory = V4L2_MEMORY_MMAP;
	if (ioctl(c->fd, VIDIOC_REQBUFS, rb) < 0 ||
		rb->count < NUM_MIN_BUFS)
		goto fail;

	/* Now map the buffers in userspace */
	for (int buf_index = 0; buf_index < rb->count; buf_index++) {
		/* Tell the driver to allocate a new buffer in kernel... */
		buf.index = buf_index;
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		if (ioctl(c->fd, VIDIOC_QUERYBUF, &buf) < 0)
			goto fail;

		/* ...and map it in userspace */
		c->buffers[buf_index] = mmap(NULL,
				buf.length,
				PROT_READ, MAP_SHARED, c->fd,
				buf.m.offset);
		if (c->buffers[buf_index] == MAP_FAILED)
			goto fail;
		c->buflens[buf_index] = buf.length;

		if (ioctl(c->fd, VIDIOC_QBUF, &buf) < 0)
			goto fail;
	}

	return true;

fail:
	fprintf(stderr, "ERROR: %d\n", errno);
	perror(NULL);
	if (c->buffers)
		uvc_unmap_buffers(c);

	return false;
}

struct camera *uvc_open()
{
	char default_dev_path[] = "/dev/video0";
	struct camera *c = ec_malloc(sizeof(struct camera));

	c->dev_path = ec_malloc(sizeof(default_dev_path));
	c->internal = ec_malloc(sizeof(struct camera_internal *));
	memcpy(c->dev_path, default_dev_path, sizeof(default_dev_path));

	c->frame = NULL;
	c->frame_size = 0;

	c->internal->fd = open(c->dev_path, O_RDWR);
	if (c->internal->fd == -1)
		goto abort;

	if (ioctl(c->internal->fd, VIDIOC_QUERYCAP, &c->internal->cap) < 0)
		goto abort;

	if (!(c->internal->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
		!(c->internal->cap.capabilities & V4L2_CAP_STREAMING))
		goto abort;

	return c;

abort:
	if (c->internal->fd != -1)
		close(c->internal->fd);
	free(c->internal);
	free(c);

	return NULL;
}

bool uvc_alloc_frame(struct camera *c)
{
	/*
	 * We only allow V4L2_PIX_FMT_YUYV for now.
	 * It's easy to extend this API in the future, but
	 * keep in mind the calculations that follow are all performed
	 * assuming YUYV format is being used.
	 */
	if (!c || !c->internal || c->format != V4L2_PIX_FMT_YUYV)
		goto fail;

	/* Set up desired frame format */
	if (!uvc_setup_format(c->internal, &c->width, &c->height, c->format))
		goto fail;

	c->frame_size = c->width * c->height * 2;
	if (c->frame_size == 0)
		goto fail;
	c->frame = ec_malloc(c->frame_size);

	/* Map frame buffers into userspace */
	if (!uvc_map_buffers(c->internal))
		goto fail;

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
		if (c->internal) {
			if (c->internal->buffers)
				uvc_unmap_buffers(c->internal);

			close(c->internal->fd);
			//free(c->internal);
		}
		if (c->dev_path)
			free(c->dev_path);
		if (c->frame)
			free(c->frame);
		free(c);
	}
}
