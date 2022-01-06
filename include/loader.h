// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Thu Jan 06, 2022 at 04:32 AM +0100

#ifndef _FIT_DEMO_LOADER_H_
#define _FIT_DEMO_LOADER_H_

#include <any>
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

using std::cerr;
using std::cout;
using std::endl;

using std::any;

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
  Config(std::map<std::string, any>& initMap);

  any& operator[](std::string key) { return m_map[key]; };

  template <typename T>
  T const get(std::string const key);
  void    set(std::string const key, any val);

 private:
  std::map<std::string, std::any> m_map;
};

Config::Config() : m_map() {}
Config::Config(std::map<std::string, std::any>& initMap) : m_map(initMap) {}

template <typename T>
T const Config::get(std::string const key) {
  if (m_map.find(key) == m_map.end()) {
    cerr << "Key " << key << " not found!" << endl;
    exit(KEY_NOT_FOUND);  // Terminate early for easier debug
  }

  return std::any_cast<T>(m_map[key]);
}

void Config::set(std::string const key, any val) { m_map[key] = val; }

//////////////////////
// Histogram loader //
//////////////////////

class HistoLoader {
 public:
  HistoLoader(std::string inputFolder, bool verbose);
  HistoLoader(std::string inputFolder) : HistoLoader(inputFolder, false){};
  ~HistoLoader();

  void   load();
  Config get_config() { return m_config; };

 private:
  bool                m_verbose;
  TString             m_dir;
  TString             m_yml;
  TString             m_yml_basename = "spec.yml";
  Config              m_config;
  std::vector<TFile*> m_ntps;
  std::vector<TH1*>   m_histos;

  YAML::Node load_yml(TString yamlFile);
  YAML::Node load_yml() { return load_yml(m_yml); }

  void load_histo(TFile* ntp, std::string key, TString histoName);
  void load_histo(TFile* ntp, std::string key, std::string histoName) {
    load_histo(ntp, key, TString(histoName));
  };

  TString abs_dir(TString folder);
  TString abs_dir(std::string folder) { return abs_dir(TString(folder)); }

  bool file_exist(TString file);
  bool file_exist(std::string file) { return file_exist(TString(file)); };

  double norm_fac(TH1* histo);
};

// Constructor/destructor //////////////////////////////////////////////////////

HistoLoader::HistoLoader(std::string inputFolder, bool verbose)
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
    auto ntpName = TString(it->first.as<std::string>());
    if (m_verbose) cout << "Working on n-tuple: " << ntpName << endl;

    auto ntpFullPath = m_dir + "/" + ntpName;
    if (!file_exist(ntpFullPath)) {
      cerr << "n-tuple " << ntpFullPath << " doesn't exist!" << endl;
      exit(FILE_NOT_FOUND);
    }

    auto ntp = new TFile(ntpFullPath);
    m_ntps.push_back(ntp);

    for (auto iit = it->second.begin(); iit != it->second.end(); iit++) {
      auto key       = iit->first.as<std::string>();
      auto histoName = iit->second.as<std::string>();
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

void HistoLoader::load_histo(TFile* ntp, std::string key, TString histoName) {
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
