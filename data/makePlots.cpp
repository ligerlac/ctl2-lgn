#include <TFile.h>
#include <TKey.h>
#include <TH1.h>
#include <TCanvas.h>
#include <TClass.h>
#include <TSystem.h>
#include <iostream>

int makePlots(const char* fileName) {
  int argc = 1;
  char* argv[] = { (char*)"App", nullptr };   
  TApplication theApp("Plotter", &argc, argv);
  // 1. Open the ROOT file in read mode
  TFile *file = TFile::Open(fileName, "READ");
  if (!file || file->IsZombie()) {
    std::cerr << "Error: Could not open file " << fileName << std::endl;
    return 1;
  }

  // 2. Create a canvas to render the histograms
  TCanvas *canvas = new TCanvas("canvas", "Histogram Canvas", 800, 600);

  // 3. Loop over all keys (objects) inside the file
  TIter nextKey(file->GetListOfKeys());
  TKey *key;

  int exportCount = 0;
  while ((key = (TKey*)nextKey())) {
    // Look up the class type of the object without loading it fully into memory
    TClass *cl = TClass::GetClass(key->GetClassName());
        
    // Check if the object inherits from the base histogram class (TH1)
    if (cl && cl->InheritsFrom(TH1::Class())) {
      // Read the object from the file and cast it to TH1*
      TH1 *hist = (TH1*)key->ReadObj();
      if (!hist) continue;

      // 4. Draw the histogram and save it to a PNG file
      canvas->cd();
      hist->Draw();
            
      // Define the output filename using the object's internal name
      std::string fullName(fileName);
      std::string pngName = fullName.erase(fullName.rfind(".")) + std::string("-") + std::string(hist->GetName()) + ".png";
      if (pngName.find("PT") != std::string::npos) {
	canvas->SetLogx(1);
      }
      else {
	canvas->SetLogx(0);
      }
      canvas->SetLogy(1);
      canvas->SaveAs(pngName.c_str());

      std::cout << "Successfully saved: " << pngName << std::endl;
      exportCount++;

      // Clean up the object from memory to prevent leaks
      delete hist;
    }
  }

  // 5. Clean up allocated environment objects
  delete canvas;
  file->Close();
  delete file;

  std::cout << "Done! Total exported histograms: " << exportCount << std::endl;

  return 0;

}
