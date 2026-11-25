// Slot dinamici per l'HMI: pulsanti configurabili dal display senza riprogrammare.
//
// Ogni slot ha una configurazione salvata in un'entità `text` del dispositivo
// (persistente in flash e visibile/modificabile anche da HA) nel formato:
//     tipo;entity_id;etichetta;indice_icona
//   tipo: t = on/off (toggle), s = valore sensore, c = telecamera
//         (per le telecamere il secondo campo è l'URL dell'immagine)
//   es.:  t;switch.luce_esempio;Luce;0
//
// Da Home Assistant arrivano (vedi homeassistant/hmi_template.yaml):
//   - lo stato di tutti gli slot in una stringa "on|off|21.5 °C|..."
//   - l'elenco delle entità selezionabili (nomi e id, separati da '\n')
#pragma once

#include <cstdlib>
#include <string>
#include <vector>

namespace hmi_slots {

// 0..11 = pagina Dispositivi (Slot 1-12), 12..23 = Home (Slot 13-24)
static const int NUM_SLOTS = 24;

// Icone selezionabili (devono essere presenti nel font mdi_slot_icons_40).
// L'ordine conta: l'indice viene salvato nella configurazione dello slot.
static const char *const ICONS[] = {
    "\U000F0335",  // 0  lightbulb
    "\U000F0769",  // 1  ceiling_light
    "\U000F06B5",  // 2  lamp
    "\U000F07D6",  // 3  led_strip
    "\U000F06A5",  // 4  power_plug
    "\U000F07E7",  // 5  power_socket_eu
    "\U000F0210",  // 6  fan
    "\U000F0438",  // 7  radiator
    "\U000F1A45",  // 8  heat_wave
    "\U000F0717",  // 9  snowflake
    "\U000F0D43",  // 10 air_filter
    "\U000F0F92",  // 11 water_boiler
    "\U000F072A",  // 12 washing_machine
    "\U000F0AAC",  // 13 dishwasher
    "\U000F0502",  // 14 television
    "\U000F04C3",  // 15 speaker
    "\U000F06D9",  // 16 garage
    "\U000F081A",  // 17 door
    "\U000F033E",  // 18 lock
    "\U000F050F",  // 19 thermometer
    "\U000F058E",  // 20 water_percent
    "\U000F140B",  // 21 lightning_bolt
    "\U000F029A",  // 22 gauge
    "\U000F1A25",  // 23 toggle_switch_variant
    "\U000F07AE",  // 24 cctv (telecamera)
};
static const int NUM_ICONS = sizeof(ICONS) / sizeof(ICONS[0]);
static const char *const ICON_EMPTY = "\U000F0415";  // plus

// Colori allineati a color.yaml / substitutions
static const uint32_t COL_BTN_ON = 0xF37320;    // ep_orange
static const uint32_t COL_BTN_OFF = 0x313131;   // very_dark_gray
static const uint32_t COL_ICON_ON = 0xE7C12C;   // yellow
static const uint32_t COL_ICON_OFF = 0x666666;  // gray
static const uint32_t COL_TEXT_ON = 0xF2F0EB;   // white
static const uint32_t COL_SENSOR_BG = 0x1F3A5F; // blu scuro per i sensori
static const uint32_t COL_CAMERA_BG = 0x2D4A2F; // verde scuro per le telecamere

struct Slot {
  esphome::text::Text *cfg = nullptr;
  lv_obj_t *btn = nullptr, *icon = nullptr, *val = nullptr, *name = nullptr;
  char type = 0;  // 0 = vuoto, 't' = toggle, 's' = sensore, 'c' = telecamera
  std::string entity, label;
  int icon_idx = 0;
  std::string state;
};

inline Slot slots[NUM_SLOTS];

// Elenchi entità selezionabili ricevuti da HA
inline std::vector<std::string> toggle_ids, sensor_ids, camera_ids;
inline std::string toggle_names, sensor_names, camera_names;  // separati da '\n' (formato roller LVGL)

inline const std::string &names_for(char t) {
  return t == 's' ? sensor_names : (t == 'c' ? camera_names : toggle_names);
}
inline const std::vector<std::string> &ids_for(char t) {
  return t == 's' ? sensor_ids : (t == 'c' ? camera_ids : toggle_ids);
}

// Stato dell'editor
inline int edit_idx = -1;
inline char edit_type = 't';

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

inline bool valid(int i) { return i >= 0 && i < NUM_SLOTS; }

// ---------------------------------------------------------------- visualizzazione

inline void refresh(int i) {
  if (!valid(i)) return;
  Slot &s = slots[i];
  if (s.btn == nullptr) return;

  if (s.type == 0) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(COL_BTN_OFF), 0);
    lv_label_set_text(s.icon, ICON_EMPTY);
    lv_obj_set_style_text_color(s.icon, lv_color_hex(COL_ICON_OFF), 0);
    lv_label_set_text(s.val, "");
    lv_label_set_text(s.name, "Vuoto");
    lv_obj_set_style_text_color(s.name, lv_color_hex(COL_ICON_OFF), 0);
    return;
  }

  int ic = (s.icon_idx >= 0 && s.icon_idx < NUM_ICONS) ? s.icon_idx : 0;
  lv_label_set_text(s.icon, ICONS[ic]);
  lv_label_set_text(s.name, s.label.c_str());

  if (s.type == 'c') {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(COL_CAMERA_BG), 0);
    lv_label_set_text(s.val, "");
    lv_obj_set_style_text_color(s.icon, lv_color_hex(COL_ICON_ON), 0);
    lv_obj_set_style_text_color(s.name, lv_color_hex(COL_TEXT_ON), 0);
    return;
  }

  if (s.type == 's') {
    bool ok = !s.state.empty() && s.state.rfind("unavailable", 0) != 0 && s.state.rfind("unknown", 0) != 0;
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(COL_SENSOR_BG), 0);
    lv_label_set_text(s.val, ok ? s.state.c_str() : "--");
    lv_obj_set_style_text_color(s.icon, lv_color_hex(COL_ICON_ON), 0);
    lv_obj_set_style_text_color(s.name, lv_color_hex(COL_TEXT_ON), 0);
    return;
  }

  // toggle: "on" (switch/light/input_boolean) o "on"/"off" anche per fan
  bool on = (s.state == "on" || s.state == "open" || s.state == "playing" || s.state == "heat");
  bool unavailable = (s.state == "unavailable");
  lv_obj_set_style_bg_color(s.btn, lv_color_hex(on ? COL_BTN_ON : COL_BTN_OFF), 0);
  lv_obj_set_style_text_color(s.icon, lv_color_hex(on ? COL_ICON_ON : COL_ICON_OFF), 0);
  lv_obj_set_style_text_color(s.name, lv_color_hex(on ? COL_TEXT_ON : COL_ICON_OFF), 0);
  lv_label_set_text(s.val, unavailable ? "n.d." : "");
}

inline void refresh_all() {
  for (int i = 0; i < NUM_SLOTS; i++) refresh(i);
}

// ---------------------------------------------------------------- configurazione

// Interpreta la stringa di configurazione dello slot i
inline void load(int i, const std::string &cfg) {
  if (!valid(i)) return;
  Slot &s = slots[i];
  std::string old_entity = s.entity;
  auto parts = split(cfg, ';');
  if (parts.size() < 2 || parts[1].empty() || (parts[0] != "t" && parts[0] != "s" && parts[0] != "c")) {
    s.type = 0;
    s.entity.clear();
    s.label.clear();
    s.icon_idx = 0;
  } else {
    s.type = parts[0][0];
    s.entity = parts[1];
    s.label = parts.size() > 2 && !parts[2].empty() ? parts[2] : parts[1];
    s.icon_idx = parts.size() > 3 ? atoi(parts[3].c_str()) : 0;
  }
  // Mantiene lo stato se l'entità non è cambiata (HA lo rimanda solo quando cambia)
  if (s.entity != old_entity) s.state.clear();
  refresh(i);
}

// Collega i widget e l'entità text allo slot i (chiamato all'avvio)
inline void attach(int i, esphome::text::Text *cfg, lv_obj_t *btn, lv_obj_t *icon, lv_obj_t *val, lv_obj_t *name) {
  if (!valid(i)) return;
  Slot &s = slots[i];
  s.cfg = cfg;
  s.btn = btn;
  s.icon = icon;
  s.val = val;
  s.name = name;
  lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
  load(i, cfg->state);
}

// Stato di tutti gli slot da HA: "on|off|21.5 °C|..."
inline void apply_states(const std::string &all) {
  auto parts = split(all, '|');
  for (int i = 0; i < NUM_SLOTS; i++) {
    slots[i].state = i < (int) parts.size() ? parts[i] : "";
    refresh(i);
  }
}

inline void set_list(char type, const std::string &names, const std::string &ids) {
  if (type == 'c') {
    camera_names = names;
    camera_ids = split(ids, '\n');
  } else if (type == 's') {
    sensor_names = names;
    sensor_ids = split(ids, '\n');
  } else {
    toggle_names = names;
    toggle_ids = split(ids, '\n');
  }
}

// Usati dalle azioni YAML
inline bool edit_on_home() { return edit_idx >= 12; }
inline char type_of(int i) { return valid(i) ? slots[i].type : 0; }
inline std::string entity_of(int i) { return valid(i) ? slots[i].entity : std::string(); }
inline std::string label_of(int i) { return valid(i) ? slots[i].label : std::string(); }

// ---------------------------------------------------------------- editor

// Riempie il roller delle entità con l'elenco del tipo corrente e seleziona
// l'entità già assegnata allo slot (se presente).
inline void editor_fill(lv_obj_t *roller, lv_obj_t *info) {
  const std::string &names = names_for(edit_type);
  const std::vector<std::string> &ids = ids_for(edit_type);
  if (names.empty()) {
    lv_roller_set_options(roller, "(elenco non ancora ricevuto da HA)", LV_ROLLER_MODE_NORMAL);
    lv_label_set_text(info, "Controlla il sensore template in HA");
    return;
  }
  lv_roller_set_options(roller, names.c_str(), LV_ROLLER_MODE_NORMAL);
  uint16_t sel = 0;
  if (valid(edit_idx)) {
    for (size_t k = 0; k < ids.size(); k++) {
      if (ids[k] == slots[edit_idx].entity) {
        sel = k;
        break;
      }
    }
  }
  lv_roller_set_selected(roller, sel, LV_ANIM_OFF);
  char buf[48];
  snprintf(buf, sizeof(buf), "%u entita' disponibili", (unsigned) ids.size());
  lv_label_set_text(info, buf);
}

inline void editor_open(int i, lv_obj_t *title, lv_obj_t *roller, lv_obj_t *icon_roller, lv_obj_t *info) {
  if (!valid(i)) return;
  edit_idx = i;
  edit_type = slots[i].type ? slots[i].type : 't';
  char buf[24];
  snprintf(buf, sizeof(buf), "%s - slot %d", i >= 12 ? "Home" : "Dispositivi", (i % 12) + 1);
  lv_label_set_text(title, buf);
  int ic = slots[i].type ? slots[i].icon_idx : 0;
  lv_roller_set_selected(icon_roller, (ic >= 0 && ic < NUM_ICONS) ? ic : 0, LV_ANIM_OFF);
  editor_fill(roller, info);
}

// Restituisce il nome (etichetta) all'indice k dell'elenco corrente
inline std::string name_at(size_t k) {
  auto names = split(names_for(edit_type), '\n');
  return k < names.size() ? names[k] : std::string();
}

// Massimo numero di caratteri del nome mostrato sul pulsante
static const size_t LABEL_MAX = 24;

// Taglia una stringa UTF-8 a max_chars caratteri senza spezzare le lettere accentate
inline std::string utf8_cut(const std::string &s, size_t max_chars) {
  size_t chars = 0, i = 0;
  while (i < s.size()) {
    unsigned char c = s[i];
    size_t len = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : 4;
    if (chars == max_chars) return s.substr(0, i);
    i += len;
    chars++;
  }
  return s;
}

// Indice dell'entità scelta nel roller, in attesa che l'utente confermi il nome
inline int pending_k = -1;

// Passo 1 (pulsante "Avanti"): controlla la scelta e propone il nome nella textarea.
// Se l'entità è quella già assegnata allo slot propone l'etichetta attuale.
inline bool editor_prepare_name(lv_obj_t *roller, lv_obj_t *textarea, lv_obj_t *hint) {
  if (!valid(edit_idx) || slots[edit_idx].cfg == nullptr) return false;
  const std::vector<std::string> &ids = ids_for(edit_type);
  uint16_t k = lv_roller_get_selected(roller);
  if (k >= ids.size() || ids[k].empty()) return false;
  pending_k = k;
  const Slot &s = slots[edit_idx];
  std::string full = name_at(k);
  std::string proposal = (s.type == edit_type && s.entity == ids[k] && !s.label.empty()) ? s.label : full;
  lv_textarea_set_text(textarea, utf8_cut(proposal, LABEL_MAX).c_str());
  std::string h = "Entita': " + full;
  lv_label_set_text(hint, h.c_str());
  return true;
}

// Ripristina nella textarea il nome completo dell'entità (tagliato a LABEL_MAX)
inline void editor_reset_name(lv_obj_t *textarea) {
  if (pending_k >= 0) lv_textarea_set_text(textarea, utf8_cut(name_at(pending_k), LABEL_MAX).c_str());
}

// Passo 2: salva la configurazione con il nome scelto. Scrive l'entità text
// (persistente) che a sua volta richiama load() tramite on_value.
inline bool editor_save(const std::string &name, lv_obj_t *icon_roller) {
  if (!valid(edit_idx) || slots[edit_idx].cfg == nullptr) return false;
  const std::vector<std::string> &ids = ids_for(edit_type);
  if (pending_k < 0 || pending_k >= (int) ids.size() || ids[pending_k].empty()) return false;
  std::string label = name;
  // toglie spazi iniziali/finali
  size_t a = label.find_first_not_of(' '), b = label.find_last_not_of(' ');
  label = a == std::string::npos ? std::string() : label.substr(a, b - a + 1);
  if (label.empty()) label = name_at(pending_k);
  for (char &c : label)
    if (c == ';' || c == '|' || c == '\n') c = ' ';
  label = utf8_cut(label, 40);
  std::string cfg = std::string(1, edit_type) + ";" + ids[pending_k] + ";" + label + ";" +
                    std::to_string(lv_roller_get_selected(icon_roller));
  auto call = slots[edit_idx].cfg->make_call();
  call.set_value(cfg);
  call.perform();
  pending_k = -1;
  return true;
}

inline void editor_clear() {
  if (!valid(edit_idx) || slots[edit_idx].cfg == nullptr) return;
  auto call = slots[edit_idx].cfg->make_call();
  call.set_value("");
  call.perform();
}

// Opzioni del roller icone (glifi separati da '\n')
inline std::string icon_options() {
  std::string out;
  for (int k = 0; k < NUM_ICONS; k++) {
    if (k) out += '\n';
    out += ICONS[k];
  }
  return out;
}

}  // namespace hmi_slots
