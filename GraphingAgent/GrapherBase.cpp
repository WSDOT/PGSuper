///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// PGSuperGrapherImp.cpp : Implementation of CPGSuperGrapherImp
#include "stdafx.h"
#include "GrapherBase.h"

// Interfaces
#include <IGraphManager.h>
#include <IFace\Project.h>

// Graph Builders
#include <Graphs\AnalysisResultsGraphBuilder.h>
#include <Graphs\SegmentAnalysisResultsGraphBuilder.h>
#include <Graphs\EffectivePrestressGraphBuilder.h>
#include <Graphs\FinishedElevationGraphBuilder.h>
#include <Graphs\StabilityGraphBuilder.h>
#include <Graphs\StressHistoryGraphBuilder.h>
#include <Graphs\GirderPropertiesGraphBuilder.h>
#include <Graphs\ConcretePropertyGraphBuilder.h>
#include <Graphs\DeflectionHistoryGraphBuilder.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CGrapherBase::InitCommonGraphBuilders()
{
   GET_IFACE(IGraphManager,pGraphMgr);

   pGraphMgr->SortByName(false); // don't sort alphabetically

   // add the graphs to the graph manager in near alphabetical order
   pGraphMgr->AddGraphBuilder(std::move(std::make_unique<CSegmentAnalysisResultsGraphBuilder>())); // Analysis Results - Before Erection
   pGraphMgr->AddGraphBuilder(std::move(std::make_unique<CAnalysisResultsGraphBuilder>())); // Analysis Results - After Erection
   pGraphMgr->AddGraphBuilder(std::move(std::make_unique<CConcretePropertyGraphBuilder>()));
   pGraphMgr->AddGraphBuilder(std::move(std::make_unique<CDeflectionHistoryGraphBuilder>()));
   pGraphMgr->AddGraphBuilder(std::move(std::make_unique<CFinishedElevationGraphBuilder>()));
   pGraphMgr->AddGraphBuilder(std::move(std::make_unique<CEffectivePrestressGraphBuilder>()));
   pGraphMgr->AddGraphBuilder(std::move(std::make_unique<CGirderPropertiesGraphBuilder>()));
   pGraphMgr->AddGraphBuilder(std::move(std::make_unique<CStabilityGraphBuilder>()));
   pGraphMgr->AddGraphBuilder(std::move(std::make_unique<CStressHistoryGraphBuilder>()));
}

STDMETHODIMP CGrapherBase::SetBroker(IBroker* pBroker)
{
   m_pBroker = pBroker;
   return S_OK;
}
