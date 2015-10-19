/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <linux/fb.h>
#include <linux/kd.h>

#include <time.h>

#include "font_10x18.h"
#include "minui.h"
#include "graphics.h"

/*
 *get prop  include .h
 *ActionsCode(author:liaotianyang, type:change_code)
 */
#include "cutils/properties.h"


typedef struct {
    GRSurface* texture;
    int cwidth;
    int cheight;
} GRFont;

static GRFont* gr_font = NULL;
static minui_backend* gr_backend = NULL;

static int overscan_percent = OVERSCAN_PERCENT;
static int overscan_offset_x = 0;
static int overscan_offset_y = 0;

static int gr_vt_fd = -1;

static unsigned char gr_current_r = 255;
static unsigned char gr_current_g = 255;
static unsigned char gr_current_b = 255;
static unsigned char gr_current_a = 255;

static GRSurface* gr_draw = NULL;

/*
 *support rotate of framebuffer, define rotate, 0 = no, 1= 90, 2=180, 3=270
 *ActionsCode(author:liaotianyang, type:change_code)
 */
static int rotate  = 0;

/*
 *get lcd show rotate  by get prop
 *ActionsCode(author:liaotianyang, type:new_method)
 */
static void lcd_rotate_init(void)
{
	char prop_str[PROPERTY_VALUE_MAX];
	int  hw_rotate;
	prop_str[0]= 0;
	property_get("ro.lcd.rec_rotation", prop_str, NULL );	
	if( prop_str[0] >='0' && prop_str[0] <= '3' ) {
		printf("ro.lcd.rec_rotation=%s\n", prop_str);
		rotate = prop_str[0]-'0';
		return;
	}
	
	prop_str[0]= 0;
	property_get("ro.sf.default_rotation", prop_str, NULL );
	printf("ro.sf.default_rotation=%s\n", prop_str);
	if( prop_str[0] >='0' && prop_str[0] <= '3' )
		rotate = prop_str[0]-'0';
	
	prop_str[0]= 0;
	hw_rotate = 0;
	property_get("ro.sf.hwrotation", prop_str, NULL );
	printf("ro.sf.hwrotation=%s\n", prop_str);
	if( prop_str[0] >='0' && prop_str[0] <= '9' )
		hw_rotate = atoi(prop_str);	
	printf("hwrotation=%d\n", hw_rotate);

	rotate = (rotate+ hw_rotate/90)%4;
	printf("hwrotation=%d, rotate=%d\n", hw_rotate, rotate);
}


/*
 *pixle rgb to rgb565
 *ActionsCode(author:liaotianyang, type:new_method)
 */
static void  rgb_2_rgb565(unsigned char *dst, unsigned char r, unsigned char g, unsigned char b)
{
	unsigned short v;
	v = (r>>3) & 0x1f;
	v = (v << 6) | ((g >>2)&0x3f);
	v = (v << 5) | ((b >>3)&0x1f);
    dst[0] = v & 0xff ;
	dst[1] = (v>>8) & 0xff;
}

/*
 * now is suport rgb565 PIXEL_FORMAT, if want to suport else, welecome to change code
 *ActionsCode(author:liaotianyang, type:new_method)
 */
static int pixel_write_byColor(unsigned char *px )
{
	rgb_2_rgb565(px, gr_current_r, gr_current_g, gr_current_b);
	return 2;
}

/*
 *support rotate of framebuffer 270 
 *ActionsCode(author:liaotianyang, type:new_method)
 */
static void rotate270(char *dst, char *src, int sw, int sh, int pix_b )
{
	int x, y;
	for ( y = 0; y < sh; y++ ) 
		for ( x = 0; x < sw; x++)
			memcpy(dst+(x*sh+sh-1-y)*pix_b, src+(sw*y+x)*pix_b, pix_b);
			//dst[x*sh+sh-1-y] = src[sw*y+x];	
}

/*
 *support rotate of framebuffer 90
 *ActionsCode(author:liaotianyang, type:new_method)
 */
static void rotate90(char *dst, char *src, int sw, int sh, int pix_b )
{
	int x, y;
	for ( y = 0; y < sh; y++ ) 
		for ( x = 0; x < sw; x++)
			memcpy(dst+((sw-1-x)*sh+y)*pix_b, src+(sw*y+x)*pix_b, pix_b);
			//dst[(sw-1-x)*sh+y] = src[sw*y+x];	
}

/*
 *support rotate of framebuffer 180
 *ActionsCode(author:liaotianyang, type:new_method)
 */
static void rotate180(char *dst, char *src, int sw, int sh, int pix_b )
{
	int x, y;
	for(x = 0 ; x < sw ; x++)
		for(y = 0 ; y < sh; y++)
			memcpy(dst+(sw*y+x)*pix_b, src+(sw*(sh-y-1)+(sw-x-1))*pix_b, pix_b);
			//dst[sw  * y + x] = src [sw * (sh - y - 1) + (sw - x -1)];	

}

/*
 *support rotate of framebuffer, now suport lcd  rgb565  PIXEL_FORMAT
 *if want to suport else, welecome to change code
 *ActionsCode(author:liaotianyang, type:new_method)
 */
void roate_framebuf(char *dst, char *src)
{
	switch(rotate){
        case 0:
            memcpy(dst, src,  gr_draw->height * gr_draw->row_bytes);
            break; 
        case 1:  
        	rotate90(dst, src, gr_draw->width, gr_draw->height,gr_draw->pixel_bytes);
        	break;     		
        case 2:
			rotate180(dst, src, gr_draw->width, gr_draw->height,gr_draw->pixel_bytes);
            break; 
        case 3:
        	rotate270(dst, src, gr_draw->width, gr_draw->height,gr_draw->pixel_bytes);
        	break;             
        default :
         break;
    }  

}


static bool outside(int x, int y)
{
    return x < 0 || x >= gr_draw->width || y < 0 || y >= gr_draw->height;
}

int gr_measure(const char *s)
{
    return gr_font->cwidth * strlen(s);
}

void gr_font_size(int *x, int *y)
{
    *x = gr_font->cwidth;
    *y = gr_font->cheight;
}

static void text_blend(unsigned char* src_p, int src_row_bytes,
                       unsigned char* dst_p, int dst_row_bytes,
                       int width, int height)
{
    int i, j;
    for (j = 0; j < height; ++j) {
        unsigned char* sx = src_p;
        unsigned char* px = dst_p;
        for (i = 0; i < width; ++i) {			
            unsigned char a = *sx++;
			/*
			*now suport lcd  rgb565  PIXEL_FORMAT,if want to suport else, welecome to change code
			*ActionsCode(author:liaotianyang, type:change_code)
			*/			
			if (gr_draw->pixel_bytes == 4 ) {
	            if (gr_current_a < 255) a = ((int)a * gr_current_a) / 255;
	            if (a == 255) {
	                *px++ = gr_current_r;
	                *px++ = gr_current_g;
	                *px++ = gr_current_b;
	                px++;                
	            } else if (a > 0) {
	                *px = (*px * (255-a) + gr_current_r * a) / 255;
	                ++px;
	                *px = (*px * (255-a) + gr_current_g * a) / 255;
	                ++px;
	                *px = (*px * (255-a) + gr_current_b * a) / 255;
	                ++px;
	                ++px;                
	            } else {
	                px += 4;
	            }
			}else {
				if ( a > 0 )
					px += pixel_write_byColor(px);
				else
					px += 2;
			}
        }
        src_p += src_row_bytes;
        dst_p += dst_row_bytes;
    }
}


void gr_text(int x, int y, const char *s, int bold)
{
    GRFont *font = gr_font;
    unsigned off;

    if (!font->texture) return;
    if (gr_current_a == 0) return;

    bold = bold && (font->texture->height != font->cheight);

    x += overscan_offset_x;
    y += overscan_offset_y;

    while((off = *s++)) {
        off -= 32;
        if (outside(x, y) || outside(x+font->cwidth-1, y+font->cheight-1)) break;
        if (off < 96) {

            unsigned char* src_p = font->texture->data + (off * font->cwidth) +
                (bold ? font->cheight * font->texture->row_bytes : 0);
            unsigned char* dst_p = gr_draw->data + y*gr_draw->row_bytes + x*gr_draw->pixel_bytes;

            text_blend(src_p, font->texture->row_bytes,
                       dst_p, gr_draw->row_bytes,
                       font->cwidth, font->cheight);

        }
        x += font->cwidth;
    }
}

void gr_texticon(int x, int y, GRSurface* icon) {
    if (icon == NULL) return;

    if (icon->pixel_bytes != 1) {
        printf("gr_texticon: source has wrong format\n");
        return;
    }

    x += overscan_offset_x;
    y += overscan_offset_y;

    if (outside(x, y) || outside(x+icon->width-1, y+icon->height-1)) return;

    unsigned char* src_p = icon->data;
    unsigned char* dst_p = gr_draw->data + y*gr_draw->row_bytes + x*gr_draw->pixel_bytes;

    text_blend(src_p, icon->row_bytes,
               dst_p, gr_draw->row_bytes,
               icon->width, icon->height);
}

void gr_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    gr_current_r = r;
    gr_current_g = g;
    gr_current_b = b;
    gr_current_a = a;
}

void gr_clear()
{
    if (gr_current_r == gr_current_g &&
        gr_current_r == gr_current_b) {
        memset(gr_draw->data, gr_current_r, gr_draw->height * gr_draw->row_bytes);
    } else {
        int x, y;
        unsigned char* px = gr_draw->data;
        for (y = 0; y < gr_draw->height; ++y) {
            for (x = 0; x < gr_draw->width; ++x) {
				/*
				*now suport lcd  rgb565  PIXEL_FORMAT,if want to suport else, welecome to change code
				*ActionsCode(author:liaotianyang, type:change_code)
				*/	
				if (gr_draw->pixel_bytes == 4 ) {
	                *px++ = gr_current_r;
	                *px++ = gr_current_g;
	                *px++ = gr_current_b;
	                px++;
				}else {
               		px += pixel_write_byColor(px);
				}
            }
            px += gr_draw->row_bytes - (gr_draw->width * gr_draw->pixel_bytes);
        }
    }
}

void gr_fill(int x1, int y1, int x2, int y2)
{
    x1 += overscan_offset_x;
    y1 += overscan_offset_y;

    x2 += overscan_offset_x;
    y2 += overscan_offset_y;

    if (outside(x1, y1) || outside(x2-1, y2-1)) return;

    unsigned char* p = gr_draw->data + y1 * gr_draw->row_bytes + x1 * gr_draw->pixel_bytes;
    if (gr_current_a == 255) {
        int x, y;
        for (y = y1; y < y2; ++y) {
            unsigned char* px = p;
            for (x = x1; x < x2; ++x) {
				/*
				*now suport lcd  rgb565  PIXEL_FORMAT,if want to suport else, welecome to change code
				*ActionsCode(author:liaotianyang, type:change_code)
				*/
				if ( gr_draw->pixel_bytes == 4 ) {
	                *px++ = gr_current_r;
	                *px++ = gr_current_g;
	                *px++ = gr_current_b;
	                px++;
				}else {
                	px += pixel_write_byColor(px);
				}
            }
            p += gr_draw->row_bytes;
        }
    } else if (gr_current_a > 0) {
        int x, y;
        for (y = y1; y < y2; ++y) {
            unsigned char* px = p;
            for (x = x1; x < x2; ++x) {
				/*
				*now suport lcd  rgb565  PIXEL_FORMAT,if want to suport else, welecome to change code
				*ActionsCode(author:liaotianyang, type:change_code)
				*/
				if ( gr_draw->pixel_bytes == 4 ) {
	                *px = (*px * (255-gr_current_a) + gr_current_r * gr_current_a) / 255;
	                ++px;
	                *px = (*px * (255-gr_current_a) + gr_current_g * gr_current_a) / 255;
	                ++px;
	                *px = (*px * (255-gr_current_a) + gr_current_b * gr_current_a) / 255;
	                ++px;
	                ++px;
				}else {
                	px += pixel_write_byColor(px);
				}
            }
            p += gr_draw->row_bytes;
        }
    }
}

/*
 *support rotate of framebuffer, now suport lcd  rgb565  PIXEL_FORMAT
 *if want to suport else, welecome to change code
 *ActionsCode(author:liaotianyang, type:new_method)
 */
static void pixel_write(unsigned char* dst_p, unsigned char* src_p, int src_pixel_bytes)
{
	
	if ( gr_draw->pixel_bytes == src_pixel_bytes ) {
		memcpy(dst_p, src_p, src_pixel_bytes );
		return ;
	}
	if ( src_pixel_bytes == 4 ) // rgb to rgb565
		rgb_2_rgb565(dst_p, src_p[0], src_p[1],src_p[2]);
}
void gr_blit(GRSurface* source, int sx, int sy, int w, int h, int dx, int dy) {
    if (source == NULL) return;

    if (gr_draw->pixel_bytes != source->pixel_bytes) {
        printf("gr_blit: source has wrong format, %d, %d\n",gr_draw->pixel_bytes,source->pixel_bytes  );
        //return;
    }

    dx += overscan_offset_x;
    dy += overscan_offset_y;

    if (outside(dx, dy) || outside(dx+w-1, dy+h-1)) return;

    unsigned char* src_p = source->data + sy*source->row_bytes + sx*source->pixel_bytes;
    unsigned char* dst_p = gr_draw->data + dy*gr_draw->row_bytes + dx*gr_draw->pixel_bytes;

    int i;
    for (i = 0; i < h; ++i) {
		/*
		*now suport lcd  rgb565  PIXEL_FORMAT
		*ActionsCode(author:liaotianyang, type:change_code)
		*/
		#if 0
        memcpy(dst_p, src_p, w * source->pixel_bytes);
		#else
		int j;
        for (j = 0; j < w; j++ )
			pixel_write(dst_p+j*gr_draw->pixel_bytes, src_p+j*source->pixel_bytes, source->pixel_bytes);
		#endif
		src_p += source->row_bytes;
        dst_p += gr_draw->row_bytes;
    }
}

unsigned int gr_get_width(GRSurface* surface) {
    if (surface == NULL) {
        return 0;
    }
    return surface->width;
}

unsigned int gr_get_height(GRSurface* surface) {
    if (surface == NULL) {
        return 0;
    }
    return surface->height;
}

static void gr_init_font(void)
{
    gr_font = calloc(sizeof(*gr_font), 1);

    int res = res_create_alpha_surface("font", &(gr_font->texture));
    if (res == 0) {
        // The font image should be a 96x2 array of character images.  The
        // columns are the printable ASCII characters 0x20 - 0x7f.  The
        // top row is regular text; the bottom row is bold.
        gr_font->cwidth = gr_font->texture->width / 96;
        gr_font->cheight = gr_font->texture->height / 2;
    } else {
        printf("failed to read font: res=%d\n", res);

        // fall back to the compiled-in font.
        gr_font->texture = malloc(sizeof(*gr_font->texture));
        gr_font->texture->width = font.width;
        gr_font->texture->height = font.height;
        gr_font->texture->row_bytes = font.width;
        gr_font->texture->pixel_bytes = 1;

        unsigned char* bits = malloc(font.width * font.height);
        gr_font->texture->data = (void*) bits;

        unsigned char data;
        unsigned char* in = font.rundata;
        while((data = *in++)) {
            memset(bits, (data & 0x80) ? 255 : 0, data & 0x7f);
            bits += (data & 0x7f);
        }

        gr_font->cwidth = font.cwidth;
        gr_font->cheight = font.cheight;
    }
}

#if 0
// Exercises many of the gr_*() functions; useful for testing.
static void gr_test() {
    GRSurface** images;
    int frames;
    int result = res_create_multi_surface("icon_installing", &frames, &images);
    if (result < 0) {
        printf("create surface %d\n", result);
        gr_exit();
        return;
    }

    time_t start = time(NULL);
    int x;
    for (x = 0; x <= 1200; ++x) {
        if (x < 400) {
            gr_color(0, 0, 0, 255);
        } else {
            gr_color(0, (x-400)%128, 0, 255);
        }
        gr_clear();

        gr_color(255, 0, 0, 255);
        gr_surface frame = images[x%frames];
        gr_blit(frame, 0, 0, frame->width, frame->height, x, 0);

        gr_color(255, 0, 0, 128);
        gr_fill(400, 150, 600, 350);

        gr_color(255, 255, 255, 255);
        gr_text(500, 225, "hello, world!", 0);
        gr_color(255, 255, 0, 128);
        gr_text(300+x, 275, "pack my box with five dozen liquor jugs", 1);

        gr_color(0, 0, 255, 128);
        gr_fill(gr_draw->width - 200 - x, 300, gr_draw->width - x, 500);

        gr_draw = gr_backend->flip(gr_backend);
    }
    printf("getting end time\n");
    time_t end = time(NULL);
    printf("got end time\n");
    printf("start %ld end %ld\n", (long)start, (long)end);
    if (end > start) {
        printf("%.2f fps\n", ((double)x) / (end-start));
    }
}
#endif

void gr_flip() {
    gr_draw = gr_backend->flip(gr_backend);
}

int gr_init(void)
{
    gr_init_font();

    gr_vt_fd = open("/dev/tty0", O_RDWR | O_SYNC);
    if (gr_vt_fd < 0) {
        // This is non-fatal; post-Cupcake kernels don't have tty0.
        perror("can't open /dev/tty0");
    } else if (ioctl(gr_vt_fd, KDSETMODE, (void*) KD_GRAPHICS)) {
        // However, if we do open tty0, we expect the ioctl to work.
        perror("failed KDSETMODE to KD_GRAPHICS on tty0");
        gr_exit();
        return -1;
    }

    gr_backend = open_adf();
    if (gr_backend) {
        gr_draw = gr_backend->init(gr_backend);
        if (!gr_draw) {
            gr_backend->exit(gr_backend);
        }
    }

    if (!gr_draw) {
        gr_backend = open_fbdev();
        gr_draw = gr_backend->init(gr_backend);
        if (gr_draw == NULL) {
            return -1;
        }
    }
	/*
	 *support rotate of framebuffer
	 *ActionsCode(author:liaotianyang, type:change_code)
	 */	
	lcd_rotate_init();	
	if ( rotate == 1 || rotate == 3 ) {
	 	int tmp;
		tmp = gr_draw->width;
		gr_draw->width = gr_draw->height ;
		gr_draw->height = tmp;
		gr_draw->row_bytes = gr_draw->width * gr_draw->pixel_bytes;
  	}
	// end ActionsCode

    overscan_offset_x = gr_draw->width * overscan_percent / 100;
    overscan_offset_y = gr_draw->height * overscan_percent / 100;

    gr_flip();
    gr_flip();

    return 0;
}

void gr_exit(void)
{
    gr_backend->exit(gr_backend);

    ioctl(gr_vt_fd, KDSETMODE, (void*) KD_TEXT);
    close(gr_vt_fd);
    gr_vt_fd = -1;
}

int gr_fb_width(void)
{
    return gr_draw->width - 2*overscan_offset_x;
}

int gr_fb_height(void)
{
    return gr_draw->height - 2*overscan_offset_y;
}

void gr_fb_blank(bool blank)
{
    gr_backend->blank(gr_backend, blank);
}
