// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Thu Jan 06, 2022 at 01:55 AM +0100

#ifndef _FIT_DEMO_LOADER_H_
#define _FIT_DEMO_LOADER_H_

#include <any>
#include <map>
#include <string>

class Config {
 public:
  Config();
  Config(std::map<std::string, std::any>& initMap);
  template <typename T>
  T const get(std::string const key);
  void    set(std::string const key, std::any val);

 private:
  std::map<std::string, std::any> m_map;
};

Config::Config() : m_map() {}
Config::Config(std::map<std::string, std::any>& initMap) : m_map(initMap) {}

template <typename T>
T const Config::get(std::string const key) {
  return std::any_cast<T>(m_map[key]);
}

void Config::set(std::string const key, std::any val) { m_map[key] = val; }

#endif
