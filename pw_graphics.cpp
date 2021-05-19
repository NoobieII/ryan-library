/*April 14, 2019
 *revised pw_draw_bytes and pw_draw_image
 *Now they are fixed from segmentation fault bug and return an int,
 *0 for success and -1 on failure.
 
 *April 17, 2019
 *made function prefixes PWG for consistency and less vagueness
 *
 *September 21, 2020
 *fixed a bug in PWG_draw_area
 */
 

#include "pw_graphics.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

PWGraphicsContext *PWG_get_context(int framerate, int width, int height, char background){
	PWGraphicsContext *ctx = (PWGraphicsContext*) malloc(sizeof(PWGraphicsContext));
	memset(ctx, 0, sizeof(PWGraphicsContext));
	
	ctx->width = width;
	ctx->height = height;
	ctx->screen = (char*) malloc(width * height);
	memset(ctx->screen, background, width * height);
	ctx->last_render_frame = 0;
	ctx->last_render = (char*) malloc(width * height);
	memset(ctx->last_render, background, width * height);
	ctx->framerate = framerate;
	ctx->background = background;
	
	return ctx;
}

void PWG_free_context(PWGraphicsContext *ctx){
	free(ctx->screen);
	free(ctx->last_render);
	free(ctx);
}

void PWG_render(PWGraphicsContext *ctx){
	int i;
	char *tmp;
	for(i = 0; i < ctx->height; ++i){
		ctx->screen[i * ctx->width + ctx->width - 1] = '\n';
	}
	fwrite("\n", 1, 1, stdout);
	fwrite(ctx->screen, ctx->width * ctx->height - 1, 1, stdout);
	
	/*make the rendered array the last rendered array*/
	tmp = ctx->screen;
	ctx->screen = ctx->last_render;
	ctx->last_render = tmp;
	
	ctx->last_render_frame = ctx->framerate * clock() / CLOCKS_PER_SEC;
	
	/*add the null terminating string*/
	/*ctx->last_render[ctx->width * ctx->height - 1] = '\0';*/
	/*make the screen being operated on blank*/
	memset(ctx->screen, ctx->background, ctx->width * ctx->height);
}

int PWG_draw_bytes(PWGraphicsContext *ctx, const char* str, int x, int y, int width, int height, int frame){
	/*
	int i, j;
	char *screen_str = ctx->screen + y * ctx->width + x;
	const char *byte = str + frame * width * height;

	for(j = 0; j < height && y + j < ctx->height; ++j){
		if(j < 0){
			continue;
		}
		for(i = 0; i < width && x + i < ctx->width; ++i){
			if(i < 0){
				continue;
			}
			if(*screen_str == 0){
				continue;
			}
			screen_str[i] = byte[i];
		}
		screen_str += ctx->width;
		byte += width;
	}
	*/
	/*find the bounds of the image to draw*/
	int bound_top, bound_bottom, bound_left, bound_right;
	int i,j;
	char *screen_buf = NULL;
	const char *sprite_buf = NULL;
	
	if(ctx == NULL){
		return -1;
	}
	
	bound_top = y;
	bound_bottom = y + height;
	bound_left = x;
	bound_right = x + width;
	
	/*this implies that none of the image is within the screen bounds*/
	if(bound_bottom < 1){
		return 0;
	}
	if(bound_top >= ctx->height){
		return 0;
	}
	if(bound_right < 1){
		return 0;
	}
	if(bound_left >= ctx->width){
		return 0;
	}
	if(bound_bottom > ctx->height){
		bound_bottom = ctx->height;
	}
	if(bound_top < 0){
		bound_top = 0;
	}
	if(bound_right > ctx->width){
		bound_right = ctx->width;
	}
	if(bound_left < 0){
		bound_left = 0;
	}
	
	screen_buf = ctx->screen + bound_top * ctx->width + bound_left;

	sprite_buf = str + (bound_top - y) * width + (bound_left - x)
				+ height * width * frame;
	
	for(i = bound_top; i < bound_bottom; ++i){
		/*memcpy(screen_buf, sprite_buf, bound_right - bound_left);*/
		for(j = 0; j < bound_right - bound_left; ++j){
			if(sprite_buf[j]) screen_buf[j] = sprite_buf[j];
		}
		screen_buf += ctx->width;
		sprite_buf += bound_right - bound_left;
	}
	return 0;
}

int PWG_draw_str(PWGraphicsContext *ctx, const char* str, int x, int y, int width, int height, int length){
	int offset_x = 0, offset_y = 0, i = 0, is_character = 0;
	char *screen_str = ctx->screen + y * ctx->width + x;
	
	if((!ctx) || (!str)){
		return -1;
	}
	
	while(str[i] && i < length){
		is_character = 0;
		
		switch(str[i]){
		case '\n':
		case '\r':
		case '\t':
			break;
		case '\0':
			return 0;
			break;
		default:
			is_character = 1;
		}
		
		if(is_character){
			/*check the bounds*/
			if((y + offset_y) >= 0 && (x + offset_x) >= 0 && 
				(y + offset_y) < (ctx->height) && (x + offset_x) < (ctx->width)){
				/*ctx->screen[(y + offset_y) * ctx->width + x + offset_x] = str[i];*/
				screen_str[offset_x] = str[i];
			}
		}
		
		/*check the offsets for the character*/
		switch(str[i]){
		case '\n':
			offset_y++;
			screen_str += ctx->width;
			break;
		case '\r':
			offset_x = 0;
			break;
		case '\t':
			offset_x += 4;
			break;
		case '\0':
			break;
		default:
			offset_x++;
			is_character = 1;
			break;
		}
		
		/*check the offset and adjust if necessary*/
		if(offset_x >= width){
			offset_y++;
			screen_str += ctx->width;
			offset_x = 0;
		}
		if(offset_y >= height){
			return 0;
		}
		
		i++;
	}
}

void PWG_draw_str_wrapped(PWGraphicsContext *ctx, const char* str, int x, int y, int width, int height, int length){
	int offset_x = 0, offset_y = 0, is_character = 0, word_length = 0;
	char *word_buf = 0;
	char *screen_str = ctx->screen + ctx->width * y + x;
	int i = 0, word_index;
	
	word_buf = (char*) malloc(256);
	
	/*in each iteration, print the entire word at a time, or whitespace one at a time*/
	while(str[i] && i < length){
		is_character = 0;
		
		switch(str[i]){
		case '\n':
		case '\r':
		case '\t':
		case ' ':
			break;
		case '\0':
			return;
			break;
		default:
			is_character = 1;
		}
		
		/*if a character is read, then print the entire word (non-whitespace)*/
		if(is_character){
			sscanf(str + i, "%s", word_buf);
			word_length = strlen(word_buf);
			/*if it fits in the current line and the current line is within bounds*/
			if((offset_x + word_length - 1) < width){
				/*iterate by a separate variable word_index*/
				for(word_index = 0; word_index < word_length; ++word_index, ++offset_x){
					/*for each character, check...
					 *
					 * -within text box height... offset_y < height
					 * -within context width... x + offset_x >= 0 && x + offset_x < ctx->width
					 * -within context height... y + offset_y >= 0 && y + offset_y < ctx->height
					 *
					 * NO within text box width... offset_x < width ...BECAUSE by default
					 * the text will wrap from right back to left:
					 *
					 *	offset_x = 0;
					 *	offset_y++;
					 */
					
					/*here the text box width doesn't have to be checked because it fits in the line*/
					if(offset_y >= height || (x + offset_x) < 0 || (x + offset_x) >= (ctx->width) ||
						(y + offset_y) < 0 || (y + offset_y) >= (ctx->height)){
						continue;
					}
					screen_str[offset_x] = word_buf[word_index];
				}
			}
			/*if it doesn't fit the current line but can fit in the next line*/
			else if(word_length < width){
				offset_x = 0;
				offset_y++;
				screen_str += ctx->width;
				
				/*iterate by a separate variable word index*/
				for(word_index = 0; word_index < word_length; ++word_index, ++offset_x){
					/*here the text box width doesn't have to be checked*/
					if(offset_y >= height || (x + offset_x) < 0 || (x + offset_x) >= (ctx->width) ||
						(y + offset_y) < 0 || (y + offset_y) >= (ctx->height)){
						continue;
					}
					screen_str[offset_x] = word_buf[word_index];
				}
			}
			/*else the word spans more than one line. Start where the offset is currently at and
			 *continue on from there, in a raster from left to right*/
			else{
				for(word_index = 0; word_index < word_length; ++word_index, ++offset_x){
					/*test all conditions listed above*/
					if(offset_x >= width){
						offset_y++;
						screen_str += ctx->width;
						offset_x = 0;
					}
					if(offset_y >= height || (x + offset_x) < 0 || (x + offset_x) >= (ctx->width) ||
						(y + offset_y) < 0 || (y + offset_y) >= (ctx->height)){
						continue;
					}
					screen_str[offset_x] = word_buf[word_index];
				}
			}
			i += word_length;
		}
		/*else if it is a whitspace*/
		else{
			/*check the offsets for the character*/
			switch(str[i]){
			case '\n':
				offset_y++;
				screen_str += ctx->width;
				break;
			case '\r':
				offset_x = 0;
				break;
			case '\t':
				offset_x += 4;
				break;
			case ' ':
				offset_x++;
				break;
			}
			i++;
		}
		/*check the offset and adjust if necessary*/
		if(offset_x >= width){
			offset_y++;
			screen_str += ctx->width;
			offset_x = 0;
		}
		if(offset_y >= height){
			break;
		}
	}
	free(word_buf);
}

void PWG_draw_area(PWGraphicsContext *ctx, int val, int x, int y, int width, int height){
	int i;
	int j;
	int left = x, right = x + width;
	/*if the area is clipped on the left side*/
	if(x < 0){
		left = 0;
	}
	/*if the area is clipped on the right side*/
	if(x + width > ctx->width){
		right = ctx->width;
	}
	for(j = 0; j < height; ++j){
		/*if the area is clipped on the top edge*/
		if(j + y < 0){
			continue;
		}
		/*if the area is clipped on the bottom edge*/
		if(j + y > ctx->height){
			break;
		}
		for (i = left; i < right; ++i)
			ctx->screen[(y + j) * ctx->width + i] = val;
		//memset(ctx->screen + (y + j) * ctx->width + left, val, right - left);
	}
}

void PWG_draw_sqr(PWGraphicsContext *ctx, int val, int x, int y, int width, int height){
	int start_x, end_x, start_y, end_y, i;
	
	/*draw the top side of the square*/
	if(y > -1 && y < ctx->height){
		start_x = x;
		if(start_x > (ctx->width - 1)){
			return;
		}
		end_x = x + width - 1;
		if(start_x < 0){
			start_x = 0;
		}
		if(end_x > ctx->width){
			end_x = ctx->width;
		}
		for(i = start_x; i < end_x; ++i){
			ctx->screen[y * ctx->width + i] = val;
		}
	}
	/*draw the left side of the square*/
	if(x > -1 && x < ctx->width){
		start_y = y;
		if(start_y > (ctx->height - 1)){
			return;
		}
		end_y = y + height;
		if(start_y < 0){
			start_y = 0;
		}
		if(end_y > ctx->height){
			end_y = ctx->height;
		}
		for(i = start_y; i < end_y; ++i){
			ctx->screen[i * ctx->width + x] = val;
		}
	}
	
	/*draw the right side of the square*/
	if((x + width) > 0 && (x + width - 1) < ctx->width){
		start_y = y;
		end_y = y + height;
		if(start_y < 0){
			start_y = 0;
		}
		if(end_y > ctx->height){
			end_y = ctx->height;
		}
		for(i = start_y; i < end_y; ++i){
			ctx->screen[i * ctx->width + x + width - 1] = val;
		}
	}
	
	/*draw the bottom side of the square*/
	if((y + height) > 0 && (y + height - 1) < ctx->height){
		start_x = x;
		end_x = x + width;
		if(start_x < 0){
			start_x = 0;
		}
		if(end_x > ctx->width){
			end_x = ctx->width;
		}
		for(i = start_x; i < end_x; ++i){
			ctx->screen[(y + height - 1) * ctx->width + i] = val;
		}
	}
}

void PWG_draw_line(PWGraphicsContext *ctx, int val, int x, int y, int x2, int y2){
	/*start at (x, y)*/
	/*make the the parameters (x,y) have lower absolute value from origin*/

	/*determine whether x or y is the independent component when drawing the line*/
	char increment_by_y = 0;
	float slope;
	int i = 0; /*loop iterator*/printf("1");

	int temp;
	
	/*measure which axis to increment for slope*/
	if((y2 - y) * (y2 - y) > (x2 - x) * (x2 - x)){
		increment_by_y = 1;
	}
	
	/*needed for the loop to function correctly*/printf("2");
	if(increment_by_y && y > y2){
		temp = x;
		x = x2;
		x2 = temp;
		
		temp = y;
		y = y2;
		y2 = temp;
	}
	else if(x > x2){
		temp = x;
		x = x2;
		x2 = temp;
		
		temp = y;
		y = y2;
		y2 = temp;
	}
	
	/*the first if statement is to prevent division by zero for lines
	 *whose two ends are the same points*/printf("3");
	if(x == x2 && y == y2){
		slope = 1.0;
	}
	else if(increment_by_y){
		slope = (float)(x2 - x) / (y2 - y);
	}
	else{
		slope = (float)(y2 - y) / (x2 - x);
	}printf("4\n");
	
	if(increment_by_y){
		for(i = 0; (i <= y2 - y) && (y + i < ctx->height) && ((int)(x + i * slope) < ctx->width); ++i){
			/*if the point of the line has y < 0 or x < 0*/
			if((y + i < 0) || ((int)(x + i * slope) < 0)){
				continue;
			}
			ctx->screen[(y + i) * ctx->width + x + (int)(i * slope)] = val;
		}
	}
	else{
		for(i = 0; (i <= x2 - x) && (x + i < ctx->width) && ((int)(y + i * slope) < ctx->height); ++i){
			/*if the point of the line has y < 0 or x < 0*/
			if((x + i < 0) || ((int)(y + i * slope) < 0)){
				continue;
			}
			ctx->screen[((int)(y + i * slope)) * ctx->width + x + i] = val;
		}
	}
}

int PWG_draw_image(PWGraphicsContext *ctx, PWSprite* s, int x, int y, int frame){
	/*find the bounds of the image to draw*/
	int bound_top, bound_bottom, bound_left, bound_right;
	int i;
	char *screen_buf = NULL;
	char *sprite_buf = NULL;
	
	if(ctx == NULL || s == NULL){
		return -1;
	}
	
	bound_top = y;
	bound_bottom = y + s->height;
	bound_left = x;
	bound_right = x + s->width;
	
	/*this implies that none of the image is within the screen bounds*/
	if(bound_bottom < 1){
		return 0;
	}
	if(bound_top >= ctx->height){
		return 0;
	}
	if(bound_right < 1){
		return 0;
	}
	if(bound_left >= ctx->width){
		return 0;
	}
	if(bound_bottom > ctx->height){
		bound_bottom = ctx->height;
	}
	if(bound_top < 0){
		bound_top = 0;
	}
	if(bound_right > ctx->width){
		bound_right = ctx->width;
	}
	if(bound_left < 0){
		bound_left = 0;
	}
	
	screen_buf = ctx->screen + bound_top * ctx->width + bound_left;
	if(s->is_loop && s->is_animation){
		sprite_buf = s->sprite + (bound_top - y) * s->width + (bound_left - x)
					+ s->height * s->width * (((clock() * ctx->framerate) / (CLOCKS_PER_SEC)) % s->animation_size);
	}
	else{
		sprite_buf = s->sprite + (bound_top - y) * s->width + (bound_left - x)
					+ s->height * s->width * (frame % s->animation_size);
	}
	
	for(i = bound_top; i < bound_bottom; ++i){
		memcpy(screen_buf, sprite_buf, bound_right - bound_left);
		screen_buf += ctx->width;
		sprite_buf += bound_right - bound_left;
	}
	return 0;
}

static char draw_int_num_buf[16];

void PWG_draw_int_left_end(PWGraphicsContext *ctx, int num, int x, int y){
	char *screen_str = ctx->screen + y * ctx->width + x;
	int i;

	if(y >= ctx->height || y < 0){
		return;
	}
	
	sprintf(draw_int_num_buf, "%d", num);
	
	for(i = 0; draw_int_num_buf[i] && x + i < ctx->width; ++i){
		if(x + i < 0){
			continue;
		}
		*screen_str = draw_int_num_buf[i];
		screen_str++;
	}
}
	
void PWG_draw_int_right_end(PWGraphicsContext *ctx, int num, int x, int y){
	char *screen_str = ctx->screen + y * ctx->width + x;
	int i;

	if(y >= ctx->height || y < 0){
		return;
	}
	
	sprintf(draw_int_num_buf, "%d", num);
	
	for(i = strlen(draw_int_num_buf) - 1; i >= 0; --i){
		if(x + i >= ctx->width){
			continue;
		}
		*screen_str = draw_int_num_buf[i];
		screen_str++;
	}
}

void PWG_draw_image_clip(PWGraphicsContext *ctx, PWSprite* s, int x, int y,
	int x_from_sprite, int y_from_sprite, int width, int height, int frame){
	/*do bounds checking for both the graphics context and sprite*/
	int i, j;
	char *screen_str = ctx->screen + y * ctx->width + x;
	char *sprite_str;
	
	if(s->is_loop && s->is_animation){
		sprite_str = s->sprite + s->height * s->width * (((clock() * ctx->framerate) / (CLOCKS_PER_SEC)) % s->animation_size)
			+ s->width * y_from_sprite + x_from_sprite;
	}
	else{
		sprite_str = s->sprite + s->height * s->width * (frame % s->animation_size) + s->width * y_from_sprite + x_from_sprite;
	}
	
	for(j = 0; j < height && y + j < ctx->height && y_from_sprite + j < s->height; ++j){
		/*bounds checking for graphics context*/
		if(y + j < 0 || y_from_sprite + j < 0){
			screen_str += ctx->width;
			sprite_str += s->width;
			continue;
		}
		for(i = 0; i < width && x + i < ctx->width && x_from_sprite + i < s->width; ++i){
			if(x + i < 0 || x_from_sprite + i < 0){
				continue;
			}
			/*screen_str[(y + j) * ctx->width + x + i] = sprite_str[(y_from_sprite + j) * sprite->width + x_from_sprite + i];*/
			screen_str[i] = sprite_str[i];
		}
		screen_str += ctx->width;
		sprite_str += s->width;
	}
}

void PWG_draw_context(PWGraphicsContext *ctx, const PWGraphicsContext *ctx2, int x, int y,
	int x_from_ctx2, int y_from_ctx2, int width, int height){
	int i, j;
	char *screen_str = ctx->screen + y * ctx->width + x;
	const char *ctx_str = ctx2->screen + y_from_ctx2 * ctx2->width + x_from_ctx2;
	
	for(j = 0; j < height && y + j < ctx->height && y_from_ctx2 + j < ctx2->height; ++j){
		/*bounds checking for graphics context*/
		if(y + j < 0 || y_from_ctx2 + j < 0){
			screen_str += ctx->width;
			ctx_str += ctx2->width;
			continue;
		}
		for(i = 0; i < width && x + i < ctx->width && x_from_ctx2 + i < ctx2->width; ++i){
			if(x + i < 0 || x_from_ctx2 + i < 0){
				continue;
			}
			screen_str[i] = ctx_str[i];
		}
		screen_str += ctx->width;
		ctx_str += ctx2->width;
	}
}

int PWG_current_frame(PWGraphicsContext *ctx){
	return ctx->framerate * clock() / CLOCKS_PER_SEC;
}

int PWG_get_last_render_frame(PWGraphicsContext *ctx){
	return ctx->last_render_frame;
}

const char* PWG_last_render(PWGraphicsContext *ctx){
	return ctx->last_render;
}

PWSprite *get_image_clip(PWGraphicsContext *ctx, int x, int y, int width, int height){
	char *screen_str = ctx->screen + ctx->width * y + x;
	/*temporary variable to hold the characters before instantiation of the sprite*/
	char *sprite_str = 0;
	PWSprite *sprite = 0;
	
	int i, j;
	
	if(width < 0 || width > 255 || height < 0 || height > 255){
		return 0;
	}
	
	sprite_str = (char*) calloc(width * height, 1);
	
	for(j = 0; j < height && j + y < ctx->height; ++j){
		if(j < 0){
			continue;
		}
		
		for(i = 0; i < width && i + x < ctx->width; ++i){
			if(i < 0){
				continue;
			}
			sprite_str[i] = screen_str[i];
		}
		
		screen_str += ctx->width;
		sprite_str += width;
	}
	
	sprite = PWG_sprite_create(0, width, height, sprite_str, 0, 0, 1, 1);
	
	free(sprite_str);
	return sprite;
}

PWSprite *PWG_sprite_create(int id, int width, int height, const char* sprite, char is_animation,
			char is_loop, int frame_duration, int animation_size){
	PWSprite *pwsprite = 0;
	
	if(width < 0 || width > 255 || height < 0 || height > 255){
		return 0;
	}
	
	if(animation_size < 1){
		animation_size = 1;
	}
	if(frame_duration < 1){
		frame_duration = 1;
	}
	
	pwsprite = (PWSprite*) malloc(sizeof(PWSprite));
	
	pwsprite->id = id;
	pwsprite->width = height;
	pwsprite->height = height;
	pwsprite->sprite = (char*) malloc(width * height * animation_size);
	memcpy(pwsprite->sprite, sprite, width * height * animation_size);
	pwsprite->is_animation = is_animation;
	pwsprite->is_loop = is_loop;
	pwsprite->frame_duration = frame_duration;
	pwsprite->animation_size = animation_size;
	
	return pwsprite;
}

void PWG_free_sprite(PWSprite *s){
	free(s->sprite);
	free(s);
}

PWSprite *PWG_sprite_read(FILE *in){
	int id, width, height, frame_duration, animation_size;
	char *sprite_buf;
	char is_animation, is_loop;
	PWSprite *sprite = 0;
	
	fread(&id, sizeof(int), 1, in);
	fread(&width, sizeof(int), 1, in);
	fread(&height, sizeof(int), 1, in);
	fread(&is_animation, 1, 1, in);
	fread(&is_loop, 1, 1, in);
	fread(&frame_duration, sizeof(int), 1, in);
	fread(&animation_size, sizeof(int), 1, in);
	
	if(width * height * animation_size > 65535){
		return 0;
	}
	
	sprite_buf = (char*) malloc(width * height * animation_size);
	fread(sprite_buf, width * height * animation_size, 1, in);
	
	sprite = PWG_sprite_create(id, width, height, sprite_buf, is_animation, is_loop, frame_duration, animation_size);
	
	free(sprite_buf);
	return sprite;
}

void PWG_sprite_write(FILE *out, PWSprite *s){
	fwrite(&(s->id), sizeof(int), 1, out);
	fwrite(&(s->width), sizeof(int), 1, out);
	fwrite(&(s->height), sizeof(int), 1, out);
	fwrite(&(s->is_animation), 1, 1, out);
	fwrite(&(s->is_loop), 1, 1, out);
	fwrite(&(s->frame_duration), sizeof(int), 1, out);
	fwrite(&(s->animation_size), sizeof(int), 1, out);
	fwrite(s->sprite, s->width * s->height * s->animation_size, 1, out);
}
	
PWSprite *PWG_sprite_read_by_filename(const char *filename){
	PWSprite *sprite = 0;
	FILE *in = 0;
	
	in = fopen(filename, "rb");
	if(!in){
		return 0;
	}
	
	sprite = PWG_sprite_read(in);
	fclose(in);
	
	return sprite;
}

void PWG_sprite_write_by_filename(const char *filename, PWSprite *s){
	FILE *out = 0;
	
	out = fopen(filename, "wb");
	if(!out){
		return;
	}
	
	PWG_sprite_write(out, s);
	fclose(out);
}