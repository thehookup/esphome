#pragma once

#include <string>

#include "esphome/core/component.h"
#include "esphome/core/esphal.h"
#include "esphome/core/defines.h"

namespace esphome {

class ESPPreferenceObject {
 public:
  ESPPreferenceObject();
  ESPPreferenceObject(size_t offset, size_t length, uint32_t type);

  template<typename T> bool save(T *src, bool immediate_sync = false);

  template<typename T> bool load(T *dest);

  bool is_initialized() const;

 protected:
  friend class ESPPreferences;

  bool save_(bool immediate_sync);
  bool load_();
  bool save_internal_();
  bool load_internal_();

  uint32_t calculate_crc_() const;

  size_t offset_;
  size_t length_words_;
  uint32_t type_;
  uint32_t *data_;
#ifdef ARDUINO_ARCH_ESP8266
  bool in_flash_{false};
#endif
};

#ifdef ARDUINO_ARCH_ESP8266
#ifdef USE_ESP8266_PREFERENCES_FLASH
static const bool DEFAULT_IN_FLASH = true;
#else
static const bool DEFAULT_IN_FLASH = false;
#endif
#endif

#ifdef ARDUINO_ARCH_ESP32
static const bool DEFAULT_IN_FLASH = true;
#endif

class ESPPreferences : public Component {
 public:
  ESPPreferences();
  void begin(uint32_t flash_write_interval);
  ESPPreferenceObject make_preference(size_t length, uint32_t type, bool in_flash = DEFAULT_IN_FLASH);
  template<typename T> ESPPreferenceObject make_preference(uint32_t type, bool in_flash = DEFAULT_IN_FLASH);

  // ========== INTERNAL METHODS ==========
  // (In most use cases you won't need these)
  void pre_setup(uint32_t flash_write_interval);
  void setup();
  float get_setup_priority() const override;
  void dump_config() override;
  void loop() override;

#ifdef ARDUINO_ARCH_ESP8266
  /** On the ESP8266, we can't override the first 128 bytes during OTA uploads
   * as the eboot parameters are stored there. Writing there during an OTA upload
   * would invalidate applying the new firmware. During normal operation, we use
   * this part of the RTC user memory, but stop writing to it during OTA uploads.
   *
   * @param prevent Whether to prevent writing to the first 32 words of RTC user memory.
   */
  void prevent_write(bool prevent);
  bool is_prevent_write();
#endif

 protected:
  friend ESPPreferenceObject;

  uint32_t flash_write_interval_;

  uint32_t current_offset_;

  bool commit_to_flash_();
  bool flash_dirty_{false};
  uint32_t last_write_time_{0};
  /**
   * Commit pending writes to flash.
   *
   * @return true if write is successful.
   */
  bool sync_();
#ifdef ARDUINO_ARCH_ESP32
  uint32_t nvs_handle_;
#endif
#ifdef ARDUINO_ARCH_ESP8266
  bool prevent_write_{false};
  uint32_t *flash_storage_;
  uint32_t current_flash_offset_;
#endif
};

extern ESPPreferences global_preferences;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

template<typename T> ESPPreferenceObject ESPPreferences::make_preference(uint32_t type, bool in_flash) {
  return this->make_preference((sizeof(T) + 3) / 4, type, in_flash);
}

template<typename T> bool ESPPreferenceObject::save(T *src, bool immediate_sync) {
  if (!this->is_initialized())
    return false;
  memset(this->data_, 0, this->length_words_ * 4);
  memcpy(this->data_, src, sizeof(T));
  return this->save_(immediate_sync);
}

template<typename T> bool ESPPreferenceObject::load(T *dest) {
  memset(this->data_, 0, this->length_words_ * 4);
  if (!this->load_())
    return false;

  memcpy(dest, this->data_, sizeof(T));
  return true;
}

}  // namespace esphome
