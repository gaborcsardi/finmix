#include "PriorStudentFix.h"
#include "ParStudentFix.h"
#include "distributions.h"
#include "DataClass.h"
#include "likelihood.h"
#include "prior_likelihood.h"

PriorStudentFix::PriorStudentFix () : HIER(false),
    INDEPENDENT(false) {}

PriorStudentFix::PriorStudentFix (const FinmixPrior& prior) :
    HIER(prior.hier),
    INDEPENDENT(prior.type == "condconjugate" ? false : true)
{
    Rcpp::List tmpMu((SEXP) prior.par["mu"]);
    Rcpp::List tmpSigma((SEXP) prior.par["sigma"]);
    Rcpp::List tmpDf((SEXP) prior.par["df"]);
    Rcpp::NumericMatrix tmpb((SEXP) tmpMu["b"]);
    const unsigned int M = tmpb.nrow();
    const unsigned int K = tmpb.ncol();
    if (INDEPENDENT) {
        Rcpp::NumericMatrix tmpBinv((SEXP) tmpMu["Binv"]);
        BStart = arma::mat(tmpBinv.begin(),M, K, true, true);
        BPost = BStart;
    } else {
        Rcpp::NumericMatrix tmpN((SEXP) tmpMu["N0"]);
        BStart = arma::mat(tmpN.begin(), M, K, true, true);
        BPost = BStart;
    }  
    bStart = arma::mat(tmpb.begin(), M, K, true, true);
    Rcpp::NumericMatrix tmpc((SEXP) tmpSigma["c"]);
    Rcpp::NumericMatrix tmpC((SEXP) tmpSigma["C"]);
    cStart = arma::mat(tmpc.begin(), M, K, true, true);
    cPost = cStart;
    CStart = arma::mat(tmpC.begin(), M, K, true, true);
    CPost = CStart;
    if (HIER) {
        g = tmpSigma["g"];
        G = tmpSigma["G"];
    }
    dftype  = Rcpp::as<std::string>(tmpDf["type"]);
    trans   = Rcpp::as<double>(tmpDf["trans"]);
    a0      = Rcpp::as<double>(tmpDf["a0"]);
    b0      = Rcpp::as<double>(tmpDf["b0"]);
    d       = Rcpp::as<double>(tmpDf["d"]);
    Rcpp::NumericVector tmpMhTune((SEXP) tmpDf["mhtune"]);
    mhTune  = arma::rowvec(tmpMhTune.begin(), K, true, true);
}

inline
void PriorStudentFix::update (const unsigned int& K, const arma::mat& y,
        arma::ivec& S, const arma::vec& T, ParStudentFix& par)
{
    arma::mat repY      = arma::repmat(y, 1, K);
    arma::imat repS     = arma::repmat(S, 1, K);
    arma::imat compM    = arma::ones<arma::imat>(S.n_elem, K);
    for(unsigned int k = 0; k < K; ++k) {
        compM.col(k) = compM.col(k) * (k + 1);
    }
    arma::umat ind      = (repS == compM);
    arma::mat indDouble = arma::conv_to<arma::mat>::from(ind);
    repY                %= indDouble;
    arma::rowvec sprod  = sum(repY, 0);
    arma::rowvec sind   = sum(indDouble, 0);
    // OMEGAIND !!!
    if (INDEPENDENT) {
        if (!par.INDEPENDENT) {
            par.INDEPENDENT = true;
        }
        cPost = cStart + 0.5 * sind;
        for (unsigned int k = 0; k < K; ++k) {
            CPost(k)    = CStart(k);
            arma::uvec yind = find(repY.col(k) != 0.0);
            arma::mat y = repY.rows(yind);
            arma::vec b = y.col(k) - par.mu(k);
            CPost(k)    += 0.5 * arma::as_scalar(arma::trans(b) * b);
        }
        par.sigma               = 1.0 / rgammaprod(cPost, CPost);
        arma::rowvec BinvPost   = BStart + sind % (1.0 / par.sigma);
        BPost                   = 1.0 / BinvPost;
        bPost                   = BStart % bStart;            
        bPost                   += 1.0 / par.sigma % sprod;
        bPost                   %= BPost;
    } else { /* conditionally conjugate prior */
          
          arma::rowvec N0Post       = BStart + sind;
          BPost = 1.0 / N0Post;
          bPost                     = (bStart % BStart + sprod) / N0Post;
          cPost                     = cStart + 0.5 * sind;
          arma::rowvec ck           = BStart % sind / N0Post;
          for (unsigned int k = 0; k < K; ++k) {
              if (sind(k) > 0) {
                  double yk = sprod(k) / sind(k);  
                  CPost(k)  = CStart(k);
                  arma::uvec yind = find(repY.col(k) != 0.0);
                  arma::mat y = repY.rows(yind);
                  arma::vec sk = y.col(k) - yk;
                  CPost(k)  += 0.5 * arma::as_scalar(arma::trans(sk) * sk);                              
                  CPost(k)  += 0.5 * (yk - bStart(k)) * (yk - bStart(k)) * ck(k); 
              } else {                                         
                  CPost(k)  = CStart(k);
                  CPost(k)  += 0.5 * (sprod(k) - bStart(k)) * (sprod(k) - bStart(k)) * ck(k); 
              }
          }
     } 
    
    /* The parameter update is done here, as we need data 'y' 
     * and classifications 'S' for the Metropolis-Hastings 
     * algorithm for the degrees of freedoms update */
    if (par.INDEPENDENT) {
        par.mu      = rnormal(bPost, BPost);
    } else { /* conditionally conjugate prior */
        par.sigma   = 1.0 / rgammaprod(cPost, CPost);
        par.mu      = rnormal(bPost, BPost);
    }
    updateDf(K, y, S, par);
}

inline 
void PriorStudentFix::updateDf (const unsigned int& K, const arma::mat& y, 
        const arma::ivec& S, ParStudentFix& par) 
{
    arma::rowvec dfnew = par.df;
    liklist lik     = likelihood_student(y, par.mu, par.sigma, par.df);
    DataClass dataC;
    double loglik   = 0.0;
    double priorlik = priormixlik_student(INDEPENDENT, HIER, bStart,
            BStart, cStart, CStart, par.mu, par.sigma, g, G, 
            par.df, trans, a0, b0, d);
    dataC   = classification_fix(K, S, lik);
    loglik  = arma::sum(dataC.logLikCd);
    double urnd         = 0.0;
    double logliknew    = 0.0;
    double priorliknew  = 0.0;
    double acc          = 0.0;
    Rcpp::RNGScope scope;
    for(unsigned int k = 0; k < K; ++k) {
        urnd            = mhTune(k) * (2.0 * R::runif(0.0, 1.0) - 1.0);
        dfnew(k)        = trans + (par.df(k) - trans) * std::exp(urnd);
        liklist lik2    = likelihood_student(y, par.mu, par.sigma, dfnew);
        dataC           = classification_fix(K, S, lik2);
        logliknew       = arma::sum(dataC.logLikCd);           
        priorliknew     = priormixlik_student(INDEPENDENT, HIER, bStart,
                BStart, cStart, CStart, par.mu, par.sigma, g, G, 
                dfnew, trans, a0, b0, d);
        acc             = logliknew + priorliknew - (loglik + priorlik) + urnd;
        if (std::log(R::runif(0.0, 1.0)) < acc) {
            par.df(k)   = dfnew(k);
            loglik      = logliknew;
            priorlik    = priorliknew;
            par.acc(k)  = 1.0;
        }
    }
}

inline
void PriorStudentFix::updateHier (const ParStudentFix& par) 
{
    double gN   = arma::sum(cStart) + g;
    double GN   = arma::sum(1.0 / par.sigma) + G;
    CStart.fill(R::rgamma(gN, 1.0));
    CStart      /= GN;
}   

