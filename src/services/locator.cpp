#include "locator.hpp"
#include "../util.hpp"

Mode Locator::mode = Mode::UNLOADED;

ae::VLC* Locator::get_vlc_service() { 
  return vlc_service ? vlc_service : &vlc_null;
}

void Locator::provide_vlc_service(ae::VLC* service) {
  if (vlc_service) free_vlc_service();
  vlc_service = service;
}

void Locator::free_vlc_service() {
  util::delnull(vlc_service);
}

ae::Script* Locator::get_script_service() {
  return script_service ? script_service : &script_null;
}

void Locator::provide_script_service(ae::Script* service) {
  if (script_service) free_script_service();
  script_service = service;
}

void Locator::free_script_service() {
  util::delnull(script_service);
}