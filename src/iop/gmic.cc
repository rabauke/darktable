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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "bauhaus/bauhaus.h"
#include "common/darktable.h"
#include "common/file_location.h"
#include "develop/imageop.h"
#include "develop/imageop_math.h"
#include "gui/gtk.h"
#include "iop/iop_api.h"
#include <dirent.h>
#include <gtk/gtk.h>
#include <limits.h>
#include <sys/types.h>

#include <CImg.h>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <gmic.h>
#include <iostream>
#include <string>
#include <vector>

typedef enum filter_type
{
  none,
  expert_mode,
  sepia,
  film_emulation,
  custom_film_emulation,
  freaky_details,
  sharpen_Richardson_Lucy,
  sharpen_Gold_Meinel,
  sharpen_inverse_diffusion,
  magic_details,
  basic_color_adjustments,
  equalize_shadow,
  add_grain,
  pop_shadows,
  smooth_bilateral,
  smooth_guided,
  light_glow,
  lomo
} filter_type;

typedef struct dt_iop_gmic_params_t
{
  filter_type filter;
  char parameters[1024];
} dt_iop_gmic_params_t;


struct parameter_interface
{
  virtual dt_iop_gmic_params_t to_gmic_params() const = 0;
  virtual filter_type get_filter() const = 0;
  virtual void gui_init(dt_iop_module_t *self) const = 0;
  virtual void gui_update(dt_iop_module_t *self) const = 0;
  virtual void gui_reset(dt_iop_module_t *self) = 0;
};

struct dt_iop_gmic_gui_data_t;

// --- none filter

struct dt_iop_gmic_none_params_t : public parameter_interface
{
  dt_iop_gmic_none_params_t() = default;
  dt_iop_gmic_none_params_t(const dt_iop_gmic_params_t &);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
};

struct dt_iop_gmic_none_gui_data_t
{
  dt_iop_gmic_none_params_t parameters;
};

// --- expert mode

struct dt_iop_gmic_expert_mode_params_t : public parameter_interface
{
  char command[1024]{ "" };
  dt_iop_gmic_expert_mode_params_t() = default;
  dt_iop_gmic_expert_mode_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void command_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_expert_mode_gui_data_t
{
  GtkWidget *box;
  GtkWidget *command;
  dt_iop_gmic_expert_mode_params_t parameters;
};

// --- sepia

struct dt_iop_gmic_sepia_params_t : public parameter_interface
{
  float brightness{ 0.f }, contrast{ 0.f }, gamma{ 0.f };
  dt_iop_gmic_sepia_params_t() = default;
  dt_iop_gmic_sepia_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void brightness_callback(GtkWidget *w, dt_iop_module_t *self);
  static void contrast_callback(GtkWidget *w, dt_iop_module_t *self);
  static void gamma_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_sepia_gui_data_t
{
  GtkWidget *box;
  GtkWidget *brightness, *contrast, *gamma;
  dt_iop_gmic_sepia_params_t parameters;
};

// --- film emulation

struct film_map
{
  std::string film_type;
  std::string printable;
  film_map(const char *film_type_, const char *printable_) : film_type(film_type_), printable(printable_)
  {
  }

  film_map(const std::string &film_type_, const std::string &printable_)
    : film_type(film_type_), printable(printable_)
  {
  }
};

struct dt_iop_gmic_film_emulation_params_t : public parameter_interface
{
  char film[128]{ "agfa_apx_25" };
  float strength{ 1.f }, brightness{ 0.f }, contrast{ 0.f }, gamma{ 0.f }, hue{ 0.f }, saturation{ 0.f };
  int normalize_colors{ 0 };
  static const std::vector<film_map> film_maps;
  dt_iop_gmic_film_emulation_params_t() = default;
  dt_iop_gmic_film_emulation_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void film_callback(GtkWidget *w, dt_iop_module_t *self);
  static void strength_callback(GtkWidget *w, dt_iop_module_t *self);
  static void brightness_callback(GtkWidget *w, dt_iop_module_t *self);
  static void contrast_callback(GtkWidget *w, dt_iop_module_t *self);
  static void gamma_callback(GtkWidget *w, dt_iop_module_t *self);
  static void hue_callback(GtkWidget *w, dt_iop_module_t *self);
  static void saturation_callback(GtkWidget *w, dt_iop_module_t *self);
  static void normalize_colors_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_film_emulation_gui_data_t
{
  GtkWidget *box;
  GtkWidget *film, *strength, *brightness, *contrast, *gamma, *hue, *saturation, *normalize_colors;
  std::vector<std::string> film_list;
  dt_iop_gmic_film_emulation_params_t parameters;
};

// --- custom film emulation

struct dt_iop_gmic_custom_film_emulation_params_t : public parameter_interface
{
  char film[1024]{ "" };
  float strength{ 1.f }, brightness{ 0.f }, contrast{ 0.f }, gamma{ 0.f }, hue{ 0.f }, saturation{ 0.f };
  int normalize_colors{ 0 };
  static const std::vector<film_map> film_maps;
  dt_iop_gmic_custom_film_emulation_params_t();
  dt_iop_gmic_custom_film_emulation_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void film_callback(GtkWidget *w, dt_iop_module_t *self);
  static void strength_callback(GtkWidget *w, dt_iop_module_t *self);
  static void brightness_callback(GtkWidget *w, dt_iop_module_t *self);
  static void contrast_callback(GtkWidget *w, dt_iop_module_t *self);
  static void gamma_callback(GtkWidget *w, dt_iop_module_t *self);
  static void hue_callback(GtkWidget *w, dt_iop_module_t *self);
  static void saturation_callback(GtkWidget *w, dt_iop_module_t *self);
  static void normalize_colors_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_custom_film_emulation_gui_data_t
{
  GtkWidget *box;
  GtkWidget *film, *strength, *brightness, *contrast, *gamma, *hue, *saturation, *normalize_colors;
  std::vector<std::string> film_list;
  dt_iop_gmic_custom_film_emulation_params_t parameters;
};

// --- freaky details

struct dt_iop_gmic_freaky_details_params_t : public parameter_interface
{
  int amplitude{ 2 };
  float scale{ 10.f };
  int iterations{ 1 }, channel{ 11 };
  dt_iop_gmic_freaky_details_params_t() = default;
  dt_iop_gmic_freaky_details_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void amplitude_callback(GtkWidget *w, dt_iop_module_t *self);
  static void scale_callback(GtkWidget *w, dt_iop_module_t *self);
  static void iterations_callback(GtkWidget *w, dt_iop_module_t *self);
  static void channel_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_freaky_details_gui_data_t
{
  GtkWidget *box;
  GtkWidget *ampltude, *scale, *iterations, *channel;
  dt_iop_gmic_freaky_details_params_t parameters;
};

// --- sharpen Richardson Lucy

struct dt_iop_gmic_sharpen_Richardson_Lucy_params_t : public parameter_interface
{
  float sigma{ 1.f };
  int iterations{ 10 }, blur{ 1 }, channel{ 11 };
  dt_iop_gmic_sharpen_Richardson_Lucy_params_t() = default;
  dt_iop_gmic_sharpen_Richardson_Lucy_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void sigma_callback(GtkWidget *w, dt_iop_module_t *self);
  static void iterations_callback(GtkWidget *w, dt_iop_module_t *self);
  static void blur_callback(GtkWidget *w, dt_iop_module_t *self);
  static void channel_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_sharpen_Richardson_Lucy_gui_data_t
{
  GtkWidget *box;
  GtkWidget *sigma, *iterations, *blur, *channel;
  dt_iop_gmic_sharpen_Richardson_Lucy_params_t parameters;
};

// --- sharpen Gold Meinel

struct dt_iop_gmic_sharpen_Gold_Meinel_params_t : public parameter_interface
{
  float sigma{ 1.f };
  int iterations{ 5 };
  float acceleration{ 1.f };
  int blur{ 1 }, channel{ 11 };
  dt_iop_gmic_sharpen_Gold_Meinel_params_t() = default;
  dt_iop_gmic_sharpen_Gold_Meinel_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void sigma_callback(GtkWidget *w, dt_iop_module_t *self);
  static void iterations_callback(GtkWidget *w, dt_iop_module_t *self);
  static void acceleration_callback(GtkWidget *w, dt_iop_module_t *self);
  static void blur_callback(GtkWidget *w, dt_iop_module_t *self);
  static void channel_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_sharpen_Gold_Meinel_gui_data_t
{
  GtkWidget *box;
  GtkWidget *sigma, *iterations, *acceleration, *blur, *channel;
  dt_iop_gmic_sharpen_Gold_Meinel_params_t parameters;
};

// --- sharpen inverse diffusion

struct dt_iop_gmic_sharpen_inverse_diffusion_params_t : public parameter_interface
{
  float amplitude{ 50.f };
  int iterations{ 2 }, channel{ 11 };
  dt_iop_gmic_sharpen_inverse_diffusion_params_t() = default;
  dt_iop_gmic_sharpen_inverse_diffusion_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void amplitude_callback(GtkWidget *w, dt_iop_module_t *self);
  static void iterations_callback(GtkWidget *w, dt_iop_module_t *self);
  static void channel_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_sharpen_inverse_diffusion_gui_data_t
{
  GtkWidget *box;
  GtkWidget *ampltude, *iterations, *channel;
  dt_iop_gmic_sharpen_inverse_diffusion_params_t parameters;
};

// --- magic details

struct dt_iop_gmic_magic_details_params_t : public parameter_interface
{
  float amplitude{ 6.f };
  float spatial_scale{ 3.f };
  float value_scale{ 15.f };
  float edges{ -0.5f };
  float smoothness{ 2.f };
  int channel{ 27 };
  dt_iop_gmic_magic_details_params_t() = default;
  dt_iop_gmic_magic_details_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void amplitude_callback(GtkWidget *w, dt_iop_module_t *self);
  static void spatial_scale_callback(GtkWidget *w, dt_iop_module_t *self);
  static void value_scale_callback(GtkWidget *w, dt_iop_module_t *self);
  static void edges_callback(GtkWidget *w, dt_iop_module_t *self);
  static void smoothness_callback(GtkWidget *w, dt_iop_module_t *self);
  static void channel_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_magic_details_gui_data_t
{
  GtkWidget *box;
  GtkWidget *ampltude, *spatial_scale, *value_scale, *edges, *smoothness, *channel;
  dt_iop_gmic_magic_details_params_t parameters;
};

// --- basic color adjustments

struct dt_iop_gmic_basic_color_adjustments_params_t : public parameter_interface
{
  float brightness{ 0.f }, contrast{ 0.f }, gamma{ 0.f }, hue{ 0.f }, saturation{ 0.f };
  dt_iop_gmic_basic_color_adjustments_params_t() = default;
  dt_iop_gmic_basic_color_adjustments_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void brightness_callback(GtkWidget *w, dt_iop_module_t *self);
  static void contrast_callback(GtkWidget *w, dt_iop_module_t *self);
  static void gamma_callback(GtkWidget *w, dt_iop_module_t *self);
  static void hue_callback(GtkWidget *w, dt_iop_module_t *self);
  static void saturation_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_basic_color_adjustments_gui_data_t
{
  GtkWidget *box;
  GtkWidget *brightness, *contrast, *gamma, *hue, *saturation;
  dt_iop_gmic_basic_color_adjustments_params_t parameters;
};

// --- equalize shadow

struct dt_iop_gmic_equalize_shadow_params_t : public parameter_interface
{
  float amplitude{ 1.f };
  dt_iop_gmic_equalize_shadow_params_t() = default;
  dt_iop_gmic_equalize_shadow_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void amplitude_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_equalize_shadow_gui_data_t
{
  GtkWidget *box;
  GtkWidget *amplitude;
  dt_iop_gmic_equalize_shadow_params_t parameters;
};

// --- add grain

struct dt_iop_gmic_add_grain_params_t : public parameter_interface
{
  int preset{ 0 }, blend_mode{ 1 };
  float opacity{ 0.2f }, scale{ 100.f };
  int color_grain{ 0 };
  float brightness{ 0.f }, contrast{ 0.f }, gamma{ 0.f }, hue{ 0.f }, saturation{ 0.f };
  dt_iop_gmic_add_grain_params_t() = default;
  dt_iop_gmic_add_grain_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void preset_callback(GtkWidget *w, dt_iop_module_t *self);
  static void blend_mode_callback(GtkWidget *w, dt_iop_module_t *self);
  static void opacity_callback(GtkWidget *w, dt_iop_module_t *self);
  static void scale_callback(GtkWidget *w, dt_iop_module_t *self);
  static void color_grain_callback(GtkWidget *w, dt_iop_module_t *self);
  static void brightness_callback(GtkWidget *w, dt_iop_module_t *self);
  static void contrast_callback(GtkWidget *w, dt_iop_module_t *self);
  static void gamma_callback(GtkWidget *w, dt_iop_module_t *self);
  static void hue_callback(GtkWidget *w, dt_iop_module_t *self);
  static void saturation_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_add_grain_gui_data_t
{
  GtkWidget *box;
  GtkWidget *preset, *blend_mode, *opacity, *scale, *color_grain, *brightness, *contrast, *gamma, *hue,
      *saturation;
  dt_iop_gmic_add_grain_params_t parameters;
};

// --- pop shadows

struct dt_iop_gmic_pop_shadows_params_t : public parameter_interface
{
  float strength{ 0.75f };
  float scale{ 5.f };
  dt_iop_gmic_pop_shadows_params_t() = default;
  dt_iop_gmic_pop_shadows_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void strength_callback(GtkWidget *w, dt_iop_module_t *self);
  static void scale_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_pop_shadows_gui_data_t
{
  GtkWidget *box;
  GtkWidget *strength, *scale;
  dt_iop_gmic_pop_shadows_params_t parameters;
};

// --- smooth bilateral

struct dt_iop_gmic_smooth_bilateral_params_t : public parameter_interface
{
  float spatial_scale{ 5.f };
  float value_scale{ 0.02f };
  int iterations{ 2 };
  int channel{ 0 };
  dt_iop_gmic_smooth_bilateral_params_t() = default;
  dt_iop_gmic_smooth_bilateral_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void spatial_scale_callback(GtkWidget *w, dt_iop_module_t *self);
  static void value_scale_callback(GtkWidget *w, dt_iop_module_t *self);
  static void iterations_callback(GtkWidget *w, dt_iop_module_t *self);
  static void channel_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_smooth_bilateral_gui_data_t
{
  GtkWidget *box;
  GtkWidget *spatial_scale, *value_scale, *iterations, *channel;
  dt_iop_gmic_smooth_bilateral_params_t parameters;
};

// --- smooth guided

struct dt_iop_gmic_smooth_guided_params_t : public parameter_interface
{
  float radius{ 5.f };
  float smoothness{ 0.05f };
  int iterations{ 1 };
  int channel{ 0 };
  dt_iop_gmic_smooth_guided_params_t() = default;
  dt_iop_gmic_smooth_guided_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void radius_callback(GtkWidget *w, dt_iop_module_t *self);
  static void smoothness_callback(GtkWidget *w, dt_iop_module_t *self);
  static void iterations_callback(GtkWidget *w, dt_iop_module_t *self);
  static void channel_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_smooth_guided_gui_data_t
{
  GtkWidget *box;
  GtkWidget *radius, *smoothness, *iterations, *channel;
  dt_iop_gmic_smooth_guided_params_t parameters;
};

// --- light glow

struct dt_iop_gmic_light_glow_params_t : public parameter_interface
{
  float density{ 0.3f };
  float amplitude{ 0.5f };
  int blend_mode{ 8 };
  float opacity{ 0.8f };
  int channel{ 0 };
  dt_iop_gmic_light_glow_params_t() = default;
  dt_iop_gmic_light_glow_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void density_callback(GtkWidget *w, dt_iop_module_t *self);
  static void amplitude_callback(GtkWidget *w, dt_iop_module_t *self);
  static void blend_mode_callback(GtkWidget *w, dt_iop_module_t *self);
  static void opacity_callback(GtkWidget *w, dt_iop_module_t *self);
  static void channel_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_light_glow_gui_data_t
{
  GtkWidget *box;
  GtkWidget *density, *amplitude, *blend_mode, *opacity, *channel;
  dt_iop_gmic_light_glow_params_t parameters;
};

// --- lomo

struct dt_iop_gmic_lomo_params_t : public parameter_interface
{
  float vignette_size{ 0.2f };
  dt_iop_gmic_lomo_params_t() = default;
  dt_iop_gmic_lomo_params_t(const dt_iop_gmic_params_t &other);
  dt_iop_gmic_params_t to_gmic_params() const override;
  static const char *get_custom_command();
  filter_type get_filter() const override;
  void gui_init(dt_iop_module_t *self) const override;
  void gui_update(dt_iop_module_t *self) const override;
  void gui_reset(dt_iop_module_t *self) override;
  static void vignette_size_callback(GtkWidget *w, dt_iop_module_t *self);
};

struct dt_iop_gmic_lomo_gui_data_t
{
  GtkWidget *box;
  GtkWidget *vignette_size;
  dt_iop_gmic_lomo_params_t parameters;
};

//----------------------------------------------------------------------
// implement the module api
//----------------------------------------------------------------------

DT_MODULE_INTROSPECTION(1, dt_iop_gmic_params_t)

// types  dt_iop_gmic_params_t and dt_iop_gmic_data_t are
// equal, thus no commit_params function needs to be implemented
typedef dt_iop_gmic_params_t dt_iop_gmic_data_t;

struct dt_iop_gmic_gui_data_t
{
  GtkWidget *gmic_filter;
  dt_iop_gmic_none_gui_data_t none;
  dt_iop_gmic_expert_mode_gui_data_t expert_mode;
  dt_iop_gmic_sepia_gui_data_t sepia;
  dt_iop_gmic_film_emulation_gui_data_t film_emulation;
  dt_iop_gmic_custom_film_emulation_gui_data_t custom_film_emulation;
  dt_iop_gmic_freaky_details_gui_data_t freaky_details;
  dt_iop_gmic_sharpen_Richardson_Lucy_gui_data_t sharpen_Richardson_Lucy;
  dt_iop_gmic_sharpen_Gold_Meinel_gui_data_t sharpen_Gold_Meinel;
  dt_iop_gmic_sharpen_inverse_diffusion_gui_data_t sharpen_inverse_diffusion;
  dt_iop_gmic_magic_details_gui_data_t magic_details;
  dt_iop_gmic_basic_color_adjustments_gui_data_t basic_color_adjustments;
  dt_iop_gmic_equalize_shadow_gui_data_t equalize_shadow;
  dt_iop_gmic_add_grain_gui_data_t add_grain;
  dt_iop_gmic_pop_shadows_gui_data_t pop_shadows;
  dt_iop_gmic_smooth_bilateral_gui_data_t smooth_bilateral;
  dt_iop_gmic_smooth_guided_gui_data_t smooth_guided;
  dt_iop_gmic_light_glow_gui_data_t light_glow;
  dt_iop_gmic_lomo_gui_data_t lomo;
  const std::vector<parameter_interface *> filter_list{ &none.parameters,
                                                        &basic_color_adjustments.parameters,
                                                        &sharpen_Richardson_Lucy.parameters,
                                                        &sharpen_Gold_Meinel.parameters,
                                                        &sharpen_inverse_diffusion.parameters,
                                                        &smooth_bilateral.parameters,
                                                        &smooth_guided.parameters,
                                                        &freaky_details.parameters,
                                                        &magic_details.parameters,
                                                        &equalize_shadow.parameters,
                                                        &pop_shadows.parameters,
                                                        &light_glow.parameters,
                                                        &sepia.parameters,
                                                        &film_emulation.parameters,
                                                        &custom_film_emulation.parameters,
                                                        &add_grain.parameters,
                                                        &lomo.parameters,
                                                        &expert_mode.parameters };
  dt_iop_gmic_gui_data_t() = default;
  dt_iop_gmic_gui_data_t(const dt_iop_gmic_gui_data_t &) = delete;
  void operator=(const dt_iop_gmic_gui_data_t &) = delete;
};

struct dt_iop_gmic_global_data_t
{
};

extern "C" const char *name()
{
  return _("G'MIC");
}

extern "C" int flags()
{
  return IOP_FLAGS_INCLUDE_IN_STYLES | IOP_FLAGS_SUPPORTS_BLENDING;
}

extern "C" int default_group()
{
  return IOP_GROUP_EFFECT;
}

extern "C" int operation_tags()
{
  return IOP_TAG_NONE;
}

extern "C" dt_iop_colorspace_type_t default_colorspace(dt_iop_module_t *self, dt_dev_pixelpipe_t *pipe, dt_dev_pixelpipe_iop_t *piece)
{
  return IOP_CS_RGB;
}

extern "C" void init_pipe(dt_iop_module_t *self, dt_dev_pixelpipe_t *pipe, dt_dev_pixelpipe_iop_t *piece)
{
  piece->data = std::calloc(1, sizeof(dt_iop_gmic_data_t));
  self->commit_params(self, self->default_params, pipe, piece);
}

extern "C" void cleanup_pipe(dt_iop_module_t *self, dt_dev_pixelpipe_t *pipe, dt_dev_pixelpipe_iop_t *piece)
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
  self->params_size = sizeof(dt_iop_gmic_params_t);
  self->gui_data = nullptr;
  dt_iop_gmic_params_t tmp{ none, "" };
  std::memcpy(self->params, &tmp, sizeof(dt_iop_gmic_params_t));
  std::memcpy(self->default_params, &tmp, sizeof(dt_iop_gmic_params_t));
}

extern "C" void cleanup(dt_iop_module_t *self)
{
  std::free(self->params);
  self->params = nullptr;
}

static void filter_callback(GtkWidget *w, dt_iop_module_t *self);

extern "C" void gui_init(dt_iop_module_t *self)
{
  self->gui_data = reinterpret_cast<dt_iop_gui_data_t *>(new dt_iop_gmic_gui_data_t);
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  self->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  g->gmic_filter = dt_bauhaus_combobox_new(self);
  dt_bauhaus_widget_set_label(g->gmic_filter, NULL, _("G'MIC filter"));
  gtk_widget_set_tooltip_text(g->gmic_filter, _("choose an image processing filter"));
  gtk_box_pack_start(GTK_BOX(self->widget), g->gmic_filter, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->gmic_filter), "value-changed", G_CALLBACK(filter_callback), self);
  for(const auto f : g->filter_list) f->gui_init(self);
}

extern "C" void gui_update(dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  auto i_filter = std::find_if(g->filter_list.begin(), g->filter_list.end(),
                               [p](parameter_interface *i) { return p->filter == i->get_filter(); });
  if(i_filter != g->filter_list.end()) dt_bauhaus_combobox_set(g->gmic_filter, i_filter - g->filter_list.begin());
  for(const auto f : g->filter_list) f->gui_update(self);
}

extern "C" void gui_reset(dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  for(const auto f : g->filter_list) f->gui_reset(self);
}

extern "C" void gui_cleanup(dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  delete g;
  self->gui_data = nullptr;
}

extern "C" void process(dt_iop_module_t *self, dt_dev_pixelpipe_iop_t *piece, const void *const ivoid,
                        void *const ovoid, const dt_iop_roi_t *const roi_in, const dt_iop_roi_t *const roi_out)
{
  static const std::string gmic_custom_commands = [] {
    std::string res;
    res += dt_iop_gmic_none_params_t::get_custom_command();
    res += dt_iop_gmic_expert_mode_params_t::get_custom_command();
    res += dt_iop_gmic_sepia_params_t::get_custom_command();
    res += dt_iop_gmic_film_emulation_params_t::get_custom_command();
    res += dt_iop_gmic_custom_film_emulation_params_t::get_custom_command();
    res += dt_iop_gmic_freaky_details_params_t::get_custom_command();
    res += dt_iop_gmic_sharpen_Richardson_Lucy_params_t::get_custom_command();
    res += dt_iop_gmic_sharpen_Gold_Meinel_params_t::get_custom_command();
    res += dt_iop_gmic_sharpen_inverse_diffusion_params_t::get_custom_command();
    res += dt_iop_gmic_magic_details_params_t::get_custom_command();
    res += dt_iop_gmic_basic_color_adjustments_params_t::get_custom_command();
    res += dt_iop_gmic_equalize_shadow_params_t::get_custom_command();
    res += dt_iop_gmic_add_grain_params_t::get_custom_command();
    res += dt_iop_gmic_pop_shadows_params_t::get_custom_command();
    res += dt_iop_gmic_smooth_bilateral_params_t::get_custom_command();
    res += dt_iop_gmic_smooth_guided_params_t::get_custom_command();
    res += dt_iop_gmic_light_glow_params_t::get_custom_command();
    res += dt_iop_gmic_lomo_params_t::get_custom_command();
    return res;
  }();

  const std::size_t ch = piece->colors;
  const std::size_t width = roi_in->width;
  const std::size_t height = roi_in->height;

  const float *const in = reinterpret_cast<const float *>(DT_IS_ALIGNED(ivoid));
  float *const out = reinterpret_cast<float *>(DT_IS_ALIGNED(ovoid));

  cimg_library::CImg<float> in_img(width, height, 1, 3);
  for(std::size_t j = 0; j < height; ++j)
    for(std::size_t i = 0; i < width; ++i)
    {
      const float *pixel_in = in + (j * width + i) * ch;
      in_img(i, j, 0, 0) = pixel_in[0] * 255;
      in_img(i, j, 0, 1) = pixel_in[1] * 255;
      in_img(i, j, 0, 2) = pixel_in[2] * 255;
    }
  cimg_library::CImgList<float> image_list;
  in_img.move_to(image_list);
  cimg_library::CImgList<char> image_names;
  cimg_library::CImg<char>::string("input image").move_to(image_names);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(piece->data);
  try
  {
    std::cerr << "### G'MIC : " << p->parameters << std::endl;
    gmic(p->parameters, image_list, image_names, gmic_custom_commands.c_str());
  }
  catch(gmic_exception &e)
  {
    if(piece->pipe->type == DT_DEV_PIXELPIPE_FULL || piece->pipe->type == DT_DEV_PIXELPIPE_PREVIEW)
      dt_control_log("G'MIC error: %s", e.what());
  }

  if(image_list.is_empty())
  {
    std::memcpy(out, in, width * height * ch * sizeof(float));
  }
  else
  {
    const cimg_library::CImg<float> &out_img(image_list[0]);
    auto ch_out_img = out_img._spectrum;
    for(std::size_t j = 0; j < std::min(height, static_cast<std::size_t>(out_img._height)); ++j)
    {
      for(std::size_t i = 0; i < std::min(width, static_cast<std::size_t>(out_img._width)); ++i)
      {
        float *pixel_out = out + (j * width + i) * ch;
        pixel_out[0] = ch_out_img > 0 ? out_img(i, j, 0, 0) / 255 : 0.f;
        pixel_out[1] = ch_out_img > 1 ? out_img(i, j, 0, 1) / 255 : 0.f;
        pixel_out[2] = ch_out_img > 2 ? out_img(i, j, 0, 2) / 255 : 0.f;
      }
      for(std::size_t i = std::min(width, static_cast<std::size_t>(out_img._width)); i < width; ++i)
      {
        float *pixel_out = out + (j * width + i) * ch;
        pixel_out[0] = 0;
        pixel_out[1] = 0;
        pixel_out[2] = 0;
      }
    }
    for(std::size_t j = std::min(height, static_cast<std::size_t>(out_img._height)); j < height; ++j)
      for(std::size_t i = 0; i < width; ++i)
      {
        float *pixel_out = out + (j * width + i) * ch;
        pixel_out[0] = 0;
        pixel_out[1] = 0;
        pixel_out[2] = 0;
      }
  }

  if(piece->pipe->mask_display & DT_DEV_PIXELPIPE_DISPLAY_MASK)
    dt_iop_alpha_copy(in, out, roi_out->width, roi_out->height);
}

// --- helper funcions

template <typename T> T clamp(const T a, const T b, const T x)
{
  return std::min(b, std::max(a, x));
}


template <typename F> static void callback(GtkWidget *w, dt_iop_module_t *self, F f)
{
  if(darktable.gui->reset) return;
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  *p = f(g, w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}


static const std::vector<const char *> color_channels{ _("all"),
                                                       _("RGBA (all)"),
                                                       _("RGB (all)"),
                                                       _("RGB (red)"),
                                                       _("RGB (green)"),
                                                       _("RGB (blue)"),
                                                       _("RGBA (alpha)"),
                                                       _("linear RGB (all)"),
                                                       _("linear RGB (red)"),
                                                       _("linear RGB (green)"),
                                                       _("linear RGB (blue)"),
                                                       _("YCbCr (luminance)"),
                                                       _("YCbCr (blue-red chrominances)"),
                                                       _("YCbCr (blue chrominance)"),
                                                       _("YCbCr (red chrominance)"),
                                                       _("YCbCr (green chrominance)"),
                                                       _("Lab (lightness)"),
                                                       _("Lab (ab-chrominances)"),
                                                       _("Lab (a-chrominance)"),
                                                       _("Lab (b-chrominance)"),
                                                       _("Lch (ch-chrominances)"),
                                                       _("Lch (c-chrominance)"),
                                                       _("Lch (h-chrominance)"),
                                                       _("HSV (hue)"),
                                                       _("HSV (saturation)"),
                                                       _("HSV (value)"),
                                                       _("HSI (intensity)"),
                                                       _("HSL (lightness)"),
                                                       _("CMYK (cyan)"),
                                                       _("CMYK (magenta)"),
                                                       _("CMYK (yellow)"),
                                                       _("CMYK (key)"),
                                                       _("YIQ (luma)"),
                                                       _("YIQ (chromas)") };


void filter_callback(GtkWidget *w, dt_iop_module_t *self)
{
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  *p = g->filter_list[dt_bauhaus_combobox_get(w)]->to_gmic_params();
  for(const auto f : g->filter_list) f->gui_update(self);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

// --- none filter

dt_iop_gmic_none_params_t::dt_iop_gmic_none_params_t(const dt_iop_gmic_params_t &) : dt_iop_gmic_none_params_t()
{
}

dt_iop_gmic_params_t dt_iop_gmic_none_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = none;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "%s", "");
  return ret;
}

const char *dt_iop_gmic_none_params_t::get_custom_command()
{
  return "";
}

filter_type dt_iop_gmic_none_params_t::get_filter() const
{
  return none;
}

void dt_iop_gmic_none_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_bauhaus_combobox_add(g->gmic_filter, _("none"));
  g->none.parameters = dt_iop_gmic_none_params_t();
}

void dt_iop_gmic_none_params_t::gui_update(dt_iop_module_t *self) const
{
}

void dt_iop_gmic_none_params_t::gui_reset(dt_iop_module_t *self)
{
}

// --- expert mode filter

dt_iop_gmic_expert_mode_params_t::dt_iop_gmic_expert_mode_params_t(const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_expert_mode_params_t()
{
  if(other.filter == expert_mode)
    std::strncpy(command, other.parameters, std::min(sizeof(command), sizeof(other.parameters)));
}

dt_iop_gmic_params_t dt_iop_gmic_expert_mode_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = expert_mode;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "%s", command);
  return ret;
}

const char *dt_iop_gmic_expert_mode_params_t::get_custom_command()
{
  return "";
}

filter_type dt_iop_gmic_expert_mode_params_t::get_filter() const
{
  return expert_mode;
}

void dt_iop_gmic_expert_mode_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == expert_mode)
    g->expert_mode.parameters = *p;
  else
    g->expert_mode.parameters = dt_iop_gmic_expert_mode_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("expert mode"));
  g->expert_mode.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->expert_mode.box, TRUE, TRUE, 0);

  g->expert_mode.command = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(g->expert_mode.command), g->expert_mode.parameters.command);
  gtk_widget_set_tooltip_text(g->expert_mode.command, _("G'MIC script, confirm with enter"));
  gtk_entry_set_max_length(GTK_ENTRY(g->expert_mode.command), sizeof(command) - 1);
  gtk_box_pack_start(GTK_BOX(g->expert_mode.box), GTK_WIDGET(g->expert_mode.command), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->expert_mode.command), "activate",
                   G_CALLBACK(dt_iop_gmic_expert_mode_params_t::command_callback), self);

  gtk_widget_show_all(g->expert_mode.box);
  gtk_widget_set_no_show_all(g->expert_mode.box, TRUE);
  gtk_widget_set_visible(g->expert_mode.box, p->filter == expert_mode ? TRUE : FALSE);
}

void dt_iop_gmic_expert_mode_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->expert_mode.box, p->filter == expert_mode ? TRUE : FALSE);
  if(p->filter == expert_mode)
  {
    g->expert_mode.parameters = *p;
    gtk_entry_set_text(GTK_ENTRY(g->expert_mode.command), g->expert_mode.parameters.command);
  }
}

void dt_iop_gmic_expert_mode_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_expert_mode_params_t();
}

void dt_iop_gmic_expert_mode_params_t::command_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    std::snprintf(G->expert_mode.parameters.command, sizeof(G->expert_mode.parameters.command), "%s",
                  gtk_entry_get_text(GTK_ENTRY(W)));
    return G->expert_mode.parameters.to_gmic_params();
  });
}

// --- sepia filter

dt_iop_gmic_sepia_params_t::dt_iop_gmic_sepia_params_t(const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_sepia_params_t()
{
  dt_iop_gmic_sepia_params_t p;
  if(other.filter == sepia
     and std::sscanf(other.parameters, "dt_sepia %g,%g,%g", &p.brightness, &p.contrast, &p.gamma) == 3)
  {
    p.brightness = clamp(-1.f, 1.f, p.brightness);
    p.contrast = clamp(-1.f, 1.f, p.contrast);
    p.gamma = clamp(-1.f, 1.f, p.gamma);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_sepia_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = sepia;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_sepia %g,%g,%g", brightness, contrast, gamma);
  return ret;
}

const char *dt_iop_gmic_sepia_params_t::get_custom_command() {
  // clang-format off
  return R"raw(
dt_sepia :
  sepia adjust_colors {100*$1},{100*$2},{100*$3},0,0,0,255
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_sepia_params_t::get_filter() const
{
  return sepia;
}

void dt_iop_gmic_sepia_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == sepia)
    g->sepia.parameters = *p;
  else
    g->sepia.parameters = dt_iop_gmic_sepia_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("sepia"));
  g->sepia.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->sepia.box, TRUE, TRUE, 0);

  g->sepia.brightness = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->sepia.parameters.brightness, 3);
  dt_bauhaus_widget_set_label(g->sepia.brightness, NULL, _("brightness"));
  gtk_widget_set_tooltip_text(g->sepia.brightness, _("brightness of the sepia effect"));
  gtk_box_pack_start(GTK_BOX(g->sepia.box), GTK_WIDGET(g->sepia.brightness), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sepia.brightness), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sepia_params_t::brightness_callback), self);

  g->sepia.contrast = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->sepia.parameters.contrast, 3);
  dt_bauhaus_widget_set_label(g->sepia.contrast, NULL, _("contrast"));
  gtk_widget_set_tooltip_text(g->sepia.contrast, _("contrast of the sepia effect"));
  gtk_box_pack_start(GTK_BOX(g->sepia.box), GTK_WIDGET(g->sepia.contrast), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sepia.contrast), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sepia_params_t::contrast_callback), self);

  g->sepia.gamma = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->sepia.parameters.gamma, 3);
  dt_bauhaus_widget_set_label(g->sepia.gamma, NULL, _("gamma"));
  gtk_widget_set_tooltip_text(g->sepia.gamma, _("gamma value of the sepia effect"));
  gtk_box_pack_start(GTK_BOX(g->sepia.box), GTK_WIDGET(g->sepia.gamma), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sepia.gamma), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sepia_params_t::gamma_callback), self);

  gtk_widget_show_all(g->sepia.box);
  gtk_widget_set_no_show_all(g->sepia.box, TRUE);
  gtk_widget_set_visible(g->sepia.box, p->filter == sepia ? TRUE : FALSE);
}

void dt_iop_gmic_sepia_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->sepia.box, p->filter == sepia ? TRUE : FALSE);
  if(p->filter == sepia)
  {
    g->sepia.parameters = *p;
    dt_bauhaus_slider_set(g->sepia.brightness, g->sepia.parameters.brightness);
    dt_bauhaus_slider_set(g->sepia.contrast, g->sepia.parameters.contrast);
    dt_bauhaus_slider_set(g->sepia.gamma, g->sepia.parameters.gamma);
  }
}

void dt_iop_gmic_sepia_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_sepia_params_t();
}

void dt_iop_gmic_sepia_params_t::brightness_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->sepia.parameters.brightness = dt_bauhaus_slider_get(W);
    return G->sepia.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_sepia_params_t::contrast_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->sepia.parameters.contrast = dt_bauhaus_slider_get(W);
    return G->sepia.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_sepia_params_t::gamma_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->sepia.parameters.gamma = dt_bauhaus_slider_get(W);
    return G->sepia.parameters.to_gmic_params();
  });
}

// --- film emulation filter

dt_iop_gmic_film_emulation_params_t::dt_iop_gmic_film_emulation_params_t(const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_film_emulation_params_t()
{
  dt_iop_gmic_film_emulation_params_t p;
  if(other.filter == film_emulation
     and std::sscanf(other.parameters, "dt_film_emulation \"%127[^\"]\",%g,%g,%g,%g,%g,%g,%i",
                     reinterpret_cast<char *>(&p.film), &p.strength, &p.brightness, &p.contrast, &p.gamma, &p.hue,
                     &p.saturation, &p.normalize_colors)
             == 8)
  {
    p.strength = clamp(0.f, 1.f, p.strength);
    p.brightness = clamp(-1.f, 1.f, p.brightness);
    p.contrast = clamp(-1.f, 1.f, p.contrast);
    p.gamma = clamp(-1.f, 1.f, p.gamma);
    p.hue = clamp(-1.f, 1.f, p.hue);
    p.saturation = clamp(-1.f, 1.f, p.saturation);
    p.normalize_colors = clamp(0, 1, p.normalize_colors);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_film_emulation_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = film_emulation;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_film_emulation \"%s\",%g,%g,%g,%g,%g,%g,%i", film,
                strength, brightness, contrast, gamma, hue, saturation, normalize_colors);
  return ret;
}

const char *dt_iop_gmic_film_emulation_params_t::get_custom_command()
{
  // clang-format off
  return R"raw(
dt_film_emulation :
  clut "$1"
  repeat {$!-1}
    if {$8%2} balance_gamma[$>] , fi
    if {$2<1} +map_clut[$>] . j[$>] .,0,0,0,0,{$2} rm.
    else map_clut[$>] .
    fi
  done
  rm.
  adjust_colors {100*$3},{100*$4},{100*$5},{100*$6},{100*$7},0,255
  if {$8>1} repeat $! l[$>] split_opacity n[0] 0,255 a c endl done fi
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_film_emulation_params_t::get_filter() const
{
  return film_emulation;
}

void dt_iop_gmic_film_emulation_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == film_emulation)
    g->film_emulation.parameters = *p;
  else
    g->film_emulation.parameters = dt_iop_gmic_film_emulation_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("film emulation"));
  g->film_emulation.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->film_emulation.box, TRUE, TRUE, 0);

  g->film_emulation.film = dt_bauhaus_combobox_new(self);
  for(const auto &film_map : film_maps)
  {
    dt_bauhaus_combobox_add_aligned(g->film_emulation.film, film_map.printable.c_str(),
                                    DT_BAUHAUS_COMBOBOX_ALIGN_LEFT);
    g->film_emulation.film_list.push_back(film_map.film_type);
  }
  // dt_bauhaus_widget_set_label(g->film_emulation.film, NULL, _("film type"));
  gtk_widget_set_tooltip_text(g->film_emulation.film, _("choose emulated film type"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), g->film_emulation.film, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.film), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::film_callback), self);

  g->film_emulation.strength
      = dt_bauhaus_slider_new_with_range(self, 0, 1, 0.01, g->film_emulation.parameters.strength, 3);
  dt_bauhaus_widget_set_label(g->film_emulation.strength, NULL, _("strength"));
  gtk_widget_set_tooltip_text(g->film_emulation.strength, _("strength of the film emulation effect"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), GTK_WIDGET(g->film_emulation.strength), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.strength), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::strength_callback), self);

  g->film_emulation.brightness
      = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->film_emulation.parameters.brightness, 3);
  dt_bauhaus_widget_set_label(g->film_emulation.brightness, NULL, _("brightness"));
  gtk_widget_set_tooltip_text(g->film_emulation.brightness, _("brightness of the film emulation effect"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), GTK_WIDGET(g->film_emulation.brightness), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.brightness), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::brightness_callback), self);

  g->film_emulation.contrast
      = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->film_emulation.parameters.contrast, 3);
  dt_bauhaus_widget_set_label(g->film_emulation.contrast, NULL, _("contrast"));
  gtk_widget_set_tooltip_text(g->film_emulation.contrast, _("contrast of the film emulation effect"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), GTK_WIDGET(g->film_emulation.contrast), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.contrast), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::contrast_callback), self);

  g->film_emulation.gamma
      = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->film_emulation.parameters.gamma, 3);
  dt_bauhaus_widget_set_label(g->film_emulation.gamma, NULL, _("gamma"));
  gtk_widget_set_tooltip_text(g->film_emulation.gamma, _("gamma value of the film emulation effect"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), GTK_WIDGET(g->film_emulation.gamma), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.gamma), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::gamma_callback), self);

  g->film_emulation.hue = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->film_emulation.parameters.hue, 3);
  dt_bauhaus_widget_set_label(g->film_emulation.hue, NULL, _("hue"));
  gtk_widget_set_tooltip_text(g->film_emulation.hue, _("hue shift of the film emulation effect"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), GTK_WIDGET(g->film_emulation.hue), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.hue), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::hue_callback), self);

  g->film_emulation.saturation
      = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->film_emulation.parameters.saturation, 3);
  dt_bauhaus_widget_set_label(g->film_emulation.saturation, NULL, _("saturation"));
  gtk_widget_set_tooltip_text(g->film_emulation.saturation, _("saturation of the film emulation effect"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), GTK_WIDGET(g->film_emulation.saturation), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.saturation), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::saturation_callback), self);

  g->film_emulation.normalize_colors = dt_bauhaus_combobox_new(self);
  dt_bauhaus_combobox_add(g->film_emulation.normalize_colors, _("none"));
  dt_bauhaus_combobox_add(g->film_emulation.normalize_colors, _("pre-process"));
  dt_bauhaus_combobox_add(g->film_emulation.normalize_colors, _("post-process"));
  dt_bauhaus_combobox_add(g->film_emulation.normalize_colors, _("both"));
  dt_bauhaus_combobox_set(g->film_emulation.normalize_colors, g->film_emulation.parameters.normalize_colors);
  dt_bauhaus_widget_set_label(g->film_emulation.normalize_colors, NULL, _("normalize colors"));
  gtk_widget_set_tooltip_text(g->film_emulation.normalize_colors, _("choose how to normalize colors"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), g->film_emulation.normalize_colors, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.normalize_colors), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::normalize_colors_callback), self);

  gtk_widget_show_all(g->film_emulation.box);
  gtk_widget_set_no_show_all(g->film_emulation.box, TRUE);
  gtk_widget_set_visible(g->film_emulation.box, p->filter == film_emulation ? TRUE : FALSE);
}

void dt_iop_gmic_film_emulation_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->film_emulation.box, p->filter == film_emulation ? TRUE : FALSE);
  if(p->filter == film_emulation)
  {
    g->film_emulation.parameters = *p;
    auto i = std::find(g->film_emulation.film_list.begin(), g->film_emulation.film_list.end(),
                       g->film_emulation.parameters.film);
    if(i != g->film_emulation.film_list.end())
      dt_bauhaus_combobox_set(g->film_emulation.film, static_cast<int>(i - g->film_emulation.film_list.begin()));
    dt_bauhaus_slider_set(g->film_emulation.strength, g->film_emulation.parameters.strength);
    dt_bauhaus_slider_set(g->film_emulation.brightness, g->film_emulation.parameters.brightness);
    dt_bauhaus_slider_set(g->film_emulation.contrast, g->film_emulation.parameters.contrast);
    dt_bauhaus_slider_set(g->film_emulation.gamma, g->film_emulation.parameters.gamma);
    dt_bauhaus_slider_set(g->film_emulation.hue, g->film_emulation.parameters.hue);
    dt_bauhaus_slider_set(g->film_emulation.saturation, g->film_emulation.parameters.saturation);
    dt_bauhaus_combobox_set(g->film_emulation.normalize_colors, g->film_emulation.parameters.normalize_colors);
  }
}

void dt_iop_gmic_film_emulation_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_film_emulation_params_t();
}

void dt_iop_gmic_film_emulation_params_t::film_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    std::snprintf(G->film_emulation.parameters.film, sizeof(G->film_emulation.parameters.film), "%s",
                  film_maps[dt_bauhaus_combobox_get(W)].film_type.c_str());
    return G->film_emulation.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_film_emulation_params_t::strength_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->film_emulation.parameters.strength = dt_bauhaus_slider_get(W);
    return G->film_emulation.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_film_emulation_params_t::brightness_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->film_emulation.parameters.brightness = dt_bauhaus_slider_get(W);
    return G->film_emulation.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_film_emulation_params_t::contrast_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->film_emulation.parameters.contrast = dt_bauhaus_slider_get(W);
    return G->film_emulation.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_film_emulation_params_t::gamma_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->film_emulation.parameters.gamma = dt_bauhaus_slider_get(W);
    return G->film_emulation.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_film_emulation_params_t::hue_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->film_emulation.parameters.hue = dt_bauhaus_slider_get(W);
    return G->film_emulation.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_film_emulation_params_t::saturation_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->film_emulation.parameters.saturation = dt_bauhaus_slider_get(W);
    return G->film_emulation.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_film_emulation_params_t::normalize_colors_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->film_emulation.parameters.normalize_colors = dt_bauhaus_combobox_get(W);
    return G->film_emulation.parameters.to_gmic_params();
  });
}

const std::vector<film_map> dt_iop_gmic_film_emulation_params_t::film_maps{
  // black and white
  { "agfa_apx_25", _("Agfa APX 25") },
  { "agfa_apx_100", _("Agfa APX 100") },
  { "fuji_neopan_1600_-", _("Fuji Neopan 1600 -") },
  { "fuji_neopan_1600", _("Fuji Neopan 1600") },
  { "fuji_neopan_1600_+", _("Fuji Neopan 1600 +") },
  { "fuji_neopan_1600_++", _("Fuji Neopan 1600 ++") },
  { "fuji_neopan_acros_100", _("Fuji Neopan Acros 100") },
  { "ilford_delta_100", _("Ilford Delta 100") },
  { "ilford_delta_400", _("Ilford Delta 400") },
  { "ilford_delta_3200", _("Ilford Delta 3200") },
  { "ilford_fp4_plus_125", _("Ilford FP4 plus 125") },
  { "ilford_hp5_plus_400", _("Ilford HP5 plus 400") },
  { "ilford_hps_800", _("Ilford HPS 800") },
  { "ilford_pan_f_plus_50", _("Ilford Pan F plus 50") },
  { "ilford_xp2", _("Ilford XP2") },
  { "kodak_bw_400_cn", _("Kodak BW 400 CN") },
  { "kodak_hie_(hs_infra)", _("Kodak hie (hs infra)") },
  { "kodak_t-max_100", _("Kodak T-Max 100") },
  { "kodak_t-max_400", _("Kodak T-Max 400") },
  { "kodak_t-max_3200", _("Kodak T-Max 3200") },
  { "kodak_tri-x_400_-", _("Kodak Tri-X 400 -") },
  { "kodak_tri-x_400", _("Kodak Tri-X 400") },
  { "kodak_tri-x_400_+", _("Kodak Tri-X 400 +") },
  { "kodak_tri-x_400_++", _("Kodak Tri-X 400 ++") },
  { "polaroid_664", _("Polaroid 664") },
  { "polaroid_667", _("Polaroid 667") },
  { "polaroid_672", _("Polaroid 672") },
  { "rollei_ir_400", _("Rollei IR 400") },
  { "rollei_ortho_25", _("Rollei Ortho 25") },
  { "rollei_retro_100_tonal", _("Rollei Retro 100 tonal") },
  { "rollei_retro_80s", _("Rollei Retro 80s") },
  // Fuji X-Trans
  { "fuji_xtrans_ii_astia_v2", _("Fuji X-Trans II Astia") },
  { "fuji_xtrans_ii_classic_chrome_v1", _("Fuji X-Trans II Classic Chrome") },
  { "fuji_xtrans_ii_pro_neg_hi_v2", _("Fuji X-Trans II Pro Neg hi") },
  { "fuji_xtrans_ii_pro_neg_std_v2", _("Fuji X-Trans II Pro Neg std") },
  { "fuji_xtrans_ii_provia_v2", _("Fuji X-Trans II Provia") },
  { "fuji_xtrans_ii_velvia_v2", _("Fuji X-Trans II Velvia") },
  // instant consumer
  { "polaroid_px-100uv+_cold_--", _("Polaroid PX-100uv+ cold --") },
  { "polaroid_px-100uv+_cold_-", _("Polaroid PX-100uv+ cold -") },
  { "polaroid_px-100uv+_cold", _("Polaroid PX-100uv+ cold") },
  { "polaroid_px-100uv+_cold_+", _("Polaroid PX-100uv+ cold +") },
  { "polaroid_px-100uv+_cold_++", _("Polaroid PX-100uv+ cold ++") },
  { "polaroid_px-100uv+_cold_+++", _("Polaroid PX-100uv+ cold +++") },
  { "polaroid_px-100uv+_warm_--", _("Polaroid PX-100uv+ warm --") },
  { "polaroid_px-100uv+_warm_-", _("Polaroid PX-100uv+ warm -") },
  { "polaroid_px-100uv+_warm", _("Polaroid PX-100uv+ warm") },
  { "polaroid_px-100uv+_warm_+", _("Polaroid PX-100uv+ warm +") },
  { "polaroid_px-100uv+_warm_++", _("Polaroid PX-100uv+ warm ++") },
  { "polaroid_px-100uv+_warm_+++", _("Polaroid PX-100uv+ warm +++") },
  { "polaroid_px-680_--", _("Polaroid PX-680 --") },
  { "polaroid_px-680_-", _("Polaroid PX-680 -") },
  { "polaroid_px-680", _("Polaroid PX-680") },
  { "polaroid_px-680_+", _("Polaroid PX-680 +") },
  { "polaroid_px-680_++", _("Polaroid PX-680 ++") },
  { "polaroid_px-680_cold_--", _("Polaroid PX-680 cold --") },
  { "polaroid_px-680_cold_-", _("Polaroid PX-680 cold -") },
  { "polaroid_px-680_cold", _("Polaroid PX-680 cold") },
  { "polaroid_px-680_cold_+", _("Polaroid PX-680 cold +") },
  { "polaroid_px-680_cold_++", _("Polaroid PX-680 cold ++") },
  { "polaroid_px-680_cold_++_alt", _("Polaroid PX-680 cold ++ alt") },
  { "polaroid_px-680_warm_--", _("Polaroid PX-680 warm --") },
  { "polaroid_px-680_warm_-", _("Polaroid PX-680 warm -") },
  { "polaroid_px-680_warm", _("Polaroid PX-680 warm") },
  { "polaroid_px-680_warm_+", _("Polaroid PX-680 warm +") },
  { "polaroid_px-680_warm_++", _("Polaroid PX-680 warm ++") },
  { "polaroid_px-70_--", _("Polaroid PX-70 --") },
  { "polaroid_px-70_-", _("Polaroid PX-70 -") },
  { "polaroid_px-70", _("Polaroid PX-70") },
  { "polaroid_px-70_+", _("Polaroid PX-70 +") },
  { "polaroid_px-70_++", _("Polaroid PX-70 ++") },
  { "polaroid_px-70_+++", _("Polaroid PX-70 +++") },
  { "polaroid_px-70_cold_--", _("Polaroid PX-70 cold --") },
  { "polaroid_px-70_cold_-", _("Polaroid PX-70 cold -") },
  { "polaroid_px-70_cold", _("Polaroid PX-70 cold") },
  { "polaroid_px-70_cold_+", _("Polaroid PX-70 cold +") },
  { "polaroid_px-70_cold_++", _("Polaroid PX-70 cold ++") },
  { "polaroid_px-70_warm_--", _("Polaroid PX-70 warm --") },
  { "polaroid_px-70_warm_-", _("Polaroid PX-70 warm -") },
  { "polaroid_px-70_warm", _("Polaroid PX-70 warm") },
  { "polaroid_px-70_warm_+", _("Polaroid PX-70 warm +") },
  { "polaroid_px-70_warm_++", _("Polaroid PX-70 warm ++") },
  { "polaroid_time_zero_(expired)_---", _("Polaroid time zero (expired) ---") },
  { "polaroid_time_zero_(expired)_--", _("Polaroid time zero (expired) --") },
  { "polaroid_time_zero_(expired)_-", _("Polaroid time zero (expired) -") },
  { "polaroid_time_zero_(expired)", _("Polaroid time zero (expired)") },
  { "polaroid_time_zero_(expired)_+", _("Polaroid time zero (expired) +") },
  { "polaroid_time_zero_(expired)_++", _("Polaroid time zero (expired) ++") },
  { "polaroid_time_zero_(expired)_cold_---", _("Polaroid time zero (expired) cold ---") },
  { "polaroid_time_zero_(expired)_cold_--", _("Polaroid time zero (expired) cold --") },
  { "polaroid_time_zero_(expired)_cold_-", _("Polaroid time zero (expired) cold -") },
  { "polaroid_time_zero_(expired)_cold", _("Polaroid time zero (expired) cold") },
  // instant pro
  { "fuji_fp-100c_--", _("Fuji FP-100c --") },
  { "fuji_fp-100c_-", _("Fuji FP-100c -") },
  { "fuji_fp-100c", _("Fuji FP-100c") },
  { "fuji_fp-100c_+", _("Fuji FP-100c +") },
  { "fuji_fp-100c_++", _("Fuji FP-100c ++") },
  { "fuji_fp-100c_++_alt", _("Fuji FP-100c ++ alt") },
  { "fuji_fp-100c_+++", _("Fuji FP-100c +++") },
  { "fuji_fp-100c_cool_--", _("Fuji FP-100c cool --") },
  { "fuji_fp-100c_cool_-", _("Fuji FP-100c cool -") },
  { "fuji_fp-100c_cool", _("Fuji FP-100c cool") },
  { "fuji_fp-100c_cool_+", _("Fuji FP-100c cool +") },
  { "fuji_fp-100c_cool_++", _("Fuji FP-100c cool ++") },
  { "fuji_fp-100c_negative_--", _("Fuji FP-100c negative --") },
  { "fuji_fp-100c_negative_-", _("Fuji FP-100c negative -") },
  { "fuji_fp-100c_negative", _("Fuji FP-100c negative") },
  { "fuji_fp-100c_negative_+", _("Fuji FP-100c negative +") },
  { "fuji_fp-100c_negative_++", _("Fuji FP-100c negative ++") },
  { "fuji_fp-100c_negative_++_alt", _("Fuji FP-100c negative ++ alt") },
  { "fuji_fp-100c_negative_+++", _("Fuji FP-100c negative +++") },
  { "fuji_fp-3000b_--", _("Fuji FP-3000b --") },
  { "fuji_fp-3000b_-", _("Fuji FP-3000b -") },
  { "fuji_fp-3000b", _("Fuji FP-3000b") },
  { "fuji_fp-3000b_+", _("Fuji FP-3000b +") },
  { "fuji_fp-3000b_++", _("Fuji FP-3000b ++") },
  { "fuji_fp-3000b_+++", _("Fuji FP-3000b +++") },
  { "fuji_fp-3000b_hc", _("Fuji FP-3000b hc") },
  { "fuji_fp-3000b_negative_--", _("Fuji FP-3000b negative --") },
  { "fuji_fp-3000b_negative_-", _("Fuji FP-3000b negative -") },
  { "fuji_fp-3000b_negative", _("Fuji FP-3000b negative") },
  { "fuji_fp-3000b_negative_+", _("Fuji FP-3000b negative +") },
  { "fuji_fp-3000b_negative_++", _("Fuji FP-3000b negative ++") },
  { "fuji_fp-3000b_negative_+++", _("Fuji FP-3000b negative +++") },
  { "fuji_fp-3000b_negative_early", _("Fuji FP-3000b negative early") },
  { "polaroid_665_-", _("Polaroid 665 -") },
  { "polaroid_665_--", _("Polaroid 665 --") },
  { "polaroid_665", _("Polaroid 665") },
  { "polaroid_665_+", _("Polaroid 665 +") },
  { "polaroid_665_++", _("Polaroid 665 ++") },
  { "polaroid_665_negative", _("Polaroid 665 negative") },
  { "polaroid_665_negative_+", _("Polaroid 665 negative +") },
  { "polaroid_665_negative_-", _("Polaroid 665 negative -") },
  { "polaroid_665_negative_hc", _("Polaroid 665 negative hc") },
  { "polaroid_669_--", _("Polaroid 669 --") },
  { "polaroid_669_-", _("Polaroid 669 -") },
  { "polaroid_669", _("Polaroid 669") },
  { "polaroid_669_+", _("Polaroid 669 +") },
  { "polaroid_669_++", _("Polaroid 669 ++") },
  { "polaroid_669_+++", _("Polaroid 669 +++") },
  { "polaroid_669_cold_--", _("Polaroid 669 cold --") },
  { "polaroid_669_cold_-", _("Polaroid 669 cold -") },
  { "polaroid_669_cold", _("Polaroid 669 cold") },
  { "polaroid_669_cold_+", _("Polaroid 669 cold +") },
  { "polaroid_690_--", _("Polaroid 690 --") },
  { "polaroid_690_-", _("Polaroid 690 -") },
  { "polaroid_690", _("Polaroid 690") },
  { "polaroid_690_+", _("Polaroid 690 +") },
  { "polaroid_690_++", _("Polaroid 690 ++") },
  { "polaroid_690_cold_--", _("Polaroid 690 cold --") },
  { "polaroid_690_cold_-", _("Polaroid 690 cold -") },
  { "polaroid_690_cold", _("Polaroid 690 cold") },
  { "polaroid_690_cold_+", _("Polaroid 690 cold +") },
  { "polaroid_690_cold_++", _("Polaroid 690 cold ++") },
  { "polaroid_690_warm_--", _("Polaroid 690 warm --") },
  { "polaroid_690_warm_-", _("Polaroid 690 warm -") },
  { "polaroid_690_warm", _("Polaroid 690 warm") },
  { "polaroid_690_warm_+", _("Polaroid 690 warm +") },
  { "polaroid_690_warm_++", _("Polaroid 690 warm ++") },
  // negative color
  { "agfa_ultra_color_100", _("Agfa Ultra color 100") },
  { "agfa_vista_200", _("Agfa Vista 200") },
  { "fuji_superia_200", _("Fuji Superia 200") },
  { "fuji_superia_hg_1600", _("Fuji Superia hg 1600") },
  { "fuji_superia_reala_100", _("Fuji Superia Reala 100") },
  { "fuji_superia_x-tra_800", _("Fuji Superia X-Tra 800") },
  { "kodak_elite_100_xpro", _("Kodak Elite 100 XPRO") },
  { "kodak_elite_color_200", _("Kodak Elite Color 200") },
  { "kodak_elite_color_400", _("Kodak Elite Color 400") },
  { "kodak_portra_160_nc_-", _("Kodak Portra 160 NC -") },
  { "kodak_portra_160_nc", _("Kodak Portra 160 NC") },
  { "kodak_portra_160_nc_+", _("Kodak Portra 160 NC +") },
  { "kodak_portra_160_nc_++", _("Kodak Portra 160 NC ++") },
  { "kodak_portra_160_vc_-", _("Kodak Portra 160 VC -") },
  { "kodak_portra_160_vc", _("Kodak Portra 160 VC") },
  { "kodak_portra_160_vc_+", _("Kodak Portra 160 VC +") },
  { "kodak_portra_160_vc_++", _("Kodak Portra 160 VC ++") },
  { "lomography_redscale_100", _("Lomography Redscale 100") },
  // negative new
  { "fuji_160c_-", _("Fuji 160C -") },
  { "fuji_160c", _("Fuji 160C") },
  { "fuji_160c_+", _("Fuji 160C +") },
  { "fuji_160c_++", _("Fuji 160C ++") },
  { "fuji_400h_-", _("Fuji 400H -") },
  { "fuji_400h", _("Fuji 400H") },
  { "fuji_400h_+", _("Fuji 400H +") },
  { "fuji_400h_++", _("Fuji 400H ++") },
  { "fuji_800z_-", _("Fuji 800Z -") },
  { "fuji_800z", _("Fuji 800Z") },
  { "fuji_800z_+", _("Fuji 800Z +") },
  { "fuji_800z_++", _("Fuji 800Z ++") },
  { "fuji_800z_-", _("Fuji 800Z -") },
  { "fuji_ilford_hp5_-", _("Fuji Ilford HP5 -") },
  { "fuji_ilford_hp5", _("Fuji Ilford HP5") },
  { "fuji_ilford_hp5_+", _("Fuji Ilford HP5 +") },
  { "fuji_ilford_hp5_++", _("Fuji Ilford HP5 ++") },
  { "kodak_portra_160_-", _("Kodak Portra 160 -") },
  { "kodak_portra_160", _("Kodak Portra 160") },
  { "kodak_portra_160_+", _("Kodak Portra 160 +") },
  { "kodak_portra_160_++", _("Kodak Portra 160 ++") },
  { "kodak_portra_400_-", _("Kodak Portra 400 -") },
  { "kodak_portra_400", _("Kodak Portra 400") },
  { "kodak_portra_400_+", _("Kodak Portra 400 +") },
  { "kodak_portra_400_++", _("Kodak Portra 400 ++") },
  { "kodak_portra_800_-", _("Kodak Portra 800 -") },
  { "kodak_portra_800", _("Kodak Portra 800") },
  { "kodak_portra_800_+", _("Kodak Portra 800 +") },
  { "kodak_portra_800_++", _("Kodak Portra 800 ++") },
  { "kodak_tmax_3200_-", _("Kodak T-Max 3200 -") },
  { "kodak_tmax_3200", _("Kodak T-Max 3200") },
  { "kodak_tmax_3200_+", _("Kodak T-Max 3200 +") },
  { "kodak_tmax_3200_++", _("Kodak T-Max 3200 ++") },
  { "kodak_tri-x_400_-", _("Kodak Tri-X 400 -") },
  { "kodak_tri-x_400", _("Kodak Tri-X 400") },
  { "kodak_tri-x_400_+", _("Kodak Tri-X 400 +") },
  { "kodak_tri-x_400_++", _("Kodak Tri-X 400 ++") },
  // negative old
  { "fuji_ilford_delta_3200_-", _("Fuji Ilford Delta 3200 -") },
  { "fuji_ilford_delta_3200", _("Fuji Ilford Delta 3200") },
  { "fuji_ilford_delta_3200_+", _("Fuji Ilford Delta 3200 +") },
  { "fuji_ilford_delta_3200_++", _("Fuji Ilford Delta 3200 ++") },
  { "fuji_superia_100_-", _("Fuji Superia 100 -") },
  { "fuji_superia_100", _("Fuji Superia 100") },
  { "fuji_superia_100_+", _("Fuji Superia 100 +") },
  { "fuji_superia_100_++", _("Fuji Superia 100 ++") },
  { "fuji_superia_400_-", _("Fuji Superia 400 -") },
  { "fuji_superia_400", _("Fuji Superia 400") },
  { "fuji_superia_400_+", _("Fuji Superia 400 +") },
  { "fuji_superia_400_++", _("Fuji Superia 400 ++") },
  { "fuji_superia_800_-", _("Fuji Superia 800 -") },
  { "fuji_superia_800", _("Fuji Superia 800") },
  { "fuji_superia_800_+", _("Fuji Superia 800 +") },
  { "fuji_superia_800_++", _("Fuji Superia 800 ++") },
  { "fuji_superia_1600_-", _("Fuji Superia 1600 -") },
  { "fuji_superia_1600", _("Fuji Superia 1600") },
  { "fuji_superia_1600_+", _("Fuji Superia 1600 +") },
  { "fuji_superia_1600_++", _("Fuji Superia 1600 ++") },
  { "kodak_portra_160_nc_-", _("Kodak Portra 160 NC -") },
  { "kodak_portra_160_nc", _("Kodak Portra 160 NC") },
  { "kodak_portra_160_nc_+", _("Kodak Portra 160 NC +") },
  { "kodak_portra_160_nc_++", _("Kodak Portra 160 NC ++") },
  { "kodak_portra_160_vc_-", _("Kodak Portra 160 VC -") },
  { "kodak_portra_160_vc", _("Kodak Portra 160 VC") },
  { "kodak_portra_160_vc_+", _("Kodak Portra 160 VC +") },
  { "kodak_portra_160_vc_++", _("Kodak Portra 160 VC ++") },
  { "kodak_portra_400_nc_-", _("Kodak Portra 400 NC -") },
  { "kodak_portra_400_nc", _("Kodak Portra 400 NC") },
  { "kodak_portra_400_nc_+", _("Kodak Portra 400 NC +") },
  { "kodak_portra_400_nc_++", _("Kodak Portra 400 NC ++") },
  { "kodak_portra_400_uc_-", _("Kodak Portra 400 UC -") },
  { "kodak_portra_400_uc", _("Kodak Portra 400 UC") },
  { "kodak_portra_400_uc_+", _("Kodak Portra 400 UC +") },
  { "kodak_portra_400_uc_++", _("Kodak Portra 400 UC ++") },
  { "kodak_portra_400_vc_-", _("Kodak Portra 400 VC -") },
  { "kodak_portra_400_vc", _("Kodak Portra 400 VC") },
  { "kodak_portra_400_vc_+", _("Kodak Portra 400 VC +") },
  { "kodak_portra_400_vc_++", _("Kodak Portra 400 VC ++") },
  // Picture FX
  { "analogfx_anno_1870_color", _("AnalogFX anno 1870 color") },
  { "analogfx_old_style_i", _("AnalogFX old style I") },
  { "analogfx_old_style_ii", _("AnalogFX old style II") },
  { "analogfx_old_style_iii", _("AnalogFX old style III") },
  { "analogfx_sepia_color", _("AnalogFX sepia color") },
  { "analogfx_soft_sepia_i", _("AnalogFX soft sepia I") },
  { "analogfx_soft_sepia_ii", _("AnalogFX soft sepia II") },
  { "goldfx_bright_spring_breeze", _("GoldFX bright spring breeze") },
  { "goldfx_bright_summer_heat", _("GoldFX bright summer heat") },
  { "goldfx_hot_summer_heat", _("GoldFX hot summer heat") },
  { "goldfx_perfect_sunset_01min", _("GoldFX perfect sunset 1min") },
  { "goldfx_perfect_sunset_05min", _("GoldFX perfect sunset 5min") },
  { "goldfx_perfect_sunset_10min", _("GoldFX perfect sunset 10min") },
  { "goldfx_spring_breeze", _("GoldFX spring breeze") },
  { "goldfx_summer_heat", _("GoldFX summer heat") },
  { "technicalfx_backlight_filter", _("TechnicalFX backlight filter") },
  { "zilverfx_b_w_solarization", _("ZiverFX bw solarization") },
  { "zilverfx_infrared", _("ZiverFX infrared") },
  { "zilverfx_vintage_b_w", _("ZiverFX vintage bw") },
  // film print
  { "fuji3510_constlclip", _("Fuji 3510 constlclip") },
  { "fuji3510_constlmap", _("Fuji 3510 constlmap") },
  { "fuji3510_cuspclip", _("Fuji 3510 cuspclip") },
  { "fuji3513_constlclip", _("Fuji 3513 constlclip") },
  { "fuji3513_constlmap", _("Fuji 3513 constlmap") },
  { "fuji3513_cuspclip", _("Fuji 3513 cuspclip") },
  { "kodak2383_constlclip", _("Kodak 2383 constlclip") },
  { "kodak2383_constlmap", _("Kodak 2383 constlmap") },
  { "kodak2383_cuspclip", _("Kodak 2383 cuspclip") },
  { "kodak2393_constlclip", _("Kodak 2393 constlclip") },
  { "kodak2393_constlmap", _("Kodak 2393 constlmap") },
  { "kodak2393_cuspclip", _("Kodak 2393 cuspclip") },
  // slide color
  { "agfa_precisa_100", _("Agfa Precisa 100") },
  { "fuji_astia_100f", _("Fuji Astia 100f") },
  { "fuji_fp_100c", _("Fuji FP-100c") },
  { "fuji_provia_100f", _("Fuji Provia 100F") },
  { "fuji_provia_400f", _("Fuji Provia 400F") },
  { "fuji_provia_400x", _("Fuji Provia 400X") },
  { "fuji_sensia_100", _("Fuji Sensia 100") },
  { "fuji_superia_200_xpro", _("Fuji Superia 200 XPRO") },
  { "fuji_velvia_50", _("Fuji Velvia 50") },
  { "generic_fuji_astia_100", _("generic Fuji Astia 100") },
  { "generic_fuji_provia_100", _("generic Fuji Provia 100") },
  { "generic_fuji_velvia_100", _("generic Fuji Aelvia 100") },
  { "generic_kodachrome_64", _("generic Kodachrome 64") },
  { "generic_kodak_ektachrome_100_vs", _("generic Kodak Ektachrome 100 VS") },
  { "kodak_e-100_gx_ektachrome_100", _("Kodak E-100 GX Ektachrome 100") },
  { "kodak_ektachrome_100_vs", _("Kodak Ektachrome 100 VS") },
  { "kodak_elite_chrome_200", _("Kodak Elite Chrome 200") },
  { "kodak_elite_chrome_400", _("Kodak Elite Chrome 400") },
  { "kodak_elite_extracolor_100", _("Kodak Elite Extracolor 100") },
  { "kodak_kodachrome_200", _("Kodak Kodachrome 200") },
  { "kodak_kodachrome_25", _("Kodak Kodachrome 25") },
  { "kodak_kodachrome_64", _("Kodak Kodachrome 64") },
  { "lomography_x-pro_slide_200", _("Lomography X-Pro Slide 200") },
  { "polaroid_polachrome", _("Polaroid Polachrome") },
  // various
  { "60's", _("60's") },
  { "60's_faded", _("60's faded") },
  { "60's_faded_alt", _("60's faded alt") },
  { "alien_green", _("alien green") },
  { "black_and_white", _("black and white") },
  { "bleach_bypass", _("bleach bypass") },
  { "blue_mono", _("blue mono") },
  { "color_rich", _("color rich") },
  { "faded", _("faded") },
  { "faded_alt", _("faded alt") },
  { "faded_analog", _("faded analog") },
  { "faded_extreme", _("faded extreme") },
  { "faded_vivid", _("faded vivid") },
  { "expired_fade", _("expired fade") },
  { "expired_polaroid", _("expired Polaroid") },
  { "extreme", _("extreme") },
  { "fade", _("fade") },
  { "faux_infrared", _("faux infrared") },
  { "golden", _("golden") },
  { "golden_bright", _("golden bright") },
  { "golden_fade", _("golden fade") },
  { "golden_mono", _("golden mono") },
  { "golden_vibrant", _("golden vibrant") },
  { "green_mono", _("green mono") },
  { "hong_kong", _("hong kong") },
  { "light_blown", _("light blown") },
  { "lomo", _("lomo") },
  { "mono_tinted", _("mono tinted") },
  { "muted_fade", _("muted fade") },
  { "mute_shift", _("mute shift") },
  { "natural_vivid", _("natural vivid") },
  { "nostalgic", _("nostalgic") },
  { "orange_tone", _("orange tone") },
  { "pink_fade", _("pink fade") },
  { "purple", _("purple") },
  { "retro", _("retro") },
  { "rotate_muted", _("rotate muted") },
  { "rotate_vibrant", _("rotate vibrant") },
  { "rotated", _("rotated") },
  { "rotated_crush", _("rotated crush") },
  { "smooth_cromeish", _("smooth cromeish") },
  { "smooth_fade", _("smooth fade") },
  { "soft_fade", _("soft fade") },
  { "solarized_color", _("solarized color") },
  { "solarized_color2", _("solarized color2") },
  { "summer", _("summer") },
  { "summer_alt", _("summer alt") },
  { "sunny", _("sunny") },
  { "sunny_alt", _("sunny alt") },
  { "sunny_rich", _("sunny rich") },
  { "sunny_warm", _("sunny warm") },
  { "super_warm", _("super warm") },
  { "super_warm_rich", _("super warm rich") },
  { "sutro_fx", _("Sutro FX") },
  { "vibrant", _("vibrant") },
  { "vibrant_alien", _("vibrant alien") },
  { "vibrant_contrast", _("vibrant contrast") },
  { "vibrant_cromeish", _("vibrant cromeish") },
  { "vintage", _("vintage") },
  { "vintage_alt", _("vintage alt") },
  { "vintage_brighter", _("vintage brighter") },
  { "warm", _("warm") },
  { "warm_highlight", _("warm highlight") },
  { "warm_yellow", _("warm yellow") }
};

// --- custom film emulation

dt_iop_gmic_custom_film_emulation_params_t::dt_iop_gmic_custom_film_emulation_params_t()
{
  if(not film_maps.empty()) std::snprintf(film, sizeof(film), "%s", film_maps[0].film_type.c_str());
}

dt_iop_gmic_custom_film_emulation_params_t::dt_iop_gmic_custom_film_emulation_params_t(
    const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_custom_film_emulation_params_t()
{
  dt_iop_gmic_custom_film_emulation_params_t p;
  if(other.filter == custom_film_emulation
     and std::sscanf(other.parameters, "dt_custom_film_emulation \"%1023[^\"]\",%g,%g,%g,%g,%g,%g,%i",
                     reinterpret_cast<char *>(&p.film), &p.strength, &p.brightness, &p.contrast, &p.gamma, &p.hue,
                     &p.saturation, &p.normalize_colors)
             == 8)
  {
    p.strength = clamp(0.f, 1.f, p.strength);
    p.brightness = clamp(-1.f, 1.f, p.brightness);
    p.contrast = clamp(-1.f, 1.f, p.contrast);
    p.gamma = clamp(-1.f, 1.f, p.gamma);
    p.hue = clamp(-1.f, 1.f, p.hue);
    p.saturation = clamp(-1.f, 1.f, p.saturation);
    p.normalize_colors = clamp(0, 1, p.normalize_colors);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_custom_film_emulation_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret{ none, { "" } };
  ret.filter = custom_film_emulation;
  if(std::strlen(film) > 0 and (not film_maps.empty()))
    std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_custom_film_emulation \"%s\",%g,%g,%g,%g,%g,%g,%i",
                  film, strength, brightness, contrast, gamma, hue, saturation, normalize_colors);
  return ret;
}

const char *dt_iop_gmic_custom_film_emulation_params_t::get_custom_command()
{
  // clang-format off
  return R"raw(
dt_custom_film_emulation :
  input_cube "$1"
  repeat {$!-1}
    if {$8%2} balance_gamma[$>] , fi
    if {$2<1} +map_clut[$>] . j[$>] .,0,0,0,0,{$2} rm.
    else map_clut[$>] .
    fi
  done
  rm.
  adjust_colors {100*$3},{100*$4},{100*$5},{100*$6},{100*$7},0,255
  if {$8>1} repeat $! l[$>] split_opacity n[0] 0,255 a c endl done fi
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_custom_film_emulation_params_t::get_filter() const
{
  return custom_film_emulation;
}

void dt_iop_gmic_custom_film_emulation_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == custom_film_emulation)
    g->custom_film_emulation.parameters = *p;
  else
    g->custom_film_emulation.parameters = dt_iop_gmic_custom_film_emulation_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("custom film emulation"));
  g->custom_film_emulation.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->custom_film_emulation.box, TRUE, TRUE, 0);

  if(not film_maps.empty())
  {
    g->custom_film_emulation.film = dt_bauhaus_combobox_new(self);
    for(const auto &film_map : film_maps)
    {
      dt_bauhaus_combobox_add_aligned(g->custom_film_emulation.film, film_map.printable.c_str(),
                                      DT_BAUHAUS_COMBOBOX_ALIGN_LEFT);
      g->custom_film_emulation.film_list.push_back(film_map.film_type);
    }
    // dt_bauhaus_widget_set_label(g->custom_film_emulation.film, NULL, _("film type"));
    gtk_widget_set_tooltip_text(g->custom_film_emulation.film, _("choose emulated film type"));
    gtk_box_pack_start(GTK_BOX(g->custom_film_emulation.box), g->custom_film_emulation.film, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(g->custom_film_emulation.film), "value-changed",
                     G_CALLBACK(dt_iop_gmic_custom_film_emulation_params_t::film_callback), self);

    g->custom_film_emulation.strength
        = dt_bauhaus_slider_new_with_range(self, 0, 1, 0.01, g->custom_film_emulation.parameters.strength, 3);
    dt_bauhaus_widget_set_label(g->custom_film_emulation.strength, NULL, _("strength"));
    gtk_widget_set_tooltip_text(g->custom_film_emulation.strength, _("strength of the film emulation effect"));
    gtk_box_pack_start(GTK_BOX(g->custom_film_emulation.box), GTK_WIDGET(g->custom_film_emulation.strength), TRUE,
                       TRUE, 0);
    g_signal_connect(G_OBJECT(g->custom_film_emulation.strength), "value-changed",
                     G_CALLBACK(dt_iop_gmic_custom_film_emulation_params_t::strength_callback), self);

    g->custom_film_emulation.brightness
        = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->custom_film_emulation.parameters.brightness, 3);
    dt_bauhaus_widget_set_label(g->custom_film_emulation.brightness, NULL, _("brightness"));
    gtk_widget_set_tooltip_text(g->custom_film_emulation.brightness, _("brightness of the film emulation effect"));
    gtk_box_pack_start(GTK_BOX(g->custom_film_emulation.box), GTK_WIDGET(g->custom_film_emulation.brightness),
                       TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(g->custom_film_emulation.brightness), "value-changed",
                     G_CALLBACK(dt_iop_gmic_custom_film_emulation_params_t::brightness_callback), self);

    g->custom_film_emulation.contrast
        = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->custom_film_emulation.parameters.contrast, 3);
    dt_bauhaus_widget_set_label(g->custom_film_emulation.contrast, NULL, _("contrast"));
    gtk_widget_set_tooltip_text(g->custom_film_emulation.contrast, _("contrast of the film emulation effect"));
    gtk_box_pack_start(GTK_BOX(g->custom_film_emulation.box), GTK_WIDGET(g->custom_film_emulation.contrast), TRUE,
                       TRUE, 0);
    g_signal_connect(G_OBJECT(g->custom_film_emulation.contrast), "value-changed",
                     G_CALLBACK(dt_iop_gmic_custom_film_emulation_params_t::contrast_callback), self);

    g->custom_film_emulation.gamma
        = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->custom_film_emulation.parameters.gamma, 3);
    dt_bauhaus_widget_set_label(g->custom_film_emulation.gamma, NULL, _("gamma"));
    gtk_widget_set_tooltip_text(g->custom_film_emulation.gamma, _("gamma value of the film emulation effect"));
    gtk_box_pack_start(GTK_BOX(g->custom_film_emulation.box), GTK_WIDGET(g->custom_film_emulation.gamma), TRUE,
                       TRUE, 0);
    g_signal_connect(G_OBJECT(g->custom_film_emulation.gamma), "value-changed",
                     G_CALLBACK(dt_iop_gmic_custom_film_emulation_params_t::gamma_callback), self);

    g->custom_film_emulation.hue
        = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->custom_film_emulation.parameters.hue, 3);
    dt_bauhaus_widget_set_label(g->custom_film_emulation.hue, NULL, _("hue"));
    gtk_widget_set_tooltip_text(g->custom_film_emulation.hue, _("hue shift of the film emulation effect"));
    gtk_box_pack_start(GTK_BOX(g->custom_film_emulation.box), GTK_WIDGET(g->custom_film_emulation.hue), TRUE, TRUE,
                       0);
    g_signal_connect(G_OBJECT(g->custom_film_emulation.hue), "value-changed",
                     G_CALLBACK(dt_iop_gmic_custom_film_emulation_params_t::hue_callback), self);

    g->custom_film_emulation.saturation
        = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->custom_film_emulation.parameters.saturation, 3);
    dt_bauhaus_widget_set_label(g->custom_film_emulation.saturation, NULL, _("saturation"));
    gtk_widget_set_tooltip_text(g->custom_film_emulation.saturation, _("saturation of the film emulation effect"));
    gtk_box_pack_start(GTK_BOX(g->custom_film_emulation.box), GTK_WIDGET(g->custom_film_emulation.saturation),
                       TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(g->custom_film_emulation.saturation), "value-changed",
                     G_CALLBACK(dt_iop_gmic_custom_film_emulation_params_t::saturation_callback), self);

    g->custom_film_emulation.normalize_colors = dt_bauhaus_combobox_new(self);
    dt_bauhaus_combobox_add(g->custom_film_emulation.normalize_colors, _("none"));
    dt_bauhaus_combobox_add(g->custom_film_emulation.normalize_colors, _("pre-process"));
    dt_bauhaus_combobox_add(g->custom_film_emulation.normalize_colors, _("post-process"));
    dt_bauhaus_combobox_add(g->custom_film_emulation.normalize_colors, _("both"));
    dt_bauhaus_combobox_set(g->custom_film_emulation.normalize_colors,
                            g->custom_film_emulation.parameters.normalize_colors);
    dt_bauhaus_widget_set_label(g->custom_film_emulation.normalize_colors, NULL, _("normalize colors"));
    gtk_widget_set_tooltip_text(g->custom_film_emulation.normalize_colors, _("choose how to normalize colors"));
    gtk_box_pack_start(GTK_BOX(g->custom_film_emulation.box), g->custom_film_emulation.normalize_colors, TRUE,
                       TRUE, 0);
    g_signal_connect(G_OBJECT(g->custom_film_emulation.normalize_colors), "value-changed",
                     G_CALLBACK(dt_iop_gmic_custom_film_emulation_params_t::normalize_colors_callback), self);
  }
  gtk_widget_show_all(g->custom_film_emulation.box);
  gtk_widget_set_no_show_all(g->custom_film_emulation.box, TRUE);
  gtk_widget_set_visible(g->custom_film_emulation.box, p->filter == custom_film_emulation ? TRUE : FALSE);
}

void dt_iop_gmic_custom_film_emulation_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->custom_film_emulation.box, p->filter == custom_film_emulation ? TRUE : FALSE);
  if(p->filter == custom_film_emulation and (not film_maps.empty()))
  {
    g->custom_film_emulation.parameters = *p;
    auto i = std::find(g->custom_film_emulation.film_list.begin(), g->custom_film_emulation.film_list.end(),
                       g->custom_film_emulation.parameters.film);
    if(i != g->custom_film_emulation.film_list.end())
      dt_bauhaus_combobox_set(g->custom_film_emulation.film,
                              static_cast<int>(i - g->custom_film_emulation.film_list.begin()));
    dt_bauhaus_slider_set(g->custom_film_emulation.strength, g->custom_film_emulation.parameters.strength);
    dt_bauhaus_slider_set(g->custom_film_emulation.brightness, g->custom_film_emulation.parameters.brightness);
    dt_bauhaus_slider_set(g->custom_film_emulation.contrast, g->custom_film_emulation.parameters.contrast);
    dt_bauhaus_slider_set(g->custom_film_emulation.gamma, g->custom_film_emulation.parameters.gamma);
    dt_bauhaus_slider_set(g->custom_film_emulation.hue, g->custom_film_emulation.parameters.hue);
    dt_bauhaus_slider_set(g->custom_film_emulation.saturation, g->custom_film_emulation.parameters.saturation);
    dt_bauhaus_combobox_set(g->custom_film_emulation.normalize_colors,
                            g->custom_film_emulation.parameters.normalize_colors);
  }
}

void dt_iop_gmic_custom_film_emulation_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_custom_film_emulation_params_t();
}

void dt_iop_gmic_custom_film_emulation_params_t::film_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    std::snprintf(G->custom_film_emulation.parameters.film, sizeof(G->custom_film_emulation.parameters.film), "%s",
                  film_maps[dt_bauhaus_combobox_get(W)].film_type.c_str());
    return G->custom_film_emulation.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_custom_film_emulation_params_t::strength_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->custom_film_emulation.parameters.strength = dt_bauhaus_slider_get(W);
    return G->custom_film_emulation.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_custom_film_emulation_params_t::brightness_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->custom_film_emulation.parameters.brightness = dt_bauhaus_slider_get(W);
    return G->custom_film_emulation.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_custom_film_emulation_params_t::contrast_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->custom_film_emulation.parameters.contrast = dt_bauhaus_slider_get(W);
    return G->custom_film_emulation.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_custom_film_emulation_params_t::gamma_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->custom_film_emulation.parameters.gamma = dt_bauhaus_slider_get(W);
    return G->custom_film_emulation.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_custom_film_emulation_params_t::hue_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->custom_film_emulation.parameters.hue = dt_bauhaus_slider_get(W);
    return G->custom_film_emulation.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_custom_film_emulation_params_t::saturation_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->custom_film_emulation.parameters.saturation = dt_bauhaus_slider_get(W);
    return G->custom_film_emulation.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_custom_film_emulation_params_t::normalize_colors_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->custom_film_emulation.parameters.normalize_colors = dt_bauhaus_combobox_get(W);
    return G->custom_film_emulation.parameters.to_gmic_params();
  });
}

const std::vector<film_map> dt_iop_gmic_custom_film_emulation_params_t::film_maps{ []() {
  std::vector<film_map> map;
  char p[PATH_MAX + 1];
  dt_loc_get_user_config_dir(p, sizeof(p));
  std::string path(p);
  path += "/luts/";
  DIR *dirp = opendir(path.c_str());
  if(dirp != nullptr)
  {
    struct dirent *dp;
    while((dp = readdir(dirp)) != NULL)
    {
      if(dp->d_type == DT_REG)
      {
        map.push_back(film_map(path + dp->d_name, dp->d_name));
      }
    }
    closedir(dirp);
  }
  std::sort(map.begin(), map.end(), [](const film_map &a, const film_map &b) { return a.printable < b.printable; });
  return map;
}() };

// --- freaky details

dt_iop_gmic_freaky_details_params_t::dt_iop_gmic_freaky_details_params_t(const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_freaky_details_params_t()
{
  dt_iop_gmic_freaky_details_params_t p;
  if(other.filter == freaky_details
     and std::sscanf(other.parameters, "dt_freaky_details %d,%g,%d,%d", &p.amplitude, &p.scale, &p.iterations,
                     &p.channel)
             == 4)
  {
    p.amplitude = clamp(1, 5, p.amplitude);
    p.scale = clamp(1.f, 100.f, p.scale);
    p.iterations = clamp(1, 4, p.iterations);
    p.channel = clamp(0, static_cast<int>(color_channels.size() - 1), p.channel);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_freaky_details_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = freaky_details;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_freaky_details %d,%g,%d,%d", amplitude, scale,
                iterations, channel);
  return ret;
}

const char *dt_iop_gmic_freaky_details_params_t::get_custom_command() {
  // clang-format off
  return R"raw(
_dt_freaky_details :
  repeat $! l[$>]
    repeat $3
      . +-. 255 *. -1
      repeat $1 bilateral. $2,{1.5*$2} done
      blend[-2,-1] vividlight blend overlay
    done
  endl done

dt_freaky_details :
  ac "_dt_freaky_details $1,$2,$3",$4,0
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_freaky_details_params_t::get_filter() const
{
  return freaky_details;
}

void dt_iop_gmic_freaky_details_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == freaky_details)
    g->freaky_details.parameters = *p;
  else
    g->freaky_details.parameters = dt_iop_gmic_freaky_details_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("freaky details"));
  g->freaky_details.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->freaky_details.box, TRUE, TRUE, 0);

  g->freaky_details.ampltude
      = dt_bauhaus_slider_new_with_range(self, 1, 5, 1, g->freaky_details.parameters.amplitude, 1);
  dt_bauhaus_widget_set_label(g->freaky_details.ampltude, NULL, _("amplitude"));
  gtk_widget_set_tooltip_text(g->freaky_details.ampltude, _("amplitude of the freaky details filter"));
  gtk_box_pack_start(GTK_BOX(g->freaky_details.box), GTK_WIDGET(g->freaky_details.ampltude), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->freaky_details.ampltude), "value-changed",
                   G_CALLBACK(dt_iop_gmic_freaky_details_params_t::amplitude_callback), self);

  g->freaky_details.scale
      = dt_bauhaus_slider_new_with_range(self, 0, 100, 1, g->freaky_details.parameters.scale, 1);
  dt_bauhaus_widget_set_label(g->freaky_details.scale, NULL, _("scale"));
  gtk_widget_set_tooltip_text(g->freaky_details.scale, _("scale of the freaky details filter"));
  gtk_box_pack_start(GTK_BOX(g->freaky_details.box), GTK_WIDGET(g->freaky_details.scale), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->freaky_details.scale), "value-changed",
                   G_CALLBACK(dt_iop_gmic_freaky_details_params_t::scale_callback), self);

  g->freaky_details.iterations
      = dt_bauhaus_slider_new_with_range(self, 1, 4, 1, g->freaky_details.parameters.iterations, 0);
  dt_bauhaus_widget_set_label(g->freaky_details.iterations, NULL, _("iterations"));
  gtk_widget_set_tooltip_text(g->freaky_details.iterations, _("number of iterations"));
  gtk_box_pack_start(GTK_BOX(g->freaky_details.box), GTK_WIDGET(g->freaky_details.iterations), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->freaky_details.iterations), "value-changed",
                   G_CALLBACK(dt_iop_gmic_freaky_details_params_t::iterations_callback), self);

  g->freaky_details.channel = dt_bauhaus_combobox_new(self);
  for(auto str : color_channels) dt_bauhaus_combobox_add(g->freaky_details.channel, str);
  dt_bauhaus_widget_set_label(g->freaky_details.channel, NULL, _("channel"));
  gtk_widget_set_tooltip_text(g->freaky_details.channel, _("apply filter to specific color channel(s)"));
  gtk_box_pack_start(GTK_BOX(g->freaky_details.box), g->freaky_details.channel, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->freaky_details.channel), "value-changed",
                   G_CALLBACK(dt_iop_gmic_freaky_details_params_t::channel_callback), self);

  gtk_widget_show_all(g->freaky_details.box);
  gtk_widget_set_no_show_all(g->freaky_details.box, TRUE);
  gtk_widget_set_visible(g->freaky_details.box, p->filter == freaky_details ? TRUE : FALSE);
}

void dt_iop_gmic_freaky_details_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->freaky_details.box, p->filter == freaky_details ? TRUE : FALSE);
  if(p->filter == freaky_details)
  {
    g->freaky_details.parameters = *p;
    dt_bauhaus_slider_set(g->freaky_details.ampltude, g->freaky_details.parameters.amplitude);
    dt_bauhaus_slider_set(g->freaky_details.scale, g->freaky_details.parameters.scale);
    dt_bauhaus_slider_set(g->freaky_details.iterations, g->freaky_details.parameters.iterations);
    dt_bauhaus_combobox_set(g->freaky_details.channel, g->freaky_details.parameters.channel);
  }
}

void dt_iop_gmic_freaky_details_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_freaky_details_params_t();
}

void dt_iop_gmic_freaky_details_params_t::amplitude_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->freaky_details.parameters.amplitude = static_cast<int>(std::round(dt_bauhaus_slider_get(W)));
    return G->freaky_details.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_freaky_details_params_t::scale_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->freaky_details.parameters.scale = dt_bauhaus_slider_get(W);
    return G->freaky_details.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_freaky_details_params_t::iterations_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->freaky_details.parameters.iterations = static_cast<int>(std::round(dt_bauhaus_slider_get(W)));
    return G->freaky_details.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_freaky_details_params_t::channel_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->freaky_details.parameters.channel = dt_bauhaus_combobox_get(W);
    return G->freaky_details.parameters.to_gmic_params();
  });
}

// --- sharpen Richardson Lucy

dt_iop_gmic_sharpen_Richardson_Lucy_params_t::dt_iop_gmic_sharpen_Richardson_Lucy_params_t(
    const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_sharpen_Richardson_Lucy_params_t()
{
  dt_iop_gmic_sharpen_Richardson_Lucy_params_t p;
  if(other.filter == sharpen_Richardson_Lucy
     and std::sscanf(other.parameters, "dt_sharpen_Richardson_Lucy %g,%d,%d,%d", &p.sigma, &p.iterations, &p.blur,
                     &p.channel)
             == 4)
  {
    p.sigma = clamp(0.5f, 10.f, p.sigma);
    p.iterations = clamp(1, 100, p.iterations);
    p.blur = clamp(0, 1, p.blur);
    p.channel = clamp(0, static_cast<int>(color_channels.size() - 1), p.channel);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_sharpen_Richardson_Lucy_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = sharpen_Richardson_Lucy;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_sharpen_Richardson_Lucy %g,%d,%d,%d", sigma,
                iterations, blur, channel);
  return ret;
}

const char *dt_iop_gmic_sharpen_Richardson_Lucy_params_t::get_custom_command() {
  // clang-format off
  return R"raw(
dt_sharpen_Richardson_Lucy :
  ac "apply_parallel_overlap \"deblur_richardsonlucy $1,$2,$3\",24,0",$4,0
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_sharpen_Richardson_Lucy_params_t::get_filter() const
{
  return sharpen_Richardson_Lucy;
}

void dt_iop_gmic_sharpen_Richardson_Lucy_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == sharpen_Richardson_Lucy)
    g->sharpen_Richardson_Lucy.parameters = *p;
  else
    g->sharpen_Richardson_Lucy.parameters = dt_iop_gmic_sharpen_Richardson_Lucy_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("sharpen (Richardson-Lucy)"));
  g->sharpen_Richardson_Lucy.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->sharpen_Richardson_Lucy.box, TRUE, TRUE, 0);

  g->sharpen_Richardson_Lucy.sigma
      = dt_bauhaus_slider_new_with_range(self, 0.5, 10, 0.05, g->sharpen_Richardson_Lucy.parameters.sigma, 2);
  dt_bauhaus_widget_set_label(g->sharpen_Richardson_Lucy.sigma, NULL, _("sigma"));
  gtk_widget_set_tooltip_text(g->sharpen_Richardson_Lucy.sigma, _("width of the sharpening filter"));
  gtk_box_pack_start(GTK_BOX(g->sharpen_Richardson_Lucy.box), GTK_WIDGET(g->sharpen_Richardson_Lucy.sigma), TRUE,
                     TRUE, 0);
  g_signal_connect(G_OBJECT(g->sharpen_Richardson_Lucy.sigma), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sharpen_Richardson_Lucy_params_t::sigma_callback), self);

  g->sharpen_Richardson_Lucy.iterations
      = dt_bauhaus_slider_new_with_range(self, 1, 100, 1, g->sharpen_Richardson_Lucy.parameters.iterations, 0);
  dt_bauhaus_widget_set_label(g->sharpen_Richardson_Lucy.iterations, NULL, _("iterations"));
  gtk_widget_set_tooltip_text(g->sharpen_Richardson_Lucy.iterations, _("number of iterations"));
  gtk_box_pack_start(GTK_BOX(g->sharpen_Richardson_Lucy.box), GTK_WIDGET(g->sharpen_Richardson_Lucy.iterations),
                     TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sharpen_Richardson_Lucy.iterations), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sharpen_Richardson_Lucy_params_t::iterations_callback), self);

  g->sharpen_Richardson_Lucy.blur = dt_bauhaus_combobox_new(self);
  dt_bauhaus_combobox_add(g->sharpen_Richardson_Lucy.blur, _("expnential"));
  dt_bauhaus_combobox_add(g->sharpen_Richardson_Lucy.blur, _("Gaussian"));
  dt_bauhaus_widget_set_label(g->sharpen_Richardson_Lucy.blur, NULL, _("blur type"));
  gtk_widget_set_tooltip_text(g->sharpen_Richardson_Lucy.blur, _("choose blur method"));
  gtk_box_pack_start(GTK_BOX(g->sharpen_Richardson_Lucy.box), g->sharpen_Richardson_Lucy.blur, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sharpen_Richardson_Lucy.blur), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sharpen_Richardson_Lucy_params_t::blur_callback), self);

  g->sharpen_Richardson_Lucy.channel = dt_bauhaus_combobox_new(self);
  for(auto str : color_channels) dt_bauhaus_combobox_add(g->sharpen_Richardson_Lucy.channel, str);
  dt_bauhaus_widget_set_label(g->sharpen_Richardson_Lucy.channel, NULL, _("channel"));
  gtk_widget_set_tooltip_text(g->sharpen_Richardson_Lucy.channel, _("apply filter to specific color channel(s)"));
  gtk_box_pack_start(GTK_BOX(g->sharpen_Richardson_Lucy.box), g->sharpen_Richardson_Lucy.channel, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sharpen_Richardson_Lucy.channel), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sharpen_Richardson_Lucy_params_t::channel_callback), self);

  gtk_widget_show_all(g->sharpen_Richardson_Lucy.box);
  gtk_widget_set_no_show_all(g->sharpen_Richardson_Lucy.box, TRUE);
  gtk_widget_set_visible(g->sharpen_Richardson_Lucy.box, p->filter == sharpen_Richardson_Lucy ? TRUE : FALSE);
}

void dt_iop_gmic_sharpen_Richardson_Lucy_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->sharpen_Richardson_Lucy.box, p->filter == sharpen_Richardson_Lucy ? TRUE : FALSE);
  if(p->filter == sharpen_Richardson_Lucy)
  {
    g->sharpen_Richardson_Lucy.parameters = *p;
    dt_bauhaus_slider_set(g->sharpen_Richardson_Lucy.sigma, g->sharpen_Richardson_Lucy.parameters.sigma);
    dt_bauhaus_slider_set(g->sharpen_Richardson_Lucy.iterations, g->sharpen_Richardson_Lucy.parameters.iterations);
    dt_bauhaus_combobox_set(g->sharpen_Richardson_Lucy.blur, g->sharpen_Richardson_Lucy.parameters.blur);
    dt_bauhaus_combobox_set(g->sharpen_Richardson_Lucy.channel, g->sharpen_Richardson_Lucy.parameters.channel);
  }
}

void dt_iop_gmic_sharpen_Richardson_Lucy_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_sharpen_Richardson_Lucy_params_t();
}

void dt_iop_gmic_sharpen_Richardson_Lucy_params_t::sigma_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->sharpen_Richardson_Lucy.parameters.sigma = dt_bauhaus_slider_get(W);
    return G->sharpen_Richardson_Lucy.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_sharpen_Richardson_Lucy_params_t::iterations_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->sharpen_Richardson_Lucy.parameters.iterations = static_cast<int>(std::round(dt_bauhaus_slider_get(W)));
    return G->sharpen_Richardson_Lucy.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_sharpen_Richardson_Lucy_params_t::blur_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->sharpen_Richardson_Lucy.parameters.blur = dt_bauhaus_combobox_get(W);
    return G->sharpen_Richardson_Lucy.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_sharpen_Richardson_Lucy_params_t::channel_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->sharpen_Richardson_Lucy.parameters.channel = dt_bauhaus_combobox_get(W);
    return G->sharpen_Richardson_Lucy.parameters.to_gmic_params();
  });
}

// --- sharpen Gold Meinel

dt_iop_gmic_sharpen_Gold_Meinel_params_t::dt_iop_gmic_sharpen_Gold_Meinel_params_t(const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_sharpen_Gold_Meinel_params_t()
{
  dt_iop_gmic_sharpen_Gold_Meinel_params_t p;
  if(other.filter == sharpen_Gold_Meinel
     and std::sscanf(other.parameters, "dt_sharpen_Gold_Meinel %g,%d,%g,%d,%d", &p.sigma, &p.iterations,
                     &p.acceleration, &p.blur, &p.channel)
             == 5)
  {
    p.sigma = clamp(0.5f, 10.f, p.sigma);
    p.iterations = clamp(1, 15, p.iterations);
    p.acceleration = clamp(1.f, 3.f, p.acceleration);
    p.blur = clamp(0, 1, p.blur);
    p.channel = clamp(0, static_cast<int>(color_channels.size() - 1), p.channel);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_sharpen_Gold_Meinel_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = sharpen_Gold_Meinel;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_sharpen_Gold_Meinel %g,%d,%g,%d,%d", sigma, iterations,
                acceleration, blur, channel);
  return ret;
}

const char *dt_iop_gmic_sharpen_Gold_Meinel_params_t::get_custom_command() {
  // clang-format off
  return R"raw(
dt_sharpen_Gold_Meinel :
  ac "apply_parallel_overlap \"deblur_richardsonlucy $1,$2,$3,$4\",24,0",$5,0
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_sharpen_Gold_Meinel_params_t::get_filter() const
{
  return sharpen_Gold_Meinel;
}

void dt_iop_gmic_sharpen_Gold_Meinel_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == sharpen_Gold_Meinel)
    g->sharpen_Gold_Meinel.parameters = *p;
  else
    g->sharpen_Gold_Meinel.parameters = dt_iop_gmic_sharpen_Gold_Meinel_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("sharpen (Gold-Meinel)"));
  g->sharpen_Gold_Meinel.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->sharpen_Gold_Meinel.box, TRUE, TRUE, 0);

  g->sharpen_Gold_Meinel.sigma
      = dt_bauhaus_slider_new_with_range(self, 0.5, 10, 0.05, g->sharpen_Gold_Meinel.parameters.sigma, 2);
  dt_bauhaus_widget_set_label(g->sharpen_Gold_Meinel.sigma, NULL, _("sigma"));
  gtk_widget_set_tooltip_text(g->sharpen_Gold_Meinel.sigma, _("width of the sharpening filter"));
  gtk_box_pack_start(GTK_BOX(g->sharpen_Gold_Meinel.box), GTK_WIDGET(g->sharpen_Gold_Meinel.sigma), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sharpen_Gold_Meinel.sigma), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sharpen_Gold_Meinel_params_t::sigma_callback), self);

  g->sharpen_Gold_Meinel.iterations
      = dt_bauhaus_slider_new_with_range(self, 1, 15, 1, g->sharpen_Gold_Meinel.parameters.iterations, 0);
  dt_bauhaus_widget_set_label(g->sharpen_Gold_Meinel.iterations, NULL, _("iterations"));
  gtk_widget_set_tooltip_text(g->sharpen_Gold_Meinel.iterations, _("number of iterations"));
  gtk_box_pack_start(GTK_BOX(g->sharpen_Gold_Meinel.box), GTK_WIDGET(g->sharpen_Gold_Meinel.iterations), TRUE,
                     TRUE, 0);
  g_signal_connect(G_OBJECT(g->sharpen_Gold_Meinel.iterations), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sharpen_Gold_Meinel_params_t::iterations_callback), self);

  g->sharpen_Gold_Meinel.acceleration
      = dt_bauhaus_slider_new_with_range(self, 1, 3, 0.05, g->sharpen_Gold_Meinel.parameters.acceleration, 2);
  dt_bauhaus_widget_set_label(g->sharpen_Gold_Meinel.acceleration, NULL, _("acceleration"));
  gtk_widget_set_tooltip_text(g->sharpen_Gold_Meinel.acceleration, _("acceleration of the sharpening filter"));
  gtk_box_pack_start(GTK_BOX(g->sharpen_Gold_Meinel.box), GTK_WIDGET(g->sharpen_Gold_Meinel.acceleration), TRUE,
                     TRUE, 0);
  g_signal_connect(G_OBJECT(g->sharpen_Gold_Meinel.acceleration), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sharpen_Gold_Meinel_params_t::acceleration_callback), self);

  g->sharpen_Gold_Meinel.blur = dt_bauhaus_combobox_new(self);
  dt_bauhaus_combobox_add(g->sharpen_Gold_Meinel.blur, _("expnential"));
  dt_bauhaus_combobox_add(g->sharpen_Gold_Meinel.blur, _("Gaussian"));
  dt_bauhaus_widget_set_label(g->sharpen_Gold_Meinel.blur, NULL, _("blur type"));
  gtk_widget_set_tooltip_text(g->sharpen_Gold_Meinel.blur, _("choose blur method"));
  gtk_box_pack_start(GTK_BOX(g->sharpen_Gold_Meinel.box), g->sharpen_Gold_Meinel.blur, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sharpen_Gold_Meinel.blur), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sharpen_Gold_Meinel_params_t::blur_callback), self);

  g->sharpen_Gold_Meinel.channel = dt_bauhaus_combobox_new(self);
  for(auto str : color_channels) dt_bauhaus_combobox_add(g->sharpen_Gold_Meinel.channel, str);
  dt_bauhaus_widget_set_label(g->sharpen_Gold_Meinel.channel, NULL, _("channel"));
  gtk_widget_set_tooltip_text(g->sharpen_Gold_Meinel.channel, _("apply filter to specific color channel(s)"));
  gtk_box_pack_start(GTK_BOX(g->sharpen_Gold_Meinel.box), g->sharpen_Gold_Meinel.channel, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sharpen_Gold_Meinel.channel), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sharpen_Gold_Meinel_params_t::channel_callback), self);

  gtk_widget_show_all(g->sharpen_Gold_Meinel.box);
  gtk_widget_set_no_show_all(g->sharpen_Gold_Meinel.box, TRUE);
  gtk_widget_set_visible(g->sharpen_Gold_Meinel.box, p->filter == sharpen_Gold_Meinel ? TRUE : FALSE);
}

void dt_iop_gmic_sharpen_Gold_Meinel_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->sharpen_Gold_Meinel.box, p->filter == sharpen_Gold_Meinel ? TRUE : FALSE);
  if(p->filter == sharpen_Gold_Meinel)
  {
    g->sharpen_Gold_Meinel.parameters = *p;
    dt_bauhaus_slider_set(g->sharpen_Gold_Meinel.sigma, g->sharpen_Gold_Meinel.parameters.sigma);
    dt_bauhaus_slider_set(g->sharpen_Gold_Meinel.iterations, g->sharpen_Gold_Meinel.parameters.iterations);
    dt_bauhaus_slider_set(g->sharpen_Gold_Meinel.acceleration, g->sharpen_Gold_Meinel.parameters.acceleration);
    dt_bauhaus_combobox_set(g->sharpen_Gold_Meinel.blur, g->sharpen_Gold_Meinel.parameters.blur);
    dt_bauhaus_combobox_set(g->sharpen_Gold_Meinel.channel, g->sharpen_Gold_Meinel.parameters.channel);
  }
}

void dt_iop_gmic_sharpen_Gold_Meinel_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_sharpen_Gold_Meinel_params_t();
}

void dt_iop_gmic_sharpen_Gold_Meinel_params_t::sigma_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->sharpen_Gold_Meinel.parameters.sigma = dt_bauhaus_slider_get(W);
    return G->sharpen_Gold_Meinel.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_sharpen_Gold_Meinel_params_t::iterations_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->sharpen_Gold_Meinel.parameters.iterations = static_cast<int>(std::round(dt_bauhaus_slider_get(W)));
    return G->sharpen_Gold_Meinel.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_sharpen_Gold_Meinel_params_t::acceleration_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->sharpen_Gold_Meinel.parameters.acceleration = static_cast<int>(std::round(dt_bauhaus_slider_get(W)));
    return G->sharpen_Gold_Meinel.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_sharpen_Gold_Meinel_params_t::blur_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->sharpen_Gold_Meinel.parameters.blur = dt_bauhaus_combobox_get(W);
    return G->sharpen_Gold_Meinel.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_sharpen_Gold_Meinel_params_t::channel_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->sharpen_Gold_Meinel.parameters.channel = dt_bauhaus_combobox_get(W);
    return G->sharpen_Gold_Meinel.parameters.to_gmic_params();
  });
}

// --- sharpen inverse diffusion

dt_iop_gmic_sharpen_inverse_diffusion_params_t::dt_iop_gmic_sharpen_inverse_diffusion_params_t(
    const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_sharpen_inverse_diffusion_params_t()
{
  dt_iop_gmic_sharpen_inverse_diffusion_params_t p;
  if(other.filter == sharpen_inverse_diffusion
     and std::sscanf(other.parameters, "dt_sharpen_inverse_diffusion %g,%d,%d", &p.amplitude, &p.iterations,
                     &p.channel)
             == 3)
  {
    p.amplitude = clamp(1.f, 300.f, p.amplitude);
    p.iterations = clamp(1, 10, p.iterations);
    p.channel = clamp(0, static_cast<int>(color_channels.size() - 1), p.channel);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_sharpen_inverse_diffusion_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = sharpen_inverse_diffusion;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_sharpen_inverse_diffusion %g,%d,%d", amplitude,
                iterations, channel);
  return ret;
}

const char *dt_iop_gmic_sharpen_inverse_diffusion_params_t::get_custom_command() {
  // clang-format off
  return R"raw(
dt_sharpen_inverse_diffusion :
  ac "apply_parallel_overlap \"repeat $2 sharpen $1 done\",24,0",$3,0
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_sharpen_inverse_diffusion_params_t::get_filter() const
{
  return sharpen_inverse_diffusion;
}

void dt_iop_gmic_sharpen_inverse_diffusion_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == sharpen_inverse_diffusion)
    g->sharpen_inverse_diffusion.parameters = *p;
  else
    g->sharpen_inverse_diffusion.parameters = dt_iop_gmic_sharpen_inverse_diffusion_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("sharpen (inverse diffusion)"));
  g->sharpen_inverse_diffusion.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->sharpen_inverse_diffusion.box, TRUE, TRUE, 0);

  g->sharpen_inverse_diffusion.ampltude
      = dt_bauhaus_slider_new_with_range(self, 0, 300, 1, g->sharpen_inverse_diffusion.parameters.amplitude, 1);
  dt_bauhaus_widget_set_label(g->sharpen_inverse_diffusion.ampltude, NULL, _("amplitude"));
  gtk_widget_set_tooltip_text(g->sharpen_inverse_diffusion.ampltude, _("amplitude of the sharpening filter"));
  gtk_box_pack_start(GTK_BOX(g->sharpen_inverse_diffusion.box), GTK_WIDGET(g->sharpen_inverse_diffusion.ampltude),
                     TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sharpen_inverse_diffusion.ampltude), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sharpen_inverse_diffusion_params_t::amplitude_callback), self);

  g->sharpen_inverse_diffusion.iterations
      = dt_bauhaus_slider_new_with_range(self, 1, 15, 1, g->sharpen_inverse_diffusion.parameters.iterations, 0);
  dt_bauhaus_widget_set_label(g->sharpen_inverse_diffusion.iterations, NULL, _("iterations"));
  gtk_widget_set_tooltip_text(g->sharpen_inverse_diffusion.iterations, _("number of iterations"));
  gtk_box_pack_start(GTK_BOX(g->sharpen_inverse_diffusion.box),
                     GTK_WIDGET(g->sharpen_inverse_diffusion.iterations), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sharpen_inverse_diffusion.iterations), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sharpen_inverse_diffusion_params_t::iterations_callback), self);

  g->sharpen_inverse_diffusion.channel = dt_bauhaus_combobox_new(self);
  for(auto str : color_channels) dt_bauhaus_combobox_add(g->sharpen_inverse_diffusion.channel, str);
  dt_bauhaus_widget_set_label(g->sharpen_inverse_diffusion.channel, NULL, _("channel"));
  gtk_widget_set_tooltip_text(g->sharpen_inverse_diffusion.channel,
                              _("apply filter to specific color channel(s)"));
  gtk_box_pack_start(GTK_BOX(g->sharpen_inverse_diffusion.box), g->sharpen_inverse_diffusion.channel, TRUE, TRUE,
                     0);
  g_signal_connect(G_OBJECT(g->sharpen_inverse_diffusion.channel), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sharpen_inverse_diffusion_params_t::channel_callback), self);

  gtk_widget_show_all(g->sharpen_inverse_diffusion.box);
  gtk_widget_set_no_show_all(g->sharpen_inverse_diffusion.box, TRUE);
  gtk_widget_set_visible(g->sharpen_inverse_diffusion.box, p->filter == sharpen_inverse_diffusion ? TRUE : FALSE);
}

void dt_iop_gmic_sharpen_inverse_diffusion_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->sharpen_inverse_diffusion.box, p->filter == sharpen_inverse_diffusion ? TRUE : FALSE);
  if(p->filter == sharpen_inverse_diffusion)
  {
    g->sharpen_inverse_diffusion.parameters = *p;
    dt_bauhaus_slider_set(g->sharpen_inverse_diffusion.ampltude, g->sharpen_inverse_diffusion.parameters.amplitude);
    dt_bauhaus_slider_set(g->sharpen_inverse_diffusion.iterations,
                          g->sharpen_inverse_diffusion.parameters.iterations);
    dt_bauhaus_combobox_set(g->sharpen_inverse_diffusion.channel, g->sharpen_inverse_diffusion.parameters.channel);
  }
}

void dt_iop_gmic_sharpen_inverse_diffusion_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_sharpen_inverse_diffusion_params_t();
}

void dt_iop_gmic_sharpen_inverse_diffusion_params_t::amplitude_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->sharpen_inverse_diffusion.parameters.amplitude = dt_bauhaus_slider_get(W);
    return G->sharpen_inverse_diffusion.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_sharpen_inverse_diffusion_params_t::iterations_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->sharpen_inverse_diffusion.parameters.iterations = static_cast<int>(std::round(dt_bauhaus_slider_get(W)));
    return G->sharpen_inverse_diffusion.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_sharpen_inverse_diffusion_params_t::channel_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->sharpen_inverse_diffusion.parameters.channel = dt_bauhaus_combobox_get(W);
    return G->sharpen_inverse_diffusion.parameters.to_gmic_params();
  });
}

// --- magic details

dt_iop_gmic_magic_details_params_t::dt_iop_gmic_magic_details_params_t(const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_magic_details_params_t()
{
  dt_iop_gmic_magic_details_params_t p;
  if(other.filter == magic_details
     and std::sscanf(other.parameters, "dt_magic_details %g,%g,%g,%g,%g,%d", &p.amplitude, &p.spatial_scale,
                     &p.value_scale, &p.edges, &p.smoothness, &p.channel)
             == 6)
  {
    p.amplitude = clamp(0.f, 30.f, p.amplitude);
    p.spatial_scale = clamp(0.f, 10.f, p.spatial_scale);
    p.value_scale = clamp(0.f, 20.f, p.value_scale);
    p.edges = clamp(-3.0f, 3.0f, p.edges);
    p.smoothness = clamp(0.f, 20.f, p.smoothness);
    p.channel = clamp(0, static_cast<int>(color_channels.size() - 1), p.channel);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_magic_details_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = magic_details;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_magic_details %g,%g,%g,%g,%g,%d", amplitude,
                spatial_scale, value_scale, edges, smoothness, channel);
  return ret;
}

const char *dt_iop_gmic_magic_details_params_t::get_custom_command() {
  // clang-format off
  return R"raw(
_dt_magic_details :
  repeat $! l[$>]
    +bilateral $2,$3
    +gradient_norm.. +. 1
    pow. {$4>=0?3.1-$4:-3.1-$4}
    b. $5 n. 1,{1+$1}
    -... .. *[-3,-1] + c 0,255
  endl done

dt_magic_details :
  ac "_dt_magic_details ${1-5}",$6,0
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_magic_details_params_t::get_filter() const
{
  return magic_details;
}

void dt_iop_gmic_magic_details_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == magic_details)
    g->magic_details.parameters = *p;
  else
    g->magic_details.parameters = dt_iop_gmic_magic_details_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("magic details"));
  g->magic_details.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->magic_details.box, TRUE, TRUE, 0);

  g->magic_details.ampltude
      = dt_bauhaus_slider_new_with_range(self, 0, 30, 0.1, g->magic_details.parameters.amplitude, 1);
  dt_bauhaus_widget_set_label(g->magic_details.ampltude, NULL, _("amplitude"));
  gtk_widget_set_tooltip_text(g->magic_details.ampltude, _("amplitude of the magic details filter"));
  gtk_box_pack_start(GTK_BOX(g->magic_details.box), GTK_WIDGET(g->magic_details.ampltude), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->magic_details.ampltude), "value-changed",
                   G_CALLBACK(dt_iop_gmic_magic_details_params_t::amplitude_callback), self);

  g->magic_details.spatial_scale
      = dt_bauhaus_slider_new_with_range(self, 0, 10, 0.1, g->magic_details.parameters.spatial_scale, 1);
  dt_bauhaus_widget_set_label(g->magic_details.spatial_scale, NULL, _("spatial scale"));
  gtk_widget_set_tooltip_text(g->magic_details.spatial_scale, _("spatial scale of the magic details filter"));
  gtk_box_pack_start(GTK_BOX(g->magic_details.box), GTK_WIDGET(g->magic_details.spatial_scale), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->magic_details.spatial_scale), "value-changed",
                   G_CALLBACK(dt_iop_gmic_magic_details_params_t::spatial_scale_callback), self);

  g->magic_details.value_scale
      = dt_bauhaus_slider_new_with_range(self, 0, 20, 0.1, g->magic_details.parameters.value_scale, 1);
  dt_bauhaus_widget_set_label(g->magic_details.value_scale, NULL, _("value scale"));
  gtk_widget_set_tooltip_text(g->magic_details.value_scale, _("value scale of the magic details filter"));
  gtk_box_pack_start(GTK_BOX(g->magic_details.box), GTK_WIDGET(g->magic_details.value_scale), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->magic_details.value_scale), "value-changed",
                   G_CALLBACK(dt_iop_gmic_magic_details_params_t::value_scale_callback), self);

  g->magic_details.edges
      = dt_bauhaus_slider_new_with_range(self, -3, 3, 0.1, g->magic_details.parameters.edges, 1);
  dt_bauhaus_widget_set_label(g->magic_details.edges, NULL, _("edges"));
  gtk_widget_set_tooltip_text(g->magic_details.edges, _("edges of the magic details filter"));
  gtk_box_pack_start(GTK_BOX(g->magic_details.box), GTK_WIDGET(g->magic_details.edges), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->magic_details.edges), "value-changed",
                   G_CALLBACK(dt_iop_gmic_magic_details_params_t::edges_callback), self);

  g->magic_details.smoothness
      = dt_bauhaus_slider_new_with_range(self, 0, 20, 0.1, g->magic_details.parameters.smoothness, 1);
  dt_bauhaus_widget_set_label(g->magic_details.smoothness, NULL, _("smoothness"));
  gtk_widget_set_tooltip_text(g->magic_details.smoothness, _("smoothness of the magic details filter"));
  gtk_box_pack_start(GTK_BOX(g->magic_details.box), GTK_WIDGET(g->magic_details.smoothness), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->magic_details.smoothness), "value-changed",
                   G_CALLBACK(dt_iop_gmic_magic_details_params_t::smoothness_callback), self);

  g->magic_details.channel = dt_bauhaus_combobox_new(self);
  for(auto str : color_channels) dt_bauhaus_combobox_add(g->magic_details.channel, str);
  dt_bauhaus_widget_set_label(g->magic_details.channel, NULL, _("channel"));
  gtk_widget_set_tooltip_text(g->magic_details.channel, _("apply filter to specific color channel(s)"));
  gtk_box_pack_start(GTK_BOX(g->magic_details.box), g->magic_details.channel, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->magic_details.channel), "value-changed",
                   G_CALLBACK(dt_iop_gmic_magic_details_params_t::channel_callback), self);

  gtk_widget_show_all(g->magic_details.box);
  gtk_widget_set_no_show_all(g->magic_details.box, TRUE);
  gtk_widget_set_visible(g->magic_details.box, p->filter == magic_details ? TRUE : FALSE);
}

void dt_iop_gmic_magic_details_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->magic_details.box, p->filter == magic_details ? TRUE : FALSE);
  if(p->filter == magic_details)
  {
    g->magic_details.parameters = *p;
    dt_bauhaus_slider_set(g->magic_details.ampltude, g->magic_details.parameters.amplitude);
    dt_bauhaus_slider_set(g->magic_details.spatial_scale, g->magic_details.parameters.spatial_scale);
    dt_bauhaus_slider_set(g->magic_details.value_scale, g->magic_details.parameters.value_scale);
    dt_bauhaus_slider_set(g->magic_details.edges, g->magic_details.parameters.edges);
    dt_bauhaus_slider_set(g->magic_details.smoothness, g->magic_details.parameters.smoothness);
    dt_bauhaus_combobox_set(g->magic_details.channel, g->magic_details.parameters.channel);
  }
}

void dt_iop_gmic_magic_details_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_magic_details_params_t();
}

void dt_iop_gmic_magic_details_params_t::amplitude_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->magic_details.parameters.amplitude = dt_bauhaus_slider_get(W);
    return G->magic_details.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_magic_details_params_t::spatial_scale_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->magic_details.parameters.spatial_scale = dt_bauhaus_slider_get(W);
    return G->magic_details.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_magic_details_params_t::value_scale_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->magic_details.parameters.value_scale = dt_bauhaus_slider_get(W);
    return G->magic_details.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_magic_details_params_t::edges_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->magic_details.parameters.edges = dt_bauhaus_slider_get(W);
    return G->magic_details.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_magic_details_params_t::smoothness_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->magic_details.parameters.smoothness = dt_bauhaus_slider_get(W);
    return G->magic_details.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_magic_details_params_t::channel_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->magic_details.parameters.channel = dt_bauhaus_combobox_get(W);
    return G->magic_details.parameters.to_gmic_params();
  });
}

// --- basic color adjustments

dt_iop_gmic_basic_color_adjustments_params_t::dt_iop_gmic_basic_color_adjustments_params_t(
    const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_basic_color_adjustments_params_t()
{
  dt_iop_gmic_basic_color_adjustments_params_t p;
  if(other.filter == basic_color_adjustments
     and std::sscanf(other.parameters, "dt_basic_color_adjustments %g,%g,%g,%g,%g", &p.brightness, &p.contrast,
                     &p.gamma, &p.hue, &p.saturation)
             == 5)
  {
    p.brightness = clamp(-1.f, 1.f, p.brightness);
    p.contrast = clamp(-1.f, 1.f, p.contrast);
    p.gamma = clamp(-1.f, 1.f, p.gamma);
    p.hue = clamp(-1.f, 1.f, p.hue);
    p.saturation = clamp(-1.f, 1.f, p.saturation);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_basic_color_adjustments_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = basic_color_adjustments;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_basic_color_adjustments %g,%g,%g,%g,%g", brightness,
                contrast, gamma, hue, saturation);
  return ret;
}

const char *dt_iop_gmic_basic_color_adjustments_params_t::get_custom_command()
{
  // clang-format off
  return R"raw(
dt_basic_color_adjustments :
  adjust_colors {100*$1},{100*$2},{100*$3},{100*$4},{100*$5},0,255
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_basic_color_adjustments_params_t::get_filter() const
{
  return basic_color_adjustments;
}

void dt_iop_gmic_basic_color_adjustments_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == basic_color_adjustments)
    g->basic_color_adjustments.parameters = *p;
  else
    g->basic_color_adjustments.parameters = dt_iop_gmic_basic_color_adjustments_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("basic color adjustmens"));
  g->basic_color_adjustments.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->basic_color_adjustments.box, TRUE, TRUE, 0);

  g->basic_color_adjustments.brightness
      = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->basic_color_adjustments.parameters.brightness, 3);
  dt_bauhaus_widget_set_label(g->basic_color_adjustments.brightness, NULL, _("brightness"));
  gtk_widget_set_tooltip_text(g->basic_color_adjustments.brightness, _("brightness adjustment"));
  gtk_box_pack_start(GTK_BOX(g->basic_color_adjustments.box), GTK_WIDGET(g->basic_color_adjustments.brightness),
                     TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->basic_color_adjustments.brightness), "value-changed",
                   G_CALLBACK(dt_iop_gmic_basic_color_adjustments_params_t::brightness_callback), self);

  g->basic_color_adjustments.contrast
      = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->basic_color_adjustments.parameters.contrast, 3);
  dt_bauhaus_widget_set_label(g->basic_color_adjustments.contrast, NULL, _("contrast"));
  gtk_widget_set_tooltip_text(g->basic_color_adjustments.contrast, _("contrast adjustment"));
  gtk_box_pack_start(GTK_BOX(g->basic_color_adjustments.box), GTK_WIDGET(g->basic_color_adjustments.contrast),
                     TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->basic_color_adjustments.contrast), "value-changed",
                   G_CALLBACK(dt_iop_gmic_basic_color_adjustments_params_t::contrast_callback), self);

  g->basic_color_adjustments.gamma
      = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->basic_color_adjustments.parameters.gamma, 3);
  dt_bauhaus_widget_set_label(g->basic_color_adjustments.gamma, NULL, _("gamma"));
  gtk_widget_set_tooltip_text(g->basic_color_adjustments.gamma, _("gamma adjustment"));
  gtk_box_pack_start(GTK_BOX(g->basic_color_adjustments.box), GTK_WIDGET(g->basic_color_adjustments.gamma), TRUE,
                     TRUE, 0);
  g_signal_connect(G_OBJECT(g->basic_color_adjustments.gamma), "value-changed",
                   G_CALLBACK(dt_iop_gmic_basic_color_adjustments_params_t::gamma_callback), self);

  g->basic_color_adjustments.hue
      = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->basic_color_adjustments.parameters.hue, 3);
  dt_bauhaus_widget_set_label(g->basic_color_adjustments.hue, NULL, _("hue"));
  gtk_widget_set_tooltip_text(g->basic_color_adjustments.hue, _("hue shift"));
  gtk_box_pack_start(GTK_BOX(g->basic_color_adjustments.box), GTK_WIDGET(g->basic_color_adjustments.hue), TRUE,
                     TRUE, 0);
  g_signal_connect(G_OBJECT(g->basic_color_adjustments.hue), "value-changed",
                   G_CALLBACK(dt_iop_gmic_basic_color_adjustments_params_t::hue_callback), self);

  g->basic_color_adjustments.saturation
      = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->basic_color_adjustments.parameters.saturation, 3);
  dt_bauhaus_widget_set_label(g->basic_color_adjustments.saturation, NULL, _("saturation"));
  gtk_widget_set_tooltip_text(g->basic_color_adjustments.saturation, _("saturation adjustment"));
  gtk_box_pack_start(GTK_BOX(g->basic_color_adjustments.box), GTK_WIDGET(g->basic_color_adjustments.saturation),
                     TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->basic_color_adjustments.saturation), "value-changed",
                   G_CALLBACK(dt_iop_gmic_basic_color_adjustments_params_t::saturation_callback), self);

  gtk_widget_show_all(g->basic_color_adjustments.box);
  gtk_widget_set_no_show_all(g->basic_color_adjustments.box, TRUE);
  gtk_widget_set_visible(g->basic_color_adjustments.box, p->filter == basic_color_adjustments ? TRUE : FALSE);
}

void dt_iop_gmic_basic_color_adjustments_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->basic_color_adjustments.box, p->filter == basic_color_adjustments ? TRUE : FALSE);
  if(p->filter == basic_color_adjustments)
  {
    g->basic_color_adjustments.parameters = *p;
    dt_bauhaus_slider_set(g->basic_color_adjustments.brightness, g->basic_color_adjustments.parameters.brightness);
    dt_bauhaus_slider_set(g->basic_color_adjustments.contrast, g->basic_color_adjustments.parameters.contrast);
    dt_bauhaus_slider_set(g->basic_color_adjustments.gamma, g->basic_color_adjustments.parameters.gamma);
    dt_bauhaus_slider_set(g->basic_color_adjustments.hue, g->basic_color_adjustments.parameters.hue);
    dt_bauhaus_slider_set(g->basic_color_adjustments.saturation, g->basic_color_adjustments.parameters.saturation);
  }
}

void dt_iop_gmic_basic_color_adjustments_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_basic_color_adjustments_params_t();
}

void dt_iop_gmic_basic_color_adjustments_params_t::brightness_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->basic_color_adjustments.parameters.brightness = dt_bauhaus_slider_get(W);
    return G->basic_color_adjustments.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_basic_color_adjustments_params_t::contrast_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->basic_color_adjustments.parameters.contrast = dt_bauhaus_slider_get(W);
    return G->basic_color_adjustments.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_basic_color_adjustments_params_t::gamma_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->basic_color_adjustments.parameters.gamma = dt_bauhaus_slider_get(W);
    return G->basic_color_adjustments.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_basic_color_adjustments_params_t::hue_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->basic_color_adjustments.parameters.hue = dt_bauhaus_slider_get(W);
    return G->basic_color_adjustments.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_basic_color_adjustments_params_t::saturation_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->basic_color_adjustments.parameters.saturation = dt_bauhaus_slider_get(W);
    return G->basic_color_adjustments.parameters.to_gmic_params();
  });
}

// --- equalize shadow

dt_iop_gmic_equalize_shadow_params_t::dt_iop_gmic_equalize_shadow_params_t(const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_equalize_shadow_params_t()
{
  dt_iop_gmic_equalize_shadow_params_t p;
  if(other.filter == equalize_shadow and std::sscanf(other.parameters, "dt_equalize_shadow %g", &p.amplitude) == 1)
  {
    p.amplitude = clamp(0.f, 1.f, p.amplitude);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_equalize_shadow_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = equalize_shadow;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_equalize_shadow %g", amplitude);
  return ret;
}

const char *dt_iop_gmic_equalize_shadow_params_t::get_custom_command()
{
  // clang-format off
  return R"raw(
dt_equalize_shadow :
  +negate blend softlight,$1
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_equalize_shadow_params_t::get_filter() const
{
  return equalize_shadow;
}

void dt_iop_gmic_equalize_shadow_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == equalize_shadow)
    g->equalize_shadow.parameters = *p;
  else
    g->equalize_shadow.parameters = dt_iop_gmic_equalize_shadow_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("equalize shadow"));
  g->equalize_shadow.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->equalize_shadow.box, TRUE, TRUE, 0);

  g->equalize_shadow.amplitude
      = dt_bauhaus_slider_new_with_range(self, 0, 1, 0.01, g->equalize_shadow.parameters.amplitude, 3);
  dt_bauhaus_widget_set_label(g->equalize_shadow.amplitude, NULL, _("amplitude"));
  gtk_widget_set_tooltip_text(g->equalize_shadow.amplitude, _("amount of shadow equalization"));
  gtk_box_pack_start(GTK_BOX(g->equalize_shadow.box), GTK_WIDGET(g->equalize_shadow.amplitude), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->equalize_shadow.amplitude), "value-changed",
                   G_CALLBACK(dt_iop_gmic_equalize_shadow_params_t::amplitude_callback), self);

  gtk_widget_show_all(g->equalize_shadow.box);
  gtk_widget_set_no_show_all(g->equalize_shadow.box, TRUE);
  gtk_widget_set_visible(g->equalize_shadow.box, p->filter == equalize_shadow ? TRUE : FALSE);
}

void dt_iop_gmic_equalize_shadow_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->equalize_shadow.box, p->filter == equalize_shadow ? TRUE : FALSE);
  if(p->filter == equalize_shadow)
  {
    g->equalize_shadow.parameters = *p;
    dt_bauhaus_slider_set(g->equalize_shadow.amplitude, g->equalize_shadow.parameters.amplitude);
  }
}

void dt_iop_gmic_equalize_shadow_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_equalize_shadow_params_t();
}

void dt_iop_gmic_equalize_shadow_params_t::amplitude_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->equalize_shadow.parameters.amplitude = dt_bauhaus_slider_get(W);
    return G->equalize_shadow.parameters.to_gmic_params();
  });
}

// --- add grain

dt_iop_gmic_add_grain_params_t::dt_iop_gmic_add_grain_params_t(const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_add_grain_params_t()
{
  dt_iop_gmic_add_grain_params_t p;
  if(other.filter == add_grain
     and std::sscanf(other.parameters, "dt_add_grain %d,%d,%g,%g,%d,%g,%g,%g,%g,%g", &p.preset, &p.blend_mode,
                     &p.opacity, &p.scale, &p.color_grain, &p.brightness, &p.contrast, &p.gamma, &p.hue,
                     &p.saturation)
             == 10)
  {
    p.preset = clamp(0, 4, p.preset);
    p.blend_mode = clamp(0, 5, p.blend_mode);
    p.opacity = clamp(0.f, 1.f, p.opacity);
    p.scale = clamp(30.f, 100.f, p.scale);
    p.color_grain = clamp(0, 1, p.color_grain);
    p.brightness = clamp(-1.f, 1.f, p.brightness);
    p.contrast = clamp(-1.f, 1.f, p.contrast);
    p.gamma = clamp(-1.f, 1.f, p.gamma);
    p.hue = clamp(-1.f, 1.f, p.hue);
    p.saturation = clamp(-1.f, 1.f, p.saturation);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_add_grain_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = add_grain;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_add_grain %d,%d,%g,%g,%d,%g,%g,%g,%g,%g", preset,
                blend_mode, opacity, scale, color_grain, brightness, contrast, gamma, hue, saturation);
  return ret;
}

const char *dt_iop_gmic_add_grain_params_t::get_custom_command()
{
  // clang-format off
  return R"raw(
dt_add_grain :
  fx_emulate_grain $1,$2,$3,$4,$5,{100*$6},{100*$7},{100*$8},{100*$9},{100*$10}
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_add_grain_params_t::get_filter() const
{
  return add_grain;
}

void dt_iop_gmic_add_grain_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == add_grain)
    g->add_grain.parameters = *p;
  else
    g->add_grain.parameters = dt_iop_gmic_add_grain_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("add film grain"));
  g->add_grain.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->add_grain.box, TRUE, TRUE, 0);

  g->add_grain.preset = dt_bauhaus_combobox_new(self);
  for(auto i : { _("ORWO NP20"), _("Kodak TMAX 400"), _("Kodak TMAX 3200"), _("Kodak TRI-X 1600"), _("Unknown") })
    dt_bauhaus_combobox_add(g->add_grain.preset, i);
  dt_bauhaus_widget_set_label(g->add_grain.preset, NULL, _("grain preset"));
  gtk_widget_set_tooltip_text(g->add_grain.preset, _("emulate grain of given film type"));
  gtk_box_pack_start(GTK_BOX(g->add_grain.box), g->add_grain.preset, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->add_grain.preset), "value-changed",
                   G_CALLBACK(dt_iop_gmic_add_grain_params_t::preset_callback), self);

  g->add_grain.blend_mode = dt_bauhaus_combobox_new(self);
  for(auto i : { _("alpha"), _("grain merge"), _("hard light"), _("overlay"), _("soft light"), _("grain only") })
    dt_bauhaus_combobox_add(g->add_grain.blend_mode, i);
  dt_bauhaus_widget_set_label(g->add_grain.blend_mode, NULL, _("blend mode"));
  gtk_widget_set_tooltip_text(g->add_grain.blend_mode, _("how to blend grain into picture"));
  gtk_box_pack_start(GTK_BOX(g->add_grain.box), g->add_grain.blend_mode, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->add_grain.blend_mode), "value-changed",
                   G_CALLBACK(dt_iop_gmic_add_grain_params_t::blend_mode_callback), self);

  g->add_grain.opacity = dt_bauhaus_slider_new_with_range(self, 0, 1, 0.01, g->add_grain.parameters.opacity, 3);
  dt_bauhaus_widget_set_label(g->add_grain.opacity, NULL, _("opacity"));
  gtk_widget_set_tooltip_text(g->add_grain.opacity, _("grain opacity"));
  gtk_box_pack_start(GTK_BOX(g->add_grain.box), GTK_WIDGET(g->add_grain.opacity), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->add_grain.opacity), "value-changed",
                   G_CALLBACK(dt_iop_gmic_add_grain_params_t::opacity_callback), self);

  g->add_grain.scale = dt_bauhaus_slider_new_with_range(self, 30, 200, 1, g->add_grain.parameters.scale, 1);
  dt_bauhaus_widget_set_label(g->add_grain.scale, NULL, _("scale"));
  gtk_widget_set_tooltip_text(g->add_grain.scale, _("grain scale"));
  gtk_box_pack_start(GTK_BOX(g->add_grain.box), GTK_WIDGET(g->add_grain.scale), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->add_grain.scale), "value-changed",
                   G_CALLBACK(dt_iop_gmic_add_grain_params_t::scale_callback), self);

  g->add_grain.color_grain = dt_bauhaus_combobox_new(self);
  for(auto i : { _("monochrome grain"), _("color grain") }) dt_bauhaus_combobox_add(g->add_grain.color_grain, i);
  dt_bauhaus_widget_set_label(g->add_grain.color_grain, NULL, _("grain type"));
  gtk_widget_set_tooltip_text(g->add_grain.color_grain, _("select monochromatic or color grain"));
  gtk_box_pack_start(GTK_BOX(g->add_grain.box), g->add_grain.color_grain, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->add_grain.color_grain), "value-changed",
                   G_CALLBACK(dt_iop_gmic_add_grain_params_t::color_grain_callback), self);

  g->add_grain.brightness
      = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->add_grain.parameters.brightness, 3);
  dt_bauhaus_widget_set_label(g->add_grain.brightness, NULL, _("brightness"));
  gtk_widget_set_tooltip_text(g->add_grain.brightness, _("brightness adjustment"));
  gtk_box_pack_start(GTK_BOX(g->add_grain.box), GTK_WIDGET(g->add_grain.brightness), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->add_grain.brightness), "value-changed",
                   G_CALLBACK(dt_iop_gmic_add_grain_params_t::brightness_callback), self);

  g->add_grain.contrast = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->add_grain.parameters.contrast, 3);
  dt_bauhaus_widget_set_label(g->add_grain.contrast, NULL, _("contrast"));
  gtk_widget_set_tooltip_text(g->add_grain.contrast, _("contrast adjustment"));
  gtk_box_pack_start(GTK_BOX(g->add_grain.box), GTK_WIDGET(g->add_grain.contrast), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->add_grain.contrast), "value-changed",
                   G_CALLBACK(dt_iop_gmic_add_grain_params_t::contrast_callback), self);

  g->add_grain.gamma = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->add_grain.parameters.gamma, 3);
  dt_bauhaus_widget_set_label(g->add_grain.gamma, NULL, _("gamma"));
  gtk_widget_set_tooltip_text(g->add_grain.gamma, _("gamma adjustment"));
  gtk_box_pack_start(GTK_BOX(g->add_grain.box), GTK_WIDGET(g->add_grain.gamma), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->add_grain.gamma), "value-changed",
                   G_CALLBACK(dt_iop_gmic_add_grain_params_t::gamma_callback), self);

  g->add_grain.hue = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->add_grain.parameters.hue, 3);
  dt_bauhaus_widget_set_label(g->add_grain.hue, NULL, _("hue"));
  gtk_widget_set_tooltip_text(g->add_grain.hue, _("hue shift"));
  gtk_box_pack_start(GTK_BOX(g->add_grain.box), GTK_WIDGET(g->add_grain.hue), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->add_grain.hue), "value-changed",
                   G_CALLBACK(dt_iop_gmic_add_grain_params_t::hue_callback), self);

  g->add_grain.saturation
      = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, g->add_grain.parameters.saturation, 3);
  dt_bauhaus_widget_set_label(g->add_grain.saturation, NULL, _("saturation"));
  gtk_widget_set_tooltip_text(g->add_grain.saturation, _("saturation adjustment"));
  gtk_box_pack_start(GTK_BOX(g->add_grain.box), GTK_WIDGET(g->add_grain.saturation), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->add_grain.saturation), "value-changed",
                   G_CALLBACK(dt_iop_gmic_add_grain_params_t::saturation_callback), self);

  gtk_widget_show_all(g->add_grain.box);
  gtk_widget_set_no_show_all(g->add_grain.box, TRUE);
  gtk_widget_set_visible(g->add_grain.box, p->filter == add_grain ? TRUE : FALSE);
}

void dt_iop_gmic_add_grain_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->add_grain.box, p->filter == add_grain ? TRUE : FALSE);
  if(p->filter == add_grain)
  {
    g->add_grain.parameters = *p;
    dt_bauhaus_combobox_set(g->add_grain.preset, g->add_grain.parameters.preset);
    dt_bauhaus_combobox_set(g->add_grain.blend_mode, g->add_grain.parameters.blend_mode);
    dt_bauhaus_slider_set(g->add_grain.opacity, g->add_grain.parameters.opacity);
    dt_bauhaus_slider_set(g->add_grain.scale, g->add_grain.parameters.scale);
    dt_bauhaus_combobox_set(g->add_grain.color_grain, g->add_grain.parameters.color_grain);
    dt_bauhaus_slider_set(g->add_grain.brightness, g->add_grain.parameters.brightness);
    dt_bauhaus_slider_set(g->add_grain.contrast, g->add_grain.parameters.contrast);
    dt_bauhaus_slider_set(g->add_grain.gamma, g->add_grain.parameters.gamma);
    dt_bauhaus_slider_set(g->add_grain.hue, g->add_grain.parameters.hue);
    dt_bauhaus_slider_set(g->add_grain.saturation, g->add_grain.parameters.saturation);
  }
}

void dt_iop_gmic_add_grain_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_add_grain_params_t();
}

void dt_iop_gmic_add_grain_params_t::preset_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->add_grain.parameters.preset = dt_bauhaus_combobox_get(W);
    return G->add_grain.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_add_grain_params_t::blend_mode_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->add_grain.parameters.blend_mode = dt_bauhaus_combobox_get(W);
    return G->add_grain.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_add_grain_params_t::opacity_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->add_grain.parameters.opacity = dt_bauhaus_slider_get(W);
    return G->add_grain.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_add_grain_params_t::scale_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->add_grain.parameters.scale = dt_bauhaus_slider_get(W);
    return G->add_grain.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_add_grain_params_t::color_grain_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->add_grain.parameters.color_grain = dt_bauhaus_combobox_get(W);
    return G->add_grain.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_add_grain_params_t::brightness_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->add_grain.parameters.brightness = dt_bauhaus_slider_get(W);
    return G->add_grain.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_add_grain_params_t::contrast_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->add_grain.parameters.contrast = dt_bauhaus_slider_get(W);
    return G->add_grain.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_add_grain_params_t::gamma_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->add_grain.parameters.gamma = dt_bauhaus_slider_get(W);
    return G->add_grain.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_add_grain_params_t::hue_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->add_grain.parameters.hue = dt_bauhaus_slider_get(W);
    return G->add_grain.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_add_grain_params_t::saturation_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->add_grain.parameters.saturation = dt_bauhaus_slider_get(W);
    return G->add_grain.parameters.to_gmic_params();
  });
}

// --- pop shadows

dt_iop_gmic_pop_shadows_params_t::dt_iop_gmic_pop_shadows_params_t(const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_pop_shadows_params_t()
{
  dt_iop_gmic_pop_shadows_params_t p;
  if(other.filter == pop_shadows
     and std::sscanf(other.parameters, "dt_pop_shadows %g,%g", &p.strength, &p.scale) == 2)
  {
    p.strength = clamp(0.f, 1.f, p.strength);
    p.scale = clamp(0.f, 20.f, p.scale);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_pop_shadows_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = pop_shadows;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_pop_shadows %g,%g", strength, scale);
  return ret;
}

const char *dt_iop_gmic_pop_shadows_params_t::get_custom_command()
{
  // clang-format off
  return R"raw(
dt_pop_shadows :
  split_opacity local[0]
    .x2
    luminance.. negate.. imM={-2,[im,iM]} blur.. $2% normalize.. $imM
    blend[0,1] overlay,$1
    max
  endlocal
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_pop_shadows_params_t::get_filter() const
{
  return pop_shadows;
}

void dt_iop_gmic_pop_shadows_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == pop_shadows)
    g->pop_shadows.parameters = *p;
  else
    g->pop_shadows.parameters = dt_iop_gmic_pop_shadows_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("pop shadows"));
  g->pop_shadows.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->pop_shadows.box, TRUE, TRUE, 0);

  g->pop_shadows.strength
      = dt_bauhaus_slider_new_with_range(self, 0, 1, 0.01, g->pop_shadows.parameters.strength, 3);
  dt_bauhaus_widget_set_label(g->pop_shadows.strength, NULL, _("strength"));
  gtk_widget_set_tooltip_text(g->pop_shadows.strength, _("strength of shadow brightening"));
  gtk_box_pack_start(GTK_BOX(g->pop_shadows.box), GTK_WIDGET(g->pop_shadows.strength), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->pop_shadows.strength), "value-changed",
                   G_CALLBACK(dt_iop_gmic_pop_shadows_params_t::strength_callback), self);

  g->pop_shadows.scale = dt_bauhaus_slider_new_with_range(self, 0, 20, 0.01, g->pop_shadows.parameters.scale, 3);
  dt_bauhaus_widget_set_label(g->pop_shadows.scale, NULL, _("scale"));
  gtk_widget_set_tooltip_text(g->pop_shadows.scale, _("scale of blur used in shadow brightening"));
  gtk_box_pack_start(GTK_BOX(g->pop_shadows.box), GTK_WIDGET(g->pop_shadows.scale), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->pop_shadows.scale), "value-changed",
                   G_CALLBACK(dt_iop_gmic_pop_shadows_params_t::scale_callback), self);

  gtk_widget_show_all(g->pop_shadows.box);
  gtk_widget_set_no_show_all(g->pop_shadows.box, TRUE);
  gtk_widget_set_visible(g->pop_shadows.box, p->filter == pop_shadows ? TRUE : FALSE);
}

void dt_iop_gmic_pop_shadows_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->pop_shadows.box, p->filter == pop_shadows ? TRUE : FALSE);
  if(p->filter == pop_shadows)
  {
    g->pop_shadows.parameters = *p;
    dt_bauhaus_slider_set(g->pop_shadows.strength, g->pop_shadows.parameters.strength);
    dt_bauhaus_slider_set(g->pop_shadows.scale, g->pop_shadows.parameters.scale);
  }
}

void dt_iop_gmic_pop_shadows_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_pop_shadows_params_t();
}

void dt_iop_gmic_pop_shadows_params_t::strength_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->pop_shadows.parameters.strength = dt_bauhaus_slider_get(W);
    return G->pop_shadows.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_pop_shadows_params_t::scale_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->pop_shadows.parameters.scale = dt_bauhaus_slider_get(W);
    return G->pop_shadows.parameters.to_gmic_params();
  });
}

// --- smooth bilateral

dt_iop_gmic_smooth_bilateral_params_t::dt_iop_gmic_smooth_bilateral_params_t(const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_smooth_bilateral_params_t()
{
  dt_iop_gmic_smooth_bilateral_params_t p;
  if(other.filter == smooth_bilateral
     and std::sscanf(other.parameters, "dt_smooth_bilateral %g,%g,%d,%d", &p.spatial_scale, &p.value_scale,
                     &p.iterations, &p.channel)
             == 4)
  {
    p.spatial_scale = clamp(0.f, 100.f, p.spatial_scale);
    p.value_scale = clamp(0.f, 1.f, p.value_scale);
    p.iterations = clamp(1, 10, p.iterations);
    p.channel = clamp(0, static_cast<int>(color_channels.size() - 1), p.channel);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_smooth_bilateral_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = smooth_bilateral;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_smooth_bilateral %g,%g,%d,%d", spatial_scale,
                value_scale, iterations, channel);
  return ret;
}

const char *dt_iop_gmic_smooth_bilateral_params_t::get_custom_command() {
  // clang-format off
  return R"raw(
dt_smooth_bilateral :
  apply_channels "repeat $3 bilateral $1,{255*$2} done",$4
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_smooth_bilateral_params_t::get_filter() const
{
  return smooth_bilateral;
}

void dt_iop_gmic_smooth_bilateral_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == smooth_bilateral)
    g->smooth_bilateral.parameters = *p;
  else
    g->smooth_bilateral.parameters = dt_iop_gmic_smooth_bilateral_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("smooth bilateral"));
  g->smooth_bilateral.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->smooth_bilateral.box, TRUE, TRUE, 0);

  g->smooth_bilateral.spatial_scale
      = dt_bauhaus_slider_new_with_range(self, 0, 100, 0.5, g->smooth_bilateral.parameters.spatial_scale, 2);
  dt_bauhaus_widget_set_label(g->smooth_bilateral.spatial_scale, NULL, _("spatial scale"));
  gtk_widget_set_tooltip_text(g->smooth_bilateral.spatial_scale,
                              _("spatial standard deviation of Gaussian kernel"));
  gtk_box_pack_start(GTK_BOX(g->smooth_bilateral.box), GTK_WIDGET(g->smooth_bilateral.spatial_scale), TRUE, TRUE,
                     0);
  g_signal_connect(G_OBJECT(g->smooth_bilateral.spatial_scale), "value-changed",
                   G_CALLBACK(dt_iop_gmic_smooth_bilateral_params_t::spatial_scale_callback), self);

  g->smooth_bilateral.value_scale
      = dt_bauhaus_slider_new_with_range(self, 0, 1, 0.01, g->smooth_bilateral.parameters.spatial_scale, 3);
  dt_bauhaus_widget_set_label(g->smooth_bilateral.value_scale, NULL, _("value scale"));
  gtk_widget_set_tooltip_text(g->smooth_bilateral.value_scale,
                              _("color/luminance standard deviation of Gaussian kernel"));
  gtk_box_pack_start(GTK_BOX(g->smooth_bilateral.box), GTK_WIDGET(g->smooth_bilateral.value_scale), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->smooth_bilateral.value_scale), "value-changed",
                   G_CALLBACK(dt_iop_gmic_smooth_bilateral_params_t::value_scale_callback), self);

  g->smooth_bilateral.iterations
      = dt_bauhaus_slider_new_with_range(self, 1, 10, 1, g->smooth_bilateral.parameters.iterations, 0);
  dt_bauhaus_widget_set_label(g->smooth_bilateral.iterations, NULL, _("iterations"));
  gtk_widget_set_tooltip_text(g->smooth_bilateral.iterations, _("number of iterations"));
  gtk_box_pack_start(GTK_BOX(g->smooth_bilateral.box), GTK_WIDGET(g->smooth_bilateral.iterations), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->smooth_bilateral.iterations), "value-changed",
                   G_CALLBACK(dt_iop_gmic_smooth_bilateral_params_t::iterations_callback), self);

  g->smooth_bilateral.channel = dt_bauhaus_combobox_new(self);
  for(auto str : color_channels) dt_bauhaus_combobox_add(g->smooth_bilateral.channel, str);
  dt_bauhaus_widget_set_label(g->smooth_bilateral.channel, NULL, _("channel"));
  gtk_widget_set_tooltip_text(g->smooth_bilateral.channel, _("apply filter to specific color channel(s)"));
  gtk_box_pack_start(GTK_BOX(g->smooth_bilateral.box), g->smooth_bilateral.channel, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->smooth_bilateral.channel), "value-changed",
                   G_CALLBACK(dt_iop_gmic_smooth_bilateral_params_t::channel_callback), self);

  gtk_widget_show_all(g->smooth_bilateral.box);
  gtk_widget_set_no_show_all(g->smooth_bilateral.box, TRUE);
  gtk_widget_set_visible(g->smooth_bilateral.box, p->filter == smooth_bilateral ? TRUE : FALSE);
}

void dt_iop_gmic_smooth_bilateral_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->smooth_bilateral.box, p->filter == smooth_bilateral ? TRUE : FALSE);
  if(p->filter == smooth_bilateral)
  {
    g->smooth_bilateral.parameters = *p;
    dt_bauhaus_slider_set(g->smooth_bilateral.spatial_scale, g->smooth_bilateral.parameters.spatial_scale);
    dt_bauhaus_slider_set(g->smooth_bilateral.value_scale, g->smooth_bilateral.parameters.value_scale);
    dt_bauhaus_slider_set(g->smooth_bilateral.iterations, g->smooth_bilateral.parameters.iterations);
    dt_bauhaus_combobox_set(g->smooth_bilateral.channel, g->smooth_bilateral.parameters.channel);
  }
}

void dt_iop_gmic_smooth_bilateral_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_smooth_bilateral_params_t();
}

void dt_iop_gmic_smooth_bilateral_params_t::spatial_scale_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->smooth_bilateral.parameters.spatial_scale = dt_bauhaus_slider_get(W);
    return G->smooth_bilateral.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_smooth_bilateral_params_t::value_scale_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->smooth_bilateral.parameters.value_scale = dt_bauhaus_slider_get(W);
    return G->smooth_bilateral.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_smooth_bilateral_params_t::iterations_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->smooth_bilateral.parameters.iterations = static_cast<int>(std::round(dt_bauhaus_slider_get(W)));
    return G->smooth_bilateral.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_smooth_bilateral_params_t::channel_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->smooth_bilateral.parameters.channel = dt_bauhaus_combobox_get(W);
    return G->smooth_bilateral.parameters.to_gmic_params();
  });
}

// --- smooth guided

dt_iop_gmic_smooth_guided_params_t::dt_iop_gmic_smooth_guided_params_t(const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_smooth_guided_params_t()
{
  dt_iop_gmic_smooth_guided_params_t p;
  if(other.filter == smooth_guided
     and std::sscanf(other.parameters, "dt_smooth_guided %g,%g,%d,%d", &p.radius, &p.smoothness, &p.iterations,
                     &p.channel)
             == 4)
  {
    p.radius = clamp(0.f, 100.f, p.radius);
    p.smoothness = clamp(0.f, 1.f, p.smoothness);
    p.iterations = clamp(1, 10, p.iterations);
    p.channel = clamp(0, static_cast<int>(color_channels.size() - 1), p.channel);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_smooth_guided_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = smooth_guided;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_smooth_guided %g,%g,%d,%d", radius, smoothness,
                iterations, channel);
  return ret;
}

const char *dt_iop_gmic_smooth_guided_params_t::get_custom_command() {
  // clang-format off
  return R"raw(
dt_smooth_guided :
  apply_channels "repeat $3 guided $1,{512*$2} done",$4
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_smooth_guided_params_t::get_filter() const
{
  return smooth_guided;
}

void dt_iop_gmic_smooth_guided_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == smooth_guided)
    g->smooth_guided.parameters = *p;
  else
    g->smooth_guided.parameters = dt_iop_gmic_smooth_guided_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("smooth guided"));
  g->smooth_guided.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->smooth_guided.box, TRUE, TRUE, 0);

  g->smooth_guided.radius
      = dt_bauhaus_slider_new_with_range(self, 0, 100, 0.5, g->smooth_guided.parameters.radius, 2);
  dt_bauhaus_widget_set_label(g->smooth_guided.radius, NULL, _("radius"));
  gtk_widget_set_tooltip_text(g->smooth_guided.radius, _("radius of the guided filer"));
  gtk_box_pack_start(GTK_BOX(g->smooth_guided.box), GTK_WIDGET(g->smooth_guided.radius), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->smooth_guided.radius), "value-changed",
                   G_CALLBACK(dt_iop_gmic_smooth_guided_params_t::radius_callback), self);

  g->smooth_guided.smoothness
      = dt_bauhaus_slider_new_with_range(self, 0, 1, 0.01, g->smooth_guided.parameters.smoothness, 3);
  dt_bauhaus_widget_set_label(g->smooth_guided.smoothness, NULL, _("smoothness"));
  gtk_widget_set_tooltip_text(g->smooth_guided.smoothness, _("smoothness of guided filter"));
  gtk_box_pack_start(GTK_BOX(g->smooth_guided.box), GTK_WIDGET(g->smooth_guided.smoothness), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->smooth_guided.smoothness), "value-changed",
                   G_CALLBACK(dt_iop_gmic_smooth_guided_params_t::smoothness_callback), self);

  g->smooth_guided.iterations
      = dt_bauhaus_slider_new_with_range(self, 1, 10, 1, g->smooth_guided.parameters.iterations, 0);
  dt_bauhaus_widget_set_label(g->smooth_guided.iterations, NULL, _("iterations"));
  gtk_widget_set_tooltip_text(g->smooth_guided.iterations, _("number of iterations"));
  gtk_box_pack_start(GTK_BOX(g->smooth_guided.box), GTK_WIDGET(g->smooth_guided.iterations), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->smooth_guided.iterations), "value-changed",
                   G_CALLBACK(dt_iop_gmic_smooth_guided_params_t::iterations_callback), self);

  g->smooth_guided.channel = dt_bauhaus_combobox_new(self);
  for(auto str : color_channels) dt_bauhaus_combobox_add(g->smooth_guided.channel, str);
  dt_bauhaus_widget_set_label(g->smooth_guided.channel, NULL, _("channel"));
  gtk_widget_set_tooltip_text(g->smooth_guided.channel, _("apply filter to specific color channel(s)"));
  gtk_box_pack_start(GTK_BOX(g->smooth_guided.box), g->smooth_guided.channel, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->smooth_guided.channel), "value-changed",
                   G_CALLBACK(dt_iop_gmic_smooth_guided_params_t::channel_callback), self);

  gtk_widget_show_all(g->smooth_guided.box);
  gtk_widget_set_no_show_all(g->smooth_guided.box, TRUE);
  gtk_widget_set_visible(g->smooth_guided.box, p->filter == smooth_guided ? TRUE : FALSE);
}

void dt_iop_gmic_smooth_guided_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->smooth_guided.box, p->filter == smooth_guided ? TRUE : FALSE);
  if(p->filter == smooth_guided)
  {
    g->smooth_guided.parameters = *p;
    dt_bauhaus_slider_set(g->smooth_guided.radius, g->smooth_guided.parameters.radius);
    dt_bauhaus_slider_set(g->smooth_guided.smoothness, g->smooth_guided.parameters.smoothness);
    dt_bauhaus_slider_set(g->smooth_guided.iterations, g->smooth_guided.parameters.iterations);
    dt_bauhaus_combobox_set(g->smooth_guided.channel, g->smooth_guided.parameters.channel);
  }
}

void dt_iop_gmic_smooth_guided_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_smooth_guided_params_t();
}

void dt_iop_gmic_smooth_guided_params_t::radius_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->smooth_guided.parameters.radius = dt_bauhaus_slider_get(W);
    return G->smooth_guided.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_smooth_guided_params_t::smoothness_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->smooth_guided.parameters.smoothness = dt_bauhaus_slider_get(W);
    return G->smooth_guided.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_smooth_guided_params_t::iterations_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->smooth_guided.parameters.iterations = static_cast<int>(std::round(dt_bauhaus_slider_get(W)));
    return G->smooth_guided.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_smooth_guided_params_t::channel_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->smooth_guided.parameters.channel = dt_bauhaus_combobox_get(W);
    return G->smooth_guided.parameters.to_gmic_params();
  });
}

// --- light glow

dt_iop_gmic_light_glow_params_t::dt_iop_gmic_light_glow_params_t(const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_light_glow_params_t()
{
  dt_iop_gmic_light_glow_params_t p;
  if(other.filter == light_glow
     and std::sscanf(other.parameters, "dt_light_glow %g,%g,%d,%g,%d", &p.density, &p.amplitude, &p.blend_mode,
                     &p.opacity, &p.channel)
             == 5)
  {
    p.density = clamp(0.f, 1.f, p.density);
    p.amplitude = clamp(0.f, 2.f, p.amplitude);
    p.blend_mode = clamp(0, 12, p.blend_mode);
    p.opacity = clamp(0.f, 1.f, p.opacity);
    p.channel = clamp(0, static_cast<int>(color_channels.size() - 1), p.channel);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_light_glow_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = light_glow;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_light_glow %g,%g,%d,%g,%d", density, amplitude,
                blend_mode, opacity, channel);
  return ret;
}

const char *dt_iop_gmic_light_glow_params_t::get_custom_command() {
  // clang-format off
  return R"raw(
_dt_light_glow :
  mode=${arg\ 1+$3,burn,dodge,freeze,grainmerge,hardlight,interpolation,lighten,multiply,overlay,reflect,softlight,stamp,value}
  repeat $!
    +gradient_norm. >=. {100-$1}% distance. 1 ^. $2 *. -1 n. 0,255 blend $mode,$4
  mv. 0 done

dt_light_glow :
  apply_channels "_dt_light_glow {100*$1},$2,$3,$4",$5
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_light_glow_params_t::get_filter() const
{
  return light_glow;
}

void dt_iop_gmic_light_glow_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == light_glow)
    g->light_glow.parameters = *p;
  else
    g->light_glow.parameters = dt_iop_gmic_light_glow_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("light glow"));
  g->light_glow.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->light_glow.box, TRUE, TRUE, 0);

  g->light_glow.density = dt_bauhaus_slider_new_with_range(self, 0, 1, 0.01, g->light_glow.parameters.density, 3);
  dt_bauhaus_widget_set_label(g->light_glow.density, NULL, _("density"));
  gtk_widget_set_tooltip_text(g->light_glow.density, _("density of the light glow filter"));
  gtk_box_pack_start(GTK_BOX(g->light_glow.box), GTK_WIDGET(g->light_glow.density), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->light_glow.density), "value-changed",
                   G_CALLBACK(dt_iop_gmic_light_glow_params_t::density_callback), self);

  g->light_glow.amplitude
      = dt_bauhaus_slider_new_with_range(self, 0, 2, 0.02, g->light_glow.parameters.amplitude, 3);
  dt_bauhaus_widget_set_label(g->light_glow.amplitude, NULL, _("amplitude"));
  gtk_widget_set_tooltip_text(g->light_glow.amplitude, _("amplitude of the light glow filter"));
  gtk_box_pack_start(GTK_BOX(g->light_glow.box), GTK_WIDGET(g->light_glow.amplitude), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->light_glow.amplitude), "value-changed",
                   G_CALLBACK(dt_iop_gmic_light_glow_params_t::amplitude_callback), self);

  g->light_glow.blend_mode = dt_bauhaus_combobox_new(self);
  for(auto i : { _("burn"), _("dodge"), _("freeze"), _("grain merge"), _("hard light"), _("interpolation"),
                 _("lighten"), _("multiply"), _("overlay"), _("reflect"), _("soft light"), _("stamp"), _("Value") })
    dt_bauhaus_combobox_add(g->light_glow.blend_mode, i);
  dt_bauhaus_widget_set_label(g->light_glow.blend_mode, NULL, _("blend mode"));
  gtk_widget_set_tooltip_text(g->light_glow.blend_mode, _("blend mode of light glow filter"));
  gtk_box_pack_start(GTK_BOX(g->light_glow.box), g->light_glow.blend_mode, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->light_glow.blend_mode), "value-changed",
                   G_CALLBACK(dt_iop_gmic_light_glow_params_t::blend_mode_callback), self);

  g->light_glow.opacity = dt_bauhaus_slider_new_with_range(self, 0, 1, 0.01, g->light_glow.parameters.opacity, 3);
  dt_bauhaus_widget_set_label(g->light_glow.opacity, NULL, _("opacity"));
  gtk_widget_set_tooltip_text(g->light_glow.opacity, _("opacity of the light glow filter"));
  gtk_box_pack_start(GTK_BOX(g->light_glow.box), GTK_WIDGET(g->light_glow.opacity), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->light_glow.opacity), "value-changed",
                   G_CALLBACK(dt_iop_gmic_light_glow_params_t::opacity_callback), self);

  g->light_glow.channel = dt_bauhaus_combobox_new(self);
  for(auto str : color_channels) dt_bauhaus_combobox_add(g->light_glow.channel, str);
  dt_bauhaus_widget_set_label(g->light_glow.channel, NULL, _("channel"));
  gtk_widget_set_tooltip_text(g->light_glow.channel, _("apply filter to specific color channel(s)"));
  gtk_box_pack_start(GTK_BOX(g->light_glow.box), g->light_glow.channel, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->light_glow.channel), "value-changed",
                   G_CALLBACK(dt_iop_gmic_light_glow_params_t::channel_callback), self);

  gtk_widget_show_all(g->light_glow.box);
  gtk_widget_set_no_show_all(g->light_glow.box, TRUE);
  gtk_widget_set_visible(g->light_glow.box, p->filter == light_glow ? TRUE : FALSE);
}

void dt_iop_gmic_light_glow_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->light_glow.box, p->filter == light_glow ? TRUE : FALSE);
  if(p->filter == light_glow)
  {
    g->light_glow.parameters = *p;
    dt_bauhaus_slider_set(g->light_glow.density, g->light_glow.parameters.density);
    dt_bauhaus_slider_set(g->light_glow.amplitude, g->light_glow.parameters.amplitude);
    dt_bauhaus_combobox_set(g->light_glow.blend_mode, g->light_glow.parameters.blend_mode);
    dt_bauhaus_slider_set(g->light_glow.opacity, g->light_glow.parameters.opacity);
    dt_bauhaus_combobox_set(g->light_glow.channel, g->light_glow.parameters.channel);
  }
}

void dt_iop_gmic_light_glow_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_light_glow_params_t();
}

void dt_iop_gmic_light_glow_params_t::density_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->light_glow.parameters.density = dt_bauhaus_slider_get(W);
    return G->light_glow.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_light_glow_params_t::amplitude_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->light_glow.parameters.amplitude = dt_bauhaus_slider_get(W);
    return G->light_glow.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_light_glow_params_t::blend_mode_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->light_glow.parameters.blend_mode = dt_bauhaus_combobox_get(W);
    return G->light_glow.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_light_glow_params_t::opacity_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->light_glow.parameters.opacity = dt_bauhaus_slider_get(W);
    return G->light_glow.parameters.to_gmic_params();
  });
}

void dt_iop_gmic_light_glow_params_t::channel_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->freaky_details.parameters.channel = dt_bauhaus_combobox_get(W);
    return G->freaky_details.parameters.to_gmic_params();
  });
}

// --- lomo

dt_iop_gmic_lomo_params_t::dt_iop_gmic_lomo_params_t(const dt_iop_gmic_params_t &other)
  : dt_iop_gmic_lomo_params_t()
{
  dt_iop_gmic_lomo_params_t p;
  if(other.filter == lomo and std::sscanf(other.parameters, "dt_lomo %g", &p.vignette_size) == 1)
  {
    p.vignette_size = clamp(0.f, 1.f, p.vignette_size);
    *this = p;
  }
}

dt_iop_gmic_params_t dt_iop_gmic_lomo_params_t::to_gmic_params() const
{
  dt_iop_gmic_params_t ret;
  ret.filter = lomo;
  std::snprintf(ret.parameters, sizeof(ret.parameters), "dt_lomo %g", vignette_size);
  return ret;
}

const char *dt_iop_gmic_lomo_params_t::get_custom_command() {
  // clang-format off
  return R"raw(
dt_lomo :
  remove_opacity repeat $! l[$>] to_rgb
    +gaussian {125-125*$1+25}%,{125-125*$1+25}% n. 0,1 *
    s c
    f[0] '255*atan((i-128)/128)'
    f[1] '255*tan((i-128)/128)'
    f[2] '255*atan((i-128)/255)'
    a c
    sharpen 1
    normalize 0,255
  endl done
)raw";
  // clang-format on
}

filter_type dt_iop_gmic_lomo_params_t::get_filter() const
{
  return lomo;
}

void dt_iop_gmic_lomo_params_t::gui_init(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  if(p->filter == lomo)
    g->lomo.parameters = *p;
  else
    g->lomo.parameters = dt_iop_gmic_lomo_params_t();
  dt_bauhaus_combobox_add(g->gmic_filter, _("lomo"));
  g->lomo.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->lomo.box, TRUE, TRUE, 0);

  g->lomo.vignette_size = dt_bauhaus_slider_new_with_range(self, 0, 1, 0.01, g->lomo.parameters.vignette_size, 3);
  dt_bauhaus_widget_set_label(g->lomo.vignette_size, NULL, _("vignette"));
  gtk_widget_set_tooltip_text(g->lomo.vignette_size, _("size of vignette"));
  gtk_box_pack_start(GTK_BOX(g->lomo.box), GTK_WIDGET(g->lomo.vignette_size), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->lomo.vignette_size), "value-changed",
                   G_CALLBACK(dt_iop_gmic_lomo_params_t::vignette_size_callback), self);

  gtk_widget_show_all(g->lomo.box);
  gtk_widget_set_no_show_all(g->lomo.box, TRUE);
  gtk_widget_set_visible(g->lomo.box, p->filter == lomo ? TRUE : FALSE);
}

void dt_iop_gmic_lomo_params_t::gui_update(dt_iop_module_t *self) const
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->lomo.box, p->filter == lomo ? TRUE : FALSE);
  if(p->filter == lomo)
  {
    g->lomo.parameters = *p;
    dt_bauhaus_slider_set(g->lomo.vignette_size, g->lomo.parameters.vignette_size);
  }
}

void dt_iop_gmic_lomo_params_t::gui_reset(dt_iop_module_t *self)
{
  *this = dt_iop_gmic_lomo_params_t();
}

void dt_iop_gmic_lomo_params_t::vignette_size_callback(GtkWidget *w, dt_iop_module_t *self)
{
  callback(w, self, [](dt_iop_gmic_gui_data_t *G, GtkWidget *W) {
    G->lomo.parameters.vignette_size = dt_bauhaus_slider_get(W);
    return G->lomo.parameters.to_gmic_params();
  });
}

// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth = 2 expandtab tabstop = 2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
