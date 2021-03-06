//LOCAL INCLUDES
#include "RazorAnalyzer.h"


bool RazorAnalyzer::photonPassesElectronVeto(int i){
    //use presence of a pixel seed as proxy for an electron veto
    return (pho_passEleVeto[i]);
}


void RazorAnalyzer::getPhotonEffAreaRun2( float eta, double& effAreaChHad, double& effAreaNHad, double& effAreaPho )
{
  if( fabs( eta ) < 1.0 )
    {
      effAreaChHad = 0.0157;
      effAreaNHad  = 0.0143;
      effAreaPho   = 0.0725;
    }
  else if( fabs( eta ) < 1.479 )
    {
      effAreaChHad = 0.0143;
      effAreaNHad  = 0.0210;
      effAreaPho   = 0.0604;
    }
  else if( fabs( eta ) < 2.0 )
    {
      effAreaChHad = 0.0115;
      effAreaNHad  = 0.0148;
      effAreaPho   = 0.0320;
    }
  else if( fabs( eta ) < 2.2 )
    {
      effAreaChHad = 0.0094;
      effAreaNHad  = 0.0082;
      effAreaPho   = 0.0512;
    }
  else if( fabs( eta ) < 2.3 )
    {
      effAreaChHad = 0.0095;
      effAreaNHad  = 0.0124;
      effAreaPho   = 0.0766;
    }
  else if( fabs( eta ) < 2.4 )
    {
      effAreaChHad = 0.0068;
      effAreaNHad  = 0.0186;
      effAreaPho   = 0.0949;
    }
  else
    {
      effAreaChHad = 0.0053;
      effAreaNHad  = 0.0320;
      effAreaPho   = 0.1160;
    }
};

void RazorAnalyzer::getPhotonEffArea90( float eta, double& effAreaChHad, double& effAreaNHad, double& effAreaPho )
{
  if( fabs( eta ) < 1.0 )
    {
      effAreaChHad = 0.0456;
      effAreaNHad  = 0.0599;
      effAreaPho   = 0.1271;
    }
  else if( fabs( eta ) < 1.479 )
    {
      effAreaChHad = 0.0500;
      effAreaNHad  = 0.0819;
      effAreaPho   = 0.1101;
    }
  else if( fabs( eta ) < 2.0 )
    {
      effAreaChHad = 0.0340;
      effAreaNHad  = 0.0696;
      effAreaPho   = 0.0756;
    }
  else if( fabs( eta ) < 2.2 )
    {
      effAreaChHad = 0.0383;
      effAreaNHad  = 0.0360;
      effAreaPho   = 0.1175;
    }
  else if( fabs( eta ) < 2.3 )
    {
      effAreaChHad = 0.0339;
      effAreaNHad  = 0.0360;
      effAreaPho   = 0.1498;
    }
  else if( fabs( eta ) < 2.4 )
    {
      effAreaChHad = 0.0303;
      effAreaNHad  = 0.0462;
      effAreaPho   = 0.1857;
    }
  else
    {
      effAreaChHad = 0.0240;
      effAreaNHad  = 0.0656;
      effAreaPho   = 0.2183;
    }
};



//photon ID and isolation cuts from https://twiki.cern.ch/twiki/bin/viewauth/CMS/EgammaIDRecipesRun2
bool RazorAnalyzer::photonPassesIsolation(int i, double PFChHadIsoCut, double PFNeuHadIsoCut, double PFPhotIsoCut, bool useEffectiveArea90){
    //get effective area for isolation calculations
    double effAreaChargedHadrons = 0.0;
    double effAreaNeutralHadrons = 0.0;
    double effAreaPhotons = 0.0;

    //get the effective areas. results are passed to variables by reference
    if (useEffectiveArea90) 
      {
	getPhotonEffArea90( pho_superClusterEta[i] , effAreaChargedHadrons, effAreaNeutralHadrons, effAreaPhotons);
      }
    else 
      {
	getPhotonEffAreaRun2( pho_superClusterEta[i] , effAreaChargedHadrons, effAreaNeutralHadrons, effAreaPhotons);
      }

    //Rho corrected PF charged hadron isolation
    double PFIsoCorrected_ChHad = max(pho_sumChargedHadronPt[i] - fixedGridRhoFastjetAll*effAreaChargedHadrons, 0.);
    if(PFIsoCorrected_ChHad > PFChHadIsoCut) return false;
    
    //Rho corrected PF neutral hadron isolation
    double PFIsoCorrected_NeuHad = max(pho_sumNeutralHadronEt[i] - fixedGridRhoFastjetAll*effAreaNeutralHadrons, 0.);
    if(PFIsoCorrected_NeuHad > PFNeuHadIsoCut) return false;
    
    //Rho corrected PF photon isolation
    double PFIsoCorrected_Photons = max(pho_sumPhotonEt[i] - fixedGridRhoFastjetAll*effAreaPhotons, 0.);
    if(PFIsoCorrected_Photons > PFPhotIsoCut) return false;

    //photon passed all cuts
    return true;
}

bool RazorAnalyzer::photonPassLooseIDWithoutEleVeto(int i, bool use25nsCuts ){

  bool pass = true;

  if (use25nsCuts) {
    if(fabs(pho_superClusterEta[i]) < 1.479){    
      if(pho_HoverE[i] > 0.05) pass = false;
      if(phoFull5x5SigmaIetaIeta[i] > 0.0102) pass = false;   
    } else { 
      if(pho_HoverE[i] > 0.05) pass = false;
      if(phoFull5x5SigmaIetaIeta[i] > 0.0274) pass = false;    
    }
  } 

  //50 ns cuts below
  else {
    if(fabs(pho_superClusterEta[i]) < 1.479){    
      if(pho_HoverE[i] > 0.05) pass = false;
      if(phoFull5x5SigmaIetaIeta[i] > 0.0103) pass = false;   
    } else { 
      if(pho_HoverE[i] > 0.05) pass = false;
      if(phoFull5x5SigmaIetaIeta[i] > 0.0277) pass = false;    
    }
  }

  return pass;
}

bool RazorAnalyzer::photonPassMediumIDWithoutEleVeto(int i, bool use25nsCuts){
  bool pass = true;

  if (use25nsCuts) {
    if(fabs(pho_superClusterEta[i]) < 1.479){    
      if(pho_HoverE[i] > 0.05) pass = false;
      if(phoFull5x5SigmaIetaIeta[i] > 0.0100) pass = false;   
    } else { 
      if(pho_HoverE[i] > 0.05) pass = false;
      if(phoFull5x5SigmaIetaIeta[i] > 0.0266) pass = false;    
    }
  }

  //50 ns cuts below
  else {
    if(fabs(pho_superClusterEta[i]) < 1.479){    
      if(pho_HoverE[i] > 0.05) pass = false;
      if(phoFull5x5SigmaIetaIeta[i] > 0.0100) pass = false;   
    } else { 
      if(pho_HoverE[i] > 0.05) pass = false;
      if(phoFull5x5SigmaIetaIeta[i] > 0.0267) pass = false;    
    }
  }

  return pass;
}

bool RazorAnalyzer::photonPassTightIDWithoutEleVeto(int i, bool use25nsCuts){
  bool pass = true;

  if (use25nsCuts) {
    if(fabs(pho_superClusterEta[i]) < 1.479){    
      if(pho_HoverE[i] > 0.05) pass = false;
      if(phoFull5x5SigmaIetaIeta[i] > 0.0099) pass = false;   
    } else { 
      if(pho_HoverE[i] > 0.05) pass = false;
      if(phoFull5x5SigmaIetaIeta[i] > 0.0266) pass = false;    
    }
  }

  //50 ns cuts below
  else {
    if(fabs(pho_superClusterEta[i]) < 1.479){    
      if(pho_HoverE[i] > 0.05) pass = false;
      if(phoFull5x5SigmaIetaIeta[i] > 0.0100) pass = false;   
    } else { 
      if(pho_HoverE[i] > 0.05) pass = false;
      if(phoFull5x5SigmaIetaIeta[i] > 0.0267) pass = false;    
    }
  }

  return pass;
}


bool RazorAnalyzer::photonPassLooseID(int i, bool use25nsCuts){

  bool pass = true;

  if (!photonPassLooseIDWithoutEleVeto(i,use25nsCuts)) pass = false;
  if(!photonPassesElectronVeto(i)) pass = false;

  return pass;
}

bool RazorAnalyzer::photonPassMediumID(int i, bool use25nsCuts){
  bool pass = true;

  if (!photonPassMediumIDWithoutEleVeto(i,use25nsCuts)) pass = false;
  if(!photonPassesElectronVeto(i)) pass = false;

  return pass;
}

bool RazorAnalyzer::photonPassTightID(int i, bool use25nsCuts){
  bool pass = true;

  if (!photonPassTightIDWithoutEleVeto(i,use25nsCuts)) pass = false;
  if(!photonPassesElectronVeto(i)) pass = false;

  return pass;
}


bool RazorAnalyzer::photonPassLooseIso(int i, bool use25nsCuts){

  if (use25nsCuts) {
    if(fabs(pho_superClusterEta[i]) < 1.479){
      return photonPassesIsolation(i, 3.32, 1.92 + 0.0014*phoPt[i] + 0.000019*phoPt[i]*phoPt[i], 0.81 + 0.0053*phoPt[i], true );
    } else {
      return photonPassesIsolation(i, 1.97, 11.86 + 0.0139*phoPt[i] + 0.000025*phoPt[i]*phoPt[i], 0.83 + 0.0034*phoPt[i], true);
    }
  } 

  //50ns cuts below
  else {
    if(fabs(pho_superClusterEta[i]) < 1.479){
      return photonPassesIsolation(i, 2.44, 2.57+exp( 0.0044*phoPt[i] + 0.5809 ), 1.92 + 0.0043*phoPt[i], false );
    } else {
      return photonPassesIsolation(i, 1.84, 4.00+exp( 0.0040*phoPt[i] + 0.9402 ), 2.15 + 0.0041*phoPt[i], false);
    }
  }
}

bool RazorAnalyzer::photonPassMediumIso(int i, bool use25nsCuts){

  if (use25nsCuts) {
    if(fabs(pho_superClusterEta[i]) < 1.479){
      return photonPassesIsolation(i, 1.31, 2.46 + 0.0014*phoPt[i] + 0.000019*phoPt[i]*phoPt[i], 0.05 + 0.0053*phoPt[i], true);
    } else {
      return photonPassesIsolation(i, 0.95, 4.97 + 0.0139*phoPt[i] + 0.000025*phoPt[i]*phoPt[i], 0.14 + 0.0034*phoPt[i], true);
    }
  }

  //50ns cuts below
  else {
    if(fabs(pho_superClusterEta[i]) < 1.479){
      return photonPassesIsolation(i, 1.31, 0.60+exp( 0.0044*phoPt[i] + 0.5809 ), 1.33 + 0.0043*phoPt[i], false);
    } else {
      return photonPassesIsolation(i, 1.25, 1.65+exp( 0.0040*phoPt[i] + 0.9402 ), 1.02 + 0.0041*phoPt[i], false);
    }
  }
}

bool RazorAnalyzer::photonPassTightIso(int i, bool use25nsCuts){

  if (use25nsCuts) {
    if(fabs(pho_superClusterEta[i]) < 1.479){
        return photonPassesIsolation(i, 0.73, 1.14 + 0.0014*phoPt[i] + 0.000019*phoPt[i]*phoPt[i], 0.05 + 0.0053*phoPt[i], true);
    } else {
      return photonPassesIsolation(i, 0.27, 3.71 + 0.0139*phoPt[i] + 0.000025*phoPt[i]*phoPt[i], 0.11 + 0.0034*phoPt[i], true);
    }
  } 

  //50ns cuts below
  else {
    if(fabs(pho_superClusterEta[i]) < 1.479){
        return photonPassesIsolation(i, 0.91, 0.33+exp( 0.0044*phoPt[i] + 0.5809 ), 0.61 + 0.0043*phoPt[i], false);
    } else {
      return photonPassesIsolation(i, 0.65, 0.93+exp( 0.0040*phoPt[i] + 0.9402 ), 0.54 + 0.0041*phoPt[i], false);
    }
  }
}

bool RazorAnalyzer::isLoosePhoton(int i, bool use25nsCuts){

  bool pass = true;
  if(!isLoosePhotonWithoutEleVeto(i,use25nsCuts)) pass = false;
  if(!photonPassesElectronVeto(i)) pass = false;

  return pass;
}

bool RazorAnalyzer::isMediumPhoton(int i, bool use25nsCuts){
  bool pass = true;

  if(!isMediumPhotonWithoutEleVeto(i,use25nsCuts)) pass = false;
  if(!photonPassesElectronVeto(i)) pass = false;

  return pass;
}

bool RazorAnalyzer::isTightPhoton(int i, bool use25nsCuts){
  bool pass = true;
  if (!isTightPhotonWithoutEleVeto(i,use25nsCuts)) pass = false;
  if(!photonPassesElectronVeto(i)) pass = false;

  return pass;
}

bool RazorAnalyzer::isLoosePhotonWithoutEleVeto(int i, bool use25nsCuts){

  bool pass = true;

  if (!photonPassLooseIDWithoutEleVeto(i,use25nsCuts)) pass = false;
  if(!photonPassLooseIso(i,use25nsCuts)) pass = false;

  return pass;
}

bool RazorAnalyzer::isMediumPhotonWithoutEleVeto(int i, bool use25nsCuts){
  bool pass = true;

  if (!photonPassMediumIDWithoutEleVeto(i,use25nsCuts)) pass = false;
  if(!photonPassMediumIso(i,use25nsCuts)) pass = false;

  return pass;
}

bool RazorAnalyzer::isTightPhotonWithoutEleVeto(int i, bool use25nsCuts){
  bool pass = true;

  if (!photonPassTightIDWithoutEleVeto(i,use25nsCuts)) pass = false;
  if(!photonPassTightIso(i,use25nsCuts)) pass = false;

  return pass;
}


bool RazorAnalyzer::matchPhotonHLTFilters(int i, string HLTFilter){
  bool match = false;

  if (HLTFilter == "DiPhoton30_18_WithPixMatch_Leg1") {
    if ( 
	//Data filters
	pho_passHLTFilter[i][8] 
	//MC filters

	 ) {
      match = true;
    }
  }
   
  if (HLTFilter == "DiPhoton30_18_WithPixMatch_Leg2") {
    if ( 
	//Data filters
	pho_passHLTFilter[i][9] 
	 ) {
      match = true;
    }
  }
   
  return match;
}

TLorentzVector RazorAnalyzer::GetCorrectedMomentum( TVector3 vtx, TVector3 phoPos, double phoE )
{
  TVector3 phoDir = phoPos - vtx;
  TVector3 phoP3  = phoDir.Unit()*phoE;
  return TLorentzVector( phoP3, phoE);
};

bool RazorAnalyzer::photonPassLooseIDWithoutEleVetoExo15004( int i )
{
  bool pass = true;
  if ( fabs(pho_superClusterEta[i]) < 1.4442 )
    {
      if( pho_HoverE[i] > 0.05 ) pass = false;
      if( phoFull5x5SigmaIetaIeta[i] > 0.0105 ) pass = false;
    }
  else
    {
      if( pho_HoverE[i] > 0.05 ) pass = false;
      if( phoFull5x5SigmaIetaIeta[i] > 0.0280 ) pass = false;
    }
  return pass;
};
 
bool RazorAnalyzer::photonPassesIsolationExo15004(int i, double PFChHadIsoCut, double PFPhotIsoCut )
{
  double effAreaPhotons = 0.0;
  double eta = pho_superClusterEta[i];
  getPhotonEffAreaExo15004( eta, effAreaPhotons );
  
  //Rho corrected PF charged hadron isolation
  //double PFIsoCorrected_ChHad = pho_sumChargedHadronPt[i];//No PU correction (Caltech Original)
  double PFIsoCorrected_ChHad = pho_pfIsoChargedHadronIso[i];//(Exo15004 default pfIso)
  if(PFIsoCorrected_ChHad > PFChHadIsoCut) return false;
    
  //Rho corrected PF photon isolation
  //double PFIsoCorrected_Photons = pho_sumPhotonEt[i] - fixedGridRhoFastjetAll*effAreaPhotons;//PU corr can go neg!(Caltech Original)
  double PFIsoCorrected_Photons = pho_pfIsoPhotonIso[i] - fixedGridRhoAll*effAreaPhotons;//PU corr can go neg!(Exo15004 default pfIso)
  if(PFIsoCorrected_Photons > PFPhotIsoCut) return false;
  
  //photon passed all cuts
  return true;
};

void RazorAnalyzer::getPhotonEffAreaExo15004( float eta, double& effAreaPho )
{
  if ( fabs( eta ) < 0.9 )
    {
      effAreaPho = 0.17;
    }
  else if ( fabs( eta ) < 1.4442 )
    {
      effAreaPho = 0.14;
    }
  else if ( fabs( eta ) < 2.0 )
    {
      effAreaPho = 0.11;
    }
  else if ( fabs( eta ) < 2.2 )
    {
      effAreaPho = 0.14;
    }
  else
    {
      effAreaPho = 0.22;
    }
};

bool RazorAnalyzer::photonPassLooseIsoExo15004(int i)
{
  if( fabs(pho_superClusterEta[i]) < 1.4442 )
    {
      return photonPassesIsolationExo15004(i, 5, (2.75 - 2.5) + 0.0045*phoPt[i] );
    } 
  else if ( fabs(pho_superClusterEta[i]) < 2.0 )
    {
      return photonPassesIsolationExo15004(i, 5, (2.0 - 2.5) + 0.003*phoPt[i] );
    }
  else
    {
      return photonPassesIsolationExo15004(i, 5, (2.0 - 2.5) + 0.003*phoPt[i] );
    }
};
