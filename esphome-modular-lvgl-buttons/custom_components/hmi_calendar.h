// Calendario del mese corrente per l'HMI (solo visualizzazione).
//
// Da Home Assistant (sensor.hmi_calendario, vedi homeassistant/hmi_template.yaml):
//   attributo "mese":      "2026-11"
//   attributo "calendari": "Nome;#44739e|Nome 2;#984ea3"   (ordine = indice calendario)
//   attributo "eventi":    una riga per evento, separate da '\n':
//                          cal|giorno_inizio|giorno_fine|ora|titolo|luogo
//                          (giorni già limitati al mese corrente, ora vuota = tutto il giorno)
//
// La griglia (6 settimane x 7 giorni) viene creata una volta in un contenitore
// YAML; toccando un giorno con eventi si apre il pannello dei dettagli.
#pragma once

#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

namespace hmi_calendar {

struct Event {
  int cal = 0, d0 = 1, d1 = 1;
  std::string ora, title, loc;
};

static const int CELLS = 42;
static const int DOTS = 4;

static const uint32_t COL_CELL = 0x313131;
static const uint32_t COL_TODAY = 0xF37320;
static const uint32_t COL_DAY = 0xF2F0EB;
static const uint32_t COL_DAY_EMPTY = 0x8A8A8A;
static const uint32_t COL_HEADER = 0x9E9E9E;

static const char *const MESI[] = {"Gennaio", "Febbraio", "Marzo",     "Aprile",  "Maggio",   "Giugno",
                                   "Luglio",  "Agosto",   "Settembre", "Ottobre", "Novembre", "Dicembre"};
static const char *const GIORNI[] = {"Lun", "Mar", "Mer", "Gio", "Ven", "Sab", "Dom"};
static const char *const GIORNI_LUNGHI[] = {"Lunedi'", "Martedi'", "Mercoledi'", "Giovedi'",
                                            "Venerdi'", "Sabato",   "Domenica"};

inline std::vector<Event> events;
inline std::vector<uint32_t> colors;
inline std::vector<std::string> cal_names;
inline int year = 0, month = 0;  // mese ricevuto da HA (1-12)
inline int today = 0;            // giorno di oggi se nel mese mostrato, altrimenti 0
inline int first_wd = 0;         // 0 = lunedì
inline int days_in_month = 30;

struct Cell {
  lv_obj_t *box = nullptr, *num = nullptr;
  lv_obj_t *dots[DOTS] = {};
  int day = 0;
};
inline Cell cells[CELLS];
inline lv_obj_t *title_lbl = nullptr, *detail = nullptr, *detail_title = nullptr, *detail_list = nullptr;
inline const lv_font_t *font_small = nullptr, *font_big = nullptr;  // nunito_14 / nunito_20

inline std::vector<std::string> split(const std::string &s, char sep) {
  std::vector<std::string> out;
  size_t start = 0;
  while (true) {
    size_t p = s.find(sep, start);
    out.push_back(s.substr(start, p == std::string::npos ? std::string::npos : p - start));
    if (p == std::string::npos) break;
    start = p + 1;
  }
  return out;
}

inline uint32_t color_of(int cal) {
  return cal >= 0 && cal < (int) colors.size() ? colors[cal] : 0x01B4DE;
}

// 0 = lunedì ... 6 = domenica (algoritmo di Sakamoto)
inline int weekday(int y, int m, int d) {
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  int w = (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;  // 0 = domenica
  return (w + 6) % 7;
}

inline int month_len(int y, int m) {
  static const int l[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return 29;
  return l[(m - 1) % 12];
}

// ---------------------------------------------------------------- dati da HA

inline void set_month(const std::string &s) {
  int y = 0, m = 0;
  if (sscanf(s.c_str(), "%d-%d", &y, &m) == 2 && m >= 1 && m <= 12) {
    year = y;
    month = m;
  }
}

inline void set_calendars(const std::string &s) {
  colors.clear();
  cal_names.clear();
  if (s.empty()) return;
  for (auto &item : split(s, '|')) {
    auto p = split(item, ';');
    cal_names.push_back(p[0]);
    std::string c = p.size() > 1 ? p[1] : "";
    if (!c.empty() && c[0] == '#') c = c.substr(1);
    colors.push_back(c.empty() ? 0x01B4DE : (uint32_t) strtoul(c.c_str(), nullptr, 16));
  }
}

inline void set_events(const std::string &s) {
  events.clear();
  if (s.empty()) return;
  for (auto &line : split(s, '\n')) {
    auto p = split(line, '|');
    if (p.size() < 5) continue;
    Event e;
    e.cal = atoi(p[0].c_str());
    e.d0 = atoi(p[1].c_str());
    e.d1 = atoi(p[2].c_str());
    if (e.d1 < e.d0) e.d1 = e.d0;
    e.ora = p[3];
    e.title = p[4];
    e.loc = p.size() > 5 ? p[5] : "";
    events.push_back(e);
  }
}

// ---------------------------------------------------------------- dettagli

inline void detail_close() {
  if (detail) lv_obj_add_flag(detail, LV_OBJ_FLAG_HIDDEN);
}

inline void detail_open(int day) {
  if (!detail || !detail_list || day < 1) return;
  char buf[48];
  int wd = weekday(year, month, day);
  snprintf(buf, sizeof(buf), "%s %d %s", GIORNI_LUNGHI[wd], day, MESI[(month - 1) % 12]);
  lv_label_set_text(detail_title, buf);

  lv_obj_clean(detail_list);
  int n = 0, y = 0;
  for (auto &e : events) {
    if (day < e.d0 || day > e.d1) continue;
    n++;
    lv_obj_t *row = lv_obj_create(detail_list);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(row, lv_color_hex(COL_CELL), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 10, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(row, 10, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(color_of(e.cal)), 0);
    lv_obj_set_style_pad_all(row, 10, 0);
    lv_obj_set_style_pad_left(row, 20, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    lv_obj_set_y(row, y);

    std::string when;
    if (e.d0 != e.d1) {
      when = "dal " + std::to_string(e.d0) + " al " + std::to_string(e.d1);
      if (!e.ora.empty()) when += "  " + e.ora;
    } else {
      when = e.ora.empty() ? "Tutto il giorno" : e.ora;
    }
    if (e.cal >= 0 && e.cal < (int) cal_names.size()) when += "  -  " + cal_names[e.cal];

    lv_obj_t *t = lv_label_create(row);
    lv_obj_set_width(t, lv_pct(100));
    lv_label_set_long_mode(t, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(t, font_big, 0);
    lv_obj_set_style_text_color(t, lv_color_hex(COL_DAY), 0);
    lv_label_set_text(t, e.title.c_str());

    lv_obj_t *w = lv_label_create(row);
    lv_obj_set_width(w, lv_pct(100));
    lv_label_set_long_mode(w, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(w, font_small, 0);
    lv_obj_set_style_text_color(w, lv_color_hex(color_of(e.cal)), 0);
    std::string sub = when;
    if (!e.loc.empty()) sub += "\n" + e.loc;
    lv_label_set_text(w, sub.c_str());

    // posizionamento manuale (il layout flex non è compilato)
    lv_obj_update_layout(t);
    lv_obj_set_y(w, lv_obj_get_height(t) + 4);
    lv_obj_update_layout(row);
    y += lv_obj_get_height(row) + 8;
  }
  if (n == 0) {
    lv_obj_t *t = lv_label_create(detail_list);
    lv_obj_set_style_text_font(t, font_big, 0);
    lv_obj_set_style_text_color(t, lv_color_hex(COL_DAY_EMPTY), 0);
    lv_label_set_text(t, "Nessun evento");
  }
  lv_obj_scroll_to_y(detail_list, 0, LV_ANIM_OFF);
  lv_obj_remove_flag(detail, LV_OBJ_FLAG_HIDDEN);
}

inline void on_cell_click(lv_event_t *e) {
  int i = (int) (intptr_t) lv_event_get_user_data(e);
  if (i >= 0 && i < CELLS && cells[i].day > 0) detail_open(cells[i].day);
}

// ---------------------------------------------------------------- griglia

// Crea intestazione dei giorni e griglia dentro `grid` (468 x 390 px)
inline void build(lv_obj_t *grid, lv_obj_t *title, lv_obj_t *det, lv_obj_t *det_title, lv_obj_t *det_list,
                  const lv_font_t *small, const lv_font_t *big) {
  font_small = small;
  font_big = big;
  title_lbl = title;
  detail = det;
  detail_title = det_title;
  detail_list = det_list;

  const int cw = 66, gap = 1, head = 26, ch = 60;
  for (int c = 0; c < 7; c++) {
    lv_obj_t *l = lv_label_create(grid);
    lv_obj_set_style_text_font(l, font_small, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(c >= 5 ? COL_TODAY : COL_HEADER), 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(l, cw);
    lv_obj_set_pos(l, c * (cw + gap), 4);
    lv_label_set_text(l, GIORNI[c]);
  }
  for (int i = 0; i < CELLS; i++) {
    Cell &k = cells[i];
    k.box = lv_obj_create(grid);
    lv_obj_remove_style_all(k.box);
    lv_obj_set_size(k.box, cw, ch);
    lv_obj_set_pos(k.box, (i % 7) * (cw + gap), head + (i / 7) * (ch + gap));
    lv_obj_set_style_radius(k.box, 8, 0);
    lv_obj_set_style_bg_opa(k.box, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(k.box, lv_color_hex(COL_CELL), 0);
    lv_obj_set_style_border_color(k.box, lv_color_hex(COL_TODAY), 0);
    lv_obj_add_flag(k.box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(k.box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(k.box, on_cell_click, LV_EVENT_CLICKED, (void *) (intptr_t) i);

    k.num = lv_label_create(k.box);
    lv_obj_set_style_text_font(k.num, font_big, 0);
    lv_obj_align(k.num, LV_ALIGN_TOP_MID, 0, 4);
    lv_label_set_text(k.num, "");

    for (int d = 0; d < DOTS; d++) {
      lv_obj_t *dot = lv_obj_create(k.box);
      lv_obj_remove_style_all(dot);
      lv_obj_set_size(dot, 10, 10);
      lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
      lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
      lv_obj_align(dot, LV_ALIGN_BOTTOM_MID, (d - (DOTS - 1) / 2.0f) * 13, -7);
      lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
      k.dots[d] = dot;
    }
  }
}

// Aggiorna la griglia. today_ym = "AAAA-MM" di oggi, today_d = giorno di oggi (0 se ora non valida)
inline void render(const std::string &today_ym, int today_d) {
  if (cells[0].box == nullptr) return;
  if (month == 0) {
    if (title_lbl) lv_label_set_text(title_lbl, "Calendario (in attesa di HA)");
    for (auto &k : cells) lv_obj_add_flag(k.box, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  char ym[8];
  snprintf(ym, sizeof(ym), "%04d-%02d", year, month);
  today = (today_ym == ym) ? today_d : 0;
  first_wd = weekday(year, month, 1);
  days_in_month = month_len(year, month);

  if (title_lbl) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%s %d", MESI[month - 1], year);
    lv_label_set_text(title_lbl, buf);
  }

  int rows = (first_wd + days_in_month + 6) / 7;
  for (int i = 0; i < CELLS; i++) {
    Cell &k = cells[i];
    int day = i - first_wd + 1;
    if (i >= rows * 7) {
      lv_obj_add_flag(k.box, LV_OBJ_FLAG_HIDDEN);
      k.day = 0;
      continue;
    }
    lv_obj_remove_flag(k.box, LV_OBJ_FLAG_HIDDEN);
    if (day < 1 || day > days_in_month) {
      k.day = 0;
      lv_label_set_text(k.num, "");
      lv_obj_set_style_bg_opa(k.box, LV_OPA_TRANSP, 0);
      lv_obj_set_style_border_width(k.box, 0, 0);
      for (auto *dot : k.dots) lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    k.day = day;
    char n[4];
    snprintf(n, sizeof(n), "%d", day);
    lv_label_set_text(k.num, n);
    lv_obj_set_style_bg_opa(k.box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(k.box, day == today ? 3 : 0, 0);

    int nd = 0;
    for (auto &e : events) {
      if (day < e.d0 || day > e.d1) continue;
      if (nd < DOTS) {
        lv_obj_set_style_bg_color(k.dots[nd], lv_color_hex(color_of(e.cal)), 0);
        lv_obj_remove_flag(k.dots[nd], LV_OBJ_FLAG_HIDDEN);
      }
      nd++;
    }
    for (int d = nd; d < DOTS; d++) lv_obj_add_flag(k.dots[d], LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(k.num, lv_color_hex(nd ? COL_DAY : COL_DAY_EMPTY), 0);
  }
}

}  // namespace hmi_calendar
