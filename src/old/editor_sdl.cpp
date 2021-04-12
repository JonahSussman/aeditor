#include <bits/stdc++.h>

// #include "imgui.h"

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>

  #include <SDL2/SDL.h>
  #include <SDL2/SDL_opengl.h>
  #include <SDL2/SDL_thread.h>
}

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900

#define FF_REFRESH_EVENT (SDL_USEREVENT)
#define FF_QUIT_EVENT (SDL_USEREVENT + 1)

static SDL_Window* window;
static SDL_Surface* screen_surface;
static std::mutex screen_mutex;

struct VideoPlayer {
    // VideoPlayer variables
    bool loaded = false;
    std::string name;

    // Libav variables
    // Libav loading stuff
    AVFormatContext* video_format_ctx;
    int video_stream_idx;
    int audio_stream_idx;
    int subtitle_stream_dix;
    AVCodecContext* video_codec_ctx;
    AVCodecContext* audio_codec_ctx;
    AVCodec* video_codec;
    AVCodec* audio_codec;
    SwsContext* sws_ctx;

    // Video data
    uint8_t* data;
    AVFrame* video_frame;
    AVFrame* video_frame_RGB;
    uint8_t* buffer;
    AVPacket packet;

  int load(const char* filename) {
    printf("[EDITOR] Loading video %s\n", filename);

    video_format_ctx = avformat_alloc_context();



    printf("[EDITOR] Successfully loaded up video\n");
    printf("[EDITOR] Video stream index: %d\n", video_stream_idx);
    printf("[EDITOR] %dx%d\n", video_codec_ctx->width, video_codec_ctx->height);

    loaded = true;
    name = filename;

    return 0;
  }
};

int main(int argc, char* argv[]) {  
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
    fprintf(stderr, "ERROR: Could not intialize SDL. %s", SDL_GetError());
    exit(1);
  }

  window = SDL_CreateWindow("Video Editor", 160, 90, 1600, 900, SDL_WINDOW_OPENGL);
  if (window == nullptr) {
    fprintf(stderr, "ERROR: SDL could not set video mode");
    exit(1);
  }

  screen_surface = SDL_GetWindowSurface(window);
  SDL_FillRect(screen_surface, nullptr, SDL_MapRGB(screen_surface->format, 0xff, 0xff, 0xff));
  SDL_UpdateWindowSurface(window);
  SDL_Delay(2000);

  SDL_DestroyWindow(window);
  SDL_Quit();

  // SDL_Event e;

  // while(true) {
  //   SDL_WaitEvent(&e);
  //   switch(e.type) {
  //     case FF_QUIT_EVENT:
  //     case SDL_QUIT:
  //       SDL_Quit();
  //       return 0;
  //       break;
  //     case FF_REFRESH_EVENT:
  //       break;
  //     default:
  //       break;
  //   }
  // }

  return 0;
}