/** main.c
 *
 * This file is part of Sub3dtool
 *
 * Sub3dtool is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Sub3dtool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Sub3dtool.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include "utils.h"
#include "subass.h"
#include "subsrt.h"
#include "subsrt_ass.h"
#include "subass3d.h"
#include "global.h"

/* global program_name */
const char * program_name;
const char * version = "0.4.2";
int		debug_mode = 0;
FILE *	error_file = NULL;
#define FORMAT_UNKNOWN	0
#define FORMAT_ASS		1
#define FORMAT_SRT		2
#define CODE_INVALID_ARGS		1
#define CODE_INPUT_ERROR		10
#define CODE_INPFORMAT_ERROR	11
#define CODE_SRT_PARSE_ERROR	15
#define CODE_ASS_PARSE_ERROR	16
#define CODE_OUTPUT_ERROR		20
#define CODE_FUNCTION_ERROR		30

int format_test (const char * path)
{
#define CHECKSIZE 64 
	ZnFile * file = znfile_open (path);
	if (file == NULL)
		return FORMAT_UNKNOWN;
	int scores		[] = { 0, 0 };
	int scores_eqv	[] = { FORMAT_SRT, FORMAT_ASS };
	const char * curline = NULL;
	int i = 0;
	while (i < CHECKSIZE
			&& (curline = znfile_linenext (file)) != NULL)
	{
		if (strstr (curline, "-->") != NULL)
			scores[0] += 16;
		else if (zn_stristr (curline, "Style:") != NULL)
			scores[1] += 16;
		else if (zn_stristr (curline, "Dialogue:") != NULL)
			scores[1] += 16;
		else if (zn_stristr (curline, "[Script Info]") != NULL)
		{
			scores[1] += 64;
			i += 16;
		}
		i++;
	}
	znfile_close (file);
	if (scores[0] > scores[1])
		return scores_eqv[0];
	else
		return scores_eqv[1];
}

void func_test ()
{
	FILE * f = error_file;
	/* Special character */
	fprintf (f, "Special characters\n");
	fprintf (f, "\\n: %d, \\r: %d, \\t: %d.\n", '\n', '\r', '\t');
	fprintf (f, "\\v: %d, \\b: %d, \\f: %d.\n", '\v', '\b', '\f');
	fprintf (f, "\\a: %d, NULL: %p\n", '\a', NULL);
	
	/* subsrt.h */
	ZnsubSRT * sub1 = calloc (1, sizeof (ZnsubSRT));
	fprintf (f, "ZnsubSRT: %ld, %ld, %p, %p.\n",
			sub1->start, sub1->end, sub1->text, (void*)sub1->next);
	free (sub1);
	sub1 = NULL;
	/*const char * tmp = "1\n01:02:03,456 --> 03:02:01,789\n\
First Line.\nSecond Line.\n\n2\n03:04:05,123 --> 05:04:03,321\n\
2nd Event.\n\n3\n06:07:08,234 --> 08:09:10,007\n\
3rd Event.";
*/

	fprintf (f, "FUNCTEST ENDED\n\n\n");
}

const char* converSubsTo3dSubs(const char *input, const char * output, int sub3d) {
    debug_mode = 0;
    error_file = stderr;

    /* parsing arguments */
    long   fontsize    = -1;
    char * font        = NULL;
    long long color_primary        = -1;
    long long color_2nd            = -1;
    long long color_outline        = -1;
    long long color_back        = -1;
    int border_style = -1;
    int outline = -1;
    int shadow = -1;
    int align = -1;
    long margin_l = LONG_MIN;
    long margin_r = LONG_MIN;
    long margin_v = LONG_MIN;
    long screen_x = -1;
    long screen_y = -1;
    long znsub_srt2ass_flag = 0;
    int     align_adjust = -1;
    
    /* input */
    if (input == NULL)
    {
        return "Invalid input file.";
    }

    /* open input stream */
    ZnFile * data = znfile_open (input);
    if (data == NULL)
    {
        return "Cannot open input file.";
    }
    /* this part check the input format and use the
     * proper parser to parse it */
    /* parse input */
    ZnsubASS * inp_ass = NULL;
    ZnsubSRT * inp_srt = NULL;
    int           inp_format = 0;
    inp_format = format_test (input);

    if (inp_format == FORMAT_ASS)
    {
        inp_ass = znsub_ass_parse (data);
        inp_format = FORMAT_ASS;
        if (sub3d == ZNSUB_ASS3D_NO3D)
        {
            znsub_ass3d_discard (inp_ass, 0);
        }
        if (inp_ass == NULL)
        {
            fprintf (error_file, "Parsing ASS subtitle failed.\n");
            exit (CODE_ASS_PARSE_ERROR);
        }
    }
    else if (inp_format == FORMAT_SRT)
    {
        inp_srt = znsub_srt_parse (data);
        inp_format = FORMAT_SRT;
        if (inp_srt == NULL)
        {
            return "Parsing SRT subtitle failed.";
        }
    }
    else
    {
        znfile_close (data);
        return "Unable to determine file format.";
    }

    znfile_close (data);

    /* output format decision */
    const char * ext = strrchr (output, '.');
    if (strncmp (ext, ".srt", 4) == 0)
    {
        /* SRT FORMAT */
        ZnsubSRT * out;
        if (inp_format == FORMAT_ASS)
            out = znsub_ass2srt (inp_ass, znsub_srt2ass_flag);
        else if (inp_format == FORMAT_SRT)
            out = inp_srt;
        else
        {
            return "Error identifying input format.";
        }
        FILE * file;
        file = fopen (output, "w");
        if (file == NULL)
        {
            if (inp_ass != NULL)
                znsub_ass_free (inp_ass);
            if (inp_srt != NULL)
                znsub_srt_free (inp_srt);
            return "Error opening output file.";
        }
        znsub_srt_tofile (out, file);
        if (inp_ass != NULL)
            znsub_ass_free (inp_ass);
        if (inp_srt != NULL)
            znsub_srt_free (inp_srt);
        if (inp_format != FORMAT_SRT)
            znsub_srt_free (out);
        inp_ass = NULL;
        inp_srt = NULL;
    }
    else {
        /* ASS FORMAT */
        /* convert to ASS */
        ZnsubASS * out = NULL;
        if (inp_format == FORMAT_ASS)
            out = inp_ass;
        else if (inp_format == FORMAT_SRT)
            out = znsub_srt2ass (inp_srt, znsub_srt2ass_flag);
        else
        {
            return "Error identifying input format.";
        }
        if (out == NULL)
        {
            if (inp_ass != NULL)
                znsub_ass_free (inp_ass);
            if (inp_srt != NULL)
                znsub_srt_free (inp_srt);
            return "Converting SRT to ASS failed.";
        }
        /* Customization */
        if (screen_x >= 0 && screen_y >= 0)
        { /* set play resolution */
            out->play_resx = screen_x;
            out->play_resy = screen_y;
        }
        /* fonts */
        ZnsubASSStyle * style = out->first_style;
        if (fontsize > 0)
            style->font_size = fontsize;
        zn_strset (&style->font_name, font);

        /* colors */
        if (color_primary >= 0)
        {
            znsub_ass_style_color_setll (style->primary_colour,
                    color_primary);
        }
        if (color_2nd >= 0)
        {
            znsub_ass_style_color_setll (style->secondary_colour,
                    color_2nd);
        }
        if (color_outline >= 0)
        {
            znsub_ass_style_color_setll (style->outline_colour,
                    color_outline);
        }
        if (color_back >= 0)
        {
            znsub_ass_style_color_setll (style->back_colour,
                    color_back);
        }

        /* border -
         * style: 1 for drop shadow + outline
         * style: 2 for black box
         * outline and shadow is limited to 0-4 according to
         * specification*/
        style->border_style = (border_style == 1 || border_style == 3 ?
                border_style : style->border_style);
        style->outline = (outline >= 0 && outline <= 4 ?
                outline : style->outline);
        style->shadow = (shadow >= 0 && shadow <= 4 ?
                shadow : style->shadow);

        /* align
         * align 1 for left, 2 for center, 3 for right
         * align_adjust +0 for bottom, +3 for middle, +6 for top */
        align = (align != -1 ? align : 2); /* default - center*/
        align += (align_adjust != -1 ? align_adjust : 0);
        style->alignment = (align > 0 ? align : style->alignment);

        /* margin: is set? new value else default */
        style->margin_l = (margin_l != LONG_MIN ? margin_l :
                style->margin_l);
        style->margin_r = (margin_r != LONG_MIN ? margin_r :
                style->margin_r);
        style->margin_v = (margin_v != LONG_MIN ? margin_v :
                style->margin_v);


        /* convert to 3d */
        if (sub3d == ZNSUB_ASS3D_SBS || sub3d == ZNSUB_ASS3D_TB)
            znsub_ass3d_convert (out, sub3d, 0);

        /* output to file */
        FILE * file;
        file = fopen (output, "w");
        if (file == NULL)
        {
            if (inp_ass != NULL)
                znsub_ass_free (inp_ass);
            if (inp_srt != NULL)
                znsub_srt_free (inp_srt);
            return "Error opening output file.";
        }

        znsub_ass_tofile (out, file);
        /* free data */
        fclose (file);
        if (inp_format != FORMAT_ASS)
            znsub_ass_free (out);
        if (inp_ass != NULL)
            znsub_ass_free (inp_ass);
        if (inp_srt != NULL)
            znsub_srt_free (inp_srt);
        inp_ass = NULL;
        inp_srt = NULL;
    }

    /* input free */
    if (font != NULL)
        free (font);
    if (inp_ass != NULL)
        znsub_ass_free (inp_ass);
    if (inp_srt != NULL)
        znsub_srt_free (inp_srt);
    return "";
}
