/*
 * sdl-window.c - Window management with SDL
 *
 *  Routines for window creation and image rendering
 *  using the SDL cross-platform library.
 *
 *  Created on: 23 Jun 2016
 *      Author: ajuaristi <a@juaristi.eus>
 */
#include <limits.h>
#include <linux/videodev2.h>
#include "SDL.h"
#include "utils.h"
#include "window.h"

#define WINDOW_TITLE "Appbase CCTV (by ajuaristi)"

struct window_internal {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
};

static void sdl_close(struct window_internal *wi)
{
	if (wi->window)
		SDL_DestroyWindow(wi->window);
	if (wi->renderer)
		SDL_DestroyRenderer(wi->renderer);
	if (wi->texture)
		SDL_DestroyTexture(wi->texture);
	SDL_Quit();
}

/* TODO implement this */
static bool sdl_render(struct frame *f)
{
	return true;
}

static bool sdl_is_closed()
{
	return SDL_QuitRequested();
}

struct window *start_window(size_t width, size_t height, int format)
{
	Uint32 sdl_pixelformat;
	struct window *w = ec_malloc(sizeof(struct window));
	struct window_internal *wi = ec_malloc(sizeof(struct window_internal));

	w->internal = wi;
	memset(wi, 0, sizeof(struct window_internal));

	/*
	 * SDL_CreateWindow() expects signed int values for width and height
	 * whereas we're using unsigned size_t values throughout the code.
	 * For the format, we also only support YUYV for now.
	 */
	if (width == 0 || height == 0 || width > INT_MAX || height > INT_MAX || format != V4L2_PIX_FMT_YUYV)
		goto fail;
	sdl_pixelformat = SDL_PIXELFORMAT_YUY2;

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		goto fail;

	wi->window = SDL_CreateWindow(WINDOW_TITLE,
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			(int) width,
			(int) height,
			SDL_WINDOW_SHOWN);
	if (!wi->window)
		goto fail_uninitialize;

	wi->renderer = SDL_CreateRenderer(wi->window, -1, SDL_RENDERER_ACCELERATED);
	if (!wi->renderer)
		goto fail_uninitialize;

	wi->texture = SDL_CreateTexture(wi->renderer, sdl_pixelformat,
			SDL_TEXTUREACCESS_STREAMING,
			(int) width,
			(int) height);
	if (!wi->texture)
		goto fail_uninitialize;

	w->render = sdl_render;
	w->is_closed = sdl_is_closed;

	return w;

fail_uninitialize:
	sdl_close(wi);

fail:
	free(wi);
	free(w);
	return NULL;
}

void destroy_window(struct window *w)
{
	if (w && w->internal) {
		sdl_close(w->internal);

		free(w->internal);
		free(w);
	}
}
