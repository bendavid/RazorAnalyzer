#define RazorAnalyzer_cxx
//LOCAL INCLUDES
#include "RazorAnalyzer.h"
#include "JetCorrectorParameters.h"
//C++ INCLUDES
#include <map>
#include <fstream>
#include <sstream>
#include <string>
//ROOT INCLUDES
#include "TH1F.h"

using namespace std;

enum HggRazorBox {
    HighPt = 0,
    Hbb = 1,
    Zbb = 2,
    HighRes = 3,
    LowRes = 4
};

struct PhotonCandidate
{                                                  
  int   Index;
  TLorentzVector photon;
  TLorentzVector photonSC;
  float scEta;
  float scPhi;
  float SigmaIetaIeta;                                                                        
  float R9;                                                                                  
  float HoverE;                                                                        
  float sumChargedHadronPt;                                                                
  float sumNeutralHadronEt;                                                     
  float sumPhotonEt;                                            
  float sigmaEOverE;
  bool  _passEleVeto;
  bool  _passIso;
};

struct evt
{
  std::string run;
  std::string event;
};

#define _phodebug 0
#define _debug    0
#define _info     0

const double EB_R = 129.0;
const double EE_Z = 317.0;

//Testing branching and merging
void RazorAnalyzer::HggRazor(string outFileName, bool combineTrees, int option, bool isData )
{
  //*****************************************************************************
  //Settings
  //*****************************************************************************
  bool use25nsSelection = false;
  if (option == 1) use25nsSelection = true;

  std::cout << "[INFO]: use25nsSelection --> " << use25nsSelection << std::endl;
  
    //initialization: create one TTree for each analysis box 
  if ( _info) std::cout << "Initializing..." << std::endl;
  cout << "Combine Trees = " << combineTrees << "\n";
  if (outFileName.empty()){
    if ( _info ) std::cout << "HggRazor: Output filename not specified!" << endl << "Using default output name HggRazor.root" << std::endl;
    outFileName = "HggRazor.root";
  }
  TFile outFile(outFileName.c_str(), "RECREATE");
  
  //Including Jet Corrections
  std::vector<JetCorrectorParameters> correctionParameters;
  //get correct directory for JEC files (different for lxplus and t3-higgs)
  char* cmsswPath;
  cmsswPath = getenv("CMSSW_BASE");
  string pathname;
  if(cmsswPath != NULL) pathname = string(cmsswPath) + "/src/RazorAnalyzer/data/JEC/";
  cout << "Getting JEC parameters from " << pathname << endl;
  if (isData) {
    std::cout << "[INFO]: getting data JEC" << std::endl;
    correctionParameters.push_back(JetCorrectorParameters(Form("%s/Summer15_25nsV2_DATA_L1FastJet_AK4PFchs.txt", pathname.c_str())));
    correctionParameters.push_back(JetCorrectorParameters(Form("%s/Summer15_25nsV2_DATA_L2Relative_AK4PFchs.txt", pathname.c_str())));
    correctionParameters.push_back(JetCorrectorParameters(Form("%s/Summer15_25nsV2_DATA_L3Absolute_AK4PFchs.txt", pathname.c_str())));
    correctionParameters.push_back(JetCorrectorParameters(Form("%s/Summer15_25nsV2_DATA_L2L3Residual_AK4PFchs.txt", pathname.c_str())));
  } else {
    std::cout << "[INFO]: getting MC JEC" << std::endl;
    correctionParameters.push_back(JetCorrectorParameters(Form("%s/Summer15_25nsV2_MC_L1FastJet_AK4PFchs.txt", pathname.c_str())));
    correctionParameters.push_back(JetCorrectorParameters(Form("%s/Summer15_25nsV2_MC_L2Relative_AK4PFchs.txt", pathname.c_str())));
    correctionParameters.push_back(JetCorrectorParameters(Form("%s/Summer15_25nsV2_MC_L3Absolute_AK4PFchs.txt", pathname.c_str())));
  }
  FactorizedJetCorrector *JetCorrector = new FactorizedJetCorrector( correctionParameters );

  //I n i t i a l i z i n g   E f f e c t i v e   A r e a   A r r a y; 
  //------------------------------------------------------------------
  //InitEffArea( ); //not needed anymore
  
  //one tree to hold all events
  TTree *razorTree = new TTree("HggRazor", "Info on selected razor inclusive events");

  /*
    This is to debug and sync with Alex's events
  */
  std::ifstream ifs ( "HggRazorMissing.txt", std::fstream::in );
  std::string r_run, e_evt;
  std::map< std::string, evt > mymap;
  if ( ifs.is_open() )
    {
      while ( ifs.good() )
	{
	  ifs >> r_run >> e_evt;
	  std::string tmp = r_run + e_evt;
	  evt tmp_evt;
	  tmp_evt.run = r_run;
	  tmp_evt.event = e_evt;
	  if ( mymap.find( tmp ) == mymap.end() )
	    {
	      mymap[tmp] = tmp_evt;
	    }
	}
    }
  else
    {
      std::cout << "[ERROR]: unable to open file" << std::endl;
    }
  
  if( _info ) std::cout << "[INFO]: map size: " << mymap.size() << std::endl;
  
  //separate trees for individual boxes
  map<string, TTree*> razorBoxes;
  vector<string> boxNames;
  boxNames.push_back("HighPt");
  boxNames.push_back("Hbb");
  boxNames.push_back("Zbb");
  boxNames.push_back("HighRes");
  boxNames.push_back("LowRes");
  for(size_t i = 0; i < boxNames.size(); i++){
    razorBoxes[boxNames[i]] = new TTree(boxNames[i].c_str(), boxNames[i].c_str());
  }
  
  //histogram containing total number of processed events (for normalization)
  TH1F *NEvents = new TH1F("NEvents", "NEvents", 1, 1, 2);
  
  //tree variables
  float weight;
  int NPU;
  int n_Jets, nLooseBTaggedJets, nMediumBTaggedJets;
  int nLooseMuons, nTightMuons, nLooseElectrons, nTightElectrons, nTightTaus;
  float theMR;
  float theRsq, t1Rsq;
  float MET, t1MET;
  int nSelectedPhotons;
  float mGammaGamma, pTGammaGamma, sigmaMoverM;
  float mbbZ, mbbH;
  bool passedDiphotonTrigger;
  HggRazorBox razorbox = LowRes;

  unsigned int run, lumi, event;
  
  //selected photon variables
  float Pho_E[2], Pho_Pt[2], Pho_Eta[2], Pho_Phi[2], Pho_SigmaIetaIeta[2], Pho_R9[2], Pho_HoverE[2];
  float PhoSC_E[2], PhoSC_Pt[2], PhoSC_Eta[2], PhoSC_Phi[2];
  float Pho_sumChargedHadronPt[2], Pho_sumNeutralHadronEt[2], Pho_sumPhotonEt[2], Pho_sigmaEOverE[2];
  bool  Pho_passEleVeto[2], Pho_passIso[2];
  int   Pho_motherID[2];

  //jet information
  float jet_E[50], jet_Pt[50], jet_Eta[50], jet_Phi[50];
  
  //set branches on big tree
  if(combineTrees){
    razorTree->Branch("weight", &weight, "weight/F");
    razorTree->Branch("run", &run, "run/i");
    razorTree->Branch("lumi", &lumi, "lumi/i");
    razorTree->Branch("event", &event, "event/i");
    razorTree->Branch("passedDiphotonTrigger", &passedDiphotonTrigger, "passedDiphotonTrigger/O");
    razorTree->Branch("NPU", &NPU, "npu/i");
    razorTree->Branch("nLooseBTaggedJets", &nLooseBTaggedJets, "nLooseBTaggedJets/I");
    razorTree->Branch("nMediumBTaggedJets", &nMediumBTaggedJets, "nMediumBTaggedJets/I");
    razorTree->Branch("nLooseMuons", &nLooseMuons, "nLooseMuons/I");
    razorTree->Branch("nTightMuons", &nTightMuons, "nTightMuons/I");
    razorTree->Branch("nLooseElectrons", &nLooseElectrons, "nLooseElectrons/I");
    razorTree->Branch("nTightElectrons", &nTightElectrons, "nTightElectrons/I");
    razorTree->Branch("nTightTaus", &nTightTaus, "nTightTaus/I");
    razorTree->Branch("MR", &theMR, "MR/F");
    razorTree->Branch("Rsq", &theRsq, "Rsq/F");
    razorTree->Branch("t1Rsq", &t1Rsq, "t1Rsq/F");
    razorTree->Branch("MET", &MET, "MET/F");
    razorTree->Branch("t1MET", &t1MET, "t1MET/F");
    razorTree->Branch("nSelectedPhotons", &nSelectedPhotons, "nSelectedPhotons/I");
    razorTree->Branch("mGammaGamma", &mGammaGamma, "mGammaGamma/F");
    razorTree->Branch("pTGammaGamma", &pTGammaGamma, "pTGammaGamma/F");
    razorTree->Branch("sigmaMoverM", &sigmaMoverM, "sigmaMoverM/F");
    razorTree->Branch("box", &razorbox, "box/I");
    
    razorTree->Branch("pho1E", &Pho_E[0], "pho1E/F");
    razorTree->Branch("pho1Pt", &Pho_Pt[0], "pho1Pt/F");
    razorTree->Branch("pho1Eta", &Pho_Eta[0], "pho1Eta/F");
    razorTree->Branch("pho1Phi", &Pho_Phi[0], "pho1Phi/F");
    razorTree->Branch("pho1SC_E", &PhoSC_E[0], "pho1SC_E/F");
    razorTree->Branch("pho1SC_Pt", &PhoSC_Pt[0], "pho1SC_Pt/F");
    razorTree->Branch("pho1SC_Eta", &PhoSC_Eta[0], "pho1SC_Eta/F");
    razorTree->Branch("pho1SC_Phi", &PhoSC_Phi[0], "pho1SC_Phi/F");
    razorTree->Branch("pho1SigmaIetaIeta", &Pho_SigmaIetaIeta[0], "pho1SigmaIetaIeta/F");
    razorTree->Branch("pho1R9", &Pho_R9[0], "pho1R9/F");
    razorTree->Branch("pho1HoverE", &Pho_HoverE[0], "pho1HoverE/F");
    razorTree->Branch("pho1sumChargedHadronPt", &Pho_sumChargedHadronPt[0], "pho1sumChargedHadronPt/F");
    razorTree->Branch("pho1sumNeutralHadronEt", &Pho_sumNeutralHadronEt[0], "pho1sumNeutralHadronEt/F");
    razorTree->Branch("pho1sumPhotonEt", &Pho_sumPhotonEt[0], "pho1sumPhotonEt/F");
    razorTree->Branch("pho1sigmaEOverE", &Pho_sigmaEOverE[0], "pho1sigmaEOverE/F");
    razorTree->Branch("pho1passEleVeto", &Pho_passEleVeto[0], "pho1passEleVeto/O");
    razorTree->Branch("pho1passIso", &Pho_passIso[0], "pho1passIso/O");
    razorTree->Branch("pho1MotherID", &Pho_motherID[0], "pho1MotherID/I");
    
    razorTree->Branch("pho2E", &Pho_E[1], "pho2E/F");
    razorTree->Branch("pho2Pt", &Pho_Pt[1], "pho2Pt/F");
    razorTree->Branch("pho2Eta", &Pho_Eta[1], "pho2Eta/F");
    razorTree->Branch("pho2Phi", &Pho_Phi[1], "pho2Phi/F");
    razorTree->Branch("pho2SC_E", &PhoSC_E[1], "pho2SC_E/F");
    razorTree->Branch("pho2SC_Pt", &PhoSC_Pt[1], "pho2SC_Pt/F");
    razorTree->Branch("pho2SC_Eta", &PhoSC_Eta[1], "pho2SC_Eta/F");
    razorTree->Branch("pho2SC_Phi", &PhoSC_Phi[1], "pho2SC_Phi/F");
    razorTree->Branch("pho2SigmaIetaIeta", &Pho_SigmaIetaIeta[1], "pho2SigmaIetaIeta/F");
    razorTree->Branch("pho2R9", &Pho_R9[1], "pho2R9/F");
    razorTree->Branch("pho2HoverE", &Pho_HoverE[1], "pho2HoverE/F");
    razorTree->Branch("pho2sumChargedHadronPt", &Pho_sumChargedHadronPt[1], "pho2sumChargedHadronPt/F");
    razorTree->Branch("pho2sumNeutralHadronEt", &Pho_sumNeutralHadronEt[1], "pho2sumNeutralHadronEt/F");
    razorTree->Branch("pho2sumPhotonEt", &Pho_sumPhotonEt[1], "pho2sumPhotonEt/F");
    razorTree->Branch("pho2sigmaEOverE", &Pho_sigmaEOverE[1], "pho2sigmaEOverE/F");
    razorTree->Branch("pho2passEleVeto", &Pho_passEleVeto[1], "pho2passEleVeto/O");
    razorTree->Branch("pho2passIso", &Pho_passIso[1], "pho2passIso/O)");
    razorTree->Branch("pho2MotherID", &Pho_motherID[1], "pho2MotherID/I");

    razorTree->Branch("mbbZ", &mbbZ, "mbbZ/F");
    razorTree->Branch("mbbH", &mbbH, "mbbH/F");
    
    razorTree->Branch("n_Jets", &n_Jets, "n_Jets/I");
    razorTree->Branch("jet_E", jet_E, "jet_E[n_Jets]/F");
    razorTree->Branch("jet_Pt", jet_Pt, "jet_Pt[n_Jets]/F");
    razorTree->Branch("jet_Eta", jet_Eta, "jet_Eta[n_Jets]/F");
    razorTree->Branch("jet_Phi", jet_Phi, "jet_Phi[n_Jets]/F");
    razorTree->Branch("HLTDecision", HLTDecision, "HLTDecision[300]/O");
    
    //GenParticles
    razorTree->Branch("nGenParticle", &nGenParticle, "nGenParticle/I");
    razorTree->Branch("gParticleMotherId", gParticleMotherId, "gParticleMotherId[nGenParticle]/I");
    razorTree->Branch("gParticleMotherIndex", gParticleMotherIndex, "gParticleMotherIndex[nGenParticle]/I");
    razorTree->Branch("gParticleId", gParticleId, "gParticleId[nGenParticle]/I");
    razorTree->Branch("gParticleStatus", gParticleStatus, "gParticleStatus[nGenParticle]/I");
    razorTree->Branch("gParticleE", gParticleE, "gParticleE[nGenParticle]/F");
    razorTree->Branch("gParticlePt", gParticlePt, "gParticlePt[nGenParticle]/F");
    razorTree->Branch("gParticlePhi", gParticlePhi, "gParticlePhi[nGenParticle]/F");
    razorTree->Branch("gParticleEta", gParticleEta, "gParticleEta[nGenParticle]/F");
  }
  //set branches on all trees
  else{ 
    for(auto& thisBox : razorBoxes){
      thisBox.second->Branch("weight", &weight, "weight/F");
      thisBox.second->Branch("run", &run, "run/i");
      thisBox.second->Branch("lumi", &lumi, "lumi/i");
      thisBox.second->Branch("event", &event, "event/i");
      thisBox.second->Branch("passedDiphotonTrigger", &passedDiphotonTrigger, "passedDiphotonTrigger/O");
      thisBox.second->Branch("NPU", &NPU, "npu/i");
      thisBox.second->Branch("nLooseBTaggedJets", &nLooseBTaggedJets, "nLooseBTaggedJets/I");
      thisBox.second->Branch("nMediumBTaggedJets", &nMediumBTaggedJets, "nMediumBTaggedJets/I");
      thisBox.second->Branch("nLooseMuons", &nLooseMuons, "nLooseMuons/I");
      thisBox.second->Branch("nTightMuons", &nTightMuons, "nTightMuons/I");
      thisBox.second->Branch("nLooseElectrons", &nLooseElectrons, "nLooseElectrons/I");
      thisBox.second->Branch("nTightElectrons", &nTightElectrons, "nTightElectrons/I");
      thisBox.second->Branch("nTightTaus", &nTightTaus, "nTightTaus/I");
      thisBox.second->Branch("MR", &theMR, "MR/F");
      thisBox.second->Branch("Rsq", &theRsq, "Rsq/F");
      thisBox.second->Branch("t1Rsq", &t1Rsq, "t1Rsq/F");
      thisBox.second->Branch("MET", &MET, "MET/F");
      thisBox.second->Branch("t1MET", &t1MET, "t1MET/F");
      thisBox.second->Branch("nSelectedPhotons", &nSelectedPhotons, "nSelectedPhotons/I");
      thisBox.second->Branch("mGammaGamma", &mGammaGamma, "mGammaGamma/F");
      thisBox.second->Branch("pTGammaGamma", &pTGammaGamma, "pTGammaGamma/F");
      thisBox.second->Branch("sigmaMoverM", &sigmaMoverM, "sigmaMoverM/F");
      
      thisBox.second->Branch("pho1E", &Pho_E[0], "pho1E/F");
      thisBox.second->Branch("pho1Pt", &Pho_Pt[0], "pho1Pt/F");
      thisBox.second->Branch("Pho1Eta", &Pho_Eta[0], "pho1Eta/F");
      thisBox.second->Branch("pho1Phi", &Pho_Phi[0], "pho1Phi/F");
      thisBox.second->Branch("pho1SC_E", &PhoSC_E[0], "pho1SC_E/F");
      thisBox.second->Branch("pho1SC_Pt", &PhoSC_Pt[0], "pho1SC_Pt/F");
      thisBox.second->Branch("pho1SC_Eta", &PhoSC_Eta[0], "pho1SC_Eta/F");
      thisBox.second->Branch("pho1SC_Phi", &PhoSC_Phi[0], "pho1SC_Phi/F");
      thisBox.second->Branch("pho1SigmaIetaIeta", &Pho_SigmaIetaIeta[0], "pho1SigmaIetaIeta/F");
      thisBox.second->Branch("pho1R9", &Pho_R9[0], "pho1R9/F");
      thisBox.second->Branch("pho1HoverE", &Pho_HoverE[0], "pho1HoverE/F");
      thisBox.second->Branch("pho1sumChargedHadronPt", &Pho_sumChargedHadronPt[0], "pho1sumChargedHadronPt/F");
      thisBox.second->Branch("pho1sumNeutralHadronEt", &Pho_sumNeutralHadronEt[0], "pho1sumNeutralHadronEt/F");
      thisBox.second->Branch("pho1sumPhotonEt", &Pho_sumPhotonEt[0], "pho1sumPhotonEt/F");
      thisBox.second->Branch("pho1sigmaEOverE", &Pho_sigmaEOverE[0], "pho1sigmaEOverE/F");
      thisBox.second->Branch("pho1passEleVeto", &Pho_passEleVeto[0], "pho1passEleVeto/O");
      thisBox.second->Branch("pho1passIso", &Pho_passIso[0], "pho1passIso/O");
      
      thisBox.second->Branch("pho2E", &Pho_E[1], "pho2E/F");
      thisBox.second->Branch("pho2Pt", &Pho_Pt[1], "pho2Pt/F");
      thisBox.second->Branch("Pho2Eta", &Pho_Eta[1], "pho2Eta/F");
      thisBox.second->Branch("pho2Phi", &Pho_Phi[1], "pho2Phi/F");
      thisBox.second->Branch("pho2SC_E", &PhoSC_E[1], "pho2SC_E/F");
      thisBox.second->Branch("pho2SC_Pt", &PhoSC_Pt[1], "pho2SC_Pt/F");
      thisBox.second->Branch("pho2SC_Eta", &PhoSC_Eta[1], "pho2SC_Eta/F");
      thisBox.second->Branch("pho2SC_Phi", &PhoSC_Phi[1], "pho2SC_Phi/F");
      thisBox.second->Branch("pho2SigmaIetaIeta", &Pho_SigmaIetaIeta[1], "pho2SigmaIetaIeta/F");
      thisBox.second->Branch("pho2R9", &Pho_R9[1], "pho2R9/F");
      thisBox.second->Branch("pho2HoverE", &Pho_HoverE[1], "pho2HoverE/F");
      thisBox.second->Branch("pho2sumChargedHadronPt", &Pho_sumChargedHadronPt[1], "pho2sumChargedHadronPt/F");
      thisBox.second->Branch("pho2sumNeutralHadronEt", &Pho_sumNeutralHadronEt[1], "pho2sumNeutralHadronEt/F");
      thisBox.second->Branch("pho2sumPhotonEt", &Pho_sumPhotonEt[1], "pho2sumPhotonEt/F");
      thisBox.second->Branch("pho2sigmaEOverE", &Pho_sigmaEOverE[1], "pho2sigmaEOverE/F");
      thisBox.second->Branch("pho2passEleVeto", &Pho_passEleVeto[1], "pho2passEleVeto/O");
      thisBox.second->Branch("pho2passIso", &Pho_passIso[1], "pho2passIso/O");
      
      thisBox.second->Branch("mbbZ", &mbbZ, "mbbZ/F");
      thisBox.second->Branch("mbbH", &mbbH, "mbbH/F");
      
      thisBox.second->Branch("n_Jets", &n_Jets, "n_Jets/I");
      thisBox.second->Branch("jet_E", jet_E, "jet_E[n_Jets]/F");
      thisBox.second->Branch("jet_Pt", jet_Pt, "jet_Pt[n_Jets]/F");
      thisBox.second->Branch("jet_Eta", jet_Eta, "jet_Eta[n_Jets]/F");
      thisBox.second->Branch("jet_Phi", jet_Phi, "jet_Phi[n_Jets]/F");
      thisBox.second->Branch("HLTDecision", HLTDecision, "HLTDecision[300]/O");

      //GenParticles
      thisBox.second->Branch("nGenParticle", &nGenParticle, "nGenParticle/I");
      thisBox.second->Branch("gParticleMotherId", gParticleMotherId, "gParticleMotherId[nGenParticle]/I");
      thisBox.second->Branch("gParticleMotherIndex", gParticleMotherIndex, "gParticleMotherIndex[nGenParticle]/I");
      thisBox.second->Branch("gParticleId", gParticleId, "gParticleId[nGenParticle]/I");
      thisBox.second->Branch("gParticleStatus", gParticleStatus, "gParticleStatus[nGenParticle]/I");
      thisBox.second->Branch("gParticleE", gParticleE, "gParticleE[nGenParticle]/F");
      thisBox.second->Branch("gParticlePt", gParticlePt, "gParticlePt[nGenParticle]/F");
      thisBox.second->Branch("gParticlePhi", gParticlePhi, "gParticlePhi[nGenParticle]/F");
      thisBox.second->Branch("gParticleEta", gParticleEta, "gParticleEta[nGenParticle]/F");
    }
  }
  
  use25nsSelection = true;
  std::cout << "use25nsSelection-->" << use25nsSelection << std::endl;
  //begin loop
  if (fChain == 0) return;
  Long64_t nentries = fChain->GetEntriesFast();
  Long64_t nbytes = 0, nb = 0;
  for (Long64_t jentry=0; jentry<nentries;jentry++) {
    //begin event
    if( _info && (jentry % 10000 == 0) ) std::cout << "[INFO]: Processing entry " << jentry << std::endl;
    //std::cout << "[INFO]: Processing entry " << jentry << std::endl;
    Long64_t ientry = LoadTree(jentry);
    if (ientry < 0) break;
    nb = fChain->GetEntry(jentry);   nbytes += nb;
    
    //fill normalization histogram    
    NEvents->SetBinContent( 1, NEvents->GetBinContent(1) + genWeight);
    weight = genWeight;

    //reset tree variables
    n_Jets = 0;
    nLooseBTaggedJets = 0;
    nMediumBTaggedJets = 0;
    nLooseMuons = 0;
    nTightMuons = 0;
    nLooseElectrons = 0;
    nTightElectrons = 0;
    nTightTaus = 0;
    theMR = -1;
    theRsq = -1;
    nSelectedPhotons = 0;
    mGammaGamma = -1;
    pTGammaGamma = -1;
    mbbZ = 0;
    mbbH = 0;
    run = runNum;
    lumi = lumiNum; 
    event = eventNum;
    passedDiphotonTrigger = false;
    
    //selected photons variables
    for ( int i = 0; i < 2; i++ )
      {
	Pho_E[i]                  = -99.;
	Pho_Pt[i]                 = -99.;
	Pho_Eta[i]                = -99.;
	Pho_Phi[i]                = -99.;
	PhoSC_E[i]                = -99.;
	PhoSC_Pt[i]               = -99.;
	PhoSC_Eta[i]              = -99.;
	PhoSC_Phi[i]              = -99.;
	Pho_SigmaIetaIeta[i]      = -99.;
	Pho_R9[i]                 = -99.;
	Pho_HoverE[i]             = -99.;
	Pho_sumChargedHadronPt[i] = -99.;
	Pho_sumNeutralHadronEt[i] = -99.;
	Pho_sumPhotonEt[i]        = -99.;
	Pho_sigmaEOverE[i]        = -99.;
	Pho_passEleVeto[i]        = false;
	Pho_passIso[i]            = false;
      }
    
    //jets
    for ( int i = 0; i < 50; i++ )
      {
	jet_E[i]   = -99.;
	jet_Pt[i]  = -99.;
	jet_Eta[i] = -99.;
	jet_Phi[i] = -99.;
      }
    
    //Fill Pileup Info
    for (int i=0; i < nBunchXing; ++i) {
      if (BunchXing[i] == 0) {
	NPU = nPUmean[i];
      }
    }

    /*
      std::stringstream ss;
      ss << run << event;
      if ( mymap.find( ss.str() ) == mymap.end() )continue;
      //if ( !( run == 206859 && event == 24345 ) ) continue;
      */
    if ( _debug ) std::cout << "============" << std::endl;
    if ( _debug ) std::cout << "run == " << run << " && evt == " << event << std::endl;
    
    if(combineTrees) razorbox = LowRes;
    
    //TODO: triggers!
    // bool passedDiphotonTrigger = true;
    passedDiphotonTrigger = ( HLTDecision[40] );
    //if(!passedDiphotonTrigger) continue;
    
    //muon selection
    for(int i = 0; i < nMuons; i++){
      if(!isLooseMuon(i)) continue;  
      if(muonPt[i] < 10) continue;
      if(abs(muonEta[i]) > 2.4) continue;
      
      nLooseMuons++;
      
      if(isTightMuon(i)){ 
	nTightMuons++;
      }
    }
    //electron selection
    for(int i = 0; i < nElectrons; i++){
      if(!isLooseElectron(i,use25nsSelection)) continue; 
      if(elePt[i] < 10) continue;
      if(abs(eleEta[i]) > 2.5) continue;
      
      nLooseElectrons++;
      
      if(isTightElectron(i,use25nsSelection))
	{ 
	  nTightElectrons++;
	}
    }
    //tau selection
    for(int i = 0; i < nTaus; i++){
      if(!isTightTau(i)) continue; 
      nTightTaus++;
    }
    
    //photon selection
    vector<TLorentzVector> GoodPhotons;
    vector<double> GoodPhotonSigmaE; // energy uncertainties of selected photons
    vector<bool> GoodPhotonPassesIso; //store whether each photon is isolated
    std::vector< PhotonCandidate > phoCand;//PhotonCandidate defined in RazorAuxPhoton.hh
    
    int nPhotonsAbove40GeV = 0;
    for(int i = 0; i < nPhotons; i++)
      {
	//ID cuts -- apply isolation after candidate pair selection
	if ( _phodebug ) std::cout << "pho# " << i << " phopt1: " << phoPt[i] << " pho_eta: " << phoEta[i] << std::endl;
	if ( !photonPassLooseIDWithoutEleVeto(i,use25nsSelection) ) 
	  {
	    if ( _phodebug ) std::cout << "[DEBUG]: failed run2 ID" << std::endl;
	    continue;
	  }
	
	//Defining Corrected Photon momentum
	//float pho_pt = phoPt[i];//nominal pt
	float pho_pt_corr = pho_RegressionE[i]/cosh(phoEta[i]);//regression corrected pt
	TVector3 vec;
	//vec.SetPtEtaPhi( pho_pt, phoEta[i], phoPhi[i] );
	vec.SetPtEtaPhi( pho_pt_corr, phoEta[i], phoPhi[i] );
	
	if ( phoPt[i] < 20.0 )
	  //if ( phoE[i]/cosh( phoEta[i] ) < 24.0 )
	  {
	    if ( _phodebug ) std::cout << "[DEBUG]: failed pt" << std::endl;
	    //continue;
	  }
	
	if( fabs(pho_superClusterEta[i]) > 2.5 )
	  {
	    //allow photons in the endcap here, but if one of the two leading photons is in the endcap, reject the event
	    if ( _phodebug ) std::cout << "[DEBUG]: failed eta" << std::endl;
	    continue; 
	  }
	
	if ( fabs(pho_superClusterEta[i]) > 1.4442 && fabs(pho_superClusterEta[i]) < 1.566 )
	  {
	    //Removing gap photons
	    if ( _phodebug ) std::cout << "[INFO]: failed gap" << std::endl;
	    continue;
	  }
	//photon passes
	if( phoPt[i] > 40.0 ) nPhotonsAbove40GeV++;
	//setting up photon 4-momentum with zero mass
	TLorentzVector thisPhoton;
	thisPhoton.SetVectM( vec, .0 );

	//uncorrected photon 4-momentum
	TVector3 vtx( pvX, pvY, pvZ );
	TVector3 phoPos;
	if ( fabs( pho_superClusterEta[i] ) < 1.479 )
	  {
	    phoPos.SetXYZ( EB_R*cos( pho_superClusterPhi[i]), EB_R*sin( pho_superClusterPhi[i] ), EB_R*sinh( pho_superClusterEta[i] ) );
	  }
	else
	  {
	    double R = fabs( EE_Z/sinh( pho_superClusterEta[i] ) );
	    
	    if ( pho_superClusterEta[i] > .0 )
	      {
		phoPos.SetXYZ( R*cos( pho_superClusterPhi[i] ), R*sin( pho_superClusterPhi[i] ), EE_Z);
	      }
	    else
	      {
		phoPos.SetXYZ( R*cos( pho_superClusterPhi[i] ), R*sin( pho_superClusterPhi[i] ), -EE_Z);
	      }
	    
	  }
	
	TLorentzVector phoSC = GetCorrectedMomentum( vtx, phoPos, pho_superClusterEnergy[i] );
	
	//std::cout << "phoSC_Pt: " << phoSC.Pt() << " phoCorrPt: " << thisPhoton.Pt() << std::endl;
	//std::cout << "phoSC_Eta: " << phoSC.Eta() << " originalSC_Eta: " << pho_superClusterEta[i] << std::endl;
	//Filling Photon Candidate
	PhotonCandidate tmp_phoCand;
	tmp_phoCand.Index = i;
	tmp_phoCand.photon = thisPhoton;
	tmp_phoCand.photonSC = phoSC;
	tmp_phoCand.scEta = pho_superClusterEta[i];
	tmp_phoCand.scEta = pho_superClusterPhi[i];
	tmp_phoCand.SigmaIetaIeta = phoSigmaIetaIeta[i];
	tmp_phoCand.R9 = phoR9[i];
	tmp_phoCand.HoverE = pho_HoverE[i];
	tmp_phoCand.sumChargedHadronPt = pho_sumChargedHadronPt[i];
	tmp_phoCand.sumNeutralHadronEt = pho_sumNeutralHadronEt[i];
	tmp_phoCand.sumPhotonEt = pho_sumPhotonEt[i];
	tmp_phoCand.sigmaEOverE = pho_RegressionEUncertainty[i]/pho_RegressionE[i];
	tmp_phoCand._passEleVeto = pho_passEleVeto[i];
	tmp_phoCand._passIso = photonPassLooseIso(i,use25nsSelection);
	phoCand.push_back( tmp_phoCand );
	
	nSelectedPhotons++;
      }
    
    //if there is no photon with pT above 40 GeV, reject the event
    if( nPhotonsAbove40GeV == 0 )
      {
	if ( _debug ) std::cout << "[DEBUG]: no photons above 40 GeV, nphotons: " << phoCand.size() << std::endl;
	continue;
      }
    
    if ( phoCand.size() < 2 )
      {
	if ( _debug ) std::cout << "[INFO]: not enough photon, nphotons: " << phoCand.size() << std::endl;
	for(int i = 0; i < nPhotons; i++)
	  {
	    if ( _debug ) std::cout << "pho# " << i << " phopt1: " << phoPt[i] << " pho_eta: " << phoEta[i] 
				    << " SIetaIeta: " << phoSigmaIetaIeta[i] << std::endl;
	  }
	continue;
      }
    

    if ( _debug ) std::cout << "[DEBUG]: nphotons--> " << phoCand.size() << " " << nSelectedPhotons << std::endl;
    //find the "best" photon pair, higher Pt!
    TLorentzVector HiggsCandidate(0,0,0,0);
    int goodPhoIndex1 = -1;
    int goodPhoIndex2 = -1;
    double bestSumPt = -99.;
    std::vector< PhotonCandidate > phoSelectedCand;
    for(size_t i = 0; i < phoCand.size(); i++){
      for(size_t j = i+1; j < phoCand.size(); j++){//I like this logic better, I find it easier to understand
	PhotonCandidate pho1 = phoCand[i];
	PhotonCandidate pho2 = phoCand[j];
        if ( _debug )
	  {
	    std::cout << "[DEBUG]: pho1-> " << pho1.photon.Pt()
		      << " [DEBUG]: pho2->" << pho2.photon.Pt() 
		      << std::endl;
	  }
	//need one photon in the pair to have pt > 40 GeV
	if ( pho1.photon.Pt() < 40.0 && pho2.photon.Pt() < 40.0 )
	  {
	    if ( _debug ) std::cout << "[DEBUG]: both photons failed PT > 40 GeV" << std::endl; 
	    //continue;
	  }
	//need diphoton mass between > 100 GeV as in AN (April 1st)
	double diphotonMass = (pho1.photon + pho2.photon).M();
	if ( _debug )
	  {
	    std::cout << "[DEBUG] Diphoton Sum pT: " << pho1.photon.Pt() + pho2.photon.Pt() << std::endl;
	  }
	
	if( diphotonMass < 100 )
	  {
	    if ( _debug ) std::cout << "[DEBUG]: Diphoton mass < 100 GeV: mgg->" << diphotonMass << std::endl;
	    if ( _debug ) std::cout << "... pho1Pt: " << pho1.photon.Pt()  << " pho2Pt: " << pho2.photon.Pt()  << std::endl;
	    continue;
	  }
        
	//if the sum of the photon pT's is larger than that of the current Higgs candidate, make this the Higgs candidate
	if( pho1.photon.Pt() + pho2.photon.Pt() > bestSumPt ){
	  bestSumPt = pho1.photon.Pt() + pho2.photon.Pt();
	  HiggsCandidate = pho1.photon + pho2.photon;
	  if ( pho1.photon.Pt() >= pho2.photon.Pt() )
	    {
	      if ( _debug ) std::cout << "assign photon candidate, pho1Pt > pho2Pt" << std::endl;
	      phoSelectedCand.push_back(pho1);
	      phoSelectedCand.push_back(pho2);
	      goodPhoIndex1 = pho1.Index;
	      goodPhoIndex2 = pho2.Index;  
	    }
	  else
	    {
	      if ( _debug ) std::cout << "assign photon candidate, pho2Pt > pho1Pt" << std::endl;
	      phoSelectedCand.push_back(pho2);
	      phoSelectedCand.push_back(pho1);
	      goodPhoIndex1 = pho2.Index;
              goodPhoIndex2 = pho1.Index;
	    }
	  
	}
      }
    }   

    
    //Filling Selected Photon Information
    TLorentzVector pho_cand_vec[2];
    int _pho_index = 0;
    for ( auto& tmpPho : phoSelectedCand )
      {
	if ( !( tmpPho.Index == goodPhoIndex1 || tmpPho.Index == goodPhoIndex2 ) ) continue;
	if( _pho_index > 1 ) std::cerr << "[ERROR]: Photon index larger than 1!" << std::endl;
	pho_cand_vec[_pho_index]           = tmpPho.photon;
	Pho_E[_pho_index]                  = tmpPho.photon.E();
	Pho_Pt[_pho_index]                 = tmpPho.photon.Pt();
	Pho_Eta[_pho_index]                = tmpPho.photon.Eta();
	Pho_Phi[_pho_index]                = tmpPho.photon.Phi();
	PhoSC_E[_pho_index]                = tmpPho.photonSC.E();
	PhoSC_Pt[_pho_index]               = tmpPho.photonSC.Pt();
	PhoSC_Eta[_pho_index]              = tmpPho.photonSC.Eta();
	PhoSC_Phi[_pho_index]              = tmpPho.photonSC.Phi();
	Pho_SigmaIetaIeta[_pho_index]      = tmpPho.SigmaIetaIeta;
	Pho_R9[_pho_index]                 = tmpPho.R9;
	Pho_HoverE[_pho_index]             = tmpPho.HoverE;
	Pho_sumChargedHadronPt[_pho_index] = tmpPho.sumChargedHadronPt;
	Pho_sumNeutralHadronEt[_pho_index] = tmpPho.sumNeutralHadronEt;
	Pho_sumPhotonEt[_pho_index]        = tmpPho.sumPhotonEt;
	//Pho_sigmaEOverE[_pho_index]        = tmpPho.sigmaEOverE - 0.0025;
	Pho_sigmaEOverE[_pho_index]        = tmpPho.sigmaEOverE;
	Pho_passEleVeto[_pho_index]        = tmpPho._passEleVeto;
	Pho_passIso[_pho_index]            = tmpPho._passIso;

	_pho_index++;
      }
    
    //removing events with less than two good photon candidates
    if ( _pho_index < 2 ) continue;
    
    if ( _debug )
      {
	std::cout << "[DEBUG]: best photon pair: " 
		  << "\n-> pho1Pt: " << Pho_Pt[0] 
		  << "\n-> pho2Pt: " << Pho_Pt[1] 
		  << std::endl;
      }

    //if the best candidate pair has pT < 20 GeV, reject the event
    if( HiggsCandidate.Pt() < 20.0 )
      {
	if ( _debug ) std::cout << "[DEBUG]: Higgs Pt < 20 GeV, H pt: " << HiggsCandidate.Pt() << std::endl; 
	continue;
      }
    
    //if the best candidate pair has a photon in the endcap, reject the event
    if ( fabs( Pho_Eta[0] ) > 1.44 || fabs( Pho_Eta[1] ) > 1.44 )
      {
	//allow for now, to sync with alex, probably good idea to keep them to debug
	//continue;
      }
    
    //if the best candidate pair has a non-isolated photon, reject the event
    if( !Pho_passIso[0] || !Pho_passIso[1] )
      {
	if ( _debug ) std::cout << "[DEBUG]: Failed ISO: pho1, pho2: " << Pho_passIso[0] << ", " << Pho_passIso[1] << std::endl;
	if ( _debug ) std::cout << "[DEBUG]: pho1Pt: " << Pho_Pt[0] << " pho2Pt: " << Pho_Pt[1] << std::endl;
	for ( auto& phoC : phoSelectedCand )
	  {
	    if ( _debug ) std::cout << "===> phopt: " << phoC.photon.Pt() << " phoEta: " << phoC.photon.Eta() << std::endl;
	  }
	continue;
      }
    //record higgs candidate info
    mGammaGamma = HiggsCandidate.M();
    pTGammaGamma = HiggsCandidate.Pt();
    if ( _debug ) std::cout << "[DEBUG]: mgg-> " << mGammaGamma << " pTgg->" << pTGammaGamma << std::endl;
    

    //***********************************************************
    //get mother ID of photons
    //***********************************************************
    // cout << "Photon1 : " << Pho_Pt[0] << " " << Pho_Eta[0] << " " << Pho_Phi[0] << "\n";
    for(int g = 0; g < nGenParticle; g++){
      if (!(deltaR(gParticleEta[g] , gParticlePhi[g], Pho_Eta[0],Pho_Phi[0]) < 0.5) ) continue;
      if(gParticleStatus[g] != 1) continue;
      if(gParticleId[g] != 22) continue;
      Pho_motherID[0] = gParticleMotherId[g];
      //cout << "Nearby GenParticle: " << gParticlePt[g] << " " << gParticleEta[g] << " " << gParticlePhi[g] << " : " << gParticleMotherId[g] << "\n";
    }

    // cout << "Photon2 : " << Pho_Pt[1] << " " << Pho_Eta[1] << " " << Pho_Phi[1] << "\n";
    for(int g = 0; g < nGenParticle; g++){
      if (!(deltaR(gParticleEta[g] , gParticlePhi[g], Pho_Eta[1],Pho_Phi[1]) < 0.5) ) continue;
      if(gParticleStatus[g] != 1) continue;
      if(gParticleId[g] != 22) continue;
      Pho_motherID[1] = gParticleMotherId[g];      
      //cout << "Nearby GenParticle: " << gParticlePt[g] << " " << gParticleEta[g] << " " << gParticlePhi[g] << " : " << gParticleMotherId[g] << "\n";
    }

    // cout << "\nGenParticles:\n";
    // for(int g = 0; g < nGenParticle; g++){
    //   cout << "GenParticle: " << gParticleId[g] << " " << gParticleStatus[g] << " : " << gParticlePt[g] << " " << gParticleEta[g] << " " << gParticlePhi[g] << " : " << gParticleMotherId[g] << "\n";
    // }
    // cout << "\n\n";




    //Jets
    vector<TLorentzVector> GoodJets;
    vector< pair<TLorentzVector, bool> > GoodCSVLJets; //contains CSVL jets passing selection.  The bool is true if the jet passes CSVM, false if not
    
    for(int i = 0; i < nJets; i++){
      //Jet Corrections                                                                      
      double JEC = JetEnergyCorrectionFactor( jetPt[i], jetEta[i], jetPhi[i], jetE[i],
					      fixedGridRhoAll, jetJetArea[i],
					      JetCorrector );
      
      TLorentzVector thisJet = makeTLorentzVector( jetPt[i]*JEC, jetEta[i], jetPhi[i], jetE[i]*JEC );
      
      if( thisJet.Pt() < 30.0 ) continue;//According to the April 1st 2015 AN
      if( fabs( thisJet.Eta() ) >= 3.0 ) continue;
      //int level = 2; //loose jet ID
      if ( !jetPassIDLoose[i] ) continue;
      //if ( !((jetPileupIdFlag[i] & (1 << level)) != 0) ) continue;
      
      //exclude selected photons from the jet collection
      double deltaRJetPhoton = min( thisJet.DeltaR( pho_cand_vec[0] ), thisJet.DeltaR( pho_cand_vec[1] ) );
      if ( deltaRJetPhoton <= 0.5 ) continue;//According to the April 1st 2015 AN
      
      GoodJets.push_back(thisJet);
      n_Jets++;
      
      /*
	Change to isCSVL and isCSVM if you want CISV
      */
      if( isCSVL(i) ){
	nLooseBTaggedJets++;
	if( isCSVM(i) ){ 
	  nMediumBTaggedJets++;
	  GoodCSVLJets.push_back(make_pair(thisJet, true));
	}
	else{
	  GoodCSVLJets.push_back(make_pair(thisJet, false));
	}
      }
    }
    
    //if there are no good jets, reject the event
    if( n_Jets == 0 )
      {
	if ( _debug ) std::cout << "[DEBUG]: No Jets Selected" << std::endl;
	continue;
      }

    //std::cout << "njets-->" << n_Jets << std::endl;
    
    int iJet = 0;
    for ( auto tmp_jet : GoodJets )
      {
	jet_E[iJet] = tmp_jet.E();
	jet_Pt[iJet] = tmp_jet.Pt();
	jet_Eta[iJet] = tmp_jet.Eta();
	jet_Phi[iJet] = tmp_jet.Phi();
	iJet++;
      }
    
    //Compute the razor variables using the selected jets and the diphoton system
    vector<TLorentzVector> JetsPlusHiggsCandidate;
    for( auto& jet : GoodJets ) JetsPlusHiggsCandidate.push_back(jet);
    JetsPlusHiggsCandidate.push_back(HiggsCandidate);
    
    TLorentzVector PFMET = makeTLorentzVectorPtEtaPhiM(metPt, 0, metPhi, 0);
    //TLorentzVector t1PFMET = makeTLorentzVectorPtEtaPhiM( metType0Plus1Pt, 0, metType0Plus1Phi, 0 );
    TLorentzVector t1PFMET = makeTLorentzVectorPtEtaPhiM( metType1Pt, 0, metType1Phi, 0 );
    
    vector<TLorentzVector> hemispheres = getHemispheres(JetsPlusHiggsCandidate);
    theMR  = computeMR(hemispheres[0], hemispheres[1]); 
    theRsq = computeRsq(hemispheres[0], hemispheres[1], PFMET);
    t1Rsq  = computeRsq(hemispheres[0], hemispheres[1], t1PFMET);
    MET = metPt;
    //t1MET = metType0Plus1Pt;
    t1MET = metType1Pt;
    //if MR < 200, reject the event
    if ( theMR < 0.0 )
      {
	if ( _debug ) std::cout << "[INFO]: MR < 150 GeV, MR: " << theMR << std::endl;
	for ( auto& jet : JetsPlusHiggsCandidate )
	  {
	    if ( _debug ) std::cout << "phoPT: " << pTGammaGamma 
				    << " jet pt : " << jet.Pt() << " eta: " << jet.Eta() << " phi: " << jet.Phi() 
				    << " h1 pt: " << hemispheres[0].Pt() << " h1 eta: " << hemispheres[0].Eta()
				    << " h2 pt: " << hemispheres[1].Pt() << " h2 eta: " << hemispheres[1].Eta() << std::endl;
	  }
	//continue;
      }
    
    //if there are two loose b-tags and one medium b-tag, look for b-bbar resonances
    if( nLooseBTaggedJets > 1 && nMediumBTaggedJets > 0 )
      {
	for(int i = 0; i < nLooseBTaggedJets; i++){
	  for(int j = i+1; j < nLooseBTaggedJets; j++){
	    //if neither of the b-jets passes CSVM, continue
	    if( !GoodCSVLJets[i].second && !GoodCSVLJets[j].second ) continue;
	    double mbb = (GoodCSVLJets[i].first + GoodCSVLJets[j].first).M();
	    //if mbb is closer to the higgs mass than mbbH, make mbbH = mbb
	    if( fabs(mbbH - 125.0) > fabs(mbb - 125.0) ) mbbH = mbb;
	    //same for mbbZ
	    if( fabs(mbbZ - 91.2) > fabs(mbb - 91.2) ) mbbZ = mbb;
	  }//end second jet loop
	}//end first jet loop
      }
  

    //------------------------------------------------
    //I n v a ri a n t   m a s s   r e s o l u t i o n
    //------------------------------------------------
    sigmaMoverM = 0.5*sqrt( Pho_sigmaEOverE[0]*Pho_sigmaEOverE[0] + Pho_sigmaEOverE[1]*Pho_sigmaEOverE[1] );
    
    if ( _debug ) std::cout << "mbbH: " << mbbH << " mbbZ: " << mbbZ << std::endl;
    //Writing output to tree
    //HighPt Box
    if ( pTGammaGamma > 110.0 )
      {
	if(combineTrees){
	  if ( _debug ) std::cout << "[DEBUG]: combineTrees" << std::endl;
	  razorbox = HighPt;
	  if ( _debug ) std::cout << "[DEBUG]: combineTrees 1" << std::endl;
	  razorTree->Fill();
	  if ( _debug ) std::cout << "[DEBUG]: combineTrees 2" << std::endl;
	}
	else razorBoxes["HighPt"]->Fill();
      }
    //Hbb Box
    else if ( mbbH > 110.0 && mbbH < 140.0 )
      {
	if(combineTrees){
	  razorbox = Hbb;
	  razorTree->Fill();
	}
	else razorBoxes["Hbb"]->Fill();
      }
    //Zbb Box
    else if( mbbZ > 76.0 && mbbZ < 106.0 )
      {
	if(combineTrees){
	  razorbox = Zbb;
	  razorTree->Fill();
	}
	else razorBoxes["Zbb"]->Fill();
      }
    //HighRes Box
    else if( Pho_sigmaEOverE[0] < 0.015 && Pho_sigmaEOverE[1] < 0.015 )
      {
	if(combineTrees){
	  razorbox = HighRes;
	  razorTree->Fill();
	}
	else razorBoxes["HighRes"]->Fill();
      }
    //LowRes Box
    else
      {
	if(combineTrees){
	  razorbox = LowRes;
	  razorTree->Fill();
	}
	else razorBoxes["LowRes"]->Fill();
      }
    if ( _debug ) std::cout << "[DEBUG]: combineTrees 3?" << std::endl;
    //end of event loop
  }
  
  if ( _info ) std::cout << "[INFO]: Number of events processed: " << NEvents->Integral() << std::endl;
  if ( _info ) std::cout << "[INFO]: Writing output trees..." << std::endl;
  if(combineTrees) razorTree->Write();
  else for(auto& box : razorBoxes) box.second->Write();
  NEvents->Write();
  
  outFile.Close();
}
