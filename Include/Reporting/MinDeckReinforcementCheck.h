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
#include <Reporter\Chapter.h>
#include <PgsExt\GirderArtifact.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CMinDeckReinforcementCheck

   LRFD 9.7.1.6 — Minimum Deck Reinforcement in Negative Moment Region


DESCRIPTION
   Reports LRFD 9.7.1.6—Minimum Deck Reinforcement in Negative Moment Region

LOG
   rdp : 04.18.2025 : Created file
*****************************************************************************/

class REPORTINGCLASS CMinDeckReinforcementCheck 
{
public:
   CMinDeckReinforcementCheck();

   //------------------------------------------------------------------------
   LPCTSTR GetName() const;

   //------------------------------------------------------------------------
   virtual void Build(rptChapter* pChapter, IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, IEAFDisplayUnits* pDisplayUnits) const;
protected:

private:
   // Prevent accidental copying and assignment
   CMinDeckReinforcementCheck(const CMinDeckReinforcementCheck&) = delete;
   CMinDeckReinforcementCheck& operator=(const CMinDeckReinforcementCheck&) = delete;
};
