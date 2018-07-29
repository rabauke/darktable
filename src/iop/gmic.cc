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
#include "bauhaus/bauhaus.h"
#include "common/darktable.h"
#include "develop/imageop.h"
#include "develop/imageop_math.h"
#include "gui/gtk.h"
#include "iop/iop_api.h"
#include <gtk/gtk.h>
}

#include <CImg.h>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <gmic.h>
#include <sstream>
#include <string>
#include <vector>


// --- no filter

struct dt_iop_gmic_none_params_t
{
  dt_iop_gmic_none_params_t()
  {
  }

  std::string gmic_command() const
  {
    return "";
  }

  static void gui_update(struct dt_iop_module_t *self);

  static void gui_init(dt_iop_module_t *self);

  static void command_callback(GtkWidget *w, dt_iop_module_t *self);
};

// --- expert mode

struct dt_iop_gmic_expert_mode_params_t
{
  char command[1024];

  dt_iop_gmic_expert_mode_params_t() : command("")
  {
  }

  std::string gmic_command() const
  {
    return command;
  }

  static void gui_update(struct dt_iop_module_t *self);

  static void gui_init(dt_iop_module_t *self);

  static void command_callback(GtkWidget *w, dt_iop_module_t *self);
};

typedef struct dt_iop_gmic_expert_mode_gui_data_t
{
  GtkWidget *box;
  GtkWidget *command;
} dt_iop_gmic_expert_mode_gui_data_t;

// --- sepia

struct dt_iop_gmic_sepia_params_t
{
  float brightness, contrast, gamma;

  dt_iop_gmic_sepia_params_t() : brightness(0.f), contrast(0.f), gamma(0.f)
  {
  }

  std::string gmic_command() const
  {
    std::stringstream str;
    str << "fx_sepia " << brightness * 100 << ',' << contrast * 100 << ',' << gamma * 100;
    return str.str();
  }

  static void gui_update(struct dt_iop_module_t *self);

  static void gui_init(dt_iop_module_t *self);

  static void brightness_callback(GtkWidget *w, dt_iop_module_t *self);

  static void contrast_callback(GtkWidget *w, dt_iop_module_t *self);

  static void gamma_callback(GtkWidget *w, dt_iop_module_t *self);
};

typedef struct dt_iop_gmic_sepia_gui_data_t
{
  GtkWidget *box;
  GtkWidget *brightness, *contrast, *gamma;
} dt_iop_gmic_sepia_gui_data_t;


enum filters
{
  none,
  expert_mode,
  sepia
};

struct dt_iop_gmic_all_params_t
{
  dt_iop_gmic_expert_mode_params_t expert_mode;
  dt_iop_gmic_sepia_params_t sepia;
};

//----------------------------------------------------------------------
// implement the module api
//----------------------------------------------------------------------

DT_MODULE_INTROSPECTION(1, dt_iop_gmic_params_t)

typedef struct dt_iop_gmic_params_t
{
  filters filter;
  dt_iop_gmic_all_params_t parmeters;
} dt_iop_gmic_params_t;

// types  dt_iop_gmic_params_t and dt_iop_gmic_data_t are
// equal, thus no commit_params function needs to be implemented
typedef dt_iop_gmic_params_t dt_iop_gmic_data_t;

typedef struct dt_iop_gmic_gui_data_t
{
  GtkWidget *gmic_filter;
  std::vector<filters> filter_list;
  dt_iop_gmic_expert_mode_gui_data_t expert_mode;
  dt_iop_gmic_sepia_gui_data_t sepia;
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
  piece->data = nullptr;
}

extern "C" void init_global(dt_iop_module_so_t *self)
{
  self->data = nullptr;
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
  self->gui_data = nullptr;
  dt_iop_gmic_params_t tmp{ none, dt_iop_gmic_all_params_t() };
  std::memcpy(self->params, &tmp, sizeof(dt_iop_gmic_params_t));
  std::memcpy(self->default_params, &tmp, sizeof(dt_iop_gmic_params_t));
}

extern "C" void cleanup(dt_iop_module_t *self)
{
  std::free(self->params);
  self->params = nullptr;
}

static void filter_callback(GtkWidget *w, dt_iop_module_t *self)
{
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  p->filter = g->filter_list[dt_bauhaus_combobox_get(w)];
  dt_iop_gmic_none_params_t::gui_update(self);
  dt_iop_gmic_sepia_params_t::gui_update(self);
  dt_iop_gmic_expert_mode_params_t::gui_update(self);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}


void dt_iop_gmic_none_params_t::gui_update(struct dt_iop_module_t *self)
{
}

void dt_iop_gmic_none_params_t::gui_init(dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_bauhaus_combobox_add(g->gmic_filter, _("none"));
  g->filter_list.push_back(none);
}


void dt_iop_gmic_expert_mode_params_t::gui_update(struct dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->expert_mode.box, p->filter == expert_mode ? TRUE : FALSE);
  gtk_entry_set_text(GTK_ENTRY(g->expert_mode.command), p->parmeters.expert_mode.command);
}

void dt_iop_gmic_expert_mode_params_t::gui_init(dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  dt_bauhaus_combobox_add(g->gmic_filter, _("expert mode"));
  g->expert_mode.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->expert_mode.box, TRUE, TRUE, 0);
  g->expert_mode.command = gtk_entry_new();
  gtk_widget_set_tooltip_text(g->expert_mode.command, _("G'MIC script, confirm with enter"));
  gtk_entry_set_max_length(GTK_ENTRY(g->expert_mode.command), sizeof(command) - 1);
  dt_gui_key_accel_block_on_focus_connect(g->expert_mode.command);
  gtk_box_pack_start(GTK_BOX(g->expert_mode.box), GTK_WIDGET(g->expert_mode.command), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->expert_mode.command), "activate",
                   G_CALLBACK(dt_iop_gmic_expert_mode_params_t::command_callback), self);

  gtk_widget_show_all(g->expert_mode.box);
  gtk_widget_set_no_show_all(g->expert_mode.box, TRUE);
  gtk_widget_set_visible(g->expert_mode.box, p->filter == expert_mode ? TRUE : FALSE);
  g->filter_list.push_back(expert_mode);
}

void dt_iop_gmic_expert_mode_params_t::command_callback(GtkWidget *w, dt_iop_module_t *self)
{
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  std::snprintf(p->parmeters.expert_mode.command, sizeof(p->parmeters.expert_mode.command), "%s",
                gtk_entry_get_text(GTK_ENTRY(w)));
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}


void dt_iop_gmic_sepia_params_t::gui_update(struct dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->sepia.box, p->filter == sepia ? TRUE : FALSE);
  p->parmeters.sepia.brightness = dt_bauhaus_slider_get(g->sepia.brightness);
  p->parmeters.sepia.contrast = dt_bauhaus_slider_get(g->sepia.contrast);
  p->parmeters.sepia.gamma = dt_bauhaus_slider_get(g->sepia.gamma);
}

void dt_iop_gmic_sepia_params_t::gui_init(dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  dt_bauhaus_combobox_add(g->gmic_filter, _("sepia"));
  g->sepia.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->sepia.box, TRUE, TRUE, 0);

  g->sepia.brightness = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, p->parmeters.sepia.brightness, 2);
  dt_bauhaus_widget_set_label(g->sepia.brightness, NULL, _("brightness"));
  gtk_widget_set_tooltip_text(g->sepia.brightness, _("brightness of the sepia effect"));
  gtk_box_pack_start(GTK_BOX(g->sepia.box), GTK_WIDGET(g->sepia.brightness), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sepia.brightness), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sepia_params_t::brightness_callback), self);

  g->sepia.contrast = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, p->parmeters.sepia.contrast, 2);
  dt_bauhaus_widget_set_label(g->sepia.contrast, NULL, _("contrast"));
  gtk_widget_set_tooltip_text(g->sepia.contrast, _("contrast of the sepia effect"));
  gtk_box_pack_start(GTK_BOX(g->sepia.box), GTK_WIDGET(g->sepia.contrast), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sepia.contrast), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sepia_params_t::contrast_callback), self);

  g->sepia.gamma = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, p->parmeters.sepia.gamma, 2);
  dt_bauhaus_widget_set_label(g->sepia.gamma, NULL, _("gamma"));
  gtk_widget_set_tooltip_text(g->sepia.gamma, _("gamma value of the sepia effect"));
  gtk_box_pack_start(GTK_BOX(g->sepia.box), GTK_WIDGET(g->sepia.gamma), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sepia.gamma), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sepia_params_t::gamma_callback), self);

  gtk_widget_show_all(g->sepia.box);
  gtk_widget_set_no_show_all(g->sepia.box, TRUE);
  gtk_widget_set_visible(g->sepia.box, p->filter == sepia ? TRUE : FALSE);
  g->filter_list.push_back(sepia);
}

void dt_iop_gmic_sepia_params_t::brightness_callback(GtkWidget *w, dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  p->parmeters.sepia.brightness = dt_bauhaus_slider_get(g->sepia.brightness);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

void dt_iop_gmic_sepia_params_t::contrast_callback(GtkWidget *w, dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  p->parmeters.sepia.contrast = dt_bauhaus_slider_get(g->sepia.contrast);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

void dt_iop_gmic_sepia_params_t::gamma_callback(GtkWidget *w, dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  p->parmeters.sepia.gamma = dt_bauhaus_slider_get(g->sepia.gamma);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}


extern "C" void gui_update(struct dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  auto i_filter = std::find(g->filter_list.begin(), g->filter_list.end(), p->filter);
  if(i_filter != g->filter_list.end()) dt_bauhaus_combobox_set(g->gmic_filter, i_filter - g->filter_list.begin());
  dt_iop_gmic_none_params_t::gui_update(self);
  dt_iop_gmic_sepia_params_t::gui_update(self);
  dt_iop_gmic_expert_mode_params_t::gui_update(self);
}

extern "C" void gui_init(dt_iop_module_t *self)
{
  self->gui_data = new dt_iop_gmic_gui_data_t;
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  self->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);

  g->gmic_filter = dt_bauhaus_combobox_new(self);
  dt_bauhaus_widget_set_label(g->gmic_filter, NULL, _("G'MIC filter"));
  gtk_widget_set_tooltip_text(g->gmic_filter, _("choose an image processing effect"));
  gtk_box_pack_start(GTK_BOX(self->widget), g->gmic_filter, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->gmic_filter), "value-changed", G_CALLBACK(filter_callback), self);

  dt_iop_gmic_none_params_t::gui_init(self);
  dt_iop_gmic_sepia_params_t::gui_init(self);
  dt_iop_gmic_expert_mode_params_t::gui_init(self);
}


extern "C" void gui_cleanup(dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  delete g;
  self->gui_data = nullptr;
}

extern "C" void process(struct dt_iop_module_t *self, dt_dev_pixelpipe_iop_t *piece, const void *const ivoid,
                        void *const ovoid, const dt_iop_roi_t *const roi_in, const dt_iop_roi_t *const roi_out)
{
  dt_iop_gmic_params_t *d = reinterpret_cast<dt_iop_gmic_params_t *>(piece->data);

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
    std::string command;
    if(d->filter == sepia) command = d->parmeters.sepia.gmic_command();
    if(d->filter == expert_mode) command = d->parmeters.expert_mode.gmic_command();
    gmic(command.c_str(), image_list, image_names);
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
