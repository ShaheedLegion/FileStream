// Copyright (c) - 2015, Shaheed Abdol.
// Use this code as you wish, but don't blame me.

#ifndef UTIL
#define UTIL

#include <chrono>
#include <cctype>
#include <sstream>

namespace util {

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::milliseconds ms;
typedef std::chrono::duration<double> fsec;

class Timer {
public:
  Timer(long rate) : m_rate(rate), m_ticks(0) { start_time = Time::now(); }

  bool measure() {
    fsec fs = Time::now() - start_time;
    ms d = std::chrono::duration_cast<ms>(fs);
    start_time = Time::now();

    m_ticks += d.count();
    if (m_ticks >= m_rate) {
      m_ticks = 0;
      return true;
    }

    return false;
  }

private:
  long m_rate;
  long long m_ticks;
  std::chrono::system_clock::time_point start_time;
};

// Mutate a bunch of text at once
void MutateString(std::vector<std::string> &text, std::pair<WPARAM, bool> &key,
                  int limit) {
  if (text.empty() && key.second) {
    std::string temp;
    if (key.first >= 0x30 && key.first <= 0x39) {
      // Append number to the string.
      if (limit == -1 ||
          (limit != -1 && static_cast<signed int>(temp.size()) < limit))
        temp.push_back(std::string::value_type(key.first));
    } else if (key.first >= 0x41 && key.first <= 0x5A) {
      // Append letter to the string.
      if (limit == -1 ||
          (limit != -1 && static_cast<signed int>(temp.size()) < limit))
        temp.push_back(std::string::value_type(key.first));
    }
    if (!temp.empty())
      text.push_back(temp);
  }
  for (auto &str : text) {
    if (!key.second) // skip over key-up events.
      continue;
    // VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
    // 0x40 : unassigned
    // VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
    if (key.first >= 0x30 && key.first <= 0x39) {
      // Append number to the string.
      if (limit == -1 ||
          (limit != -1 && static_cast<signed int>(str.size()) < limit))
        str.push_back(std::string::value_type(key.first));
    } else if (key.first >= 0x41 && key.first <= 0x5A) {
      // Append letter to the string.
      if (limit == -1 ||
          (limit != -1 && static_cast<signed int>(str.size()) < limit))
        str.push_back(std::string::value_type(key.first));
    } else if (key.first == VK_SPACE)
      str.push_back(std::string::value_type(' '));
    else if (key.first == VK_DELETE || key.first == VK_BACK && !str.empty())
      str.pop_back();
  }
}

bool MutateString(std::vector<std::pair<WPARAM, bool>> &keys,
                  std::vector<std::pair<WPARAM, LPARAM>> &chars, int limit,
                  std::string *output) {
  if (output == nullptr)
    return false;

  for (auto &i : chars) {
    switch (i.first) {
    case 0x08: // Process a backspace.
    case 0x0A: // Process a linefeed.
    case 0x1B: // Process an escape.
    case 0x09: // Process a tab.
    case 0x0D: // Process a carriage return.
      break;
    default:
      // Process displayable characters.
      if (static_cast<signed int>(output->size()) < limit)
        output->push_back(std::string::value_type(i.first));
      break;
    }
  }

  for (auto &i : keys) {
    if (!i.second)
      continue;

    if (i.first == VK_DELETE || i.first == VK_BACK && !output->empty())
      output->pop_back();
    else if (i.first == VK_RETURN)
      return true;
  }

  return false;
}

// We can do case insensitive compare for strict commands
bool icompare(std::string const &a, std::string const &b,
              bool contains = false) {
  if (a.empty() || b.empty())
    return false;

  std::string::size_type a_len{a.length()};
  std::string::size_type b_len{b.length()};

  if (a_len != b_len && !contains)
    return false;

  auto f_len = (a_len < b_len ? a_len : b_len);

  for (int i = 0; i < static_cast<signed int>(f_len); ++i) {
    if (std::tolower(a[i]) != std::tolower(b[i]))
      return false;
  }
  return true;
}

// We can do case insensitive compare for strict commands
std::string split(std::string const &a, std::string const &b,
                  bool dumpSpace = false) {
  if (a.empty() || b.empty())
    return "";

  // a's length must be greater than b
  std::string::size_type a_len{a.length()};
  std::string::size_type b_len{b.length()};

  if (b_len >= a_len)
    return "";

  auto f_len = a_len;

  std::stringstream ss;
  for (int i = b_len; i < static_cast<signed int>(f_len); ++i) {
    if (dumpSpace && a[i] == ' ')
      continue;
    ss << a[i];
  }
  return ss.str();
}

} // namespace util

#endif // UTIL