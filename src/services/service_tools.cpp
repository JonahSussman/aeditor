#include "service_tools.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <future>
#include <sstream>

#include "../util.hpp"

namespace ae {
  std::future<bool> NullTools::align(Line, Loop, const std::string&) {
    std::promise<bool> p;
    p.set_value(false);
    return p.get_future();
  }
  std::vector<Line> NullTools::consume_align_results() { return {}; }

  std::future<bool> NullTools::transcribe(Loop, const std::string&) {
    std::promise<bool> p;
    p.set_value(false);
    return p.get_future();
  }
  std::string NullTools::consume_transcribe_result() { return ""; }
  std::future<bool> NullTools::render(const std::string&, const std::string&) {
    std::promise<bool> p;
    p.set_value(false);
    return p.get_future();
  }
  std::string NullTools::get_status() { return ""; }

  void LoadedTools::set_status(const std::string& s) {
    std::lock_guard<std::mutex> lock(status_mutex);
    status = s;
  }

  std::string LoadedTools::get_status() {
    std::lock_guard<std::mutex> lock(status_mutex);
    return status;
  }

  void LoadedTools::shell_with_status(const char* cmd) {
    std::array<char, 256> buffer;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return;

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
      std::string line(buffer.data());
      while (!line.empty() && line.back() == '\n') line.pop_back();
      if (!line.empty()) {
        set_status(line);
      }
    }
  }

  std::future<bool> LoadedTools::align(Line line, Loop bounds, const std::string& filename) {
    float start = bounds.start / 1000.f;
    float duration = (bounds.end - bounds.start) / 1000.f;

    std::ofstream txt("data/mfa_input/align.txt");
    txt << line.str;
    txt.close();

    std::stringstream cmd;
    cmd << "scripts/align.sh " << filename << " " << start << " " << duration;
    std::string command = cmd.str();

    set_status("[1/4] Checking MFA installation...");

    return std::async(std::launch::async, [this, line, command]() -> bool {
      shell_with_status(command.c_str());

      set_status("Parsing results...");
      std::ifstream f("data/mfa_output/align.csv");
      if (!f.good()) { set_status("Alignment failed."); return false; }

      align_results.clear();
      bool first_line = true;
      for (util::CSVIterator c(f); c != util::CSVIterator(); ++c) {
        if (first_line) { first_line = false; continue; }
        if ((*c)[3] == "phones") break;

        s64 timestamp = std::stof((*c)[0]) * 1000 + line.timestamp;
        std::string str = (*c)[2];
        align_results.push_back({ line.season, line.episode, timestamp, str });
      }

      set_status("Done.");
      return !align_results.empty();
    });
  }

  std::vector<Line> LoadedTools::consume_align_results() {
    std::vector<Line> results;
    std::swap(results, align_results);
    return results;
  }

  std::future<bool> LoadedTools::transcribe(Loop bounds, const std::string& filename) {
    float start = bounds.start / 1000.f;
    float duration = (bounds.end - bounds.start) / 1000.f;

    std::stringstream cmd;
    cmd << "scripts/transcribe.sh " << filename << " " << start << " " << duration;
    std::string command = cmd.str();

    set_status("[1/3] Extracting audio segment...");

    return std::async(std::launch::async, [this, command]() -> bool {
      shell_with_status(command.c_str());

      set_status("Reading result...");
      std::ifstream f("data/transcribe_output.txt");
      if (!f.good()) { set_status("Transcription failed."); return false; }

      std::stringstream buf;
      buf << f.rdbuf();
      transcribe_result = buf.str();

      while (!transcribe_result.empty() && transcribe_result.back() == '\n')
        transcribe_result.pop_back();

      set_status("Done.");
      return !transcribe_result.empty();
    });
  }

  std::string LoadedTools::consume_transcribe_result() {
    std::string result;
    std::swap(result, transcribe_result);
    return result;
  }

  std::future<bool> LoadedTools::render(const std::string& manifest, const std::string& output) {
    std::stringstream cmd;
    cmd << "scripts/render_all.sh " << manifest << " " << output;
    std::string command = cmd.str();

    set_status("Starting render...");

    return std::async(std::launch::async, [this, command, output]() -> bool {
      shell_with_status(command.c_str());

      std::ifstream f(output);
      bool ok = f.good();
      set_status(ok ? "Render complete." : "Render failed.");
      return ok;
    });
  }
}
