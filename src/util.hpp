#pragma once

#include <iterator>
#include <vector>
#include <sstream>
#include <string_view>
#include <array>
#include <memory>
#include <future>
#include <chrono>

typedef int16_t  s16;
typedef uint16_t u16;
typedef int32_t  s32;
typedef uint32_t u32;
typedef int64_t  s64;
typedef uint64_t u64;

// A bunch of useful functions and classes that I've accumulated
// Thanks stackoverflow!

namespace util {
class CSVRow {
public:
  std::string const& operator[](std::size_t index) const {
    return m_data[index];
  }
  std::size_t size() const {
    return m_data.size();
  }
  void readNextRow(std::istream& str) {
    std::string         line;
    std::getline(str, line);

    std::stringstream   lineStream(line);
    std::string         cell;

    m_data.clear();
    while(std::getline(lineStream, cell, ',')) {
      m_data.push_back(cell);
    }
    // This checks for a trailing comma with no data after it.
    if (!lineStream && cell.empty()) {
      // If there was a trailing comma then add an empty element.
      m_data.push_back("");
    }
  }
private:
  std::vector<std::string>    m_data;
};
inline std::istream& operator>>(std::istream& str, CSVRow& data) {
  data.readNextRow(str);
  return str;
}  

class CSVIterator {   
public:
  typedef std::input_iterator_tag     iterator_category;
  typedef CSVRow                      value_type;
  typedef std::size_t                 difference_type;
  typedef CSVRow*                     pointer;
  typedef CSVRow&                     reference;

  CSVIterator(std::istream& str) : m_str(str.good()?&str:nullptr) { ++(*this); }
  CSVIterator()                  : m_str(nullptr) {}

  // Pre Increment
  CSVIterator& operator++()               {if (m_str) { if (!((*m_str) >> m_row)){m_str = NULL;}}return *this;}
  // Post increment
  CSVIterator operator++(int)             {CSVIterator    tmp(*this);++(*this);return tmp;}
  CSVRow const& operator*()   const       {return m_row;}
  CSVRow const* operator->()  const       {return &m_row;}

  bool operator==(CSVIterator const& rhs) {return ((this == &rhs) || ((this->m_str == NULL) && (rhs.m_str == NULL)));}
  bool operator!=(CSVIterator const& rhs) {return !((*this) == rhs);}
private:
  std::istream*       m_str;
  CSVRow              m_row;
};

template <typename T>
inline constexpr std::string_view type_name() {
  std::string_view name, prefix, suffix;
#ifdef __clang__
  name = __PRETTY_FUNCTION__;
  prefix = "std::string_view type_name() [T = ";
  suffix = "]";
#elif defined(__GNUC__)
  name = __PRETTY_FUNCTION__;
  prefix = "constexpr std::string_view type_name() [with T = ";
  suffix = "; std::string_view = std::basic_string_view<char>]";
#elif defined(_MSC_VER)
  name = __FUNCSIG__;
  prefix = "class std::basic_string_view<char,struct std::char_traits<char> > __cdecl type_name<";
  suffix = ">(void)";
#endif
  name.remove_prefix(prefix.size());
  name.remove_suffix(suffix.size());
  return name;
}

inline std::string shell(const char* cmd) {
  std::array<char, 256> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
      throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
      result += buffer.data();
  }
  return result;
}

template<typename T>
inline bool future_ready(std::future<T>& t){
  return t.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

// Deletes object referenced by pointer, and then nulls the pointer
template <typename T>
inline void delnull(T *&ptr) {
  delete ptr;
  ptr = nullptr;
}

inline bool exists (const std::string& name) {
  if (FILE *file = fopen(name.c_str(), "r")) {
    fclose(file);
    return true;
  } else {
    return false;
  }   
}

inline int rename(std::string _old, std::string _new) {
  return std::rename(_old.c_str(), _new.c_str());
}

}