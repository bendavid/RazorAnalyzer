import sys
import argparse
import ROOT as rt

#local imports
from RunCombine import exec_me
import macro.macro as macro
from macro.razorAnalysis import *
from macro.razorWeights import *
from macro.razorMacros import *

LUMI = 133 #in /pb
MCLUMI = 1 

SAMPLES_SIGNAL = ["TTV", "VV", "DYJets", "ZInv", "SingleTop", "WJets", "TTJets"]

DIR_MC= "root://eoscms:///eos/cms/store/group/phys_susy/razor/Run2Analysis/RazorInclusive/V1p19_ForFullStatus20151030/MC_FullRazorInclusive/combined"
DIR_DATA= "root://eoscms:///eos/cms/store/group/phys_susy/razor/Run2Analysis/RazorInclusive/V1p20_ForFullStatus20151030/Data"
PREFIX_SIGNAL = "RazorInclusive"
DATA_NAME='SingleLepton_Run2015D_GoodLumiUnblind_NoDuplicates_razorskim'
FILENAMES_SIGNAL = {
        "TTJets"    : DIR_MC+"/"+PREFIX_SIGNAL+"_TTJets_TuneCUETP8M1_13TeV-amcatnloFXFX-pythia8_1pb_weighted.root",
        "WJets"     : DIR_MC+"/"+PREFIX_SIGNAL+"_WJets_1pb_weighted.root",
        "SingleTop" : DIR_MC+"/"+PREFIX_SIGNAL+"_SingleTop_1pb_weighted.root",
        "VV" : DIR_MC+"/"+PREFIX_SIGNAL+"_VV_1pb_weighted.root",
        "TTV" : DIR_MC+"/"+PREFIX_SIGNAL+"_TTV_1pb_weighted.root",
        "DYJets"     : DIR_MC+"/"+PREFIX_SIGNAL+"_DYJetsToLL_1pb_weighted.root",
        "ZInv"     : DIR_MC+"/"+PREFIX_SIGNAL+"_ZJetsToNuNu_1pb_weighted.root",
        #"Data"      : DIR_DATA+'/'+PREFIX_SIGNAL+'_'+DATA_NAME+'.root',
        }

config = "config/backup.config"
FIT_DIR = "FitResultsFull"
TOYS_FILES = {
        "MuMultiJet":FIT_DIR+"/toys_Bayes_MuMultiJet.root",
        "EleMultiJet":FIT_DIR+"/toys_Bayes_EleMultiJet.root",
        }

weightOpts = ["doNVtxWeights"]
shapeErrors = []
#shapeErrors = ["muoneff", "eleeff", "jes"]
miscErrors = ["mt"]

if __name__ == "__main__":
    rt.gROOT.SetBatch()

    #parse args
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbose", help="display detailed output messages",
                                action="store_true")
    parser.add_argument("-d", "--debug", help="display excruciatingly detailed output messages",
                                action="store_true")
    args = parser.parse_args()
    debugLevel = args.verbose + 2*args.debug

    #initialize
    weightHists = loadWeightHists(weightfilenames_DEFAULT, weighthistnames_DEFAULT, debugLevel)
    sfHists = {}

    #get scale factor histograms
    #sfHists = {}
    sfHists = loadScaleFactorHists(processNames=SAMPLES_SIGNAL, debugLevel=debugLevel)

    #estimate yields in signal region
    for lepType in ["Ele"]:
    #for lepType in ["Mu", "Ele"]:
        for jets in ["MultiJet"]:
            boxName = lepType+jets
            btaglist = [0]
            #btaglist = [0,1,2]
            for btags in btaglist:
                print "\n---",boxName,"Box,",btags,"B-tags ---"
                #get correct cuts string
                thisBoxCuts = razorCuts[boxName]
                if btags < len(btaglist)-1:
                    thisBoxCuts += " && nBTaggedJets == "+str(btags)
                else:
                    thisBoxCuts += " && nBTaggedJets >= "+str(btags)

                if len(btaglist) > 1:
                    extboxName = boxName+str(btags)+"BTag"
                else:
                    extboxName = boxName
                #check fit file and create if necessary
                if not os.path.isfile(TOYS_FILES[boxName]):
                    print "Fit file",TOYS_FILES[boxName],"not found, trying to recreate it"
                    runFitAndToys(FIT_DIR, boxName, LUMI, PREFIX_SIGNAL+'_'+DATA_NAME, DIR_DATA, config=config)
                    #check
                    if not os.path.isfile(TOYS_FILES[boxName]):
                        print "Error creating fit file",TOYS_FILES[boxName]
                        sys.exit()
                makeControlSampleHists(extboxName, 
                        filenames=FILENAMES_SIGNAL, samples=SAMPLES_SIGNAL, 
                        cutsMC=thisBoxCuts, cutsData=thisBoxCuts, 
                        bins=leptonicSignalRegionBins, lumiMC=MCLUMI, lumiData=LUMI, 
                        weightHists=weightHists, sfHists=sfHists, treeName="RazorInclusive", 
                        weightOpts=weightOpts, shapeErrors=shapeErrors, miscErrors=miscErrors,
                        fitToyFiles=TOYS_FILES, boxName=boxName, debugLevel=debugLevel)
