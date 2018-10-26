///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#ifndef INCLUDED_PSFORCEENG_H_
#define INCLUDED_PSFORCEENG_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#include <Details.h>
#include <IFace\PsLossEngineer.h>
#include <IFace\Project.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IBroker;
class pgsPointOfInterest;
class rptChapter;
interface IDisplayUnits;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsPsForceEng

   Prestress force engineer.


DESCRIPTION
   Prestress force engineer. Responsible for computing prestressing forces
   and stresses.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 12.10.1998 : Created file
*****************************************************************************/

class pgsPsForceEng
{
public:
   //------------------------------------------------------------------------
   // Default constructor
   pgsPsForceEng();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsPsForceEng(const pgsPsForceEng& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsPsForceEng();

   //------------------------------------------------------------------------
   // Assignment operator
   pgsPsForceEng& operator = (const pgsPsForceEng& rOther);

   void SetBroker(IBroker* pBroker);
   void SetAgentID(long agentID);

   void Invalidate();

   //------------------------------------------------------------------------
   void ComputeLosses(const pgsPointOfInterest& poi,LOSSDETAILS* pLosses);
   void ComputeLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses);
   void ReportLosses(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDisplayUnits);

   Float64 GetPjackMax(SpanIndexType span,GirderIndexType gdr,StrandIndexType nStrands);
   Float64 GetPjackMax(SpanIndexType span,GirderIndexType gdr,const matPsStrand& strand,StrandIndexType nStrands);

   Float64 GetXferLength(SpanIndexType span,GirderIndexType gdr);
   Float64 GetXferLength(const matPsStrand& strand);
   Float64 GetXferLengthAdjustment(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   Float64 GetXferLengthAdjustment(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config);

   Float64 GetDevLength(const pgsPointOfInterest& poi,bool bDebonded);
   Float64 GetDevLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType);
   Float64 GetDevLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64 fps,Float64 fpe);
   STRANDDEVLENGTHDETAILS GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded);
   STRANDDEVLENGTHDETAILS GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded,Float64 fps,Float64 fpe);

   Float64 GetDevLength(const pgsPointOfInterest& poi,bool bDebonded,const GDRCONFIG& config);
   Float64 GetDevLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   Float64 GetDevLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config,Float64 fps,Float64 fpe);
   STRANDDEVLENGTHDETAILS GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded,const GDRCONFIG& config);
   STRANDDEVLENGTHDETAILS GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded,const GDRCONFIG& config,Float64 fps,Float64 fpe);

   Float64 GetHoldDownForce(SpanIndexType span,GirderIndexType gdr);
   Float64 GetHoldDownForce(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config);

   Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType type,pgsTypes::LossStage stage);
   Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType type,pgsTypes::LossStage stage,const GDRCONFIG& config);

   Float64 GetStrandStress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LossStage stage);
   Float64 GetStrandStress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LossStage stage,const GDRCONFIG& config);

protected:
   void MakeCopy(const pgsPsForceEng& rOther);
   void MakeAssignment(const pgsPsForceEng& rOther);

private:
   IBroker* m_pBroker;
   long m_AgentID;
   CComPtr<IPsLossEngineer> m_LossEngineer;

   // method used to compute prestress transfer length
   pgsTypes::PrestressTransferComputationType m_PrestressTransferComputationType;

   void CreateLossEngineer(SpanIndexType spanIdx,GirderIndexType gdrIdx);
};

#endif // INCLUDED_PSFORCEENG_H_
