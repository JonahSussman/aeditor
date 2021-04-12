#include <bits/stdc++.h>

#include <SFML/Graphics.hpp>

int main() {
  sf::RenderWindow window(
    sf::VideoMode(1600, 900), 
    "Video Editor!", 
    sf::Style::Titlebar | sf::Style::Close
  );
  window.setFramerateLimit(60);

  sf::CircleShape shape(100.f);
  shape.setFillColor(sf::Color::Green);

  while(window.isOpen()) {
    sf::Event e;
    while(window.pollEvent(e)) {
      switch(e.type) {
        case sf::Event::Closed:
          window.close();
          break;
        case sf::Event::KeyPressed:
          break;
      }
    }

    window.clear();
    window.draw(shape);
    window.display();
  }

  return 0;
}