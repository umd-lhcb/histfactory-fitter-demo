// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Fri Jan 07, 2022 at 04:28 PM +0100

#ifndef _FIT_DEMO_LOADER_H_
#define _FIT_DEMO_LOADER_H_

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <stdlib.h>

#include <TFile.h>
#include <TH1.h>
#include <TString.h>

#include <yaml-cpp/yaml.h>
#include <boost/any.hpp>

using std::cerr;
using std::cout;
using std::endl;

using boost::any;
using boost::any_cast;
using std::map;
using std::string;
using std::vector;

////////////////
// Error code //
////////////////

#define KEY_NOT_FOUND 1
#define FOLDER_NOT_FOUND 11
#define FILE_NOT_FOUND 12
#define HISTO_NOT_FOUND 13

//////////////////////////////////////
// Smarter way to store loaded info //
//////////////////////////////////////

class Config {
 public:
  Config();
  Config(map<string, any>& initMap);

  any& operator[](string key) { return mMap[key]; };

  template <typename T>
  T const get(string const key);
  void    set(string const key, any val);

 private:
  map<string, any> mMap;
};

Config::Config() : mMap() {}
Config::Config(map<string, any>& initMap) : mMap(initMap) {}

template <typename T>
T const Config::get(string const key) {
  if (mMap.find(key) == mMap.end()) {
    cerr << "Key " << key << " not found!" << endl;
    exit(KEY_NOT_FOUND);  // Terminate early for easier debug
  }

  return any_cast<T>(mMap[key]);
}

void Config::set(string const key, any val) { mMap[key] = val; }

//////////////////////
// Histogram loader //
//////////////////////

class HistoLoader {
 public:
  HistoLoader(string inputFolder, bool verbose);
  HistoLoader(string inputFolder) : HistoLoader(inputFolder, false){};
  ~HistoLoader();

  void   load();
  Config getConfig() { return mConfig; };

 private:
  bool           mVerbose;
  TString        mDir;
  TString        mYml;
  TString        mYmlBasename = "spec.yml";
  Config         mConfig;
  vector<TFile*> mNtps;
  vector<TH1*>   mHistos;

  YAML::Node loadYml(TString yamlFile);
  YAML::Node loadYml() { return loadYml(mYml); }

  void loadHisto(TFile* ntp, string key, TString histoName);
  void loadHisto(TFile* ntp, string key, string histoName) {
    loadHisto(ntp, key, TString(histoName));
  };

  TString absDir(TString folder);
  TString absDir(string folder) { return absDir(TString(folder)); }

  bool fileExist(TString file);
  bool fileExist(string file) { return fileExist(TString(file)); };

  double normFac(TH1* histo);
};

// Constructor/destructor //////////////////////////////////////////////////////

HistoLoader::HistoLoader(string inputFolder, bool verbose)
    : mVerbose(verbose), mConfig() {
  mDir = absDir(inputFolder);
  mYml = mDir + '/' + mYmlBasename;
}

HistoLoader::~HistoLoader() {
  if (mVerbose) cout << "Cleaning up..." << endl;
  for (auto p : mHistos) delete p;
  for (auto p : mNtps) delete p;
}

// Public //////////////////////////////////////////////////////////////////////

void HistoLoader::load() {
  auto spec = loadYml();

  for (auto it = spec.begin(); it != spec.end(); it++) {
    auto ntpName = TString(it->first.as<string>());
    if (mVerbose) cout << "Working on n-tuple: " << ntpName << endl;

    auto ntpFullPath = mDir + "/" + ntpName;
    if (!fileExist(ntpFullPath)) {
      cerr << "n-tuple " << ntpFullPath << " doesn't exist!" << endl;
      exit(FILE_NOT_FOUND);
    }

    auto ntp = new TFile(ntpFullPath);
    mNtps.push_back(ntp);

    for (auto iit = it->second.begin(); iit != it->second.end(); iit++) {
      auto key       = iit->first.as<string>();
      auto histoName = iit->second.as<string>();
      loadHisto(ntp, key, histoName);
    }
  }
}

// Private: loaders ////////////////////////////////////////////////////////////

YAML::Node HistoLoader::loadYml(TString yamlFile) {
  if (mVerbose) cout << "Loading " << yamlFile << endl;

  if (!fileExist(yamlFile)) {
    cerr << "YAML " << yamlFile << " doesn't exist!" << endl;
    exit(FILE_NOT_FOUND);
  }

  return YAML::LoadFile(yamlFile.Data());
}

void HistoLoader::loadHisto(TFile* ntp, string key, TString histoName) {
  if (mVerbose) cout << "  Loading " << histoName << " as " << key << endl;

  auto histo = static_cast<TH1*>(ntp->Get(histoName));
  if (histo == nullptr) {
    cerr << "Histo " << histoName << " doesn't exist!" << endl;
    exit(HISTO_NOT_FOUND);
  }

  mConfig[key]              = histo;
  mConfig[key + "_NormFac"] = normFac(histo);
}

// Private: helpers ////////////////////////////////////////////////////////////

bool HistoLoader::fileExist(TString file) {
  if (FILE* _file = fopen(file.Data(), "r")) {
    fclose(_file);
    return true;
  } else
    return false;
}

TString HistoLoader::absDir(TString folder) {
  auto abs_path = realpath(folder.Data(), nullptr);
  if (abs_path != nullptr) {
    if (mVerbose) cout << "Folder absolute path: " << abs_path << endl;
    return TString(abs_path);
  }

  cerr << "Folder " << folder << " cannot be resolved!" << endl;
  exit(FOLDER_NOT_FOUND);
}

double HistoLoader::normFac(TH1* histo) {
  auto fac = 1. / histo->Integral();
  if (mVerbose) cout << "    with a normalization factor " << fac << endl;
  return fac;
}

#endif
