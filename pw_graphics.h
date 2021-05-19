#ifndef CONSOLE_GRAPHICS2_H
#define CONSOLE_GRAPHICS2_H

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>

/*Ryan Homsani
 *
/*September 8, 2018
 *C-compatible library for the console graphics. Uses less statically allocated memory
 *Because it will use an object to store what is drawn, the ScreenLayer type will not 
 *be used.
 *
 *September 22, 2018
 *Finished writing pw_sprite_create, pw_get_image_clip, pw_sprite_read, pw_sprite_write,
 *pw_sprite_read_by_filename, pw_sprite_write_by_filename, pw_draw_str_wrapped, pw_draw_context.
 */

typedef struct PWGraphicsContext{
	int width;
	int height;
	char *screen;
	int last_render_frame;
	char *last_render;
	int framerate;
	char background;
}PWGraphicsContext;

typedef struct PWSprite{
	int id;/*deprecated*/
	int width;
	int height;
	char is_animation;
	char is_loop;/*deprecated*/
	int frame_duration;
	int animation_size;
	char *sprite;
}PWSprite;

PWGraphicsContext *PWG_get_context(int framerate, int width, int height, char background);
void PWG_free_context(PWGraphicsContext *ctx);
void PWG_render(PWGraphicsContext *ctx);
int  PWG_draw_bytes(PWGraphicsContext *ctx, const char* str, int x, int y, int width, int height, int frame);
int PWG_draw_str(PWGraphicsContext *ctx, const char* str, int x, int y, int width, int height, int length);
void PWG_draw_str_wrapped(PWGraphicsContext *ctx, const char* str, int x, int y, int width, int height, int length);
void PWG_draw_area(PWGraphicsContext *ctx, int val, int x, int y, int width, int height);
void PWG_draw_sqr(PWGraphicsContext *ctx, int val, int x, int y, int width, int height);
void PWG_draw_line(PWGraphicsContext *ctx, int val, int x, int y, int x2, int y2);
int  PWG_draw_image(PWGraphicsContext *ctx, PWSprite* s, int x, int y, int frame);
void PWG_draw_image_clip(PWGraphicsContext *ctx, PWSprite* s, int x, int y, int x_from_sprite, int y_from_sprite, int width, int height, int frame);
void PWG_draw_int_left_end(PWGraphicsContext *ctx, int num, int x, int y);
void PWG_draw_int_right_end(PWGraphicsContext *ctx, int num, int x, int y);
void PWG_draw_context(PWGraphicsContext *ctx, const PWGraphicsContext *ctx2, int x, int y, int x_from_ctx2, int y_from_ctx2, int width, int height);
int PWG_current_frame(PWGraphicsContext *ctx);
const char* PWG_last_render(PWGraphicsContext *ctx);
int PWG_get_last_render_frame(PWGraphicsContext *ctx);

PWSprite *PWG_sprite_create(int id, int width, int height, const char* sprite, char is_animation,
			char is_loop, int frame_duration, int animation_size);
void PWG_free_sprite(PWSprite *s);
PWSprite *PWG_get_image_clip(PWGraphicsContext *ctx, int x, int y, int width, int height);
PWSprite *PWG_sprite_read(FILE *in);
void PWG_sprite_write(FILE *out, PWSprite *s);
PWSprite *PWG_sprite_read_by_filename(const char *filename);
void PWG_sprite_write_by_filename(const char *filename, PWSprite *s);

#endif