// Finally, after attempting to make a video editor... oh God was it almost FOUR
// years ago already? Anyway, after attemping to make an editor all those years
// ago, I finally made one that... semi-works? No guarantees that this will
// compile on your machine, fair warning.

#include <bits/stdc++.h>

#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "imgui.h"
#include "imgui-SFML.h"

#include <vlc/vlc.h>

#include "util.hpp"
#include "aeditor.hpp"
#include "services.hpp"

// A bunch of defines to make the code look more a e s t h e t i c
#define ig  ImGui
#define igs ImGui::SFML
#define sfe sf::Event
#define sfk sf::Keyboard
// #define wm  WindowManager

// Controls and shortcuts
// S        : Seek to ms 
// V        : Toggle video player
// \        : Edit current line
// CTRL + O : Open new file
// CTRL + S : Save the current csv file
// CTRL + L : Toggle script list


// TODO: Use ImGUI focus stuff to control state
// TODO: Create ImGUI Shell widget
// TODO: Load part of or all of the audio into memory to prevent stuttering
// TODO: Implement seeking
// TODO: Create video player widget
// TODO: Create Script and ascociated widget
// TODO: Actually implement backend with montreal forced aligner

// Static defines as these things are pretty much global to the program
// static Mode mode = Mode::UNLOADED;
static ImGuiIO* io;
static std::string filename_no_ext;

// Forward declarations
// TODO: Figure out better way of forward declaring this stuff
bool load_files(std::string str);
bool load_script(std::string str);
bool save_script(std::string str);
s64  next_timestamp(ae::Litr itr);
s64  prev_timestamp(ae::Litr itr);
ae::Loop bounds(ae::Litr itr);

// Window manager namespace
namespace wm {
  // Simple ImGui struct containing the stats about the inside of a window
  struct ContentRegion {
    ImVec2 min, max;
    double w, h;

    ContentRegion() {
      min = ig::GetWindowContentRegionMin();
      min.x += ig::GetWindowPos().x;
      min.y += ig::GetWindowPos().y;
      
      max = ig::GetWindowContentRegionMax();
      max.x += ig::GetWindowPos().x;
      max.y += ig::GetWindowPos().y;

      w = max.x - min.x;
      h = max.y - min.y;
    }
  };


  struct Window {
    bool show = false;
    virtual void draw() = 0;
  };

  // Popups

  static struct Error : public Window {
    char message[256];

    void error(std::string msg) {
      show = true;
      strcpy(message, msg.c_str());
    }

    virtual void draw() {
      ig::OpenPopup("Error!");
      if (ig::BeginPopupModal("Error!", &show)) {
        ig::Text("An error occurred!");
        ig::TextColored(ImVec4(1.f, .5f, .5f, 1.f), message);

        if (ig::Button("OK")) {
          show = false;
          // TODO: Find out if this is really necessary
          ig::CloseCurrentPopup();
        }
        ig::EndPopup();
      }
    }
  } error;

  static struct Load : public Window {
    char buf[256];

    void close() {
      show = false;
      ig::CloseCurrentPopup();
    }

    virtual void draw() {
      ig::OpenPopup("Load");
      if (ig::BeginPopupModal("Load", &show)) {
        ig::Text("Enter filename with no extension");
        ig::InputText("Name", buf, IM_ARRAYSIZE(buf));

        if (ig::Button("Auto-find")) {
          load_files(buf);
          close();
        }
        ig::SameLine();
        if (ig::Button("Load Script")){
          load_script(buf);
          close();
        }
        ig::SameLine();        
        if (ig::Button("data/episode_720")) {
          load_files("data/episode_720");
          close();
        }
        if (io->KeysDown[sfk::Escape]) {
          close();
        }

        ig::EndPopup();
      }
    }
  } load;

  static struct Seek : public Window {
    s64 timestamp;

    void close() {
      show = false;
      ig::CloseCurrentPopup();
    }

    virtual void draw() {
      ae::VLC* vlc = Locator::get_vlc_service();

      ig::OpenPopup("Seek");
      if (ig::BeginPopupModal("Seek", &show)) {
        ig::Text("Enter time to seek to (in ms)");
        ig::InputScalar("Time", ImGuiDataType_S64, &timestamp);
        
        bool seek = ig::Button("Seek") or io->KeysDown[sfk::Enter];
        if (seek) {
          vlc->seek(timestamp);
          vlc->pause();
        }
        if (seek or io->KeysDown[sfk::Escape]) close();

        ig::EndPopup();
      }
    }
  } seek;

  static struct Save : public Window {
    char buf[256];

    Save() {
      strcpy(buf, filename_no_ext.data());
    }

    void close() {
      show = false;
      ig::CloseCurrentPopup();
    }

    virtual void draw() {
      ig::OpenPopup("Save");
      if (ig::BeginPopupModal("Save", &show)) {
        ig::Text("Enter filename with no extension.");
        ig::Text("Note, if you name the csv file anything other than the");
        ig::Text("video filename, it will not be automatically loaded.");

        ig::InputText("Name", buf, IM_ARRAYSIZE(buf));

        bool submit = ig::Button("Submit") or io->KeysDown[sfk::Enter];
        if (submit) save_script(buf);
        if (submit or io->KeysDown[sfk::Escape]) close();

        ig::EndPopup();
      }
    }
  } save;

  static struct Aligner : public Window {
    std::future<std::string>* future;
    ae::Litr itr;
    ae::Loop loop;
    std::string command;

    void align(ae::Litr litr) {
      if (show) return;

      itr = litr;

      Locator::get_vlc_service()->seek((*itr).timestamp);
      Locator::get_vlc_service()->pause();

      loop = bounds(itr);
      float start = (float) loop.start / 1000.f;
      float end   = (float) loop.end / 1000.f;
      float length = end - start;

      std::cout << "HERE!" << std::endl;

      std::stringstream con;
      con << "wsl -d Ubuntu-18.04 echo \"" << (*itr).str << "\" > data/mfa_input/align.txt";
      printf("COMMAND: %s\n", con.str().c_str());
      util::shell(con.str().c_str());

      std::stringstream cmd;
      cmd << "wsl -d Ubuntu-18.04 ./scripts/align.sh " << filename_no_ext << " " << start << " " << length;
      printf("COMMAND: %s\n", cmd.str().c_str());
      command = cmd.str();
      future = new std::future<std::string>(std::async(std::launch::async, util::shell, command.c_str()));
      printf("FUTURE CREATED!\n");
      // util::shell(cmd.str().c_str());
      show = true;
    }

    virtual void draw() {
      ig::OpenPopup("Align");
      if (ig::BeginPopupModal("Align")){      
        if (future and util::future_ready(*future)) {
          auto script = Locator::get_script_service();
          std::ifstream f("data/mfa_output/align.csv");
          // FIXME: More robust way of handling that file is not present
          if (f.good()) {
            printf("F IS GOOD!\n");
            auto first_line = true;
            auto l = *itr;
            auto setback = 10;
            printf("L DEFINED!\n");
            script->erase(itr);
            printf("ERASED ITR!\n");

            for (util::CSVIterator c(f); c != util::CSVIterator(); ++c) {
              printf("%s\n", "LINE!");
              if (first_line) { first_line = false; continue; }
              if ((*c)[3] == "phones") break;

              s64 timestamp   = std::stof((*c)[0]) * 1000 + l.timestamp;
              if (timestamp > loop.end) timestamp = loop.end - setback--;
              std::string str = (*c)[2];

              script->add({ l.season, l.episode, timestamp, str });
            }
          } else {
            printf("F NOT GOOD BRUH!\n");
          }
          util::delnull(future);
          show = false;
        } else {
          ig::TextColored(ImVec4(1.f, .5f, .5f, 1.f), "Aligning. Please wait...");
        }

        ig::EndPopup();
      }
    }
  } aligner;

  // FIXME: Figure out how to make this more... better
  static struct EditLine : public Window {
    ae::Line l;
    ae::Litr itr;
    char buf[256];

    void update_and_show(s64 ms) {
      Locator::get_vlc_service()->pause();
      itr = Locator::get_script_service()->get(ms);
      l = *itr;
      strcpy(buf, l.str.c_str());

      show = true;
    }

    void update_and_show(ae::Litr it) {
      Locator::get_vlc_service()->pause();
      itr = it;
      l = *itr;
      strcpy(buf, l.str.c_str());

      show = true;
    }

    void push_changes() {
      auto script = Locator::get_script_service();
      l.str = buf;
      itr = script->update(itr, l)++;
      if (itr == script->end()) return;
      if (l.season == (*itr).season && l.season == (*itr).season) return;
      printf("UPDATING EPISODES AND SEASONS PAST THIS ONE! ");
      while(itr != script->end()) {
        auto a = *itr;
        a.episode = l.episode;
        a.season = l.season;
        script->update(itr, a);
        itr++;
      }
      printf("DONE!\n");
    }

    void close() {
      show = false;
      ig::CloseCurrentPopup();
    }

    virtual void draw() {
      ig::OpenPopup("Edit Line");
      if (ig::BeginPopupModal("Edit Line", &show)) {
        ig::InputText("str", buf, IM_ARRAYSIZE(buf));
        
        ig::InputScalar("timestamp (ms)", ImGuiDataType_S32, &l.timestamp);
        
        ig::SameLine();

        if (ig::Button("Current")) {
          l.timestamp = Locator::get_vlc_service()->get_ms();
        }
        ig::Columns(2);

        ig::InputScalar("season", ImGuiDataType_S32, &l.season);
        ig::NextColumn();
        ig::InputScalar("episode", ImGuiDataType_S32, &l.episode);

        ig::Columns(1);

        bool submit = ig::Button("Submit");
        if (submit) push_changes();
        if (submit or io->KeysDown[sfk::Escape]) close();

        ig::EndPopup();
      }
    }
  } edit_line;

  // Windows
  static struct Video : public Window {
    sf::Texture* texture;
    sf::Sprite*  sprite;
    char buf[256];

    Video() {
      show = true;

      texture = new sf::Texture();
      sprite  = new sf::Sprite();
      texture->create(1280, 720);
      texture->update(Locator::get_vlc_service()->get_data());
      sprite->setTexture(*texture);
    }

    void update_dimensions() {
      ae::VLC* vlc = Locator::get_vlc_service();
      ae::Info info = vlc->get_info();

      delete texture;
      delete sprite;

      texture = new sf::Texture();
      sprite  = new sf::Sprite();

      texture->create(info.width, info.height);
      ae::ctx_mutex.lock();
      texture->update(Locator::get_vlc_service()->get_data());
      ae::ctx_mutex.unlock();

      sprite->setTexture(*texture);
    }
    
    virtual void draw() {
      // TODO: Implement video player stuffs (overlays, navbar, etc...)
      ig::Begin("Video", &show);

      ContentRegion region;
      ImDrawList* draw_list = ig::GetWindowDrawList();
      double start;

      ae::VLC* vlc    = Locator::get_vlc_service();
      ae::Timestamp t = vlc->get_timestamp();
      ae::Line line   = *Locator::get_script_service()->get(t.ms);
      ae::Loop loop   = vlc->get_loop();

      if (Locator::mode != Mode::UNLOADED) {
        ig::Text((filename_no_ext + ".mkv").c_str());
      } else {
        ig::PushStyleColor(ImGuiCol_Text, ae::red_light);
        ig::Text("No video loaded.");
      }

      ae::ctx_mutex.lock();
      texture->update(vlc->get_data());
      ae::ctx_mutex.unlock();

      ig::SameLine();
      sprintf(buf, "ms: %*d", -7, t.ms);
      start = region.w - ig::CalcTextSize(buf).x;
      ig::SetCursorPosX(start);
      ig::Text(buf);

      ig::SameLine();
      sprintf(buf, "[%*d->%*d]", 7, loop.start, -7, loop.end);
      start = (region.w - ig::CalcTextSize(buf).x) / 2;
      ig::SetCursorPosX(start);
      ig::Text(buf); 

      if (Locator::mode == Mode::UNLOADED) ig::PopStyleColor();

      // Way too complicated cause I'm tired when writing this
      // FIXME: Make less verbose
      ImVec2 texture_pos = ig::GetCursorScreenPos();

      double max_h = region.max.y - texture_pos.y - 10;
      double max_w = region.max.x - texture_pos.x;

      double tex_w = texture->getSize().x;
      double tex_h = texture->getSize().y;
      double w_by_h = tex_w / tex_h;

      double h_via_w = max_w / w_by_h;
      double w_via_h = max_h * w_by_h;

      double r_w, r_h;
      
      if (w_via_h > max_w) {
        r_w = max_w; r_h = h_via_w;
      } else {
        r_w = w_via_h; r_h = max_h;
      }

      sprite->setScale(r_w / tex_w, r_h / tex_h);
      ig::Image(*sprite);

      ImVec2 pos = ig::GetCursorScreenPos();
      draw_list->AddRectFilled(pos, ImVec2(pos.x + t.fraction*r_w, pos.y + 10), IM_COL24(255, 255, 255));

      ig::PushFont(io->Fonts->Fonts[1]);
      strcpy(buf, line.str.c_str());
      ImVec2 text_size = ig::CalcTextSize(buf);
      // Technically not directly at the bottom but... eh...
      ImVec2 box_min = ImVec2((r_w - text_size.x) / 2 + region.min.x - 5, r_h - text_size.y / 2 + region.min.y);
      ImVec2 box_max = ImVec2((r_w + text_size.x) / 2 + region.min.x + 5, r_h + text_size.y / 2 + region.min.y);
      ig::GetWindowDrawList()->AddRectFilled(box_min, box_max, IM_COL32(0, 0, 0, 0x7f));
      ig::GetWindowDrawList()->AddText(ImVec2(box_min.x + 5, box_min.y), ae::fg0, buf);

      ig::PopFont();

      ig::End();
    }
  } video;

  // TODO: Make so that the scrollbar updates with the timestamp
  static struct LineSelector : public Window {
    char buf[256]; 
    ae::Litr right_click_itr;

    LineSelector() {
      show = true;
    }

    virtual void draw() {
      ae::Script* script = Locator::get_script_service();
      ae::VLC* vlc = Locator::get_vlc_service();
      auto current = script->get(vlc->get_ms());

      ig::Begin("Line Selector", &show);

      ig::Text("%s", (*current).str.c_str());

      ig::Separator();

      ig::BeginChild("Lines");

      ig::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
      ImGuiListClipper clipper(script->size());

      while (clipper.Step()) {
        int i = 0;
        for (auto itr = script->begin(); itr != script->end(); ++itr, ++i) {
          if (i < clipper.DisplayStart) continue;
          if (i >= clipper.DisplayEnd) break;

          if (itr == current) {
            ig::PushStyleColor(ImGuiCol_Header, ae::aqua_dark);
            if (Locator::mode == Mode::PLAYING) { ig::SetScrollHere(); }
          } else {
            ig::PushStyleColor(ImGuiCol_Header, i%2 ? ae::bg0 : ae::bg1);
          }
          
          sprintf(buf, "%*d:\t%s", 7, (*itr).timestamp, (*itr).str.c_str());
          // In order to get good highlighting, we use "not_selected"
          bool not_selected = true;
          ig::Selectable(buf, &not_selected);

          ig::PopStyleColor();

          if (ig::BeginPopupContextItem((*itr).str.c_str())) {
            sprintf(buf, "%*d", -7, (*itr).timestamp);
            ig::Text(buf);

            ig::Separator();
            
            if (ig::Selectable("Set to current")) {
              ae::Line l = *itr;
              l.timestamp = vlc->get_ms();
              script->update(itr, l);
            }
            if (ig::Selectable("Edit")) edit_line.update_and_show(itr);
            if (ig::Selectable("Explode"));
            if (ig::Selectable("Align")) wm::aligner.align(itr);
            if (ig::Selectable("Delete")) script->erase(itr);

            ig::EndPopup();
          }

          if (!not_selected) {
            vlc->seek((*itr).timestamp);
            if (vlc->is_looping()) {
              vlc->set_loop(bounds(itr));
            } else {
              vlc->pause();
            }
          }
        }
      }
      ig::PopStyleVar();

      ig::EndChild();
      ig::End();
    }
  } line_selector;

  static struct Console : public Window {
    FILE* wsl;
    char buf[256];
    std::vector<std::string> history;
    bool enter = false;
    std::future<std::string>* future;

    Console() {
      show = true;
      
      wsl = popen ("wsl", "w");
    }

    ~Console() {
      pclose(wsl);
    }

    std::string exec(std::string cmd) {

    }

    virtual void draw() {
      ig::Begin("Console", &show);

      float m = ig::GetStyle().ItemSpacing.y + ig::GetFrameHeightWithSpacing();

      ig::BeginChild("Console List", ImVec2(0, -m));
      
      ImGuiListClipper clipper(history.size());
      while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
          ig::Text(history[i].c_str());
        }
      }

      if (future and util::future_ready(*future)) {
        ig::SetScrollHere();

        std::stringstream ss(future->get());
        std::string to;

        while(std::getline(ss, to, '\n')){
          history.push_back(to);
        }

        util::delnull(future);
        strcpy(buf, "");
      }

      ig::EndChild();

      ig::Separator();

      if (future) {
        ig::Text("Running...");
      } else {
        ig::InputText("Input", buf, IM_ARRAYSIZE(buf));
      }

      if (!future and !enter and io->KeysDown[sfk::Enter]) {
        future = new std::future<std::string>(std::async(std::launch::async, util::shell, buf));
        enter = true;
      } else {
        enter = false;
      }

      ig::End();
    }
  } console;

  void update(sf::RenderWindow& window, sf::Time dt) {
    igs::Update(window, dt);

    if (ig::BeginMainMenuBar()) { 
      if (ig::BeginMenu("File")) {   
        ig::MenuItem("Save", "CTRL+S", &wm::save.show);
        ig::MenuItem("Load Video", "CTRL+O", &wm::load.show);

        ig::EndMenu();
      }

      if (ig::BeginMenu("Edit")) {
        ig::MenuItem("Seek", "S", &wm::seek.show);
        if (ig::MenuItem("Current Line", "\\", &wm::edit_line.show)) {
          if (wm::edit_line.show) {
            edit_line.update_and_show(Locator::get_vlc_service()->get_ms());
          }
        }

        ig::EndMenu();
      }

      if (ig::BeginMenu("View")) {
        ig::MenuItem("Video", "V", &wm::video.show);
        ig::MenuItem("Line Selector", "CTRL+L", &wm::line_selector.show);
        ig::MenuItem("Console", "", &wm::console.show);

        ig::EndMenu();
      }

      float fps = io->Framerate;

      std::stringstream metric;
      metric << std::fixed << std::setprecision(2) << 1000.0f / fps;
      metric << " ms/frame (" << std::setprecision(1) << fps << " fps)";

      double size = ig::CalcTextSize(metric.str().c_str()).x;

      // std::string metric = "Application average %.3f ms/frame (%.1f FPS)", , ig::GetIO().Framerate
      ig::SetCursorPosX(window.getSize().x - size);
      ig::Text(metric.str().c_str());

      ig::EndMainMenuBar();
    }

    if (video.show) video.draw();
    if (line_selector.show) line_selector.draw();
    if (console.show) console.draw();

    if (error.show)     error.draw();
    if (load.show)      load.draw();
    if (seek.show)      seek.draw();
    if (save.show)      save.draw();
    if (edit_line.show) edit_line.draw();
    if (aligner.show)   aligner.draw();
    // ig::ShowDemoWindow();
  }
}

// Global functions here. 
bool load_files(std::string str) {
  if (Locator::mode != Mode::UNLOADED) {
    Locator::free_vlc_service();

    save_script(str);
    Locator::free_script_service();
  }

  ae::VLC* attempted_vlc_service = new ae::LoadedVLC();

  if (!attempted_vlc_service->initialize(str + ".mkv", {"--no-sub-autodetect-file"})) {
    printf("could not load video file %s\n", str + ".mkv");
    return false;
  }

  Locator::provide_vlc_service(attempted_vlc_service);

  wm::video.update_dimensions();

  if (!load_script(str)) return false;

  filename_no_ext = str;

  printf("FILENAMENOEXT: %s", filename_no_ext.c_str());

  return true;
}

bool load_script(std::string str) {
  ae::Script* attempted_script_service = new ae::LoadedScript();

  if (!attempted_script_service->initialize(str + ".csv")) {
    printf("could not load script file %s\n", str + ".csv");
    return false;
  }

  Locator::provide_script_service(attempted_script_service);

  return true;
}

bool save_script(std::string str) {
  std::fstream f;
  std::string filename = str + ".csv";

  ae::Script* script = Locator::get_script_service();

  if (util::exists(filename)) {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    auto sec = std::to_string(dur);
    auto new_filename = str + "_" + sec + ".csv";
    util::rename(filename, new_filename);
  }

  f.open(filename, std::fstream::app);

  if (f.is_open()) {  
    for (auto itr = script->begin(); itr != script->end(); ++itr) {
      auto l = *itr;

      f << l.season << "," << l.episode << "," << l.timestamp << ",," << l.str << "\n";
    }

    f.close();
    
    return true;
  } else {
    std::string message = "Could not save to file " + filename + "!";
    wm::error.error(message);
    return false;
  }
}

s64 next_timestamp(ae::Litr itr) {
  auto script = Locator::get_script_service();
  auto vlc    = Locator::get_vlc_service();
  auto next   = ++itr;
  
  return next != script->end() ? (*next).timestamp : vlc->get_info().length;
}

s64 prev_timestamp(ae::Litr itr) {
  auto script = Locator::get_script_service();
  auto vlc    = Locator::get_vlc_service();
  auto prev   = --itr;

  return prev != script->begin() ? (*prev).timestamp : 0;
}

ae::Loop bounds(ae::Litr itr) {
  return { (*itr).timestamp, next_timestamp(itr) };
}


int main(int argc, char* argv[]) {
  const int no_close = sf::Style::Titlebar | sf::Style::Close;
  sf::RenderWindow window(sf::VideoMode(1600, 900), L"æditor", no_close);
  
  // Vertical sync
  window.setFramerateLimit(60);
  window.resetGLStates();

  igs::Init(window, false);
  ae::ig_style();

  io = &ig::GetIO();

  sf::Clock clock;

  while(window.isOpen()) {
    sf::Event e;

    ae::VLC* vlc = Locator::get_vlc_service();
    ae::Script* script = Locator::get_script_service();
    ae::Litr litr = script->get(vlc->get_ms());
    ae::Line line;
    ae::Loop loop;

    while(window.pollEvent(e)) {
      igs::ProcessEvent(e);

      switch(e.type) {
        case sfe::Closed:
          window.close();
          break;
        case sfe::KeyPressed:
          if (!io->WantCaptureKeyboard) {
            switch(e.key.code) {
            case sfk::Space:
              if (Locator::mode == Mode::PLAYING) {
                vlc->pause();
              } else if (Locator::mode == Mode::PAUSED) {
                vlc->play();
              }
              break;
            case sfk::Left:
              vlc->seek(vlc->get_ms() - 5000);
              break;
            case sfk::Right:
              vlc->seek(vlc->get_ms() + 5000);
              break;
            case sfk::Up:
            case sfk::Down:
              if (e.key.code == sfk::Up) {
                if (litr != script->begin()) { litr--; }
              } else {
                litr++;
                if (litr == script->end()) { litr--; }
              }

              vlc->seek((*litr).timestamp);

              if (vlc->is_looping()) {
                vlc->set_loop(bounds(litr));
              }
              break;
            // FIXME: This looks ugly af.
            case sfk::RBracket:
            case sfk::LBracket:
              if (io->KeyShift) litr++;
              line = *litr;
              
              if (io->KeyAlt) {
                line.timestamp += e.key.code == sfk::RBracket ? 10 : -10;
              } else {
                line.timestamp += e.key.code == sfk::RBracket ? 50 : -50;
              }
              litr = script->update(litr, line);

              loop = vlc->get_loop();
              
              if (io->KeyShift) {
                if (vlc->is_looping()) {
                  loop = { loop.start, line.timestamp };
                  if (loop.start > loop.end) {
                    loop.start = prev_timestamp(litr);
                  }
                  vlc->set_loop(loop);
                }
                litr--;
              } else {
                if (vlc->is_looping()) {
                  loop = { line.timestamp, loop.end };
                  if (loop.start > loop.end) {
                    loop.end = next_timestamp(litr);
                  }
                  vlc->set_loop(loop);
                }
                vlc->seek(line.timestamp);
              }
              break;
            case sfk::BackSlash:
              if (io->KeyShift) {
                line = *litr;
                line.timestamp = vlc->get_ms();
                litr = script->update(litr, line);
              } else {
                wm::edit_line.update_and_show(vlc->get_ms());
              }
              break;
            case sfk::Delete:
              litr = --script->erase(litr);
              // litr = script->get(vlc->get_ms());
              break;
            case sfk::A:
              if (io->KeyCtrl) {
                if (io->KeyAlt && io->KeyShift) {
                  // std::cout << "FULL SEND!" << std::endl;
                  // wm::aligner.full_align(litr);
                } else {
                  std::cout << "ALIGN INITIATED!" << std::endl;
                  wm::aligner.align(litr);
                }
              }
              break;
            case sfk::C:
              line = *litr;
              line.timestamp = vlc->get_ms();
              litr = script->update(litr, line);
              break;
            case sfk::L:
              if (io->KeyCtrl) {
                wm::line_selector.show = !wm::line_selector.show;
              } else if (vlc->is_looping()) {
                vlc->reset_loop();
              } else {
                vlc->set_loop(bounds(litr));
              }
              break;
            case sfk::N:
              if (io->KeyCtrl) {
                line = *litr;
                line.str = "*";
                line.timestamp = vlc->get_ms();
                auto new_litr = script->add(line);
                if (vlc->is_looping()) {
                  vlc->set_loop(bounds(litr));
                }
                litr = new_litr;
                wm::edit_line.update_and_show(vlc->get_ms());
              }
              break;
            case sfk::O:
              if (io->KeyCtrl) {
                wm::load.show = true;
              }
              break;
            case sfk::S:
              if (io->KeyCtrl) {
                wm::save.show = true;
              } else {
                wm::seek.show = true;
              }
              break;
            case sfk::V:
              wm::video.show = !wm::video.show;
              break;
            case sfk::Z:
              if (io->KeyCtrl){
                script->add(script->pop_deleted());
              }
              break;
            }
          }
          break;
      }
    }

    sf::Time dt = clock.restart();

    if (Locator::mode == Mode::PLAYING) {
      Locator::get_vlc_service()->update(dt);
    }

    wm::update(window, dt);

    window.clear(sf::Color(29, 32, 33));

    igs::Render(window);

    window.display();
  }

  save_script("data/ayyouforgottosaveafterclosing");

  igs::Shutdown();

  return 0;
}