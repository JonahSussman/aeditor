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

ae::Tools* Locator::get_tools_service() {
  return tools_service ? tools_service : &tools_null;
}

void Locator::provide_tools_service(ae::Tools* service) {
  if (tools_service) free_tools_service();
  tools_service = service;
}

void Locator::free_tools_service() {
  util::delnull(tools_service);
}