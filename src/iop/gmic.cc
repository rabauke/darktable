/*
    This file is part of darktable,
    copyright (c) 2018 Heiko Bauke.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/

extern "C"
{
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
}

extern "C"
{
#include "bauhaus/bauhaus.h"
#include "common/darktable.h"
#include "develop/imageop.h"
#include "develop/imageop_math.h"
#include "gui/gtk.h"
#include "iop/iop_api.h"
}

#include <cfloat>
extern "C"
{
#include <gtk/gtk.h>
}
#include <CImg.h>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <gmic.h>
#include <iostream>
#include <sstream>

//----------------------------------------------------------------------
// implement the module api
//----------------------------------------------------------------------

DT_MODULE_INTROSPECTION(1, dt_iop_gmic_params_t)

constexpr int command_len = 1024;

typedef float rgb_pixel[3];

typedef struct dt_iop_gmic_params_t
{
  char command[command_len + 1];
} dt_iop_gmic_params_t;

// types  dt_iop_gmic_params_t and dt_iop_gmic_data_t are
// equal, thus no commit_params function needs to be implemented
typedef dt_iop_gmic_params_t dt_iop_gmic_data_t;

typedef struct dt_iop_gmic_gui_data_t
{
  GtkWidget *command;
} dt_iop_gmic_gui_data_t;

typedef struct dt_iop_gmic_global_data_t
{
} dt_iop_gmic_global_data_t;

extern "C" const char *name()
{
  return _("G'MIC");
}

extern "C" int flags()
{
  return IOP_FLAGS_INCLUDE_IN_STYLES | IOP_FLAGS_SUPPORTS_BLENDING;
}

extern "C" int groups()
{
  return IOP_GROUP_EFFECT;
}

extern "C" void init_pipe(struct dt_iop_module_t *self, dt_dev_pixelpipe_t *pipe, dt_dev_pixelpipe_iop_t *piece)
{
  piece->data = std::calloc(1, sizeof(dt_iop_gmic_data_t));
  self->commit_params(self, self->default_params, pipe, piece);
}

extern "C" void cleanup_pipe(struct dt_iop_module_t *self, dt_dev_pixelpipe_t *pipe, dt_dev_pixelpipe_iop_t *piece)
{
  std::free(piece->data);
  piece->data = NULL;
}

extern "C" void init_global(dt_iop_module_so_t *self)
{
  self->data = NULL;
}

extern "C" void cleanup_global(dt_iop_module_so_t *self)
{
}

extern "C" void init(dt_iop_module_t *self)
{
  self->params = std::calloc(1, sizeof(dt_iop_gmic_params_t));
  self->default_params = std::calloc(1, sizeof(dt_iop_gmic_params_t));
  self->default_enabled = 0;
  self->priority = 998; // module order created by iop_dependencies.py, do not edit!
  self->params_size = sizeof(dt_iop_gmic_params_t);
  self->gui_data = NULL;
  dt_iop_gmic_params_t tmp = (dt_iop_gmic_params_t){ "sepia" };
  std::memcpy(self->params, &tmp, sizeof(dt_iop_gmic_params_t));
  std::memcpy(self->default_params, &tmp, sizeof(dt_iop_gmic_params_t));
}

extern "C" void cleanup(dt_iop_module_t *self)
{
  std::free(self->params);
  self->params = NULL;
}

static void command_callback(GtkWidget *entry, gpointer user_data)
{
  dt_iop_module_t *self = (dt_iop_module_t *)user_data;
  if(self->dt->gui->reset) return;
  dt_iop_gmic_params_t *p = (dt_iop_gmic_params_t *)self->params;
  snprintf(p->command, sizeof(p->command), "%s", gtk_entry_get_text(GTK_ENTRY(entry)));
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

extern "C" void gui_update(struct dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = (dt_iop_gmic_gui_data_t *)self->gui_data;
  dt_iop_gmic_params_t *p = (dt_iop_gmic_params_t *)self->params;
  gtk_entry_set_text(GTK_ENTRY(g->command), p->command);
}

extern "C" void gui_init(dt_iop_module_t *self)
{
  self->gui_data = std::malloc(sizeof(dt_iop_gmic_gui_data_t));

  dt_iop_gmic_gui_data_t *g = (dt_iop_gmic_gui_data_t *)self->gui_data;
  // dt_iop_gmic_params_t *p = (dt_iop_gmic_params_t *)self->params;

  self->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  g->command = gtk_entry_new();
  gtk_widget_set_tooltip_text(g->command, _("G'MIC script, confirm with enter"));
  gtk_entry_set_max_length(GTK_ENTRY(g->command), command_len);
  dt_gui_key_accel_block_on_focus_connect(g->command);
  gtk_box_pack_start(GTK_BOX(self->widget), GTK_WIDGET(g->command), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->command), "activate", G_CALLBACK(command_callback), self);
}

extern "C" void gui_cleanup(dt_iop_module_t *self)
{
  // dt_iop_gmic_gui_data_t *g = (dt_iop_gmic_gui_data_t *)self->gui_data;
  std::free(self->gui_data);
  self->gui_data = nullptr;
}

extern "C" void process(struct dt_iop_module_t *self, dt_dev_pixelpipe_iop_t *piece, const void *const ivoid,
                        void *const ovoid, const dt_iop_roi_t *const roi_in, const dt_iop_roi_t *const roi_out)
{
  // dt_iop_gmic_gui_data_t *g = (dt_iop_gmic_gui_data_t *)self->gui_data;
  dt_iop_gmic_params_t *d = (dt_iop_gmic_params_t *)piece->data;

  const size_t ch = piece->colors;
  const size_t width = roi_in->width;
  const size_t height = roi_in->height;

  cimg_library::CImg<float> in_img(width, height, 1, 3);
  for(size_t j = 0; j < height; ++j)
    for(size_t i = 0; i < width; ++i)
    {
      const float *pixel_in = reinterpret_cast<const float *>(ivoid) + (j * width + i) * ch;
      in_img(i, j, 0, 0) = pixel_in[0] * 255;
      in_img(i, j, 0, 1) = pixel_in[1] * 255;
      in_img(i, j, 0, 2) = pixel_in[2] * 255;
    }
  cimg_library::CImgList<float> image_list;
  in_img.move_to(image_list);
  cimg_library::CImgList<char> image_names;
  cimg_library::CImg<char>::string("input image").move_to(image_names);
  try
  {
    gmic(d->command, image_list, image_names);
  }
  catch(gmic_exception &e)
  {
    if(piece->pipe->type == DT_DEV_PIXELPIPE_FULL || piece->pipe->type == DT_DEV_PIXELPIPE_PREVIEW)
      dt_control_log(_("G'MIC error"));
  }
  const cimg_library::CImg<float> &out_img(image_list[0]);
  for(size_t j = 0; j < height; ++j)
    for(size_t i = 0; i < width; ++i)
    {
      float *pixel_out = reinterpret_cast<float *>(ovoid) + (j * width + i) * ch;
      pixel_out[0] = out_img(i, j, 0, 0) / 255;
      pixel_out[1] = out_img(i, j, 0, 1) / 255;
      pixel_out[2] = out_img(i, j, 0, 2) / 255;
    }

  if(piece->pipe->mask_display & DT_DEV_PIXELPIPE_DISPLAY_MASK)
    dt_iop_alpha_copy(ivoid, ovoid, roi_out->width, roi_out->height);
}

// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
