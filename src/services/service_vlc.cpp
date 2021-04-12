#include "service_vlc.hpp"

#include <bits/stdc++.h>

#include "locator.hpp"

namespace ae {
  // A bunch of stuff that I literally just copied from the VLC SDL tutorial.
  // It's *extremely* c-like. I'd like to encapsulate this stuff in a class or
  // something, but c'est la vie.
  void* ctx_lock(void* data, void** p_pixels) {
    ctx_mutex.lock();
    struct ctx* ctx = (struct ctx*) data;
    *p_pixels = ctx;
    return nullptr;
  }
  void ctx_unlock(void* data, void* id, void* const* p_pixels) {
    struct ctx* ctx = (struct ctx*) data;
    ctx_mutex.unlock();
    assert(id == nullptr);
  }
  void ctx_display(void* data, void* id) {
    (void) data;
    assert(id == nullptr);
  }

  bool NullVLC::initialize(
    std::string filename, 
    std::vector<const char*> opts
  ) {
    printf("Attempting to intialize vlc::NullVLC. Returning false.");
    return false;
  }
  void NullVLC::play() { }
  void NullVLC::pause() { }
  void NullVLC::seek(libvlc_time_t time) { }
  void NullVLC::update(sf::Time dt) { }

  Timestamp NullVLC::get_timestamp() { return { 0, 0, 0.0 }; }
  Timestamp NullVLC::get_timestamp_crude() { return {0, 0, 0.0 }; }
  libvlc_time_t NullVLC::get_ms() { return 0; }
  // Returning a length of 1000 ms to not get any divide-by-zero errors
  Info NullVLC::get_info() { return {0, 0, 0, 0, 0.0, 1000}; }
  uint8_t* NullVLC::get_data() { 
    data.resize(1280*720*4);

    for (auto x = 0; x < 1280; ++x) {
      for (auto y = 0; y < 720; ++y) {
        auto i = 1280*y + x;
        data[4*i]   = x*256/1280;
        data[4*i+1] = y*256/720;
        data[4*i+2] = ((x/25 + y/25) % 2) * 255;
        data[4*i+3] = 255;
      }
    }

    return &data[0];
  }
  bool NullVLC::is_looping() { return false; }
  Loop NullVLC::get_loop() { return { 0, 1000 }; }
  void NullVLC::set_loop(Loop l) { }
  void NullVLC::reset_loop() { }

  bool LoadedVLC::initialize(
    std::string filename, 
    std::vector<const char*> opts
  ) {
    libvlc = libvlc_new(opts.size(), opts.data());

    if (libvlc == nullptr) {
      printf("libvlc failed to initialize.\n");
      printf("Do you have vlc installed?\n");
      printf("Try resetting your WSL instance and try again.\n");
      return false;
    }

    media = libvlc_media_new_path(libvlc, filename.c_str());

    if (media == nullptr) {
      printf("libvlc failed to create media.\n");
      printf("Try making sure the filename is correct and try again.\n");
      return false;
    }

    // All this to ignore a single deprecated declaration? Wow.
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    libvlc_media_parse(media);
    #pragma GCC diagnostic pop

    info.length = libvlc_media_get_duration(media);

    libvlc_media_track_t** tracks;
    int track_count = libvlc_media_tracks_get(media, &tracks);

    for (int i = 0; i < track_count; ++i) {
      libvlc_media_track_t* track = tracks[i];

      if (track->i_type == libvlc_track_video) {
        libvlc_video_track_t* v = track->video;
        info.width  = v->i_width;
        info.height = v->i_height;
        info.framerate_num = v->i_frame_rate_num;
        info.framerate_den = v->i_frame_rate_den;
        info.framerate = info.framerate_num / (double) info.framerate_den;

        loop.start = 0;
        loop.end   = info.length;
      }
    }

    libvlc_media_tracks_release(tracks, track_count);

    if (info.width == 0 or info.height == 0) {
      printf("libvlc encountered a weird bug reading the media\n");
      printf("Validate the media a try again.\n");
      return false;
    }

    // FIXME: Figure out what is the proper way to do this, I'm too tired rn
    libvlc_media_add_option(media, ":avcodec-hw=vda");
    // No subtitles
    libvlc_media_add_option(media, "input-repeat=-1");

    player = libvlc_media_player_new_from_media(media);
    libvlc_media_release(media);

    if (player == nullptr) {
      printf("libvlc could not create player.\n");
      return false;
    }

    data.resize(info.width * info.height * 4);

    libvlc_video_set_callbacks(player, ctx_lock, ctx_unlock, ctx_display, &(data[0]));
    libvlc_video_set_format(player, "RGBA", info.width, info.height, info.width*4);
    
    libvlc_media_player_play(player);

    libvlc_track_description_t* t = libvlc_video_get_spu_description(player);

    printf("length: %" PRId64 "\n", info.length);
    libvlc_media_player_set_pause(player, 1);

    Locator::mode = Mode::PAUSED;

    printf("libvlc intitialized.\n");

    return true;
  }

  LoadedVLC::~LoadedVLC() {
    libvlc_media_player_stop(player);
    libvlc_media_player_release(player);
    libvlc_release(libvlc);

    Locator::mode = Mode::UNLOADED;
  }

  void LoadedVLC::play() {
    Locator::mode = Mode::PLAYING;
    libvlc_media_player_set_pause(player, 0);
  }

  void LoadedVLC::pause() {
    Locator::mode = Mode::PAUSED;
    libvlc_media_player_set_pause(player, 1);
  }

  void LoadedVLC::seek(libvlc_time_t new_timestamp) {
    if (new_timestamp < 0) new_timestamp = 0;
    if (new_timestamp > info.length) new_timestamp = info.length;

    libvlc_media_player_set_time(player, new_timestamp);
    update_timestamp(libvlc_media_player_get_time(player));
  }

  void LoadedVLC::update(sf::Time dt) {
    if (Locator::mode == Mode::PLAYING) {
      libvlc_time_t new_crude_ms = libvlc_media_player_get_time(player);

      if (timestamp_crude.ms == new_crude_ms) {
        update_timestamp(timestamp.ms + dt.asMilliseconds());
      } else {
        update_timestamp(new_crude_ms);
        timestamp_crude = timestamp;
      }

      if (timestamp.ms > loop.end) seek(loop.start);
      if (timestamp.ms < loop.start) seek(loop.end);
    }
  }

  Timestamp LoadedVLC::get_timestamp()        { return timestamp; }
  Timestamp LoadedVLC::get_timestamp_crude()  { return timestamp_crude; }
  libvlc_time_t LoadedVLC::get_ms()           { return timestamp.ms; }
  Info LoadedVLC::get_info()                  { return info; }
  uint8_t* LoadedVLC::get_data()              { return &data[0]; }

  void LoadedVLC::update_timestamp(libvlc_time_t new_ms) {
    timestamp.ms = new_ms;
    timestamp.fraction = timestamp.ms / (double) info.length;
    timestamp.frame = info.framerate * timestamp.ms * 1000;
  }

  bool LoadedVLC::is_looping() { return looping; }
  Loop LoadedVLC::get_loop() { return loop; }
  void LoadedVLC::set_loop(Loop l) { 
    looping = true;
    if (l.start < 0) { l.start = 0; }
    if (l.end > info.length) { l.end = info.length; }
    loop = l; 
  }
  void LoadedVLC::reset_loop() {
    looping = false;
    loop = { 0, info.length };
  }
}