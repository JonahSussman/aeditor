/*
  I was completely baffled that there was no simple and easy to implement
  C++ library that harnesses the awesome power of ffmpeg. That's why I created
  this library. Easy FFMPEG or EFF for short. It's designed in such a way that 
  it's platform agnostic. The fundamental concept is you tell it where to
  find a video file and it gets to work spawning decoder threads, which then
  output to a queue.
  
  EFF requires C++17 and the full libav* suite.
*/

#include "easy_ffmpeg.hpp"

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
}

using namespace eff;

ECodec::ECodec() {
  stream_idx = -1;
}

ECodec::~ECodec() {
  printf("[EFF] ECodec deconstructor not implemented.\n");
}

int ECodec::load_from_stream(AVStream* stream) {
  AVCodecParameters* params = stream->codecpar;

  codec = avcodec_find_decoder(params->codec_id);
  if (codec == nullptr) {
    printf("[EFF] Unsupported codec\n");
    return -1;
  }

  codec_ctx = avcodec_alloc_context3(codec);
  if (codec_ctx == nullptr) {
    printf("[EFF] Failed to allocate codec context\n");
    return -1;
  }
  
  if (int r = avcodec_parameters_to_context(codec_ctx, params); r < 0) {
    printf("[EFF] Failed  to copy video params to context: %d\n", r);
    return false;
  }

  codec_ctx->request_sample_fmt = av_get_alt_sample_fmt(codec_ctx->sample_fmt, 0);

  time_base = stream->time_base;
}

int ECodec::open() {
  return avcodec_open2(codec_ctx, codec, nullptr);
}

Worker::Worker() {
  running = false;

  // packet = av_packet_alloc();
  // av_init_packet(packet);
}

Worker::~Worker() {
  printf("[EFF] Worker deconstructor not implemented.\n");
}

bool Worker::load(const char* filename) {
  av_register_all();

  format_ctx = avformat_alloc_context();
  if (format_ctx == nullptr) {
    fprintf(stderr, "[EFF] Error allocating format context\n");
  }

  if (int r = avformat_open_input(&format_ctx, filename, nullptr, nullptr); r != 0) {
    printf("[EFF] Could not open file. Code: %d\n", r);
    return false;
  }

  if (int r = avformat_find_stream_info(format_ctx, nullptr); r != 0) {
    printf("[EFF] Could not get stream info. Code: %d\n", r);
    return false;
  }

  av_dump_format(format_ctx, 0, filename, 0);

  for (int i = 0; i < format_ctx->nb_streams; ++i) {
    AVStream* stream = format_ctx->streams[i];
    if (stream->codecpar == nullptr) {
      printf("[EFF] Could not get codec params\n");
      return false;
    }

    if (video.stream_idx == -1 && stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      video.stream_idx = i;
      video.load_from_stream(stream);
      
    } else if (audio.stream_idx == -1 && stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audio.stream_idx = i;
      audio.load_from_stream(stream);
      audio.codec_ctx->request_sample_fmt = AV_SAMPLE_FMT_S16;
    }
  }

  if (int r = video.open(); r < 0) {
    printf("[EFF] Failed to open video codec: %d\n", r);
    return false;
  }
  if (int r = audio.open(); r < 0) {
    printf("[EFF] Failed to open audio codec: %d\n", r);
    return false;
  }

  this->filename = filename;
  return true;
} 

bool Worker::decode() {
  bool waiting_msg = false;
  AVPacket* packet = av_packet_alloc();
  av_init_packet(packet);

  while (running) {
    if (video_frame_queue.size() >= MAX_VIDEO_QUEUE_SIZE) {
      // if (!waiting_msg) fprintf(stderr, "[EFF] Video queue filled, waiting...\n");
      waiting_msg = true;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }
    if (audio_frame_queue.size() >= MAX_AUDIO_QUEUE_SIZE) {
      // if (!waiting_msg) fprintf(stderr, "[EFF] Audio queue filled, waiting...\n");
      waiting_msg = true;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    waiting_msg = false;

    if (int r = av_read_frame(format_ctx, packet); r < 0) {
      printf("[EFF] Error reading frame. Code: %d\n", r);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }
    // printf("[EFF] packet->data %p\n", _packet->data);
    // printf("[EFF] packet->pts %ld\n", _packet->pts);

    if (packet->stream_index == video.stream_idx) {
      decode_video(packet);
    } else if (packet->stream_index == audio.stream_idx) {
      decode_audio(packet);
    }
  }

  av_packet_free(&packet);
}

int Worker::decode_video(AVPacket* packet) {
  // Note, freeing must be done at calling site
  AVFrame* frame = av_frame_alloc();

  if (int r = avcodec_send_packet(video.codec_ctx, packet); r < 0) {
    printf("\033[1;31m[EFF],Sending packet failed: %d\033[0m\n", r);
    return r;
  }

  int r = avcodec_receive_frame(video.codec_ctx, frame);
  if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) {
    printf("[EFF] EAGIN or EOF. Skipping...\n");
    return r;
  } 
  if (r < 0) {
    printf("\033[1;31m[EFF] Receiving frame failed: %d\033[0m\n", r);
    return r;
  }

  video_mutex.lock();
  video_frame_queue.push(frame);
  video_mutex.unlock();
  video_cv.notify_all();
}

int Worker::decode_audio(AVPacket* packet) {
  if (int r = avcodec_send_packet(audio.codec_ctx, packet); r < 0) {
    printf("\033[1;31m[EFF],Sending packet failed: %d\033[0m\n", r);
    exit(1);
  }

  while (true) {
    AVFrame* frame = av_frame_alloc();
    int r = avcodec_receive_frame(audio.codec_ctx, frame);

    if (r == AVERROR(EAGAIN)) {
      return r;
    } else if(r == AVERROR_EOF) {
      printf("[EFF] Audio EOF. Skipping...\n");
      return r;
    } 
    if (r < 0) {
      printf("\033[1;31m[EFF] Receiving frame failed: %d\033[0m\n", r);
      return r;
    }

    audio_mutex.lock();
    audio_frame_queue.push(frame);
    audio_mutex.unlock();
    audio_cv.notify_all();
  }
}

// What happens for really short videos? Or if the queue is empty?
AVFrame* Worker::get_video_frame() {
  std::unique_lock<std::mutex> lock(video_mutex);
  video_cv.wait(lock, [&]{ return !video_frame_queue.empty(); });
  AVFrame* frame = video_frame_queue.front();
  video_frame_queue.pop();
  return frame;
}

AVFrame* Worker::get_audio_frame() {
  std::unique_lock<std::mutex> lock(audio_mutex);
  audio_cv.wait(lock, [&]{ return !audio_frame_queue.empty(); });
  AVFrame* frame = audio_frame_queue.front();
  audio_frame_queue.pop();
  return frame;
}

bool Worker::start() {
  running = true;
  decode_thread = std::thread(&Worker::decode, this);

  return true;
}

bool Worker::join() {
  running = false;
  decode_thread.join();

  return true;
}