///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#pragma once

#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;
class pgsGirderArtifact;

/*****************************************************************************
CLASS 
   CLongReinfShearCheck

   Encapsulates the construction of the longitudinal reinforcement for shear check report content.


DESCRIPTION
   Encapsulates the construction of the  longitudinal reinforcement for shear check report content.

LOG
   rdp : 06.01.1999 : Created file
*****************************************************************************/

class REPORTINGCLASS CLongReinfShearCheck
{
public:
   CLongReinfShearCheck() = default;
   CLongReinfShearCheck(const CLongReinfShearCheck& rOther) = default;
   ~CLongReinfShearCheck() = default;
   CLongReinfShearCheck& operator=(const CLongReinfShearCheck& rOther) = default;

   /// @brief Builds the check report for design limit states
   void Build(rptChapter* pChapter,
              IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
              IntervalIndexType intervalIdx,pgsTypes::LimitState ls,
              IEAFDisplayUnits* pDisplayUnits) const;


   /// @brief Builds the check report for rating limit states
   void Build(rptChapter* pChapter,
              IBroker* pBroker,const CGirderKey& girderKey,
              pgsTypes::LimitState ls,
              IEAFDisplayUnits* pDisplayUnits) const;

};
