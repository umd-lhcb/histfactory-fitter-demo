// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Thu Jan 06, 2022 at 05:21 PM +0100

#ifndef _FIT_DEMO_CMD_H_
#define _FIT_DEMO_CMD_H_

#include <map>
#include <string>

#include <boost/any.hpp>
#include <cxxopts.hpp>

using boost::any;
using boost::any_cast;
using std::map;
using std::string;

///////////////////////////////////
// Smarter way to store CLI args //
///////////////////////////////////

class ArgProxy {
 public:
  ArgProxy(cxxopts::ParseResult &parsedArgs, string const mode);
  template <typename T>
  T const get(string const key);
  void    set_mode(string const mode);
  void    set_default(string const mode, string const key, any const val);
  void    set_default(string const mode, map<string, any> defaultMap);

 private:
  cxxopts::ParseResult           m_parsed_args;
  string                         m_mode;
  map<string, map<string, any> > m_default_val;

  bool default_exist(string const mode, string const key);
};

ArgProxy::ArgProxy(cxxopts::ParseResult &parsedArgs, string const mode)
    : m_parsed_args(parsedArgs), m_mode(mode) {}

template <typename T>
T const ArgProxy::get(string const key) {
  // Here we assume the value is not specified explicitly in command line AND a
  // default value for that key in this mode exists
  if (!m_parsed_args[key].count() && default_exist(m_mode, key))
    return any_cast<T>(m_default_val[m_mode][key]);
  // Otherwise just let the parser to handle it
  return m_parsed_args[key].template as<T>();
}

void ArgProxy::set_mode(string const mode) {
  m_mode              = mode;
  m_default_val[mode] = map<string, any>();
}

void ArgProxy::set_default(string const mode, string const key, any const val) {
  m_default_val[mode][key] = val;
}

void ArgProxy::set_default(string const mode, map<string, any> defaultMap) {
  m_default_val[mode] = defaultMap;
}

bool ArgProxy::default_exist(string mode, string key) {
  if (m_default_val.find(mode) == m_default_val.end()) return false;
  if (m_default_val[mode].find(key) == m_default_val[mode].end()) return false;
  return true;
}

#endif
