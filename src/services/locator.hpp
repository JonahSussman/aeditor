#pragma once

#include "service_vlc.hpp"
#include "service_script.hpp"

// A video can either be unloaded, paused, or playing. If playing, the video
// will continuously lop between two set points.
enum class Mode {
  UNLOADED = -1,
  PAUSED,
  PLAYING
};

class Locator {
public:
  // FIXME: Prefer not to have initialize function, but due to class structure
  // it is required.
  // TODO: Use templates somehow to make code cleaner
  // static void initialize();
  static Mode mode;

  static ae::VLC* get_vlc_service();
  static void provide_vlc_service(ae::VLC* service);
  static void free_vlc_service();
  
  static ae::Script* get_script_service();
  static void provide_script_service(ae::Script* service);
  static void free_script_service();

private:
  static inline ae::NullVLC vlc_null;
  static inline ae::VLC* vlc_service;

  static inline ae::NullScript script_null;
  static inline ae::Script* script_service;
};