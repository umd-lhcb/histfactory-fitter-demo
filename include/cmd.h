// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Sat Jul 10, 2021 at 06:22 PM +0200

#ifndef _FIT_DEMO_CMD_H_
#define _FIT_DEMO_CMD_H_

#include <map>
#include <string>

#include <cxxopts.hpp>

template <typename T>
class ArgProxy {
 public:
  ArgProxy(cxxopts::ParseResult &parsed_args, std::string const mode);
  T const get(std::string const key);
  void    set_mode(std::string const mode);
  void set_default(std::string const mode, std::string const key, T const val);

 private:
  cxxopts::ParseResult                             m_parsed_args;
  std::string                                      m_mode;
  std::map<std::string, std::map<std::string, T> > m_default_val;

  bool default_exist(std::string const mode, std::string const key);
};

template <typename T>
ArgProxy<T>::ArgProxy(cxxopts::ParseResult &parsed_args, std::string const mode)
    : m_parsed_args(parsed_args), m_mode(mode) {}

template <typename T>
T const ArgProxy<T>::get(std::string const key) {
  // Here we assume the value is not specified explicitly in command line AND a
  // default value for that key in this mode exists
  if (!m_parsed_args[key].count() && default_exist(m_mode, key))
    return m_default_val[m_mode][key];
  // Otherwise just let the parser to handle it
  return m_parsed_args[key].template as<T>();
}

template <typename T>
void ArgProxy<T>::set_mode(std::string const mode) {
  m_mode              = mode;
  m_default_val[mode] = std::map<std::string, T>();
}

template <typename T>
void ArgProxy<T>::set_default(std::string const mode, std::string const key,
                              T const val) {
  m_default_val[mode][key] = val;
}

template <typename T>
bool ArgProxy<T>::default_exist(std::string mode, std::string key) {
  if (m_default_val.find(mode) == m_default_val.end()) return false;
  if (m_default_val[mode].find(key) == m_default_val[mode].end()) return false;
  return true;
}

#endif
