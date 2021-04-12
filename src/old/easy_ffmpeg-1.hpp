/*
  I was completely baffled that there was no simple and easy to implement
  C++ library that harnesses the awesome power of ffmpeg. That's why I created
  this library. Easy FFMPEG or EFF for short. It's designed in such a way that 
  it's platform agnostic. The fundamental concept is you tell it where to
  find a video file and it gets to work spawning decoder threads, which then
  output to a queue.
  
  EFF requires C++17 and the full libav* suite.
*/

#pragma once

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
}

#define MAX_VIDEO_QUEUE_SIZE 100
#define MAX_AUDIO_QUEUE_SIZE 100

namespace eff {

class EFrame : public AVFrame {

};

class Worker {
public:
  const char* filename;
  std::queue<AVFrame*> audio_frame_queue;
  std::queue<AVFrame*> video_frame_queue;
  std::atomic<bool> running;
  std::thread decode_thread;
  std::mutex video_mutex;
  std::mutex audio_mutex;
  std::condition_variable video_cv;
  std::condition_variable audio_cv;

  AVFormatContext* format_ctx; 
  AVPacket* _packet;
  AVFrame*  global_frame;

  // Too c like!
  AVCodec* video_codec;
  AVCodecContext* video_codec_ctx;
  AVCodecParameters* video_codec_params;
  int video_stream_idx;

  AVCodec* audio_codec;
  AVCodecContext* audio_codec_ctx;
  AVCodecParameters* audio_codec_params;
  int audio_stream_idx;

  Worker();
  ~Worker();

  std::string shell(const char* cmd);

  bool load(const char* filename);
  bool start();
  bool join();
  bool close();
  AVFrame* get_video_frame();
  AVFrame* get_audio_frame();

  bool decode();
};
}