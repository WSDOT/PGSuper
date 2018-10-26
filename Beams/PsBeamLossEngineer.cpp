///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// PsBeamLossEngineer.cpp : Implementation of CPsBeamLossEngineer
#include "stdafx.h"
#include "PsBeamLossEngineer.h"
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUBeamPsLossEngineer
HRESULT CPsBeamLossEngineer::FinalConstruct()
{
   return S_OK;
}

void CPsBeamLossEngineer::SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   m_Engineer.Init(m_pBroker,m_StatusGroupID);
}

LOSSDETAILS CPsBeamLossEngineer::ComputeLosses(const pgsPointOfInterest& poi)
{
   return m_Engineer.ComputeLosses((CPsLossEngineer::BeamType)m_BeamType,poi);
}

LOSSDETAILS CPsBeamLossEngineer::ComputeLossesForDesign(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   return m_Engineer.ComputeLossesForDesign((CPsLossEngineer::BeamType)m_BeamType,poi,config);
}

void CPsBeamLossEngineer::BuildReport(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   m_Engineer.BuildReport((CPsLossEngineer::BeamType)m_BeamType,span,gdr,pChapter,pDisplayUnits);
}

void CPsBeamLossEngineer::ReportFinalLosses(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   m_Engineer.ReportFinalLosses((CPsLossEngineer::BeamType)m_BeamType,span,gdr,pChapter,pDisplayUnits);
}
