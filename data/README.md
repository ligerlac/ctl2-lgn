# Pythia8 and UltraFastSim based puppi candidate data generator

## Build instructions
The main program to use is generateData. You can build it using the following commands.
```
source /afs/hep.wisc.edu/cms/sw/Xilinx/Vivado/2023.1/settings64.sh
source /cvmfs/cms.cern.ch/cmsset_default.sh; pushd /cvmfs/cms.cern.ch/el9_amd64_gcc12/cms/cmssw/CMSSW_15_1_2/src; cmsenv; popd
make
```

## Input setup examples
The minbias.cards and hlhc-z.cards are setup to simulate proton-proton
collisions at 13.6 TeV. These are Pythia8 data cards. You can generate
other types of signal events with the program as long as Pythia8
understands those files.

One can make minbias events with minbias.cards:

```
generateData minbias.cards
```

One can make plots using:

```
root -b -l makePlots.cpp\(\"minbias.root\"\)
```

The output of generateData program, using any of the <tag>.cards files are
the files <tag>.csv with 144  puppi candidates per event and <tag>.root
with histograms of those quantities.

You can use minbias data to learn about the backgrounds. Hopefully, the
anomaly detection algorithm can be used to distinguish the backgrounds from
any type of signal.

Histograms from ttbar.root, hllhc-z.root and minbias.root:

<p align="center">
    <img src="ttbar-PuppiPT.png" width="30%">
    <img src="hllhc-z-PuppiPT.png" width="30%">
    <img src="minbias-PuppiPT.png" width="30%">
</p>
<p align="center">
    <img src="ttbar-PuppiEta.png" width="30%">
    <img src="hllhc-z-PuppiEta.png" width="30%">
    <img src="minbias-PuppiEta.png" width="30%">
</p>
<p align="center">
    <img src="ttbar-PuppiPhi.png" width="30%">
    <img src="hllhc-z-PuppiPhi.png" width="30%">
    <img src="minbias-PuppiPhi.png" width="30%">
</p>
<p align="center">
    <img src="ttbar-PuppiPID.png" width="30%">
    <img src="hllhc-z-PuppiPID.png" width="30%">
    <img src="minbias-PuppiPID.png" width="30%">
</p>
