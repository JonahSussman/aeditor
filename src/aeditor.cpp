// Finally, after attempting to make a video editor... oh God was it almost FOUR
// years ago already? Anyway, after attemping to make an editor all those years
// ago, I finally made one that... semi-works? No guarantees that this will
// compile on your machine, fair warning.

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "imgui.h"
#include "imgui_internal.h"
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
    std::future<bool> future;
    bool has_future = false;
    ae::Litr itr;
    ae::Loop loop;

    void align(ae::Litr litr) {
      if (show) return;

      itr = litr;
      Locator::get_vlc_service()->seek((*itr).timestamp);
      Locator::get_vlc_service()->pause();

      loop = bounds(itr);
      auto tools = Locator::get_tools_service();
      future = tools->align(*itr, loop, filename_no_ext);
      has_future = true;
      show = true;
    }

    virtual void draw() {
      ig::OpenPopup("Align");
      if (ig::BeginPopupModal("Align")) {
        if (has_future && util::future_ready(future)) {
          bool ok = future.get();
          has_future = false;

          if (ok) {
            auto script = Locator::get_script_service();
            auto tools = Locator::get_tools_service();
            auto results = tools->consume_align_results();
            auto l = *itr;
            script->erase(itr);

            auto setback = 10;
            for (auto& r : results) {
              if (r.timestamp > loop.end)
                r.timestamp = loop.end - setback--;
              script->add(r);
            }
          }
          show = false;
        } else {
          auto status = Locator::get_tools_service()->get_status();
          ig::TextColored(ImVec4(1.f, .5f, .5f, 1.f), "%s", status.c_str());
        }
        ig::EndPopup();
      }
    }
  } aligner;

  static struct Transcriber : public Window {
    std::future<bool> future;
    bool has_future = false;
    ae::Litr itr;

    void transcribe(ae::Litr litr) {
      if (show) return;

      itr = litr;
      Locator::get_vlc_service()->seek((*itr).timestamp);
      Locator::get_vlc_service()->pause();

      auto tools = Locator::get_tools_service();
      future = tools->transcribe(bounds(itr), filename_no_ext);
      has_future = true;
      show = true;
    }

    virtual void draw() {
      ig::OpenPopup("Transcribe");
      if (ig::BeginPopupModal("Transcribe")) {
        if (has_future && util::future_ready(future)) {
          bool ok = future.get();
          has_future = false;

          if (ok) {
            auto script = Locator::get_script_service();
            auto tools = Locator::get_tools_service();
            auto text = tools->consume_transcribe_result();
            auto l = *itr;
            l.str = text;
            script->update(itr, l);
          }
          show = false;
        } else {
          auto status = Locator::get_tools_service()->get_status();
          ig::TextColored(ImVec4(1.f, .5f, .5f, 1.f), "%s", status.c_str());
        }
        ig::EndPopup();
      }
    }
  } transcriber;

  static struct Renderer : public Window {
    std::future<bool> future;
    bool has_future = false;

    static std::string sanitize(const std::string& s) {
      std::string out;
      for (char c : s) if (c != '\'') out += c;
      return out;
    }

    void render() {
      if (show) return;

      auto script = Locator::get_script_service();
      auto tools  = Locator::get_tools_service();

      struct Job {
        std::string word;
        s64 timestamp;
        float start_sec;
        float length_sec;
        int count;
        int total;
        int season;
        int episode;
      };

      std::vector<Job> jobs;
      for (auto itr = script->begin(); itr != script->end(); ++itr) {
        auto& line = *itr;
        if (line.str == "FIRST ELEMENT") continue;

        s64 duration_ms = next_timestamp(itr) - line.timestamp;
        if (duration_ms <= 0) continue;

        jobs.push_back({
          line.str,
          line.timestamp,
          line.timestamp / 1000.f,
          duration_ms / 1000.f,
          0, 0,
          line.season,
          line.episode
        });
      }

      std::sort(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        std::string al = a.word, bl = b.word;
        std::transform(al.begin(), al.end(), al.begin(), ::tolower);
        std::transform(bl.begin(), bl.end(), bl.begin(), ::tolower);
        if (al != bl) return al < bl;
        return a.timestamp < b.timestamp;
      });

      std::map<std::string, int> word_counts;
      for (int i = 0; i < (int)jobs.size(); i++) {
        std::string key = jobs[i].word;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        word_counts[key]++;
        jobs[i].count = word_counts[key];
        jobs[i].total = i + 1;
      }

      int n = (int)jobs.size();
      if (n == 0) return;

      std::string font = "${RENDER_FONT:-/home/jonah/.fonts/iosevka.ttf}";
      std::string branding = "${RENDER_BRANDING:-inothernews1}";

      // Write filter script
      std::ofstream filter("data/render_filter.txt");
      for (int i = 0; i < n; i++) {
        auto& j = jobs[i];
        std::string w = sanitize(j.word);
        filter << "[" << i << ":v]"
               << "drawtext=fontfile='$FONT':fontsize=75:fontcolor=#ffffff"
               << ":text='" << w << "':x=(w-tw)/2:y=h-th-20"
               << ":box=1:boxcolor=#0000007f:boxborderw=5,"
               << "drawtext=fontfile='$FONT':fontsize=30:fontcolor=#ffffff"
               << ":text='Times said\\: " << j.count << "':x=20:y=h-th-20"
               << ":box=1:boxcolor=#0000007f:boxborderw=5,"
               << "drawtext=fontfile='$FONT':fontsize=30:fontcolor=#ffffff"
               << ":text='Total words\\: " << j.total << "':x=20:y=20"
               << ":box=1:boxcolor=#0000007f:boxborderw=5,"
               << "drawtext=fontfile='$FONT':fontsize=30:fontcolor=#ffffff"
               << ":text='S" << j.season << " E" << j.episode << "':x=w-tw-20:y=20"
               << ":box=1:boxcolor=#0000007f:boxborderw=5,"
               << "drawtext=fontfile='$FONT':fontsize=30:fontcolor=#ffffff"
               << ":text='$BRANDING':x=w-tw-20:y=h-th-20"
               << ":box=1:boxcolor=#0000007f:boxborderw=5"
               << "[v" << i << "]; "
               << "[" << i << ":a]anull[a" << i << "];\n";
      }

      // Concat line
      for (int i = 0; i < n; i++) filter << "[v" << i << "][a" << i << "]";
      filter << "concat=n=" << n << ":v=1:a=1[outv][outa]\n";
      filter.close();

      // Write command script
      std::ofstream cmd("data/render_cmd.sh");
      cmd << "#!/bin/sh\n";
      cmd << "FONT=\"" << font << "\"\n";
      cmd << "BRANDING=\"" << branding << "\"\n";
      cmd << "export FONT BRANDING\n";
      cmd << "sed -i \"s|\\$FONT|$FONT|g;s|\\$BRANDING|$BRANDING|g\" data/render_filter.txt\n";
      cmd << "echo \"Starting render of " << n << " clips...\"\n";
      cmd << "ffmpeg -nostdin -hide_banner \\\n";
      for (int i = 0; i < n; i++) {
        cmd << "  -ss " << jobs[i].start_sec
            << " -t " << jobs[i].length_sec
            << " -i \"" << filename_no_ext << ".mkv\" \\\n";
      }
      cmd << "  -filter_complex_script data/render_filter.txt \\\n";
      cmd << "  -map \"[outv]\" -map \"[outa]\" \\\n";
      cmd << "  -c:v libx264 -c:a aac -ar 44100 -ac 1 \\\n";
      cmd << "  -avoid_negative_ts make_zero \\\n";
      cmd << "  -progress pipe:1 \\\n";
      cmd << "  data/render_output.mkv -y 2>&1\n";
      cmd << "echo \"Done.\"\n";
      cmd.close();

      future = tools->render("data/render_cmd.sh", "data/render_output.mkv");
      has_future = true;
      show = true;
    }

    virtual void draw() {
      ig::OpenPopup("Render");
      if (ig::BeginPopupModal("Render")) {
        if (has_future && util::future_ready(future)) {
          future.get();
          has_future = false;
          show = false;
        } else {
          auto status = Locator::get_tools_service()->get_status();
          ig::TextColored(ImVec4(1.f, .5f, .5f, 1.f), "%s", status.c_str());
        }
        ig::EndPopup();
      }
    }
  } renderer;

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
      ImGuiListClipper clipper;
      clipper.Begin(script->size());

      while (clipper.Step()) {
        int i = 0;
        for (auto itr = script->begin(); itr != script->end(); ++itr, ++i) {
          if (i < clipper.DisplayStart) continue;
          if (i >= clipper.DisplayEnd) break;

          if (itr == current) {
            ig::PushStyleColor(ImGuiCol_Header, ae::aqua_dark);
            if (Locator::mode == Mode::PLAYING) { ig::SetScrollHereY(); }
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
            if (ig::Selectable("Transcribe")) wm::transcriber.transcribe(itr);
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

  static struct Help : public Window {
    virtual void draw() {
      ig::SetNextWindowSizeConstraints(ImVec2(420, 0), ImVec2(FLT_MAX, FLT_MAX));
      ig::Begin("Help", &show, ImGuiWindowFlags_AlwaysAutoResize);
      ig::Text("Keyboard Shortcuts");
      ig::Separator();

      ig::Columns(2, "help_cols");
      ig::SetColumnWidth(0, 200);

      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "Space");       ig::NextColumn(); ig::Text("Play / Pause");           ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "Left / Right"); ig::NextColumn(); ig::Text("Seek +/- 5 seconds");     ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "Up / Down");    ig::NextColumn(); ig::Text("Previous / next line");    ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "S");            ig::NextColumn(); ig::Text("Seek to timestamp");       ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "V");            ig::NextColumn(); ig::Text("Toggle video window");     ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "L");            ig::NextColumn(); ig::Text("Toggle loop");             ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "C");            ig::NextColumn(); ig::Text("Set timestamp to now");    ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "\\");           ig::NextColumn(); ig::Text("Edit current line");       ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "Shift+\\");     ig::NextColumn(); ig::Text("Update timestamp");        ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "[ / ]");        ig::NextColumn(); ig::Text("Adjust timestamp +/- 50ms"); ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "Alt+[ / ]");    ig::NextColumn(); ig::Text("Adjust timestamp +/- 10ms"); ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "Delete");       ig::NextColumn(); ig::Text("Delete current line");     ig::NextColumn();

      ig::Separator();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "Ctrl+O");       ig::NextColumn(); ig::Text("Open file");               ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "Ctrl+S");       ig::NextColumn(); ig::Text("Save CSV");                ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "Ctrl+N");       ig::NextColumn(); ig::Text("New line at current time"); ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "Ctrl+L");       ig::NextColumn(); ig::Text("Toggle line selector");    ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "Ctrl+A");       ig::NextColumn(); ig::Text("Align (MFA)");             ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "Ctrl+T");       ig::NextColumn(); ig::Text("Transcribe (Whisper)");    ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "Ctrl+R");       ig::NextColumn(); ig::Text("Render compilation");      ig::NextColumn();
      ig::TextColored(ImVec4(.5f, .8f, .5f, 1.f), "Ctrl+Z");       ig::NextColumn(); ig::Text("Undo last delete");        ig::NextColumn();

      ig::Columns(1);
      ig::End();
    }
  } help;

  static struct Console : public Window {
    char buf[256];
    std::vector<std::string> history;
    bool enter = false;
    std::future<std::string>* future = nullptr;

    Console() {
      show = false;
    }

    virtual void draw() {
      ig::Begin("Console", &show);

      float m = ig::GetStyle().ItemSpacing.y + ig::GetFrameHeightWithSpacing();

      ig::BeginChild("Console List", ImVec2(0, -m));
      
      ImGuiListClipper clipper;
      clipper.Begin(history.size());
      while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
          ig::Text(history[i].c_str());
        }
      }

      if (future and util::future_ready(*future)) {
        ig::SetScrollHereY();

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
        if (ig::MenuItem("Render", "CTRL+R")) {
          wm::renderer.render();
        }

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
        ig::Separator();
        ig::MenuItem("Help", "F1", &wm::help.show);

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
    if (help.show) help.draw();

    if (error.show)     error.draw();
    if (load.show)      load.draw();
    if (seek.show)      seek.draw();
    if (save.show)      save.draw();
    if (edit_line.show) edit_line.draw();
    if (aligner.show)      aligner.draw();
    if (transcriber.show)  transcriber.draw();
    if (renderer.show)     renderer.draw();

    // Clamp all windows so they stay on screen when the main window resizes
    ImVec2 display = io->DisplaySize;
    for (int i = 0; i < ig::GetCurrentContext()->Windows.Size; i++) {
      ImGuiWindow* w = ig::GetCurrentContext()->Windows[i];
      if (!w->Active || w->Flags & ImGuiWindowFlags_NoMove) continue;

      ImVec2 pos = w->Pos;
      ImVec2 size = w->Size;
      bool changed = false;

      if (pos.x + size.x > display.x) { pos.x = display.x - size.x; changed = true; }
      if (pos.y + size.y > display.y) { pos.y = display.y - size.y; changed = true; }
      if (pos.x < 0) { pos.x = 0; changed = true; }
      if (pos.y < 0) { pos.y = 0; changed = true; }

      if (changed) ig::SetWindowPos(w, pos);
    }
  }
}

// Global functions here. 
bool load_files(std::string str) {
  if (Locator::mode != Mode::UNLOADED) {
    Locator::free_vlc_service();
    Locator::free_tools_service();

    save_script(str);
    Locator::free_script_service();
  }

  ae::VLC* attempted_vlc_service = new ae::LoadedVLC();

  if (!attempted_vlc_service->initialize(str + ".mkv", {"--no-sub-autodetect-file"})) {
    printf("could not load video file %s\n", (str + ".mkv").c_str());
    return false;
  }

  Locator::provide_vlc_service(attempted_vlc_service);
  Locator::provide_tools_service(new ae::LoadedTools());

  wm::video.update_dimensions();

  if (!load_script(str)) return false;

  filename_no_ext = str;

  printf("FILENAMENOEXT: %s", filename_no_ext.c_str());

  return true;
}

bool load_script(std::string str) {
  ae::Script* attempted_script_service = new ae::LoadedScript();

  if (!attempted_script_service->initialize(str + ".csv")) {
    printf("could not load script file %s\n", (str + ".csv").c_str());
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
  sf::RenderWindow window(sf::VideoMode(1600, 900), L"æditor", sf::Style::Default);
  
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
                wm::aligner.align(litr);
              }
              break;
            case sfk::T:
              if (io->KeyCtrl) {
                wm::transcriber.transcribe(litr);
              }
              break;
            case sfk::R:
              if (io->KeyCtrl) {
                wm::renderer.render();
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
            case sfk::F1:
              wm::help.show = !wm::help.show;
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