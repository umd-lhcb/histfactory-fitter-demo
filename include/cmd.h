// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Mon May 23, 2022 at 06:12 PM -0400

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
  void    setMode(string const mode);
  void    setDefault(string const mode, string const key, any const val);
  void    setDefault(string const mode, map<string, any> defaultMap);

 private:
  cxxopts::ParseResult           mParsedArgs;
  string                         mMode;
  map<string, map<string, any> > mDefaultVal;

  bool defaultExist(string const mode, string const key);
};

ArgProxy::ArgProxy(cxxopts::ParseResult &parsedArgs, string const mode)
    : mParsedArgs(parsedArgs), mMode(mode) {}

template <typename T>
T const ArgProxy::get(string const key) {
  // Here we assume the value is not specified explicitly in command line AND a
  // default value for that key in this mode exists
  if (!mParsedArgs[key].count() && defaultExist(mMode, key))
    return any_cast<T>(mDefaultVal[mMode][key]);
  // Otherwise just let the parser to handle it
  return mParsedArgs[key].as<T>();
}

void ArgProxy::setMode(string const mode) {
  mMode             = mode;
  mDefaultVal[mode] = map<string, any>();
}

void ArgProxy::setDefault(string const mode, string const key, any const val) {
  mDefaultVal[mode][key] = val;
}

void ArgProxy::setDefault(string const mode, map<string, any> defaultMap) {
  mDefaultVal[mode] = defaultMap;
}

bool ArgProxy::defaultExist(string mode, string key) {
  if (mDefaultVal.find(mode) == mDefaultVal.end()) return false;
  if (mDefaultVal[mode].find(key) == mDefaultVal[mode].end()) return false;
  return true;
}

#endif
