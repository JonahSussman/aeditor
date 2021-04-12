#include <bits/stdc++.h>

#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "imgui.h"
#include "imgui-SFML.h"

#include <vlc/vlc.h>

#include "util.hpp"

namespace ig  = ImGui;
namespace igs = ImGui::SFML;

#define VWIDTH  1280
#define VHEIGHT 720

static int no_close = sf::Style::Titlebar | sf::Style::Close;
static sf::RenderWindow window(sf::VideoMode(1600, 900), L"æditor", no_close);

enum Mode {
  WATCH, LOOP
};
enum State {
  STOPPED = -1, PAUSED, PLAYING
};

static Mode  mode  = WATCH;
static State state = STOPPED;

// Directly copied from VLC SDL tutorial. Way too c-like for my taste, would
// like to wrap it all up in a class or something.
struct ctx {
  uint8_t* surface;
};
static void* ctx_lock(void* data, void** p_pixels) {
  struct ctx* ctx = (struct ctx*) data;
  *p_pixels = ctx;
  return nullptr;
}
static void ctx_unlock(void* data, void* id, void* const* p_pixels) {
  struct ctx* ctx = (struct ctx*) data;
  assert(id == nullptr);
}
static void ctx_display(void* data, void* id) {
  (void) data;
  assert(id == nullptr);
}

// VLC Media player struct
struct VLC {
  libvlc_instance_t* libvlc;
  libvlc_media_t* media;
  libvlc_media_player_t* player;

  uint8_t data[VWIDTH * VHEIGHT * 4];

  double framerate;
  int width, height;
  libvlc_time_t length = 0;
  libvlc_time_t crude_timestamp = 0;
  libvlc_time_t timestamp = 0;
  double percent_done = 0.0;

  libvlc_time_t loop_start = 10000;
  libvlc_time_t loop_stop  = 11000;

  VLC(int argc, const char* const* argv, const char* filename) {
    libvlc = libvlc_new(argc, argv);

    if (libvlc == nullptr) {
      printf("libvlc failed to initialize.\n");
      printf("Do you have vlc installed? Try resetting WSL instance and try again.\n");
      exit(1);
    }

    media  = libvlc_media_new_path(libvlc, "data/episode_720.mkv");
    
    libvlc_media_parse(media);
    libvlc_media_track_t** tracks;
    int track_count = libvlc_media_tracks_get(media, &tracks);
    for (int i = 0; i < track_count; ++i) {
      libvlc_media_track_t* track = tracks[i];

      if (track->i_type == libvlc_track_video) {
        libvlc_video_track_t* v = track->video;
        width = v->i_width;
        height = v->i_height;
        framerate = v->i_frame_rate_num / (double)v->i_frame_rate_den;
        printf("%dx%d, %f fps\n", width, height, framerate);
      }
    }
    libvlc_media_tracks_release(tracks, track_count);

    libvlc_media_add_option(media, ":avcodec-hw=vda");

    player = libvlc_media_player_new_from_media(media);
    libvlc_media_release(media);

    libvlc_video_set_callbacks(player, ctx_lock, ctx_unlock, ctx_display, &data);
    libvlc_video_set_format(player, "RGBA", VWIDTH, VHEIGHT, VWIDTH*4);
    libvlc_media_player_play(player);
    
    libvlc_media_player_set_pause(player, 1);
    
    printf("LENGTH: %d\n", length);
  } 

  void play() {
    libvlc_media_player_set_pause(player, 0);
  }

  void pause() {
    libvlc_media_player_set_pause(player, 1);
  }

  void seek(libvlc_time_t time) {
    if (time < 0) time = 0;
    if (time > length) time = length;
    libvlc_media_player_set_time(player, time);

    crude_timestamp = libvlc_media_player_get_time(player);
    timestamp = crude_timestamp;
    percent_done = timestamp / (double) length;
  }

  void update(sf::Time dt) {
    if (state == PLAYING) {
      libvlc_time_t new_crude_timestamp = libvlc_media_player_get_time(player);
      if (crude_timestamp == new_crude_timestamp) {
        timestamp += dt.asMilliseconds();
      } else {
        crude_timestamp = new_crude_timestamp;
        timestamp = crude_timestamp;
      }
      length = libvlc_media_player_get_length(player);
      percent_done = timestamp / (double) length;

      if (mode == LOOP) {
        if (timestamp < loop_start) seek(loop_start);
        if (timestamp > loop_stop) seek(loop_start);
      }
    }
  }

  ~VLC() {
    libvlc_media_player_stop(player);
    libvlc_media_player_release(player);
    libvlc_release(libvlc);
  }
};

// "*" means unknown, "" means no words
struct Line {
  int season;
  int episode;
  libvlc_time_t timestamp;
  std::string str;
  // Line(int timestamp, std::string str) : timestamp(timestamp), str(str) {}
};

struct Script {
  std::vector<Line> lines;

  void load_from_stream(std::istream& stream) {
    for (util::CSVIterator loop(stream); loop != util::CSVIterator(); ++loop) {
      int season  = std::stoi((*loop)[0]);
      int episode = std::stoi((*loop)[1]);
      libvlc_time_t timestamp = std::stoi((*loop)[2]);
      std::string str = (*loop)[4];
      lines.push_back({season, episode, timestamp, str});

      std::cout << lines.back().timestamp << ": " << lines.back().str << std::endl;
    }
  }

  void load_from_string(const std::string& contents) {
    std::stringstream s(contents);
    load_from_stream(s);
  }

  void load_from_filename(const std::string& filename) {
    std::ifstream f(filename);
    load_from_stream(f);
  }
};

char const* vlc_argv[] = { };
int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
static VLC vlc(vlc_argc, vlc_argv, "data/episode_720.mkv");

static class VideoArea : public sf::Drawable {
public:
  sf::Texture texture;
  sf::Sprite  sprite;
  sf::RectangleShape progressbar;

  VideoArea() {
    texture.create(VWIDTH, VHEIGHT);
    sprite.setTexture(texture);
    sprite.setPosition(0.f, 20.f);

    progressbar.setSize(sf::Vector2f(0, 4));
    progressbar.setFillColor(sf::Color::Red);
    progressbar.setPosition(0, 720 + 20);
  }

  void update(double progress, uint8_t* frame_data) {
    texture.update(frame_data);
    progressbar.setSize(sf::Vector2f(progress*1280, 4));
  }

private:
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
    ig::Begin("REPLACE WITH VIDEO FILE NAME");
    ig::Image(sprite);
    ig::End();
    // target.draw(sprite, states);
    // target.draw(progressbar, states);
  }
} video_area; 

int main(int argc, char* argv[]) {
  std::cout << "[EDITOR] Starting up..." << std::endl;

  window.setFramerateLimit(60);
  window.resetGLStates();

  Script script;
  script.load_from_filename("data/episode_720.csv");

  // int done = 0, action = 0, pause = 0, n = 0;

  igs::Init(window, false);
  ImGuiIO* io = &ImGui::GetIO();
  io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io->Fonts->Clear();
  io->Fonts->AddFontFromFileTTF("iosevka.ttf", 18.f);
  igs::UpdateFontTexture();
  ImGuiStyle* style = &ig::GetStyle();
  style->WindowRounding    = 0.f;
  style->ScrollbarRounding = 0.f;
  style->TabRounding       = 0.f;

  sf::Clock clock;

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      igs::ProcessEvent(event);

      switch (event.type) {
      case sf::Event::Closed:
        window.close();
        break;
      case sf::Event::KeyPressed:
        switch(event.key.code) {
        case sf::Keyboard::Space:
          if (state != PLAYING) {
            state = PLAYING; vlc.play();
          } else {
            state = PAUSED; vlc.pause();
          } 
          break; 
        case sf::Keyboard::Right:
          vlc.seek(vlc.timestamp + 5000);
          break;
        case sf::Keyboard::Left:
          vlc.seek(vlc.timestamp - 5000);
          break;
        break;
        }
      }
    }
  
    sf::Time dt = clock.restart();
    igs::Update(window, dt);

    if (ig::BeginMainMenuBar()) { 
      std::string str = std::to_string(vlc.timestamp);
      ig::Text(str.c_str());

      if (ig::BeginMenu("File")) {   
        if (ig::MenuItem("Save", "CTRL+S")) {
            
        }
        if (ig::MenuItem("Load Video", "CTRL+F")) {
          // popups["Load Video"] = true;
        }

        ig::EndMenu();
      }

      if (ig::BeginMenu("Edit")) {
        ig::EndMenu();
      }

      if (ig::BeginMenu("View")) {
        ig::EndMenu();
      }

      ig::EndMainMenuBar();
    }

    ig::ShowDemoWindow();

    /*ImGui::SetNextWindowPos( ImVec2(0,0) );
    ig::SetNextWindowBgAlpha(0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::Begin("BCKGND", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus );
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    ImGui::SetWindowSize(io->DisplaySize);
    ImGui::GetWindowDrawList()->AddText( ImVec2(400,400), ImColor(1.0f,1.0f,1.0f,1.0f), "Text in Background Layer" );
    ig::GetWindowDrawList()->AddText( ImVec2(0, 0), ImColor(1.f, 1.f, 1.f), "FUCK YOU");
    // ig::Text("This is a test");
    ImGui::End();
    ImGui::PopStyleVar();*/

    vlc.update(dt);
    // double progress = ((double)libvlc_media_player_get_time(vlc.player) / libvlc_media_player_get_length(vlc.player)) * 1280;
    video_area.update(vlc.percent_done, vlc.data);
    window.clear(sf::Color(0, 43, 54, 255));

    window.draw(video_area);
    igs::Render(window);

    window.display();
  }

  igs::Shutdown();
  
  return 0;
}