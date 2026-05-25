#pragma once

#include <future>
#include <mutex>
#include <string>
#include <vector>

#include "service_script.hpp"
#include "service_vlc.hpp"

namespace ae {
  class Tools {
  public:
    virtual ~Tools() = default;

    virtual std::future<bool> align(Line line, Loop bounds, const std::string& filename) = 0;
    virtual std::vector<Line> consume_align_results() = 0;

    virtual std::future<bool> transcribe(Loop bounds, const std::string& filename) = 0;
    virtual std::string consume_transcribe_result() = 0;

    virtual std::future<bool> render(const std::string& manifest, const std::string& output) = 0;

    virtual std::string get_status() = 0;
  };

  class NullTools : public Tools {
  public:
    std::future<bool> align(Line line, Loop bounds, const std::string& filename) override;
    std::vector<Line> consume_align_results() override;
    std::future<bool> transcribe(Loop bounds, const std::string& filename) override;
    std::string consume_transcribe_result() override;
    std::future<bool> render(const std::string& manifest, const std::string& output) override;
    std::string get_status() override;
  };

  class LoadedTools : public Tools {
  public:
    std::future<bool> align(Line line, Loop bounds, const std::string& filename) override;
    std::vector<Line> consume_align_results() override;
    std::future<bool> transcribe(Loop bounds, const std::string& filename) override;
    std::string consume_transcribe_result() override;
    std::future<bool> render(const std::string& manifest, const std::string& output) override;
    std::string get_status() override;
  private:
    std::vector<Line> align_results;
    std::string transcribe_result;
    std::mutex status_mutex;
    std::string status;
    void set_status(const std::string& s);
    void shell_with_status(const char* cmd);
  };
}
