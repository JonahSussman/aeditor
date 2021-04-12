#include <bits/stdc++.h>

#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "imgui.h"
#include "imgui-SFML.h"

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#define TRUE  1
#define FALSE 0

static struct Player {
  plm_t* plm;

  int samplerate;
  int num_pixels;

  int texture_mode;

  static void on_video(plm_t* mpeg, plm_frame_t* frame, void* user) {

  }

  static void on_audio(plm_t* mpeg, plm_samples_t* samples, void* user) {

  }

  bool create(const char* filename, int texture_mode) {
    this->texture_mode = texture_mode;
    plm = plm_create_with_filename(filename);
    if (plm == nullptr) {
      printf("Couldn't open %s", filename);
      exit(1);
    }

    samplerate = plm_get_samplerate(plm);

    printf(
      "Opened %s - framerate: %f fps, samplerate: %d hz, duration %f s\n",
      filename,
      plm_get_framerate(plm),
      plm_get_samplerate(plm),
      plm_get_duration(plm)
    );

    plm_set_video_decode_callback(plm, on_video, this);
    plm_set_audio_decode_callback(plm, on_audio, this);

    plm_set_loop(plm, TRUE);
    plm_set_audio_enabled(plm, TRUE);
    plm_set_audio_stream(plm, 0);

		self->texture_rgb = app_create_texture(self, 0, "texture_rgb");
		num_pixels = plm_get_width(self->plm) * plm_get_height(self->plm);
		self->rgb_data = (uint8_t*)malloc(num_pixels * 3);

    printf("pl_mpeg intialized!\n");
    return true;
  }

} player;

int main(int argc, char* argv[]) {
  player.create("data/episode_mpg.mpg", 0);

  return 0;
}