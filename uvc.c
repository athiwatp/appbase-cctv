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
	bool is_streaming;
	struct v4l2_requestbuffers reqbufs;
	char **buffers;
	size_t *buflens;
};

static bool uvc_setup_format(struct camera_internal *c,
		size_t *width, size_t *height,
		int format)
{
	struct v4l2_format fmt;

	memset(&fmt, 0, sizeof(fmt));

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = (*width);
	fmt.fmt.pix.height = (*height);
	fmt.fmt.pix.pixelformat = format;
	fmt.fmt.pix.field = V4L2_FIELD_ANY;
	if (ioctl(c->fd, VIDIOC_S_FMT, &fmt) < 0)
		return false;

	/* Replace with actual width and height */
	*width = fmt.fmt.pix.width;
	*height = fmt.fmt.pix.height;

	return true;
}

static void uvc_unmap_buffers(struct camera_internal *c)
{
	struct v4l2_requestbuffers *rb = &c->reqbufs;

	for (int i = 0; i < rb->count; i++) {
		if (c->buffers[i])
			munmap(c->buffers[i], c->buflens[i]);
	}

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
	c->buffers = ec_malloc(rb->count * sizeof(char *));
	c->buflens = ec_malloc(rb->count * sizeof(size_t));

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
	if (c->buffers)
		uvc_unmap_buffers(c);

	return false;
}

static bool uvc_start_streaming(struct camera_internal *c)
{
	if (!c->is_streaming) {
		if (ioctl(c->fd, VIDIOC_STREAMON, &c->reqbufs.type) < 0)
			return false;
		c->is_streaming = true;
	}

	return true;
}

static void uvc_stop_streaming(struct camera_internal *c)
{
	if (c->is_streaming) {
		ioctl(c->fd, VIDIOC_STREAMOFF, &c->reqbufs.type);
		c->is_streaming = false;
	}
}

struct camera *uvc_open()
{
	char default_dev_path[] = "/dev/video0";
	struct v4l2_capability cap;
	struct camera *c = ec_malloc(sizeof(struct camera));

	c->dev_path = ec_malloc(sizeof(default_dev_path));
	c->internal = ec_malloc(sizeof(struct camera_internal));
	memcpy(c->dev_path, default_dev_path, sizeof(default_dev_path));

	c->frame = NULL;
	c->frame_size = 0;
	c->internal->is_streaming = false;

	c->internal->fd = open(c->dev_path, O_RDWR);
	if (c->internal->fd == -1)
		goto abort;

	if (ioctl(c->internal->fd, VIDIOC_QUERYCAP, &cap) < 0)
		goto abort;

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
		!(cap.capabilities & V4L2_CAP_STREAMING))
		goto abort;

	return c;

abort:
	if (c->internal->fd != -1)
		close(c->internal->fd);
	free(c->internal);
	free(c->dev_path);
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
	c->frame_bytes_used = 0;

	/* Map frame buffers into userspace */
	if (!uvc_map_buffers(c->internal))
		goto fail;

	/*
	 * Finally, activate streaming.
	 * This will tell the driver to create the incoming and outgoing queues,
	 * and will leave the camera ready for capturing frames.
	 * Subsequent calls to uvc_capture_frame() will fail at ioctl VIDIOC_DQBUF
	 * if we don't do this.
	 */
	if (!uvc_start_streaming(c->internal))
		goto fail;

	return true;

fail:
	return false;
}

bool uvc_capture_frame(struct camera *c)
{
	size_t size = 0;
	struct v4l2_buffer buf;

	/*
	 * Same as in uvc_alloc_frame(): we only support V4L2_PIX_FMT_YUYV for now.
	 */
	if (!c || !c->internal || !c->frame || c->frame_size <= 0 || c->format != V4L2_PIX_FMT_YUYV)
		goto fail;

	memset(&buf, 0, sizeof(buf));

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (ioctl(c->internal->fd, VIDIOC_DQBUF, &buf) < 0)
		goto fail;

	/* Copy time when first byte was captured */
	memcpy(&c->capture_time, &buf.timestamp, sizeof(struct timeval));


	/* Copy frame bytes */
	size = (buf.bytesused < c->frame_size ?
			buf.bytesused :
			c->frame_size);
	memcpy(c->frame, c->internal->buffers[buf.index], size);
	c->frame_bytes_used = size;

	if (ioctl(c->internal->fd, VIDIOC_QBUF, &buf) < 0)
		goto fail;

	return true;

fail:
	return false;
}

void uvc_close(struct camera *c)
{
	if (c) {
		if (c->internal) {
			uvc_stop_streaming(c->internal);

			if (c->internal->buffers)
				uvc_unmap_buffers(c->internal);

			close(c->internal->fd);
			free(c->internal);
		}
		if (c->dev_path)
			free(c->dev_path);
		if (c->frame)
			free(c->frame);
		free(c);
	}
}
