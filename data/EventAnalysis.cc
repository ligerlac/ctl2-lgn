#include "EventAnalysis.h"

#include <math.h>
#include <string>
#include <map>
#include <fstream>
#include <iomanip>
using namespace std;

#include "Pythia8/Pythia.h"
using namespace Pythia8; 

#include "TH1F.h"
#include "TTree.h"
#include "TFile.h"
#include "TSystem.h"
#include "TMath.h"

#include "UltraFastSim.h"

#include "geawis/firmware/data.h"

#include "DataFormats/L1TParticleFlow/interface/datatypes.h"
using namespace l1ct;

EventAnalysis::EventAnalysis(string inFileName) : fileName(inFileName) {
  // Define histograms
  file = new TFile((fileName + ".root").c_str(), "recreate");
  histograms["NStableParticles"] = new TH1F("NStableParticles", "Number of Stable Particles", 50, 0., 10000.);
  histograms["NVisibleParticles"] = new TH1F("NVisibleParticles", "Number of Visible Particles", 50, 0., 10000.);
  histograms["NInvisibleParticles"] = new TH1F("NInvisibleParticles", "Number of Invisible Particles", 10, 0., 10.);
  histograms["RecoParticleET"] = new TH1F("RecoParticleET", "Transverse energy of reconstructed particles (GeV)", 100, 0., 100.);
  histograms["RecoParticleEta"] = new TH1F("RecoParticleEta", "Eta of reconstructed particles", 100, -7., 7.);
  histograms["RecoParticlePhi"] = new TH1F("RecoParticlePhi", "Phi energy of reconstructed particles", 100, -7, 7.);

  histograms["PuppiPt"] = new TH1F("PuppiPT", "Transverse energy of particles (GeV)", 4096, 0., 4096.); // 12-bits
  histograms["PuppiEta"] = new TH1F("PuppiEta", "Eta of puppi candidates", 1024, -512., 512.); // 10-bits
  histograms["PuppiPhi"] = new TH1F("PuppiPhi", "Phi of puppi candidates", 1024, -512., 512.); // 10-bits
  histograms["PuppiPID"] = new TH1F("PuppiPID", "PID of puppi candidates", 10, 0., 10.); // 3-bits; but less than 8
  histograms["PuppiRelIso"] = new TH1F("PuppiRelIso", "Relative isolation of puppi candidates", 100, 0., 100.);
  histograms["PuppiShoShape"] = new TH1F("PuppiShoShape", "Shower shape of puppi candidates", 100, 0., 100.);

}

void EventAnalysis::processEvent(Event& event, UltraFastSim &ufs) {
  
  // Event counter

  static unsigned int evt = 0;
  static ofstream dofs;
  if (evt == 0) {
    dofs.open((fileName + ".csv").c_str());
    dofs << "iEvent, iParticle, pt, eta, phi, pid, reliso, shoshape" << endl;
  }
  evt++;
  
  // Process the generator level event
  
  int nStableParticles = 0;
  int nVisibleParticles = 0;
  int nInvisibleParticles = 0;
  for(int i = 0; i < event.size(); ++i) {
    Particle& particle = event[i];
    // Consider only particles with good status
    if(particle.status() > 0) {   // If status is less than zero, the particle decayed
      nStableParticles++;
      // Consider only visible particles -- ignores neutrinos
      if(particle.isVisible()) {
	nVisibleParticles++;
	// hNVisibleParticles->Fill(nVisibleParticles);
      }
      else {
	nInvisibleParticles++;
      }
    }
  }
  histograms["NStableParticles"]->Fill(nStableParticles);
  histograms["NVisibleParticles"]->Fill(nVisibleParticles);
  histograms["NInvisibleParticles"]->Fill(nInvisibleParticles);

  // Process the reconstructed event

  for(unsigned int i = 0; i < NPARTICLES; i++) {
    Particle_T puppi;
    puppi.hwPt = 0;
    puppi.hwEta = 0;
    puppi.hwPhi = 0;
    puppi.pid = l1ct::ParticleID::PID::NONE;
    puppi.reliso = 0;
    puppi.shoshape = 0;
    if (i < ufs.particleList().size()) {
      TParticle particle(ufs.particleList()[i]);
      double pt = particle.Pt();
      double eta = particle.Eta();
      double phi = particle.Phi();
      if (phi > M_PI) phi -= 2.0 * M_PI;
      histograms["RecoParticleET"]->Fill(pt);
      histograms["RecoParticleEta"]->Fill(eta);
      histograms["RecoParticlePhi"]->Fill(phi);
      puppi.hwPt = l1ct::Scales::makePtFromFloat(pt);
      puppi.hwEta = l1ct::Scales::makeGlbEta(eta);
      puppi.hwPhi = l1ct::Scales::makeGlbPhi(phi);
      if (particle.GetPdgCode() == 11) {
	puppi.pid = l1ct::ParticleID::PID::ELEMINUS;
      }
      else if (particle.GetPdgCode() == -11) {
	puppi.pid = l1ct::ParticleID::PID::ELEPLUS;
      }
      else if (particle.GetPdgCode() == 13) {
        puppi.pid = l1ct::ParticleID::PID::MUMINUS;
      }
      else if (particle.GetPdgCode() == -13) {
	puppi.pid = l1ct::ParticleID::PID::MUPLUS;
      }
      else if (particle.GetPdgCode() == 22) {
	puppi.pid = l1ct::ParticleID::PID::PHOTON;
      }
      else if (particle.GetPdgCode() == 221 | particle.GetPdgCode() == 321 | particle.GetPdgCode() == 2212) {
	puppi.pid = l1ct::ParticleID::PID::HADPLUS;
      }
      else if (particle.GetPdgCode() == -221 | particle.GetPdgCode() == -321 | particle.GetPdgCode() == -2212) {
	puppi.pid = l1ct::ParticleID::PID::HADMINUS;
      }
      else {
	puppi.pid = l1ct::ParticleID::PID::HADZERO;
      }
    }
    dofs
      << evt << ", "
      << i << ", "
      << puppi.hwPt << ", "
      << puppi.hwEta << ", "
      << puppi.hwPhi << ", "
      << puppi.pid.bits << ", "
      << puppi.reliso << ", "
      << puppi.shoshape << endl;
    histograms["PuppiPt"]->Fill(puppi.hwPt);
    histograms["PuppiEta"]->Fill(puppi.hwEta);
    histograms["PuppiPhi"]->Fill(puppi.hwPhi);
    histograms["PuppiPID"]->Fill(puppi.pid.bits);
    histograms["PuppiRelIso"]->Fill(puppi.reliso);
    histograms["PuppiShoShape"]->Fill(puppi.shoshape);
  }

}

void EventAnalysis::finalize() {
  file->cd();
  file->Write();
  file->Close();
}
