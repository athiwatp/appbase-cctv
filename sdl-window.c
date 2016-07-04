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
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "utils.h"
#include "window.h"

#define WINDOW_TITLE "Appbase CCTV (by ajuaristi)"

struct window {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
};

static void sdl_close(struct window *w)
{
	if (w->window)
		SDL_DestroyWindow(w->window);
	if (w->renderer)
		SDL_DestroyRenderer(w->renderer);
	if (w->texture)
		SDL_DestroyTexture(w->texture);
	SDL_Quit();
}

bool window_is_closed()
{
	return SDL_QuitRequested();
}

struct window *start_window(size_t width, size_t height, int format)
{
//	Uint32 sdl_pixelformat;
	struct window *w = ec_malloc(sizeof(struct window));

	/*
	 * SDL_CreateWindow() expects signed int values for width and height
	 * whereas we're using unsigned size_t values throughout the code.
	 * For the format, we also only support YUYV for now.
	 */
	if (width == 0 || height == 0 || width > INT_MAX || height > INT_MAX || format != V4L2_PIX_FMT_YUYV)
		goto fail;
//	sdl_pixelformat = SDL_PIXELFORMAT_YUY2;

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		goto fail;

	w->window = SDL_CreateWindow(WINDOW_TITLE,
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			(int) width,
			(int) height,
			SDL_WINDOW_SHOWN);
	if (!w->window)
		goto fail_uninitialize;

	w->renderer = SDL_CreateRenderer(w->window, -1, SDL_RENDERER_ACCELERATED);
	if (!w->renderer)
		goto fail_uninitialize;

//	wi->texture = SDL_CreateTexture(wi->renderer, sdl_pixelformat,
//			SDL_TEXTUREACCESS_STREAMING,
//			(int) width,
//			(int) height);
//	if (!wi->texture)
//		goto fail_uninitialize;

//	w->render = sdl_render;
//	w->is_closed = sdl_is_closed;

	return w;

fail_uninitialize:
	sdl_close(w);

fail:
	free(w);
	return NULL;
}

/* TODO implement this */
bool window_render_frame(struct window *w, struct frame *f)
{
	SDL_Surface *s = IMG_Load("picture.0.jpg");
	if (!s || !w)
		return false;

	w->texture = SDL_CreateTextureFromSurface(w->renderer, s);
	SDL_FreeSurface(s);
	if (!w->texture)
		return false;

	SDL_RenderClear(w->renderer);
	SDL_RenderCopy(w->renderer, w->texture, NULL, NULL);
	SDL_RenderPresent(w->renderer);

	return true;
}

void destroy_window(struct window *w)
{
	if (w) {
		sdl_close(w);
		free(w);
	}
}
