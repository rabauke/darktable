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

struct dt_iop_gmic_none_gui_data_t
{
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

struct dt_iop_gmic_expert_mode_gui_data_t
{
  GtkWidget *box;
  GtkWidget *command;
};

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

struct dt_iop_gmic_sepia_gui_data_t
{
  GtkWidget *box;
  GtkWidget *brightness, *contrast, *gamma;
};

// --- film emulation

struct film_map
{
  const std::string film_type;
  const std::string printable;

  film_map(const char *film_type_, const char *printable_) : film_type(film_type_), printable(printable_)
  {
  }
};

struct dt_iop_gmic_film_emulation_params_t
{
  char film[128];
  float strength, brightness, contrast, gamma, hue, saturation;
  int normalize_colors;
  static const std::vector<film_map> film_maps;

  dt_iop_gmic_film_emulation_params_t()
    : film("")
    , strength(1.f)
    , brightness(0.f)
    , contrast(0.f)
    , gamma(0.f)
    , hue(0.f)
    , saturation(0.f)
    , normalize_colors(0)
  {
    std::snprintf(film, sizeof(film), "%s", film_maps[0].film_type.c_str());
  }

  std::string gmic_command() const
  {
    std::stringstream str;
    str << "_fx_emulate_film 1,"
        << "\"" << film << "\"," << 100 * strength << ',' << 100 * brightness << ',' << 100 * contrast << ','
        << 100 * gamma << ',' << 100 * hue << ',' << 100 * saturation << ',' << normalize_colors;
    return str.str();
  }

  static void gui_update(struct dt_iop_module_t *self);

  static void gui_init(dt_iop_module_t *self);

  static void film_callback(GtkWidget *w, dt_iop_module_t *self);

  static void strength_callback(GtkWidget *w, dt_iop_module_t *self);

  static void brightness_callback(GtkWidget *w, dt_iop_module_t *self);

  static void contrast_callback(GtkWidget *w, dt_iop_module_t *self);

  static void gamma_callback(GtkWidget *w, dt_iop_module_t *self);

  static void hue_callback(GtkWidget *w, dt_iop_module_t *self);

  static void saturation_callback(GtkWidget *w, dt_iop_module_t *self);

  static void normalize_colors_callback(GtkWidget *w, dt_iop_module_t *self);
};

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
  { "fuji_Superia_200", _("Fuji Superia 200") },
  { "fuji_Superia_hg_1600", _("Fuji Superia hg 1600") },
  { "fuji_Superia_reala_100", _("Fuji Superia Reala 100") },
  { "fuji_Superia_x-tra_800", _("Fuji Superia X-Tra 800") },
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
  { "fuji_Superia_100_-", _("Fuji Superia 100 -") },
  { "fuji_Superia_100", _("Fuji Superia 100") },
  { "fuji_Superia_100_+", _("Fuji Superia 100 +") },
  { "fuji_Superia_100_++", _("Fuji Superia 100 ++") },
  { "fuji_Superia_400_-", _("Fuji Superia 400 -") },
  { "fuji_Superia_400", _("Fuji Superia 400") },
  { "fuji_Superia_400_+", _("Fuji Superia 400 +") },
  { "fuji_Superia_400_++", _("Fuji Superia 400 ++") },
  { "fuji_Superia_800_-", _("Fuji Superia 800 -") },
  { "fuji_Superia_800", _("Fuji Superia 800") },
  { "fuji_Superia_800_+", _("Fuji Superia 800 +") },
  { "fuji_Superia_800_++", _("Fuji Superia 800 ++") },
  { "fuji_Superia_1600_-", _("Fuji Superia 1600 -") },
  { "fuji_Superia_1600", _("Fuji Superia 1600") },
  { "fuji_Superia_1600_+", _("Fuji Superia 1600 +") },
  { "fuji_Superia_1600_++", _("Fuji Superia 1600 ++") },
  { "kodak_portra_160_nc_-", _("Kodak Portra 160 NC -") },
  { "kodak_portra_160_nc", _("Kodak Portra 160 NC") },
  { "kodak_portra_160_nc_+", _("Kodak Portra 160 NC +") },
  { "kodak_portra_160_nc_++", _("Kodak Portra 160 NC ++") },
  { "kodak_portra_160_uc_-", _("Kodak Portra 160 UC -") },
  { "kodak_portra_160_uc", _("Kodak Portra 160 UC") },
  { "kodak_portra_160_uc_+", _("Kodak Portra 160 UC +") },
  { "kodak_portra_160_uc_++", _("Kodak Portra 160 UC ++") },
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
  { "fuji_provia_100f", _("Fuji Pprovia 100F") },
  { "fuji_provia_400f", _("Fuji Provia 400F") },
  { "fuji_provia_400x", _("Fuji Provia 400X") },
  { "fuji_sensia_100", _("Fuji Sensia 100") },
  { "fuji_Superia_200_xpro", _("Fuji Superia 200 XPRO") },
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

struct dt_iop_gmic_film_emulation_gui_data_t
{
  GtkWidget *box;
  GtkWidget *film, *strength, *brightness, *contrast, *gamma, *hue, *saturation, *normalize_colors;
  std::vector<std::string> film_list;
};

//----------------------------------------------------------------------
// implement the module api
//----------------------------------------------------------------------

enum filters
{
  none,
  expert_mode,
  sepia,
  film_emulation
};

struct dt_iop_gmic_all_params_t
{
  dt_iop_gmic_expert_mode_params_t expert_mode;
  dt_iop_gmic_sepia_params_t sepia;
  dt_iop_gmic_film_emulation_params_t film_emulation;
};

DT_MODULE_INTROSPECTION(1, dt_iop_gmic_params_t)

struct dt_iop_gmic_params_t
{
  filters filter;
  dt_iop_gmic_all_params_t parameters;
};

// types  dt_iop_gmic_params_t and dt_iop_gmic_data_t are
// equal, thus no commit_params function needs to be implemented
typedef dt_iop_gmic_params_t dt_iop_gmic_data_t;

struct dt_iop_gmic_gui_data_t
{
  GtkWidget *gmic_filter;
  std::vector<filters> filter_list;
  dt_iop_gmic_expert_mode_gui_data_t expert_mode;
  dt_iop_gmic_sepia_gui_data_t sepia;
  dt_iop_gmic_film_emulation_gui_data_t film_emulation;
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
  dt_iop_gmic_expert_mode_params_t::gui_update(self);
  dt_iop_gmic_sepia_params_t::gui_update(self);
  dt_iop_gmic_film_emulation_params_t::gui_update(self);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

// --- none

void dt_iop_gmic_none_params_t::gui_update(struct dt_iop_module_t *self)
{
}

void dt_iop_gmic_none_params_t::gui_init(dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_bauhaus_combobox_add(g->gmic_filter, _("none"));
  g->filter_list.push_back(none);
}

// --- expert mode

void dt_iop_gmic_expert_mode_params_t::gui_update(struct dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->expert_mode.box, p->filter == expert_mode ? TRUE : FALSE);
  gtk_entry_set_text(GTK_ENTRY(g->expert_mode.command), p->parameters.expert_mode.command);
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
  std::snprintf(p->parameters.expert_mode.command, sizeof(p->parameters.expert_mode.command), "%s",
                gtk_entry_get_text(GTK_ENTRY(w)));
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

// --- sepia

void dt_iop_gmic_sepia_params_t::gui_update(struct dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->sepia.box, p->filter == sepia ? TRUE : FALSE);
  dt_bauhaus_slider_set(g->sepia.brightness, p->parameters.sepia.brightness);
  dt_bauhaus_slider_set(g->sepia.contrast, p->parameters.sepia.contrast);
  dt_bauhaus_slider_set(g->sepia.gamma, p->parameters.sepia.gamma);
}

void dt_iop_gmic_sepia_params_t::gui_init(dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  dt_bauhaus_combobox_add(g->gmic_filter, _("sepia"));
  g->sepia.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->sepia.box, TRUE, TRUE, 0);

  g->sepia.brightness = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, p->parameters.sepia.brightness, 3);
  dt_bauhaus_widget_set_label(g->sepia.brightness, NULL, _("brightness"));
  gtk_widget_set_tooltip_text(g->sepia.brightness, _("brightness of the sepia film_type"));
  gtk_box_pack_start(GTK_BOX(g->sepia.box), GTK_WIDGET(g->sepia.brightness), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sepia.brightness), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sepia_params_t::brightness_callback), self);

  g->sepia.contrast = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, p->parameters.sepia.contrast, 3);
  dt_bauhaus_widget_set_label(g->sepia.contrast, NULL, _("contrast"));
  gtk_widget_set_tooltip_text(g->sepia.contrast, _("contrast of the sepia film_type"));
  gtk_box_pack_start(GTK_BOX(g->sepia.box), GTK_WIDGET(g->sepia.contrast), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->sepia.contrast), "value-changed",
                   G_CALLBACK(dt_iop_gmic_sepia_params_t::contrast_callback), self);

  g->sepia.gamma = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, p->parameters.sepia.gamma, 3);
  dt_bauhaus_widget_set_label(g->sepia.gamma, NULL, _("gamma"));
  gtk_widget_set_tooltip_text(g->sepia.gamma, _("gamma value of the sepia film_type"));
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
  if(self->dt->gui->reset) return;
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  p->parameters.sepia.brightness = dt_bauhaus_slider_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

void dt_iop_gmic_sepia_params_t::contrast_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if(self->dt->gui->reset) return;
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  p->parameters.sepia.contrast = dt_bauhaus_slider_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

void dt_iop_gmic_sepia_params_t::gamma_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if(self->dt->gui->reset) return;
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  p->parameters.sepia.gamma = dt_bauhaus_slider_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

// --- film emulation

void dt_iop_gmic_film_emulation_params_t::gui_update(struct dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  gtk_widget_set_visible(g->film_emulation.box, p->filter == film_emulation ? TRUE : FALSE);
  auto i = std::find(g->film_emulation.film_list.begin(), g->film_emulation.film_list.end(),
                     p->parameters.film_emulation.film);
  dt_bauhaus_combobox_set(g->film_emulation.film, static_cast<int>(i - g->film_emulation.film_list.begin()));
  dt_bauhaus_slider_set(g->film_emulation.strength, p->parameters.film_emulation.strength);
  dt_bauhaus_slider_set(g->film_emulation.brightness, p->parameters.film_emulation.brightness);
  dt_bauhaus_slider_set(g->film_emulation.contrast, p->parameters.film_emulation.contrast);
  dt_bauhaus_slider_set(g->film_emulation.gamma, p->parameters.film_emulation.gamma);
  dt_bauhaus_slider_set(g->film_emulation.hue, p->parameters.film_emulation.hue);
  dt_bauhaus_slider_set(g->film_emulation.saturation, p->parameters.film_emulation.saturation);
  dt_bauhaus_combobox_set(g->film_emulation.normalize_colors, p->parameters.film_emulation.normalize_colors);
}

void dt_iop_gmic_film_emulation_params_t::gui_init(dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  dt_bauhaus_combobox_add(g->gmic_filter, _("film emulation"));
  g->film_emulation.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  gtk_box_pack_start(GTK_BOX(self->widget), g->film_emulation.box, TRUE, TRUE, 0);

  g->film_emulation.film = dt_bauhaus_combobox_new(self);
  for(const auto &film : film_maps)
  {
    dt_bauhaus_combobox_add_aligned(g->film_emulation.film, film.printable.c_str(), DT_BAUHAUS_COMBOBOX_ALIGN_LEFT);
    g->film_emulation.film_list.push_back(film.film_type);
  }
  // dt_bauhaus_widget_set_label(g->film_emulation.film, NULL, _("film type"));
  gtk_widget_set_tooltip_text(g->film_emulation.film, _("choose emulated film type"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), g->film_emulation.film, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.film), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::film_callback), self);

  g->film_emulation.strength
      = dt_bauhaus_slider_new_with_range(self, 0, 1, 0.01, p->parameters.film_emulation.strength, 3);
  dt_bauhaus_widget_set_label(g->film_emulation.strength, NULL, _("strength"));
  gtk_widget_set_tooltip_text(g->film_emulation.strength, _("strength of the film emulation film_type"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), GTK_WIDGET(g->film_emulation.strength), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.strength), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::strength_callback), self);

  g->film_emulation.brightness
      = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, p->parameters.film_emulation.brightness, 3);
  dt_bauhaus_widget_set_label(g->film_emulation.brightness, NULL, _("brightness"));
  gtk_widget_set_tooltip_text(g->film_emulation.brightness, _("brightness of the film emulation film_type"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), GTK_WIDGET(g->film_emulation.brightness), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.brightness), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::brightness_callback), self);

  g->film_emulation.contrast
      = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, p->parameters.film_emulation.contrast, 3);
  dt_bauhaus_widget_set_label(g->film_emulation.contrast, NULL, _("contrast"));
  gtk_widget_set_tooltip_text(g->film_emulation.contrast, _("contrast of the film emulation film_type"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), GTK_WIDGET(g->film_emulation.contrast), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.contrast), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::contrast_callback), self);

  g->film_emulation.gamma
      = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, p->parameters.film_emulation.gamma, 3);
  dt_bauhaus_widget_set_label(g->film_emulation.gamma, NULL, _("gamma"));
  gtk_widget_set_tooltip_text(g->film_emulation.gamma, _("gamma value of the film emulation film_type"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), GTK_WIDGET(g->film_emulation.gamma), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.gamma), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::gamma_callback), self);

  g->film_emulation.hue = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, p->parameters.film_emulation.hue, 3);
  dt_bauhaus_widget_set_label(g->film_emulation.hue, NULL, _("hue"));
  gtk_widget_set_tooltip_text(g->film_emulation.hue, _("hue-shift of the film emulation film_type"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), GTK_WIDGET(g->film_emulation.hue), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.hue), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::hue_callback), self);

  g->film_emulation.saturation
      = dt_bauhaus_slider_new_with_range(self, -1, 1, 0.01, p->parameters.film_emulation.saturation, 3);
  dt_bauhaus_widget_set_label(g->film_emulation.saturation, NULL, _("saturation"));
  gtk_widget_set_tooltip_text(g->film_emulation.saturation, _("saturation of the film emulation film_type"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), GTK_WIDGET(g->film_emulation.saturation), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.saturation), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::saturation_callback), self);

  g->film_emulation.normalize_colors = dt_bauhaus_combobox_new(self);
  dt_bauhaus_combobox_add(g->film_emulation.normalize_colors, _("none"));
  dt_bauhaus_combobox_add(g->film_emulation.normalize_colors, _("pre-process"));
  dt_bauhaus_combobox_add(g->film_emulation.normalize_colors, _("post-process"));
  dt_bauhaus_combobox_add(g->film_emulation.normalize_colors, _("both"));
  dt_bauhaus_widget_set_label(g->film_emulation.normalize_colors, NULL, _("normalize colors"));
  gtk_widget_set_tooltip_text(g->film_emulation.normalize_colors, _("choose how to normalize colors"));
  gtk_box_pack_start(GTK_BOX(g->film_emulation.box), g->film_emulation.normalize_colors, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->film_emulation.normalize_colors), "value-changed",
                   G_CALLBACK(dt_iop_gmic_film_emulation_params_t::normalize_colors_callback), self);

  gtk_widget_show_all(g->film_emulation.box);
  gtk_widget_set_no_show_all(g->film_emulation.box, TRUE);
  gtk_widget_set_visible(g->film_emulation.box, p->filter == film_emulation ? TRUE : FALSE);
  g->filter_list.push_back(film_emulation);
}

void dt_iop_gmic_film_emulation_params_t::film_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if(self->dt->gui->reset) return;
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  std::snprintf(p->parameters.film_emulation.film, sizeof(p->parameters.film_emulation.film), "%s",
                film_maps[dt_bauhaus_combobox_get(w)].film_type.c_str());
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

void dt_iop_gmic_film_emulation_params_t::strength_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if(self->dt->gui->reset) return;
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  p->parameters.film_emulation.strength = dt_bauhaus_slider_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

void dt_iop_gmic_film_emulation_params_t::brightness_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if(self->dt->gui->reset) return;
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  p->parameters.film_emulation.brightness = dt_bauhaus_slider_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

void dt_iop_gmic_film_emulation_params_t::contrast_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if(self->dt->gui->reset) return;
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  p->parameters.film_emulation.contrast = dt_bauhaus_slider_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

void dt_iop_gmic_film_emulation_params_t::gamma_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if(self->dt->gui->reset) return;
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  p->parameters.film_emulation.gamma = dt_bauhaus_slider_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

void dt_iop_gmic_film_emulation_params_t::hue_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if(self->dt->gui->reset) return;
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  p->parameters.film_emulation.hue = dt_bauhaus_slider_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

void dt_iop_gmic_film_emulation_params_t::saturation_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if(self->dt->gui->reset) return;
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  p->parameters.film_emulation.saturation = dt_bauhaus_slider_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

void dt_iop_gmic_film_emulation_params_t::normalize_colors_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if(self->dt->gui->reset) return;
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  p->parameters.film_emulation.normalize_colors = dt_bauhaus_combobox_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}


extern "C" void gui_update(struct dt_iop_module_t *self)
{
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  dt_iop_gmic_params_t *p = reinterpret_cast<dt_iop_gmic_params_t *>(self->params);
  auto i_filter = std::find(g->filter_list.begin(), g->filter_list.end(), p->filter);
  if(i_filter != g->filter_list.end()) dt_bauhaus_combobox_set(g->gmic_filter, i_filter - g->filter_list.begin());
  dt_iop_gmic_none_params_t::gui_update(self);
  dt_iop_gmic_expert_mode_params_t::gui_update(self);
  dt_iop_gmic_sepia_params_t::gui_update(self);
  dt_iop_gmic_film_emulation_params_t::gui_update(self);
}

extern "C" void gui_init(dt_iop_module_t *self)
{
  self->gui_data = new dt_iop_gmic_gui_data_t;
  dt_iop_gmic_gui_data_t *g = reinterpret_cast<dt_iop_gmic_gui_data_t *>(self->gui_data);
  self->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);

  g->gmic_filter = dt_bauhaus_combobox_new(self);
  dt_bauhaus_widget_set_label(g->gmic_filter, NULL, _("G'MIC filter"));
  gtk_widget_set_tooltip_text(g->gmic_filter, _("choose an image processing film_type"));
  gtk_box_pack_start(GTK_BOX(self->widget), g->gmic_filter, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->gmic_filter), "value-changed", G_CALLBACK(filter_callback), self);

  dt_iop_gmic_none_params_t::gui_init(self);
  dt_iop_gmic_sepia_params_t::gui_init(self);
  dt_iop_gmic_film_emulation_params_t::gui_init(self);
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
    if(d->filter == expert_mode) command = d->parameters.expert_mode.gmic_command();
    if(d->filter == sepia) command = d->parameters.sepia.gmic_command();
    if(d->filter == film_emulation) command = d->parameters.film_emulation.gmic_command();
    gmic(command.c_str(), image_list, image_names);
  }
  catch(gmic_exception &e)
  {
    if(piece->pipe->type == DT_DEV_PIXELPIPE_FULL || piece->pipe->type == DT_DEV_PIXELPIPE_PREVIEW)
      dt_control_log("G'MIC error: %s", e.what());
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
// vim: shiftwidth = 2 expandtab tabstop = 2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
