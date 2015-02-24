/*
 * Copyright © 2013 stag019 <stag019@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "gfx/main.h"

void input_png_file(struct Options opts, struct PNGImage *img) {
	FILE *f;
	int i, y, num_trans, depth, colors;
	bool has_palette;
	png_byte *trans_alpha;
	png_color_16 *trans_values;
	bool *full_alpha;
	png_color *palette;

	depth = (opts.binary ? 1 : 2);
	colors = depth * depth;

	f = fopen(opts.infile, "rb");
	if(!f) {
		err(EXIT_FAILURE, "Opening input png file '%s' failed", opts.infile);
	}

	img->png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!img->png) {
		errx(EXIT_FAILURE, "Creating png structure failed");
	}

	img->info = png_create_info_struct(img->png);
	if(!img->info) {
		errx(EXIT_FAILURE, "Creating png info structure failed");
	}

	/* Better error handling here? */
	if(setjmp(png_jmpbuf(img->png))) {
		exit(EXIT_FAILURE);
	}

	png_init_io(img->png, f);

	png_read_info(img->png, img->info);

	img->width  = png_get_image_width(img->png, img->info);
	img->height = png_get_image_height(img->png, img->info);
	img->depth  = png_get_bit_depth(img->png, img->info);
	img->type   = png_get_color_type(img->png, img->info);

	if(img->type & PNG_COLOR_MASK_ALPHA) {
		png_set_strip_alpha(img->png);
	}

	if(img->depth != depth) {
		if(opts.verbose) {
			warnx("Image bit depth is not %i.", depth);
		}

		if(img->type == PNG_COLOR_TYPE_GRAY) {
			if(img->depth < 8) {
				png_set_gray_1_2_4_to_8(img->png);
			}
			png_set_gray_to_rgb(img->png);
		} else {
			has_palette = png_get_PLTE(img->png, img->info, &palette, &colors);
		}

		if(png_get_tRNS(img->png, img->info, &trans_alpha, &num_trans, &trans_values)) {
			if(img->type == PNG_COLOR_TYPE_PALETTE) {
				full_alpha = malloc(sizeof(bool) * num_trans);

				for(i = 0; i < num_trans; i++) {
					if(trans_alpha[i] > 0) {
						full_alpha[i] = false;
					} else {
						full_alpha[i] = true;
					}
				}

				for(i = 0; i < num_trans; i++) {
					if(full_alpha[i]) {
						palette[i].red   = 0xFF;
						palette[i].green = 0x00;
						palette[i].blue  = 0xFF;
						/* Set to the lightest color in the palette. */
					}
				}

				free(full_alpha);
			} else {
				/* Set to the lightest color in the image. */
			}

			png_free_data(img->png, img->info, PNG_FREE_TRNS, -1);
		}

		if(has_palette) {
			/* Make sure palette only has the amount of colors you want. */
		} else {
			/* Eventually when this copies colors from the image itself, make sure order is lightest to darkest. */
			palette = malloc(sizeof(png_color) * colors);

			if(strequ(opts.infile, "rgb.png")) {
				palette[0].red   = 0xFF;
				palette[0].green = 0xEF;
				palette[0].blue  = 0xFF;

				palette[1].red   = 0xF7;
				palette[1].green = 0xF7;
				palette[1].blue  = 0x8C;

				palette[2].red   = 0x94;
				palette[2].green = 0x94;
				palette[2].blue  = 0xC6;

				palette[3].red   = 0x39;
				palette[3].green = 0x39;
				palette[3].blue  = 0x84;
			} else {
				palette[0].red   = 0xFF;
				palette[0].green = 0xFF;
				palette[0].blue  = 0xFF;

				palette[1].red   = 0xA9;
				palette[1].green = 0xA9;
				palette[1].blue  = 0xA9;

				palette[2].red   = 0x55;
				palette[2].green = 0x55;
				palette[2].blue  = 0x55;

				palette[3].red   = 0x00;
				palette[3].green = 0x00;
				palette[3].blue  = 0x00;
			}
		}

		/* Also unfortunately, this sets it at 8 bit, and I cant find any option to reduce to 2 or 1 bit. */
		png_set_dither(img->png, palette, colors, colors, NULL, 1);

		if(!has_palette) {
			png_set_PLTE(img->png, img->info, palette, colors);
			free(palette);
		}
	}

	/* If other useless chunks exist (sRGB, bKGD, pHYs, gAMA, cHRM, iCCP, etc.) offer to remove? */

	png_read_update_info(img->png, img->info);

	img->data = malloc(sizeof(png_byte *) * img->height);
	for(y = 0; y < img->height; y++) {
		img->data[y] = malloc(png_get_rowbytes(img->png, img->info));
	}

	png_read_image(img->png, img->data);
	png_read_end(img->png, img->info);

	fclose(f);
}

void get_text(struct PNGImage *png) {
	png_text *text;
	int i, numtxts, numremoved;

	png_get_text(png->png, png->info, &text, &numtxts);
	for(i = 0; i < numtxts; i++) {
		if(strequ(text[i].key, "h") && !*text[i].text) {
			png->horizontal = true;
			png_free_data(png->png, png->info, PNG_FREE_TEXT, i);
		} else if(strequ(text[i].key, "x")) {
			png->trim = strtoul(text[i].text, NULL, 0);
			png_free_data(png->png, png->info, PNG_FREE_TEXT, i);
		} else if(strequ(text[i].key, "t")) {
			png->mapfile = text[i].text;
			png_free_data(png->png, png->info, PNG_FREE_TEXT, i);
		} else if(strequ(text[i].key, "T") && !*text[i].text) {
			png->mapout = true;
			png_free_data(png->png, png->info, PNG_FREE_TEXT, i);
		} else if(strequ(text[i].key, "p")) {
			png->palfile = text[i].text;
			png_free_data(png->png, png->info, PNG_FREE_TEXT, i);
		} else if(strequ(text[i].key, "P") && !*text[i].text) {
			png->palout = true;
			png_free_data(png->png, png->info, PNG_FREE_TEXT, i);
		}

		/* TODO: Remove this and simply change the warning function not to warn instead. */
		for(i = 0, numremoved = 0; i < numtxts; i++) {
			if(text[i].key == NULL) {
				numremoved++;
			}
			text[i].key = text[i + numremoved].key;
			text[i].text = text[i + numremoved].text;
			text[i].compression = text[i + numremoved].compression;
		}
		png->info->num_text -= numremoved;
	}
}

void set_text(struct PNGImage *png) {
	png_text *text;
	char buffer[3];

	text = malloc(sizeof(png_text));

	if(png->horizontal) {
		text[0].key = "h";
		text[0].text = "";
		text[0].compression = PNG_TEXT_COMPRESSION_NONE;
		png_set_text(png->png, png->info, text, 1);
	}
	if(png->trim) {
		text[0].key = "x";
		snprintf(buffer, 3, "%d", png->trim);
		text[0].text = buffer;
		text[0].compression = PNG_TEXT_COMPRESSION_NONE;
		png_set_text(png->png, png->info, text, 1);
	}
	if(*png->mapfile) {
		text[0].key = "t";
		text[0].text = "";
		text[0].compression = PNG_TEXT_COMPRESSION_NONE;
		png_set_text(png->png, png->info, text, 1);
	}
	if(png->mapout) {
		text[0].key = "T";
		text[0].text = "";
		text[0].compression = PNG_TEXT_COMPRESSION_NONE;
		png_set_text(png->png, png->info, text, 1);
	}
	if(*png->palfile) {
		text[0].key = "p";
		text[0].text = "";
		text[0].compression = PNG_TEXT_COMPRESSION_NONE;
		png_set_text(png->png, png->info, text, 1);
	}
	if(png->palout) {
		text[0].key = "P";
		text[0].text = "";
		text[0].compression = PNG_TEXT_COMPRESSION_NONE;
		png_set_text(png->png, png->info, text, 1);
	}

	free(text);
}

void output_png_file(struct Options opts, struct PNGImage *png) {
	FILE *f;
	char *outfile;
	png_struct *img;

	/* Variable outfile is for debugging purposes. Eventually, opts.infile will be used directly. */
	if(opts.debug) {
		outfile = malloc(strlen(opts.infile) + 5);
		strcpy(outfile, opts.infile);
		strcat(outfile, ".out");
	} else {
		outfile = opts.infile;
	}

	f = fopen(outfile, "wb");
	if(!f) {
		err(EXIT_FAILURE, "Opening output png file '%s' failed", outfile);
	}

	img = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!img) {
		errx(EXIT_FAILURE, "Creating png structure failed");
	}

	/* Better error handling here? */
	if(setjmp(png_jmpbuf(img))) {
		exit(EXIT_FAILURE);
	}

	png_init_io(img, f);

	png_write_info(img, png->info);
	
	png_write_image(img, png->data);
	png_write_end(img, NULL);

	fclose(f);

	if(opts.debug) {
		free(outfile);
	}
}

void free_png_data(struct PNGImage *png) {
	int y;

	for(y = 0; y < png->height; y++) {
		free(png->data[y]);
	}
	free(png->data);
}

