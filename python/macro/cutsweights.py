import ROOT as rt 

def cuts_TTJetsSingleLepton(event, debug=False):
    """Run 2 TTBar single lepton selection"""
    #HLT requirement
    passedTrigger = event.HLTDecision[0] or event.HLTDecision[1] or event.HLTDecision[8] or event.HLTDecision[9]
    if not passedTrigger: return False
    if debug: print("Event passes TTBarSingleLepton trigger requirement")
    #lepton = tight ele or mu with pt > 30
    if abs(event.lep1Type) != 11 and abs(event.lep1Type) != 13: return False
    if not event.lep1PassTight: return False
    if event.lep1Pt < 30: return False
    if debug: print("Event passes TTBarSingleLepton lepton requirements")
    #MET and MT cuts
    if event.MET < 30: return False
    if event.lep1MT < 30 or event.lep1MT > 100: return False
    #b-tag requirement
    if event.NBJetsMedium < 1: return False
    if debug: print("Event passes TTBarSingleLepton MET, MT, and btag requirements")
    #razor baseline cut
    if event.MR < 300 or event.Rsq < 0.15: return False
    #passes selection
    if debug: print("Event passes run 2 TTBarSingleLepton control region selection")
    return True

def weight_mc(event, wHists, scale=1.0, debug=False):
    """Apply pileup weights and other known MC correction factors"""
    eventWeight = event.weight*scale
    #pileup reweighting
    #(comment out until PU weights are available)
    #if "pileup" in wHists:
    #    eventWeight *= wHists["pileup"].GetBinContent(wHists["pileup"].GetXaxis().FindFixBin(event.NPU_0))
    #else: 
    #    print("Error in standardRun2Weights: pileup reweighting histogram not found!")
    #    sys.exit()
    if debug: print("Applying a weight of "+str(eventWeight))
    return eventWeight

def weight_data(event, wHists, scale=1.0, debug=False):
    eventWeight = scale
    if debug: print("Applying a weight of "+str(eventWeight))
    return eventWeight