#pragma once

#include <mutex>
#include <vector>

#include <SFML/System/Time.hpp>

#include <vlc/vlc.h>

// IMPORTANT: DO NOT "using namespace" these! They have class definitions that
// *WILL* conflict! Bad design? Maybe. Constistent names? Yes.
// Namespace for all vlc related stuff

namespace ae {
  // A bunch of stuff that I literally just copied from the VLC SDL tutorial.
  // It's *extremely* c-like. I'd like to encapsulate this stuff in a class or
  // something, but c'est la vie.
  static std::mutex ctx_mutex;
  struct ctx {
    uint8_t* surface;
  };
  static void* ctx_lock(void* data, void** p_pixels);
  static void  ctx_unlock(void* data, void* id, void* const* p_pixels);
  static void  ctx_display(void* data, void* id);

  struct Timestamp {
    libvlc_time_t ms;
    int64_t       frame;
    double        fraction;
  };

  struct Info {
    uint32_t      width = 0;
    uint32_t      height = 0;
    uint32_t      framerate_num = 0;
    uint32_t      framerate_den = 0;
    double        framerate = 0;
    libvlc_time_t length;
  };

  struct Loop {
    libvlc_time_t start;
    libvlc_time_t end;
  };

  class VLC {
  public:
    virtual bool initialize(
      std::string filename, 
      std::vector<const char*> opts={ }
    ) = 0;
    virtual void play() = 0;
    virtual void pause()  = 0;
    virtual void seek(libvlc_time_t time) = 0;
    virtual void update(sf::Time dt) = 0;

    virtual Timestamp get_timestamp() = 0;
    virtual Timestamp get_timestamp_crude() = 0;
    virtual libvlc_time_t get_ms() = 0;
    virtual Info get_info() = 0;
    virtual uint8_t* get_data() = 0;
    virtual bool is_looping() = 0;
    virtual void set_loop(Loop l) = 0;
    virtual Loop get_loop() = 0;
    virtual void reset_loop() = 0;
  };

  // TODO: Rename to NullVLC
  class NullVLC : public VLC {
  public:
    virtual bool initialize(
      std::string filename, 
      std::vector<const char*> opts={ }
    );
    virtual void play();
    virtual void pause();
    virtual void seek(libvlc_time_t time);
    virtual void update(sf::Time dt);

    virtual Timestamp get_timestamp();
    virtual Timestamp get_timestamp_crude();
    virtual libvlc_time_t get_ms();
    // Returning a length of 1000 ms to not get any divide-by-zero errors
    virtual Info get_info();
    virtual uint8_t* get_data();

    virtual bool is_looping();
    virtual void set_loop(Loop l);
    virtual Loop get_loop();
    virtual void reset_loop();

private:
    std::vector<uint8_t> data;
  };

  // TODO: Implement looping (loop struct, pause on loop (?), etc...)
  // TODO: Rename to LoadedVLC
  class LoadedVLC : public VLC {
  public:
    virtual bool initialize(
      std::string filename, 
      std::vector<const char*> opts={ }
    );

    ~LoadedVLC();

    virtual void play();
    virtual void pause();
    virtual void seek(libvlc_time_t new_timestamp);
    virtual void update(sf::Time dt);

    virtual Timestamp get_timestamp();
    virtual Timestamp get_timestamp_crude();
    virtual libvlc_time_t get_ms();
    virtual Info get_info();
    virtual uint8_t* get_data();

    virtual bool is_looping();
    virtual void set_loop(Loop l);
    virtual Loop get_loop();
    virtual void reset_loop();
    
  private:
    libvlc_instance_t* libvlc;
    libvlc_media_t* media;
    libvlc_media_player_t* player;

    Timestamp timestamp_crude;
    Timestamp timestamp;

    Info info;
    Loop loop;
    bool looping = false;

    std::vector<uint8_t> data;

    void update_timestamp(libvlc_time_t new_ms);
  };
}