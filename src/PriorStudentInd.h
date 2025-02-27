/******************************************************************************
 *
 * TODO: Project Title
 *
 * Copyright (C) 2003-2009 ascolab GmbH. All Rights Reserved.
 * Web: http://www.ascolab.com
 *
 * Author: Gerhard Gappmeier <gerhard.gappmeier@ascolab.com>
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 ******************************************************************************/

#ifndef __FINMIX_PRIORSTUDENTIND_H__
#define __FINMIX_PRIORSTUDENTIND_H__

#include "PriorStudentFix.h"

/* Forward declaration */
class ParStudentInd;
class PriorStudentInd : virtual public PriorStudentFix {
    public:
        arma::rowvec weightStart;
        arma::rowvec weightPost;

        PriorStudentInd (const FinmixPrior&);
        virtual ~PriorStudentInd () {}
        virtual void update (const unsigned int&, 
                const arma::mat&, arma::ivec&,
                const arma::vec&, ParStudentInd&);
        virtual void updateDf (const unsigned int&, const arma::mat&, 
                const arma::ivec&, ParStudentInd&);
};
#endif /* __FINMIX_PRIORSTUDENTIND_H__ */



