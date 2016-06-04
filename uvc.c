/*
 * uvc.c
 *
 *  Created on: 3 Jun 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <stdlib.h>
#include <string.h>
#include "uvc.h"
#include "utils.h"

struct camera {
	char *dev_path;
};

struct camera *uvc_open()
{
	char default_dev_path[] = "/dev/video0";
	struct camera *c = ec_malloc(sizeof(struct camera));

	c->dev_path = ec_malloc(sizeof(default_dev_path));
	memcpy(c->dev_path, default_dev_path, sizeof(default_dev_path));

	return c;
}

struct frame *uvc_capture_frame()
{
	char test_data[] = "foo";
	struct frame *f = ec_malloc(sizeof(struct frame));

	f->length = sizeof(test_data);
	f->data = ec_malloc(f->length);
	memcpy(f->data, test_data, f->length);

	return f;
}

void uvc_close(struct camera *c)
{
	if (c) {
		if (c->dev_path)
			free(c->dev_path);
		free(c);
	}
}
