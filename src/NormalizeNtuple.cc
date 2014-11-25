#include <fstream>
#include <sstream>
#include <iterator>
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "SimpleTable.h"
#include "TKey.h"

using namespace std;

// Get total number of events in the sample and determine normalization weight factor
double getNormalizationWeight(string filename, string datasetName) {

  //Get Number of Events in the Sample
  TFile *file = new TFile(filename.c_str(),"READ");
  if (!file) {
    cout << "Could not open file " << filename << endl;
    return 0;
  }

  TH1F *hist = (TH1F*) file->Get("NEvents");
  if (!hist) {
    cout << "Could not find histogram NEvents"
         << " in file " << filename << endl;
    file->Close();
    delete file;
    return 0;
  }
  double NEvents = hist->GetEntries();
  cout << "Original events in the sample: " << NEvents << endl;

  //Get CrossSection
  SimpleTable xstab("data/xSections.dat");
  double CrossSection = xstab.Get(datasetName.c_str());  
  double Weight = CrossSection / NEvents;
  // weight for data is always 1 (-1 to make a trick for fakes)
  if(CrossSection < 0) Weight = -1.0;

  cout << "Cross Section = " << CrossSection << " for dataset " << datasetName << "\n";
  cout << "Events get weight: " << Weight << "\n";

  //delete dir;
  file->Close();
  delete file;
  return Weight;

}

//get list of files to open, add normalization branch to the tree in each file
int main(int argc, char* argv[]) {

    //parse input list to get names of ROOT files
    if(argc < 2){
        cerr << "usage NormalizeNtuple inputList.txt" << endl;
        return -1;
    }
    string inputList(argv[1]);

    ifstream filein(inputList.c_str());
    string curFilename;
    vector<string> inputLines;
    while(getline(filein, curFilename)){
        if(((string)curFilename).at(0) != '#') inputLines.push_back(curFilename); //'#' denotes a comment
        else cout << "(Skipping commented line in input)" << endl;
    }

    //open each ROOT file and add the normalization branch
    for(auto& line : inputLines){
        //parse input -- input lines should be in the form datasetName fileName
        istringstream buf(line);
        istream_iterator<std::string> beg(buf), end;
        vector<std::string> inputs(beg, end);

        string datasetName = inputs[0];
        string fileName = inputs[1];

        //get the weight to be applied to this dataset
        double normalizationWeight = getNormalizationWeight(fileName, datasetName);

        //create output file
        TFile *outputFile = new TFile(Form("%s_weighted.root", (fileName.substr(0, fileName.find_last_of("."))).c_str()), "RECREATE");

        //loop over all TTrees in the file and add the weight branch to each of them
        TFile inputFile(fileName.c_str(), "UPDATE");
        inputFile.cd();
        inputFile.Purge(); //purge unwanted TTree cycles in file
        TIter nextkey(inputFile.GetListOfKeys());
        TKey *key;
        while((key = (TKey*)nextkey())){
            string className = key->GetClassName();
            cout << "Getting key from file.  Class type: " << className << endl;
            if(className.compare("TTree") != 0){
                cout << "Skipping key (not a TTree)" << endl;
                continue;
            }

            TTree *inputTree = (TTree*)key->ReadObj();
            cout << "Processing tree " << inputTree->GetName() << endl;

            //create new normalized tree
            outputFile->cd();
            TTree *normalizedTree = inputTree->CloneTree(0);  
            cout << "Events in the ntuple: " << inputTree->GetEntries() << endl;

            //add weight branch
            double weight = 1;
            normalizedTree->Branch("weight", &weight, "weight/D");

            //store the weights
            for (int n=0;n<inputTree->GetEntries();n++) { 
                inputTree->GetEntry(n);
                weight = 1.0;
                if(normalizationWeight >= 0){
                    weight = weight * normalizationWeight;
                } 
                normalizedTree->Fill(); 
            }

            //save
            normalizedTree->Write();
            inputFile.cd();
        }
        inputFile.Close();
        cout << "Closing output file." << endl;
        outputFile->Close();
        delete outputFile;
    }
}
