#pragma once

#include "easy_ffmpeg.hpp"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <string>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libswresample/swresample.h>
}

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

namespace eff {
enum VideoState {
  STOPPED, PLAYING, PAUSED, LOOPING
};



class Video { // : public sf::Drawable {
public:
  class SFMLAudioStream : public sf::SoundStream {
  public:
    Video* video;
    Worker* worker;
    ECodec* audio;
    int channels;
    int sample_rate;
    SwrContext* swr_ctx;
    void init(Video* video);
  protected:
    int16_t getSample(const AVCodecContext* codecCtx, uint8_t* buffer, int sampleIndex);
    virtual bool onGetData(Chunk& data);
    virtual void onSeek(sf::Time timeOffset);
  };

  std::atomic<VideoState> state;

  std::atomic<bool> running;
  std::thread video_update_thread;
  std::mutex texture_mutex;
  std::mutex audio_mutex;
  std::condition_variable texture_cv;
  std::condition_variable audio_cv;

  sf::Clock clock;
  sf::Time  timestamp;

  Worker worker;

  sf::Texture texture;
  sf::Sprite sprite;
  SFMLAudioStream audio_stream;
  uint8_t* data;
  int width;
  int height;
  int bytes;

  AVFrame* video_frame;
  AVFrame* video_frame_RGB;
  SwsContext* sws_ctx;

  uint8_t* buffer;
  AVPacket packet;

  Video();
  ~Video();

  bool load(const char* filename);
  bool update_video_data();
  bool update_audio_data();
  bool play();
  bool pause();
  bool seek();

  void show(sf::RenderWindow& window);
};
}