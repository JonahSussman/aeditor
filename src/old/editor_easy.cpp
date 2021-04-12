#include <bits/stdc++.h>

#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "easy_ffmpeg.hpp"
#include "easy_ffmpeg_sfml.hpp"

static int no_close = sf::Style::Titlebar | sf::Style::Close;
static sf::RenderWindow window(sf::VideoMode(1600, 900), "Editor", no_close);

static sf::Texture texture;
static sf::Sprite sprite;

int main(int argc, char* argv[]) {
  std::cout << "[EDITOR] Starting up..." << std::endl;

  window.setFramerateLimit(60);
  window.resetGLStates();

  eff::Video video;
  video.load("data/episode_720.mkv");
  video.play();

  sf::Clock clock;

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      switch (event.type) {
      case sf::Event::Closed:
        window.close();
        break;
      }
    }
    sf::Time dt = clock.restart();

    window.clear(sf::Color(0, 43, 54, 255));

    video.show(window);

    window.display();
  }
  
  return 0;
}