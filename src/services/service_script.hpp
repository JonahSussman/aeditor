#pragma once

#include <set>
#include <stack>
#include <string>

#include "service_vlc.hpp"
#include "../util.hpp"

namespace ae {
  struct Line {
    int season;
    int episode;
    s64 timestamp;
    std::string str;

    friend bool operator <  (const Line& lhs, const Line& rhs);
    friend bool operator >  (const Line& lhs, const Line& rhs);
    friend bool operator == (const Line& lhs, const Line& rhs);

    // FIXME: Find out how to convert int -> Line *while also retaining*
    // Line l = { 0, 0, 0, "" }; stuff

    // Line(int s, int e, libvlc_time_t ms, std::string g) 
    //   : season(s), episode(e), timestamp(ms), str(g) { }
    // Line(libvlc_time_t ms) : timestamp(ms) { }
    // Line& operator= (const int& ms) { timestamp = ms; return *this; }
  };

  typedef std::set<Line>::iterator Litr;

  // Old implementation used a vector. This implementation uses a set.
  class Script {
  public:
    virtual bool initialize(std::string filename) = 0;

    // Returns iterator of the fonud line
    virtual Litr get(s64 ms) = 0;
    // Returns iterator of the added line
    virtual Litr add(Line l) = 0;

    // Returns and itr of the element right after the erased element
    virtual Litr erase(Litr itr) = 0;
    virtual Litr erase(s64 ms) = 0;
    virtual Line pop_deleted();
    // Returns the new line's itr
    virtual Litr update(Litr itr, Line l) = 0;
    virtual Litr update(s64 ms, Line l) = 0;

    virtual Litr begin() = 0;
    virtual Litr end() = 0;
    virtual int  size() = 0;
  };

  class NullScript : public Script {
  public:
    NullScript();

    virtual bool initialize(std::string filename);

    virtual Litr get(s64 ms);
    virtual Litr add(Line l);

    virtual Litr erase(Litr itr);
    virtual Litr erase(s64 ms);
    virtual Line pop_deleted();
    virtual Litr update(Litr itr, Line l);
    virtual Litr update(s64 ms, Line l);

    virtual Litr begin();
    virtual Litr end();
    virtual int  size();
  private:
    std::set<Line> lines;
  };



  class LoadedScript : public Script {
  public:
    LoadedScript();

    virtual bool initialize(std::string filename);

    virtual Litr get(s64 ms);
    virtual Litr add(Line l);
    
    virtual Litr erase(Litr itr);
    virtual Litr erase(s64 ms);
    virtual Line pop_deleted();
    virtual Litr update(Litr itr, Line l);
    virtual Litr update(s64 ms, Line l);

    virtual Litr begin();
    virtual Litr end();
    virtual int  size();
  private:
    std::set<Line> lines;
    std::stack<Line> deleted;
  };
}