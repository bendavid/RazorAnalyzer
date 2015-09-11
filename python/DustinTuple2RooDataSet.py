from optparse import OptionParser
import ROOT as rt
import rootTools
from framework import Config
import sys
from array import *

k_T = 689.1/424.5
k_Z = 3.*2008.4/5482.
k_W = 3.*20508.9/50100.0

k_QCD = {}
    
boxes = {'MuEle':'(box==0)',
         'MuMu':'(box==1)',
         'EleEle':'(box==2)',
         'MuSixJet':'(box==3)',
         'MuFourJet':'(box==4)',
         'MuMultiJet':'(box==3||box==4)',
         'MuJet':'(box==5)',
         'EleSixJet':'(box==6)',
         'EleFourJet':'(box==7)',
         'EleMultiJet':'(box==6||box==7)',
         'EleJet':'(box==8)',
         'LooseLeptonSixJet':'(box==9)',
         'LooseLeptonFourJet':'(box==10)',
         'LooseLeptonMultiJet':'(box==9||box==10)',
         'SixJet':'(box==11)',
         'FourJet':'(box==12)',
         'FourToSixJet':'((box==11||box==12)&&(nSelectedJets>=4&&nSelectedJets<7))',
         'SevenJet':'((box==11||box==12)&&(nSelectedJets>=7))',
         'MultiJet':'(box==11||box==12)',
         'LooseLeptonDiJet':'(box==13)',
         'DiJet':'(box==14)'}

dPhiCut = 2.8
MTCut = -1

def initializeWorkspace(w,cfg,box):
    variables = cfg.getVariablesRange(box,"variables",w)
    
    w.factory('W[1.,0.,+INF]')
    w.set('variables').add(w.var('W'))
    return w

def getSumOfWeights(tree, cfg, box, workspace, useWeight, f, lumi, lumi_in):
    if f.find('SMS')!=-1:
        k = 1.
    elif f.find('TTJets')!=-1:
        k = k_T
    elif f.find('DYJets')!=-1 or f.find('ZJets')!=-1:
        k = k_Z
    elif f.find('WJets')!=-1:
        k = k_W
    else:
        k = 1.
        
    args = workspace.set("variables")
    
    #we cut away events outside our MR window
    mRmin = args['MR'].getMin()
    mRmax = args['MR'].getMax()

    #we cut away events outside our Rsq window
    rsqMin = args['Rsq'].getMin()
    rsqMax = args['Rsq'].getMax()

    btagMin =  args['nBtag'].getMin()
    btagMax =  args['nBtag'].getMax()
    
    z = array('d', cfg.getBinning(box)[2]) # nBtag binning
    
    btagCutoff = 3
    if box in ["MuEle", "MuMu", "EleEle"]:
        btagCutoff = 1
        
    boxCut = boxes[box]

    label = f.replace('.root','').split('/')[-1]
    htemp = rt.TH1D('htemp_%s'%label,'htemp_%s'%label,len(z)-1,z)

    if useWeight:
        tree.Project(htemp.GetName(),
                    'min(nBTaggedJets,%i)'%btagCutoff,
                    '(%f/%f) * %f * weight * (MR > %f && MR < %f && Rsq > %f && Rsq < %f && min(nBTaggedJets,%i) >= %i && min(nBTaggedJets,%i) < %f && %s && abs(dPhiRazor) < %f)' % (lumi,lumi_in,k,mRmin,mRmax,rsqMin,rsqMax,btagCutoff,btagMin,btagCutoff,btagMax,boxCut,dPhiCut))
    else:
        tree.Project(htemp.GetName(),
                    'MR',
                    '(MR > %f && MR < %f && Rsq > %f && Rsq < %f && min(nBTaggedJets,%i) >= %i && min(nBTaggedJets,%i) < %f && %s && abs(dPhiRazor) < %f)' % (mRmin,mRmax,rsqMin,rsqMax,btagCutoff,btagMin,btagCutoff,btagMax,boxCut,dPhiCut))
        
    return [htemp.GetBinContent(i) for i in range(1,len(z))]
        
    
def convertTree2Dataset(tree, cfg, box, workspace, useWeight, f, lumi, lumi_in, treeName='RMRTree'):
    """This defines the format of the RooDataSet"""
    
    z = array('d', cfg.getBinning(box)[2]) # nBtag binning
    
    if 'SMS' in f:
        k = [1. for z_bin in z[:-1]]
    elif 'TTJets' in f:
        k = [k_T*k_btag for k_btag in k_QCD[box]]
    elif 'DYJets' in f or 'ZJets' in f:
        k = [k_Z*k_btag for k_btag in k_QCD[box]]
    elif 'WJets' in f:
        k = [k_W*k_btag for k_btag in k_QCD[box]]
    else:
        k = k_QCD[box]

    args = workspace.set("variables")
    data = rt.RooDataSet(treeName,'Selected R and MR',args)
    
    #we cut away events outside our MR window
    mRmin = args['MR'].getMin()
    mRmax = args['MR'].getMax()

    #we cut away events outside our Rsq window
    rsqMin = args['Rsq'].getMin()
    rsqMax = args['Rsq'].getMax()

    btagMin =  args['nBtag'].getMin()
    btagMax =  args['nBtag'].getMax()
    
    label = f.replace('.root','').split('/')[-1]
    htemp = rt.TH1D('htemp2_%s'%label,'htemp2_%s'%label,len(z)-1,z)

    btagCutoff = 3
    if box in ["MuEle", "MuMu", "EleEle"]:
        btagCutoff = 1

    #use the optimal cuts for each box
    if box in ["DiJet", "FourJet", "SixJet", "MuMu", "MuEle", "EleEle", "MultiJet"]: 
        dPhiCut = 2.8
        MTCut = -1
    else: 
        dPhiCut = 3.2
        MTCut = 100

    boxCut = boxes[box]

    cuts = 'MR > %f && MR < %f && Rsq > %f && Rsq < %f && min(nBTaggedJets,%i) >= %i && min(nBTaggedJets,%i) < %i && %s && abs(dPhiRazor) < %f' % (mRmin,mRmax,rsqMin,rsqMax,btagCutoff,btagMin,btagCutoff,btagMax,boxCut,dPhiCut)
    if MTCut >= 0: #add MT cut
        if box in ["LooseLeptonDiJet", "LooseLeptonFourJet", "LooseLeptonSixJet", "LooseLeptonMultiJet"]:
            cuts = cuts + (' && mTLoose > %f' % MTCut)
        else:
            cuts = cuts + (' && mT > %f' % MTCut)
    
    tree.Draw('>>elist',cuts,'entrylist')
        
    elist = rt.gDirectory.Get('elist')
    
    entry = -1;
    while True:
        entry = elist.Next()
        if entry == -1: break
        tree.GetEntry(entry)

        #set the RooArgSet and save
        a = rt.RooArgSet(args)
        
        a.setRealValue('MR',tree.MR)
        a.setRealValue('Rsq',tree.Rsq)
        a.setRealValue('nBtag',min(tree.nBTaggedJets,btagCutoff))
        
        if useWeight:
            btag_bin = htemp.FindBin(min(tree.nBTaggedJets,btagCutoff)) - 1
            a.setRealValue('W',tree.weight*lumi*k[btag_bin]/lumi_in)
        else:
            a.setRealValue('W',1.0)
        data.add(a)
        
    numEntries = data.numEntries()
    
    wdata = rt.RooDataSet(data.GetName(),data.GetTitle(),data,data.get(),"MR>=0.","W")
    numEntriesByBtag = []
    sumEntriesByBtag = []
    for i in z[:-1]:
        numEntriesByBtag.append(wdata.reduce('nBtag==%i'%i).numEntries())
        sumEntriesByBtag.append(wdata.reduce('nBtag==%i'%i).sumEntries())
        
    print "Filename: %s"%f
    print "Scale Factors     [ %s ] ="%box, k
    print "Number of Entries [ %s ] ="%(box), numEntriesByBtag
    print "Sum of Weights    [ %s ] ="%(box), sumEntriesByBtag

    return wdata

    
if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option('-c','--config',dest="config",type="string",default="config/run2.config",
                  help="Name of the config file to use")
    parser.add_option('-d','--dir',dest="outDir",default="./",type="string",
                  help="Output directory to store datasets")
    parser.add_option('-w','--weight',dest="useWeight",default=False,action='store_true',
                  help="use weight")
    parser.add_option('-l','--lumi',dest="lumi", default=3000.,type="float",
                  help="integrated luminosity in pb^-1")
    parser.add_option('--lumi-in',dest="lumi_in", default=1.,type="float",
                  help="integrated luminosity in pb^-1")
    parser.add_option('-b','--box',dest="box", default="MultiJet",type="string",
                  help="box name")
    parser.add_option('-q','--remove-qcd',dest="removeQCD",default=False,action='store_true',
                  help="remove QCD, while augmenting remaining MC backgrounds")

    (options,args) = parser.parse_args()
    
    cfg = Config.Config(options.config)

    box =  options.box
    lumi = options.lumi
    lumi_in = options.lumi_in
    useWeight = options.useWeight
    removeQCD = options.removeQCD
    
    print 'Input files are %s' % ', '.join(args)
    
    w = rt.RooWorkspace("w"+box)

    variables = initializeWorkspace(w,cfg,box)    
    
    ds = []
        
    btagMin =  w.var('nBtag').getMin()
    btagMax =  w.var('nBtag').getMax()

    if removeQCD:
        # first get sum of weights for each background per b-tag bin ( sumW[label] )
        sumW = {}
        sumWQCD = 0.
        for f in args:
            if f.lower().endswith('.root'):
                rootFile = rt.TFile(f)
                tree = rootFile.Get('RazorInclusive')
                if f.lower().find('sms')==-1:
                    
                    label = f.replace('.root','').split('/')[-1]
                    sumW[label] = getSumOfWeights(tree, cfg, box, w, useWeight, f, lumi, lumi_in)
                    if label.find('QCD')!=-1: sumWQCD = sumW[label]
        # get total sum of weights
        sumWTotal = [sum(allW) for allW in zip( * sumW.values() )]

        # get scale factor to scale other backgrounds by
        k_QCD[box] = [total/(total - qcd) for total, qcd in zip(sumWTotal,sumWQCD)]
         
        print "Sum of Weights Total [ %s ] ="%box, sumWTotal
        print "Sum of Weights QCD   [ %s ] ="%box, sumWQCD
        print "Scale Factor k_QCD   [ %s ] ="%box, k_QCD[box]
    else:        
        z = array('d', cfg.getBinning(box)[2]) # nBtag binning
        k_QCD[box] = [1. for iz in range(1,len(z))]

    for i, f in enumerate(args):
        if f.lower().endswith('.root'):
            rootFile = rt.TFile(f)
            tree = rootFile.Get('RazorInclusive')
            if f.lower().find('sms')==-1:
                if removeQCD and f.find('QCD')!=-1:
                    continue # do not add QCD
                else:
                    ds.append(convertTree2Dataset(tree, cfg, box, w, useWeight, f, lumi, lumi_in,  'RMRTree_%i'%i))
                
            else:
                model = f.split('-')[1].split('_')[0]
                massPoint = '_'.join(f.split('_')[2:4])
                ds.append(convertTree2Dataset(tree, cfg, box, w, useWeight, f ,lumi, lumi_in, 'signal'))
                
    wdata = ds[0].Clone('RMRTree')
    for ids in range(1,len(ds)):
        wdata.append(ds[ids])
            
    rootTools.Utils.importToWS(w,wdata)
    
    inFiles = [f for f in args if f.lower().endswith('.root')]
    
    args = w.set("variables")
    
    #we cut away events outside our MR window
    mRmin = args['MR'].getMin()
    mRmax = args['MR'].getMax()

    #we cut away events outside our Rsq window
    rsqMin = args['Rsq'].getMin()
    rsqMax = args['Rsq'].getMax()

    btagMin =  args['nBtag'].getMin()
    btagMax =  args['nBtag'].getMax()
    
    if len(inFiles)==1:
        if btagMax>btagMin+1:
            outFile = inFiles[0].split('/')[-1].replace('.root','_lumi-%.1f_%i-%ibtag_%s.root'%(lumi/1000.,btagMin,btagMax-1,box))
            outFile = outFile.replace('_1pb','')
            if not useWeight:
                outFile = outFile.replace("weighted","unweighted")
        else:
            outFile = inFiles[0].split('/')[-1].replace('.root','_lumi-%.1f_%ibtag_%s.root'%(lumi/1000.,btagMin,box))
            outFile = outFile.replace('_1pb','')
            if not useWeight:
                outFile = outFile.replace("weighted","unweighted")
    else:
        if btagMax>btagMin+1:
            if useWeight:
                outFile = 'RazorInclusive_SMCocktail_weighted_lumi-%.1f_%i-%ibtag_%s.root'%(lumi/1000.,btagMin,btagMax-1,box)
            else:
                outFile = 'RazorInclusive_SMCocktail_unweighted_lumi-%.1f_%ibtag_%s.root'%(lumi/1000.,btagMin,box)
        else:
            if useWeight:
                outFile = 'RazorInclusive_SMCocktail_weighted_lumi-%.1f_%ibtag_%s.root'%(lumi/1000.,btagMin,box)
            else:
                outFile = 'RazorInclusive_SMCocktail_unweighted_lumi-%.1f_%ibtag_%s.root'%(lumi/1000.,btagMin,box)
        

    numEntriesByBtag = []
    sumEntriesByBtag = []
    z = array('d', cfg.getBinning(box)[2]) # nBtag binning
    for i in z[:-1]:
        numEntriesByBtag.append(wdata.reduce('nBtag==%i'%i).numEntries())
        sumEntriesByBtag.append(wdata.reduce('nBtag==%i'%i).sumEntries())
        
    print "Output File: %s"%(options.outDir+"/"+outFile)
    print "Number of Entries Total [ %s ] ="%(box), numEntriesByBtag
    print "Sum of Weights Total    [ %s ] ="%(box), sumEntriesByBtag
    
    outFile = rt.TFile.Open(options.outDir+"/"+outFile,'recreate')
    outFile.cd()
    w.Write()
    outFile.Close()
    
