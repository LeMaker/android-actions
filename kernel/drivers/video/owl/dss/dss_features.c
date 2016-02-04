/*
 * linux/drivers/video/owl/dss/dss_features.c
 *
 * Copyright (C) 2010 Actions
 * Author: Hui Wang <wanghui@actions-semi.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/slab.h>

#include <video/owldss.h>

#include "dss.h"
#include "dss_features.h"

/* Defines a generic owl register field */
struct dss_reg_field {
	u8 start, end;
};

struct dss_param_range {
	int min, max;
};

struct owl_dss_features {
	const struct dss_reg_field *reg_fields;
	const int num_reg_fields;

	const u32 has_feature;

	const int num_mgrs;
	const int num_ovls;
	const enum owl_display_type *supported_displays;
	const enum owl_color_mode *supported_color_modes;
	const enum owl_overlay_caps *overlay_caps;
	const char * const *clksrc_names;
	const struct dss_param_range *dss_params;

	const u32 buffer_size_unit;
	const u32 burst_size_unit;
};

/* This struct is assigned to one of the below during initialization */
static const struct owl_dss_features *owl_current_dss_features;


static const enum owl_display_type owl_dss_supported_displays[] = {	
	/* OWL_DSS_CHANNEL_LCD */
	OWL_DISPLAY_TYPE_LCD | OWL_DISPLAY_TYPE_DSI | OWL_DISPLAY_TYPE_EDP | OWL_DISPLAY_TYPE_CVBS |
	OWL_DISPLAY_TYPE_HDMI ,	
	
	/* OWL_DSS_CHANNEL_DIGIT */
	OWL_DISPLAY_TYPE_HDMI | OWL_DISPLAY_TYPE_LCD | OWL_DISPLAY_TYPE_DSI | OWL_DISPLAY_TYPE_CVBS ,	

};


static const enum owl_color_mode owl_dss_supported_color_modes[] = {
	/* OWL_DSS_VIDEO1 */
	OWL_DSS_COLOR_RGB16 | OWL_DSS_COLOR_BGR16 |
	OWL_DSS_COLOR_ARGB32 | OWL_DSS_COLOR_ABGR32 |
	OWL_DSS_COLOR_RGBA32 | OWL_DSS_COLOR_BGRA32 |
	OWL_DSS_COLOR_RGBX32 | OWL_DSS_COLOR_XBGR32 | OWL_DSS_COLOR_XRGB32 |
	OWL_DSS_COLOR_YU12 | OWL_DSS_COLOR_NV12 | OWL_DSS_COLOR_NV21 |
	OWL_DSS_COLOR_ARGB16 | OWL_DSS_COLOR_ABGR16 |
	OWL_DSS_COLOR_RGBA16 | OWL_DSS_COLOR_BGRA16 ,

	/* OWL_DSS_VIDEO2 */
	OWL_DSS_COLOR_RGB16 | OWL_DSS_COLOR_BGR16 |
	OWL_DSS_COLOR_ARGB32 | OWL_DSS_COLOR_ABGR32 |
	OWL_DSS_COLOR_RGBA32 | OWL_DSS_COLOR_BGRA32 |
	OWL_DSS_COLOR_RGBX32 | OWL_DSS_COLOR_XBGR32 | OWL_DSS_COLOR_XRGB32 |
	OWL_DSS_COLOR_YU12 | OWL_DSS_COLOR_NV12 | OWL_DSS_COLOR_NV21 |
	OWL_DSS_COLOR_ARGB16 | OWL_DSS_COLOR_ABGR16 |
	OWL_DSS_COLOR_RGBA16 | OWL_DSS_COLOR_BGRA16 ,

    /* OWL_DSS_VIDEO3 */
	OWL_DSS_COLOR_RGB16 | OWL_DSS_COLOR_BGR16 |
	OWL_DSS_COLOR_ARGB32 | OWL_DSS_COLOR_ABGR32 |
	OWL_DSS_COLOR_RGBA32 | OWL_DSS_COLOR_BGRA32 |
	OWL_DSS_COLOR_RGBX32 | OWL_DSS_COLOR_XBGR32 | OWL_DSS_COLOR_XRGB32 |
	OWL_DSS_COLOR_YU12 | OWL_DSS_COLOR_NV12 | OWL_DSS_COLOR_NV21 |
	OWL_DSS_COLOR_ARGB16 | OWL_DSS_COLOR_ABGR16 |
	OWL_DSS_COLOR_RGBA16 | OWL_DSS_COLOR_BGRA16 ,
	/* OWL_DSS_VIDEO4 */
	OWL_DSS_COLOR_RGB16 | OWL_DSS_COLOR_BGR16 |
	OWL_DSS_COLOR_ARGB32 | OWL_DSS_COLOR_ABGR32 |
	OWL_DSS_COLOR_RGBA32 | OWL_DSS_COLOR_BGRA32 |
	OWL_DSS_COLOR_RGBX32 | OWL_DSS_COLOR_XBGR32 | OWL_DSS_COLOR_XRGB32 |
	OWL_DSS_COLOR_YU12 | OWL_DSS_COLOR_NV12 | OWL_DSS_COLOR_NV21 |
	OWL_DSS_COLOR_ARGB16 | OWL_DSS_COLOR_ABGR16 |
	OWL_DSS_COLOR_RGBA16 | OWL_DSS_COLOR_BGRA16 ,
	
};
static const enum owl_overlay_caps owl_dss_overlay_caps[] = {
	/* OWL_DSS_VIDEO0 */
	OWL_DSS_OVL_CAP_SCALE | OWL_DSS_OVL_CAP_GLOBAL_ALPHA |
		OWL_DSS_OVL_CAP_PRE_MULT_ALPHA | OWL_DSS_OVL_CAP_ZORDER,

	/* OWL_DSS_VIDEO1 */
	OWL_DSS_OVL_CAP_SCALE | OWL_DSS_OVL_CAP_GLOBAL_ALPHA |
		OWL_DSS_OVL_CAP_PRE_MULT_ALPHA | OWL_DSS_OVL_CAP_ZORDER,

	/* OWL_DSS_VIDEO2 */
	OWL_DSS_OVL_CAP_SCALE | OWL_DSS_OVL_CAP_GLOBAL_ALPHA |
		OWL_DSS_OVL_CAP_PRE_MULT_ALPHA | OWL_DSS_OVL_CAP_ZORDER,

	/* OWL_DSS_VIDEO3 */
	OWL_DSS_OVL_CAP_SCALE | OWL_DSS_OVL_CAP_GLOBAL_ALPHA |
		OWL_DSS_OVL_CAP_PRE_MULT_ALPHA | OWL_DSS_OVL_CAP_ZORDER,
};

/* OWL4 DSS Features */
/* For all the other OWL4 versions */
static const struct owl_dss_features owl_dss_features = {

	.has_feature	=
		FEAT_MGR_LCD2 |
		FEAT_CORE_CLK_DIV | FEAT_LCD_CLK_SRC |
		FEAT_DSI_DCS_CMD_CONFIG_VC | FEAT_DSI_VC_OCP_WIDTH |
		FEAT_DSI_GNQ | FEAT_HDMI_CTS_SWMODE |
		FEAT_HANDLE_UV_SEPARATE | FEAT_ATTR2 | FEAT_CPR |
		FEAT_PRELOAD | FEAT_FIR_COEF_V | FEAT_ALPHA_FREE_ZORDER,

	.num_mgrs = 2,
	.num_ovls = 4,
	.supported_displays = owl_dss_supported_displays,
	.supported_color_modes = owl_dss_supported_color_modes,
	.overlay_caps = owl_dss_overlay_caps,
	.buffer_size_unit = 16,
	.burst_size_unit = 16,
};

/* Functions returning values related to a DSS feature */
int dss_feat_get_num_mgrs(void)
{
	return owl_current_dss_features->num_mgrs;
}

int dss_feat_get_num_ovls(void)
{
	return owl_current_dss_features->num_ovls;
}

unsigned long dss_feat_get_param_min(enum dss_range_param param)
{
	return owl_current_dss_features->dss_params[param].min;
}

unsigned long dss_feat_get_param_max(enum dss_range_param param)
{
	return owl_current_dss_features->dss_params[param].max;
}

enum owl_display_type dss_feat_get_supported_displays(enum owl_de_path_id channel)
{
	return owl_current_dss_features->supported_displays[channel];
}

enum owl_color_mode dss_feat_get_supported_color_modes(enum owl_plane plane)
{
	return owl_current_dss_features->supported_color_modes[plane];
}

enum owl_overlay_caps dss_feat_get_overlay_caps(enum owl_plane plane)
{
	return owl_current_dss_features->overlay_caps[plane];
}

bool dss_feat_color_mode_supported(enum owl_plane plane,
		enum owl_color_mode color_mode)
{
	if(color_mode == OWL_DSS_COLOR_RGB16){
		return true;
	}
	return owl_current_dss_features->supported_color_modes[plane] &
			color_mode;
}

const char *dss_feat_get_clk_source_name(enum owl_dss_clk_source id)
{
	return owl_current_dss_features->clksrc_names[id];
}

u32 dss_feat_get_buffer_size_unit(void)
{
	return owl_current_dss_features->buffer_size_unit;
}

u32 dss_feat_get_burst_size_unit(void)
{
	return owl_current_dss_features->burst_size_unit;
}

/* DSS has_feature check */
bool dss_has_feature(enum dss_feat_id id)
{
	return owl_current_dss_features->has_feature & id;
}

void dss_feat_get_reg_field(enum dss_feat_reg_field id, u8 *start, u8 *end)
{
	if (id >= owl_current_dss_features->num_reg_fields)
		BUG();

	*start = owl_current_dss_features->reg_fields[id].start;
	*end = owl_current_dss_features->reg_fields[id].end;
}

void dss_features_init(void)
{
    /* should asigned feature accrording to ARCH, TODO */
    owl_current_dss_features = &owl_dss_features;
}
