#include "service_script.hpp"

#include <algorithm>
#include <fstream>
#include <set>

#include "../util.hpp"
#include "locator.hpp"

namespace ae {
  bool operator <  (const Line& lhs, const Line& rhs) {
    return lhs.timestamp <  rhs.timestamp;
  }
  bool operator >  (const Line& lhs, const Line& rhs) {
    return lhs.timestamp >  rhs.timestamp;
  }
  bool operator == (const Line& lhs, const Line& rhs) {
    return lhs.timestamp == rhs.timestamp;
  }

  // NullScript::NullScript() : lines(compare_lines) {
  NullScript::NullScript() {
    lines.insert({ 0, 0, 0, "*" });
   }

  bool NullScript::initialize(std::string filename) {
    printf("Attemping to intialize script::NullScript.\n");
    return false;
  }

  Litr NullScript::get(s64 ms) { return lines.begin(); }
  Litr NullScript::add(Line l) { return lines.begin(); }

  Litr NullScript::erase(Litr itr)          { return  lines.begin(); }
  Litr NullScript::erase(s64 ms)            { return  lines.begin(); }
  Line NullScript::pop_deleted()            { return *lines.begin(); }
  Litr NullScript::update(Litr itr, Line l) { return  lines.begin(); }
  Litr NullScript::update(s64 ms, Line l)   { return  lines.begin(); }

  Litr NullScript::begin() { return lines.begin(); }
  Litr NullScript::end()   { return lines.end(); }
  int  NullScript::size()  { return 0; }


  // LoadedScript::LoadedScript() : lines(compare_lines) {
  LoadedScript::LoadedScript() {
    lines.insert({ 0, 0, 0, "FIRST ELEMENT" });
   }

  bool LoadedScript::initialize(std::string filename) {
    std::ifstream f(filename);
    // FIXME: More robust way of handling that file is not present
    if (!f.good()) return false;

    for (util::CSVIterator loop(f); loop != util::CSVIterator(); ++loop) {
      int season      = std::stoi((*loop)[0]);
      int episode     = std::stoi((*loop)[1]);
      s64 timestamp   = std::stoi((*loop)[2]);
      std::string str = (*loop)[4];

      add({ season, episode, timestamp, str });
    }

    return true;
  }

  // lower_bound() = greater than or equal to
  Litr LoadedScript::get(s64 ms) {
    Litr itr = lines.lower_bound({ 0, 0, ms, "" } );
    if (itr == lines.begin()) { 
      return itr; 
    } else {
      return (*itr).timestamp == ms ? itr : --itr;
    }
  }

  // This function tries to add the line "l" to the set. If there's a collision,
  // it removes the offending line "o", and then adds "l" to the set. It then
  // increments o's timestamp by 1 ms, then treats o as l. Repeat until no more
  // collisions.
  // FIXME: "==" isn't working or something. result.second always returns true
  Litr LoadedScript::add(Line l) {
    // I know, I know. Never use "while (true)". But it's effective here!
    while (true) {
      auto result = get(l.timestamp);

      if ((*result).timestamp != l.timestamp) {
        return lines.insert(l).first;
      }

      Line new_l = *result;
      erase(result);
      
      lines.insert(l);

      l = new_l;
      l.timestamp += 1;
    }
  }

  Litr LoadedScript::erase(Litr itr) {
    if (itr == lines.end()) {
      printf("Attemping to erase lines.end().");
      return lines.begin();
    }

    deleted.push(*itr);

    return lines.erase(itr);
  }

  Litr LoadedScript::erase(s64 ms) {
    return erase(get(ms));
  }

  Line LoadedScript::pop_deleted() {
    Line line;
    if (!deleted.empty()) {
      line = deleted.top();
      deleted.pop();
    } else {
      line = { 0, 0, 0, "*" };
    }

    return line;
  }

  // This function removes the line at the itr/ms and then inserts the new line
  Litr LoadedScript::update(Litr itr, Line l) {
    erase(itr);
    return add(l);
  }

  Litr LoadedScript::update(s64 ms, Line l) {
    return update(get(ms), l);
  }

  Litr LoadedScript::begin() {
    return lines.begin();
  }

  Litr LoadedScript::end() {
    return lines.end();
  }

  int LoadedScript::size() {
    return lines.size();
  }
}