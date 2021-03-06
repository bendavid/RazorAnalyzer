/***************************************************************************** 
 * Project: RooFit                                                           * 
 *                                                                           * 
 * This code was autogenerated by RooClassFactory                            * 
 *****************************************************************************/ 

// Your description goes here...
#include "RooFit.h"

#include "Riostream.h" 

#include "RooSigmoid.h" 
#include "RooAbsReal.h" 
#include "RooRealVar.h"
#include "RooAbsCategory.h" 
#include <math.h> 
#include "TMath.h" 

ClassImp(RooSigmoid) 

 RooSigmoid::RooSigmoid(const char *name, const char *title, 
                        RooAbsReal& _x,
                        RooAbsReal& _mean,
                        RooAbsReal& _sigma) :
   RooAbsPdf(name,title), 
   x("x","x",this,_x),
   mean("mean","mean",this,_mean),
   sigma("sigma","sigma",this,_sigma)
 { 
 } 


 RooSigmoid::RooSigmoid(const RooSigmoid& other, const char* name) :  
   RooAbsPdf(other,name), 
   x("x",this,other.x),
   mean("mean",this,other.mean),
   sigma("sigma",this,other.sigma)
 { 
 } 



 Double_t RooSigmoid::evaluate() const 
 { 
   // ENTER EXPRESSION IN TERMS OF VARIABLE ARGUMENTS HERE 
   return 1.0-(1.0)/(1.0 + std::exp(-(x-mean)/sigma)) ; 
 } 



 Int_t RooSigmoid::getAnalyticalIntegral(RooArgSet& allVars, RooArgSet& analVars, const char* /*rangeName*/) const  
 { 
   // LIST HERE OVER WHICH VARIABLES ANALYTICAL INTEGRATION IS SUPPORTED, 
   // ASSIGN A NUMERIC CODE FOR EACH SUPPORTED (SET OF) PARAMETERS 
   // THE EXAMPLE BELOW ASSIGNS CODE 1 TO INTEGRATION OVER VARIABLE X
   // YOU CAN ALSO IMPLEMENT MORE THAN ONE ANALYTICAL INTEGRAL BY REPEATING THE matchArgs 
   // EXPRESSION MULTIPLE TIMES

   if (matchArgs(allVars,analVars,x)) return 1 ; 
   return 0 ; 
 } 



 Double_t RooSigmoid::analyticalIntegral(Int_t code, const char* rangeName) const  
 { 
   // RETURN ANALYTICAL INTEGRAL DEFINED BY RETURN CODE ASSIGNED BY getAnalyticalIntegral
   // THE MEMBER FUNCTION x.min(rangeName) AND x.max(rangeName) WILL RETURN THE INTEGRATION
   // BOUNDARIES FOR EACH OBSERVABLE x

   if (code==1) {
     return (x.max(rangeName)-x.min(rangeName)-sigma*std::log(std::exp(mean/sigma)+std::exp(x.max(rangeName)/sigma))+sigma*std::log(std::exp(mean/sigma)+std::exp(x.min(rangeName)/sigma))) ;
   } 
   return 0 ; 
 } 



