// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Thu Jan 06, 2022 at 05:22 PM +0100

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

  any& operator[](string key) { return m_map[key]; };

  template <typename T>
  T const get(string const key);
  void    set(string const key, any val);

 private:
  map<string, any> m_map;
};

Config::Config() : m_map() {}
Config::Config(map<string, any>& initMap) : m_map(initMap) {}

template <typename T>
T const Config::get(string const key) {
  if (m_map.find(key) == m_map.end()) {
    cerr << "Key " << key << " not found!" << endl;
    exit(KEY_NOT_FOUND);  // Terminate early for easier debug
  }

  return any_cast<T>(m_map[key]);
}

void Config::set(string const key, any val) { m_map[key] = val; }

//////////////////////
// Histogram loader //
//////////////////////

class HistoLoader {
 public:
  HistoLoader(string inputFolder, bool verbose);
  HistoLoader(string inputFolder) : HistoLoader(inputFolder, false){};
  ~HistoLoader();

  void   load();
  Config get_config() { return m_config; };

 private:
  bool           m_verbose;
  TString        m_dir;
  TString        m_yml;
  TString        m_yml_basename = "spec.yml";
  Config         m_config;
  vector<TFile*> m_ntps;
  vector<TH1*>   m_histos;

  YAML::Node load_yml(TString yamlFile);
  YAML::Node load_yml() { return load_yml(m_yml); }

  void load_histo(TFile* ntp, string key, TString histoName);
  void load_histo(TFile* ntp, string key, string histoName) {
    load_histo(ntp, key, TString(histoName));
  };

  TString abs_dir(TString folder);
  TString abs_dir(string folder) { return abs_dir(TString(folder)); }

  bool file_exist(TString file);
  bool file_exist(string file) { return file_exist(TString(file)); };

  double norm_fac(TH1* histo);
};

// Constructor/destructor //////////////////////////////////////////////////////

HistoLoader::HistoLoader(string inputFolder, bool verbose)
    : m_verbose(verbose), m_config() {
  m_dir = abs_dir(inputFolder);
  m_yml = m_dir + '/' + m_yml_basename;
}

HistoLoader::~HistoLoader() {
  if (m_verbose) cout << "Cleaning up..." << endl;
  for (auto p : m_histos) delete p;
  for (auto p : m_ntps) delete p;
}

// Public //////////////////////////////////////////////////////////////////////

void HistoLoader::load() {
  auto spec = load_yml();

  for (auto it = spec.begin(); it != spec.end(); it++) {
    auto ntpName = TString(it->first.as<string>());
    if (m_verbose) cout << "Working on n-tuple: " << ntpName << endl;

    auto ntpFullPath = m_dir + "/" + ntpName;
    if (!file_exist(ntpFullPath)) {
      cerr << "n-tuple " << ntpFullPath << " doesn't exist!" << endl;
      exit(FILE_NOT_FOUND);
    }

    auto ntp = new TFile(ntpFullPath);
    m_ntps.push_back(ntp);

    for (auto iit = it->second.begin(); iit != it->second.end(); iit++) {
      auto key       = iit->first.as<string>();
      auto histoName = iit->second.as<string>();
      load_histo(ntp, key, histoName);
    }
  }
}

// Private: loaders ////////////////////////////////////////////////////////////

YAML::Node HistoLoader::load_yml(TString yamlFile) {
  if (m_verbose) cout << "Loading " << yamlFile << endl;

  if (!file_exist(yamlFile)) {
    cerr << "YAML " << yamlFile << " doesn't exist!" << endl;
    exit(FILE_NOT_FOUND);
  }

  return YAML::LoadFile(yamlFile.Data());
}

void HistoLoader::load_histo(TFile* ntp, string key, TString histoName) {
  if (m_verbose) cout << "  Loading " << histoName << " as " << key << endl;

  auto histo = static_cast<TH1*>(ntp->Get(histoName));
  if (histo == nullptr) {
    cerr << "Histo " << histoName << " doesn't exist!" << endl;
    exit(HISTO_NOT_FOUND);
  }

  m_config[key]              = histo;
  m_config[key + "_NormFac"] = norm_fac(histo);
}

// Private: helpers ////////////////////////////////////////////////////////////

bool HistoLoader::file_exist(TString file) {
  if (FILE* _file = fopen(file.Data(), "r")) {
    fclose(_file);
    return true;
  } else
    return false;
}

TString HistoLoader::abs_dir(TString folder) {
  auto abs_path = realpath(folder.Data(), nullptr);
  if (abs_path != nullptr) {
    if (m_verbose) cout << "Folder absolute path: " << abs_path << endl;
    return TString(abs_path);
  }

  cerr << "Folder " << folder << " cannot be resolved!" << endl;
  exit(FOLDER_NOT_FOUND);
}

double HistoLoader::norm_fac(TH1* histo) {
  auto fac = 1. / histo->Integral();
  if (m_verbose) cout << "    with a normalization factor " << fac << endl;
  return fac;
}

#endif
