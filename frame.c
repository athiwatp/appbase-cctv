/*
 * frame.c
 *
 *  Created on: 28 Jun 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <stdio.h>
#include <string.h>
#include <jpeglib.h>
#include "main.h"
#include "utils.h"
#include "frame.h"

static void convert_to_jpeg(const unsigned char *data_in, size_t len_in,
		size_t width, size_t height,
		unsigned char **data_out, size_t *len_out)
{
#define JPEG_QUALITY 95
	struct jpeg_compress_struct info;
	struct jpeg_error_mgr error;
	unsigned char *line = ec_malloc(width * 3), *ptr;

	info.err = jpeg_std_error(&error);
	jpeg_create_compress(&info);
	jpeg_mem_dest(&info, data_out, len_out);

	info.image_width = width;
	info.image_height = height;
	info.input_components = 3;
	info.in_color_space = JCS_YCbCr;

	jpeg_set_defaults(&info);
	jpeg_set_quality(&info, JPEG_QUALITY, true);

	jpeg_start_compress(&info, true);
	while (info.next_scanline < info.image_height) {
		ptr = line;

		for (int x = 0, z = 0; x < width; x++) {
			int y, u, v;

			y = (z == 0 ? data_in[0] : data_in[2]);
			u = data_in[1];
			v = data_in[3];

			*(ptr++) = y;
			*(ptr++) = u;
			*(ptr++) = v;

			if (z++) {
				z = 0;
				data_in += 4;
			}
		}

		jpeg_write_scanlines(&info, (JSAMPARRAY) &line, 1);
	}

	jpeg_finish_compress(&info);
	jpeg_destroy_compress(&info);

	free(line);
#undef JPEG_QUALITY
}

/*
 * Overwrite 'frame_data' with the new image data in JPEG,
 * and also overwrite 'frame_bytes_used' accordingly.
 * We assume a JPEG image will always need less space than a YUYV one,
 * so we shouldn't need to realloc 'frame_data', but we account for this
 * just in case.
 */
void frame_convert_yuyv_to_jpeg(struct frame *f)
{
	/* jpeglib expects these values initialized */
	unsigned char *jpeg_frame = NULL;
	size_t jpeg_frame_len = 0;

	if (!f || !f->frame_data || !f->frame_bytes_used || !f->frame_size || !f->width || !f->height)
		return;

	convert_to_jpeg(f->frame_data, f->frame_bytes_used,
			f->width, f->height,
			&jpeg_frame, &jpeg_frame_len);

	if (jpeg_frame_len <= f->frame_size) {
		memcpy(f->frame_data, jpeg_frame, jpeg_frame_len);
		f->frame_bytes_used = jpeg_frame_len;
	} else {
		/* We need to realloc */
		free(f->frame_data);
		f->frame_data = ec_malloc_fill(jpeg_frame_len, (char *) jpeg_frame);
		f->frame_size = jpeg_frame_len;
		f->frame_bytes_used = jpeg_frame_len;
	}
}
