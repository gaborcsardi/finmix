#ifndef __FINMIX_MCMC_STUDENT_CC__
#define __FINMIX_MCMC_STUDENT_CC__

#include "FinmixData.h"
#include "FinmixModel.h"
#include "FinmixPrior.h"
#include "FinmixMCMC.h"
#include "BASE.h"
#include "ADAPTER.h"
#include "FIX.h"
#include "IND.h"
#include "HIER.h"
#include "POST.h"
#include "LogStudentFix.h"
#include "ParStudentFix.h"
#include "ParOutStudent.h"
#include "HierOutStudent.h"
#include "PostOutStudentFix.h"
#include "ParStudentInd.h"
#include "LogStudentInd.h"
#include "PostOutStudentInd.h"

//' Performs MCMC sampling for mixtures of Student-t distributions
//' 
//' @description
//' For internal usage only. This function gets passed the `fdata`, `model`,
//' `prior`, `mcmc` objects to perform MCMC sampling for a Student-t mixture 
//' model. In addition an `mcmcoutput` object is given that stores the output 
//' of MCMC sampling in R memory. Note that `finmix` relies in C++ code on 
//' so-called "mixin" layers that help to design a software by organizing code 
//' into layers that build upon each others and enable modularity in MCMC 
//' sampling by allowing to combine different sampling designs, e.g. with or 
//' without a hierarchical prior, with fixed indicators or storing posterior 
//' density parameters. See for more information on mixin layers Smaragdakis 
//' and Butory (1998). 
//' 
//' @param data_S4 An `fdata` object storing the observations and indicators.
//' @param model_S4 A `model` object specifying the Student-t finite mixture 
//'   model.
//' @param prior_S4 A `prior` object specifying the prior distribution for MCMC 
//'   sampling.
//' @param mcmc_S4 A `mcmc` object specifying the hyper-parameters for MCMC 
//'   sampling.
//' @param mcmcoutput_S4 An `mcmcoutput` object storing the outcomes from MCMC 
//'   sampling using R memory.
//' @return An `mcmcoutput` object containing the results from MCMC sampling 
//'   and using the R memory from the input argument `mcmcoutput_S4`. 
//' @export 
//' 
//' @seealso 
//' * [mixturemcmc()] for performing MCMC sampling
//' * [fdata-class] for the `fdata` class definition
//' * [model-class] for the `model` class definition
//' * [prior-class] for the `prior` class definition
//' * [mcmc-class] for the `mcmc` class definition
//' 
//' @references
//' * Smaragdakis, Y. and Butory, D. (1998), "Implementing layered designs with 
//'   mixin layers." In: Jul E. (eds) ECOOP’98 — Object-Oriented Programming. 
//'   ECOOP 1998. Lecture Notes in Computer Science, vol 1445. Springer, 
//'   Berlin, Heidelberg.
// [[Rcpp::export]]
RcppExport SEXP mcmc_student_cc(SEXP data_S4, SEXP model_S4,
                                SEXP prior_S4, SEXP mcmc_S4, SEXP mcmcoutput_S4)
{
   /* Convert S4-classes to C++-structs */
   Rcpp::S4           dataS4O(data_S4);
   Rcpp::S4           modelS4O(model_S4);
   Rcpp::S4           priorS4O(prior_S4);
   Rcpp::S4           mcmcS4O(mcmc_S4);
   Rcpp::S4           mcmcOutputS4O(mcmcoutput_S4);
   FinmixData         finData  = FinmixData(dataS4O);
   FinmixModel        finModel = FinmixModel(modelS4O);
   FinmixPrior        finPrior = FinmixPrior(priorS4O);
   FinmixMCMC         finMCMC  = FinmixMCMC(mcmcS4O);

   const bool         INDICFIX = finModel.indicFix;
   const bool         HIER_IND = finPrior.hier;
   const bool         POST_IND = finMCMC.storePost;
   const unsigned int BURNIN   = finMCMC.burnIn;
   const unsigned int M        = finMCMC.M;
   const unsigned int K        = finModel.K;

   BASE               *ptr;

   typedef FIX<PriorStudentFix, ParStudentFix, LogStudentFix,
               ParOutStudent> STUDENTFIX;
   typedef IND<FIX<PriorStudentInd, ParStudentInd, LogStudentInd,
                   ParOutStudent> > STUDENTIND;
   if (INDICFIX || K == 1)
   {
      if (HIER_IND)
      {
         if (POST_IND)
         {
            ptr = new ADAPTER<POST<HIER<STUDENTFIX,
                                        HierOutStudent>, PostOutStudentFix> >
                     (finData, finModel, finPrior, finMCMC,
                     mcmcOutputS4O);
         }
         else
         {
            ptr = new ADAPTER<HIER<STUDENTFIX, HierOutStudent> >
                     (finData, finModel, finPrior, finMCMC,
                     mcmcOutputS4O);
         }
      }
      else
      {
         if (POST_IND)
         {
            ptr = new ADAPTER<POST<STUDENTFIX, PostOutStudentFix> >
                     (finData, finModel, finPrior, finMCMC,
                     mcmcOutputS4O);
         }
         else
         {
            ptr = new ADAPTER<STUDENTFIX> (finData, finModel,
                                           finPrior, finMCMC, mcmcOutputS4O);
         }
      }
   }
   else
   {
      if (HIER_IND)
      {
         if (POST_IND)
         {
            ptr = new ADAPTER<POST<HIER<STUDENTIND,
                                        HierOutStudent>, PostOutStudentInd> >
                     (finData, finModel, finPrior, finMCMC,
                     mcmcOutputS4O);
         }
         else
         {
            ptr = new ADAPTER<HIER<STUDENTIND, HierOutStudent> >
                     (finData, finModel, finPrior, finMCMC,
                     mcmcOutputS4O);
         }
      }
      else
      {
         if (POST_IND)
         {
            ptr = new ADAPTER<POST<STUDENTIND, PostOutStudentInd> >
                     (finData, finModel, finPrior, finMCMC,
                     mcmcOutputS4O);
         }
         else
         {
            ptr = new ADAPTER<STUDENTIND> (finData, finModel,
                                           finPrior, finMCMC, mcmcOutputS4O);
         }
      }
   }
   for (unsigned int i = 0; i < BURNIN + M; ++i)
   {
      ptr->update();
      ptr->store(i);
   }
   return Rcpp::wrap(mcmcOutputS4O);
}
#endif
