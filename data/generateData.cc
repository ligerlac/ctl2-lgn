// This simple program generates events for various physics processes.

using namespace std;
#include <math.h>

#include "Pythia8/Pythia.h"
using namespace Pythia8; 

#include "UltraFastSim.h"

#include "EventAnalysis.h"

int poisson(double mean, Rndm *rndmPtr) {
  static double oldMean = -1;
  static double g;
  if(mean != oldMean) {
    oldMean = mean;
    if(mean == 0) {
      g = 0;
    }
    else {
      g = exp(-mean);
    }
  }    
  double em = -1;
  double t = 1;
  do {
    em++;
    t *= rndmPtr->flat();
  } while(t > g);
  return em;
}

int main(int argc, char **argv) {
  // Generator. Process selection. LHC initialization. Histogram.
  Pythia pythia;

  if(argc >= 2) pythia.readFile(argv[1]);
  else {
    cerr << "Command syntax: " 
	 << argv[0] 
	 << " <command file name> [<runNumber>]" 
	 << endl;
    exit(1);
  }

  int runNumber = 0;
  if(argc == 3) runNumber = atoi(argv[2]);

  int    nEvents   = pythia.mode("Main:numberOfEvents");
  int    nList     = 1;
  int    nShow     = 1;
  int    nAbort    = pythia.mode("Main:timesAllowErrors");
  bool   showCS    = true;
  bool   showAS    = true;
  bool   showCPD   = true;
  bool   showAPD   = true;

  // Initialize pythia

  pythia.init();

  const int beamA = pythia.info.idA();
  const int beamB = pythia.info.idB();
  const double cmEnergy = pythia.info.eCM();

  // List settings.
  if (showCS) pythia.settings.listChanged();
  if (showAS) pythia.settings.listAll();

  // List particle data.  
  if (showCPD) pythia.particleData.listChanged();
  if (showAPD) pythia.particleData.listAll();

  Rndm * rndmPtr = &pythia.rndm;
  rndmPtr->init(runNumber);

  // Pileup pythia for proton beams

  Pythia *pileupPythia;
  int meanPileupEventCount = 25;
  if(beamA == 2212 && beamB == 2212) {
    pileupPythia = new Pythia();
    // Use beamA = beamB = 2212 and cmEnergy = Hard Interaction cmEnergy
    pileupPythia->readString("SoftQCD:all = on");
    cout << "beam A, B, s" << beamA << ", " << beamB << ", " << cmEnergy << endl;
    pileupPythia->readString("Beams:idA = 2212");
    pileupPythia->readString("Beams:idB = 2212");
    std::stringstream ss;
    ss << "Beams:eCM = " << cmEnergy;
    pileupPythia->readString(ss.str());
    pileupPythia->init();
    pileupPythia->rndm.init(runNumber);
  }

  // Setup analysis class
  string hFileName(argv[1]);
  hFileName.erase(hFileName.rfind("."));  
  EventAnalysis eventAnalysis(hFileName);

  // Ultra Fast Simulator

  UltraFastSim ufs;

  // Begin event loop
  for (int iEvent = 0, iAbort = 0; iEvent < nEvents; ) {

    // Generate event. Skip if error. Unless, errors are too many.
    if (!pythia.next()) {
      iAbort++;
      if(iAbort > nAbort) {
	cerr << "Too many aborted events = " << nAbort << endl;
	exit(1);
      }
      continue;
    }

    // List first few events, both hard process and complete events.
    if (iEvent < nList) { 
      pythia.info.list();
      pythia.process.list();
      pythia.event.list();
    }

    // Add pileup for proton beams

    if(beamA == 2212) {
      int pileupEventCount = 0;
      if(meanPileupEventCount > 0) pileupEventCount = poisson(meanPileupEventCount, rndmPtr);
      for (int puEvent = 0; puEvent < pileupEventCount; ) {
	if(!pileupPythia->next()) continue;
	double vx = rndmPtr->gauss()*2.; // Mean beam spread is 2 mm in x-y and 75 mm in z 
	double vy = rndmPtr->gauss()*2.;
	double vz = rndmPtr->gauss()*75.;
	for(int i = 0; i < pileupPythia->event.size(); i++) {
	  Particle& particle = pileupPythia->event[i];
	  particle.xProd(vx+particle.xProd());
	  particle.yProd(vy+particle.yProd());
	  particle.zProd(vz+particle.zProd());
	  if(particle.status() > 0) {
	    if(particle.isVisible()) {
	      if(particle.pT() > 0.5) {
		pythia.event.append(particle);
	      }
	    }
	  }
	}
	puEvent++;
      }
    }

    // Ultra fast simulation of detector effects
    if(!ufs.run(pythia.event, rndmPtr))
      {
	cerr << "Ultra fast simulation failed - aborting" << endl;
	exit(2);
      }

    // Event Analysis
    eventAnalysis.processEvent(pythia.event, ufs);

    // Heartbeat
    if(!(iEvent % nShow)) cout << "Processed event " << iEvent << endl;
    iEvent++;
    
  }

  // Save output

  pythia.stat();
  eventAnalysis.finalize();

  return 0;

}
