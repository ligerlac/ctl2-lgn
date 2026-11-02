#ifndef EventAnalysis_H
#define EventAnalysis_H

#include <fstream>
#include <string>
#include <map>

using namespace std;

#include "Pythia8/Pythia.h"
using namespace Pythia8; 

class TH1F;
class TFile;
class UltraFastSim;

class EventAnalysis {
 public:
  EventAnalysis(string inFileName);
  virtual ~EventAnalysis() {;}
  void processEvent(Event& event, UltraFastSim &ufs);
  void finalize();
 private:
  map<string, TH1F*> histograms;
  TFile* file;
  string fileName;
};

#endif
