///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// AgeAdjustedMaterial.cpp : Implementation of CAgeAdjustedMaterial
#include "stdafx.h"
#include "AgeAdjustedMaterial.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAgeAdjustedMaterial

bool CAgeAdjustedMaterial::IsDeck()
{
   return m_bIsDeck;
}

bool CAgeAdjustedMaterial::IsClosureJoint()
{
   return m_bIsClosure;
}

bool CAgeAdjustedMaterial::IsSegment()
{
   return m_bIsSegment;
}

bool CAgeAdjustedMaterial::IsLongitudinalJoint()
{
   return m_bIsLongitudinalJoint;
}

HRESULT CAgeAdjustedMaterial::FinalConstruct()
{
   return S_OK;
}

// IAgeAdjustedMaterial
STDMETHODIMP CAgeAdjustedMaterial::InitSegment(const CSegmentKey& segmentKey,IMaterials* pMaterials)
{
   m_bIsSegment = true;
   m_SegmentKey = segmentKey;
   m_GirderKey = m_SegmentKey;
   m_pMaterials = pMaterials;
   return S_OK;
}

STDMETHODIMP CAgeAdjustedMaterial::InitClosureJoint(const CClosureKey& closureKey,IMaterials* pMaterials)
{
   m_bIsClosure = true;
   m_ClosureKey = closureKey;
   m_GirderKey = m_ClosureKey;
   m_pMaterials = pMaterials;
   return S_OK;
}

STDMETHODIMP CAgeAdjustedMaterial::InitDeck(const CGirderKey& girderKey,IMaterials* pMaterials)
{
   m_bIsDeck = true;
   m_GirderKey = girderKey;
   m_pMaterials = pMaterials;
   return S_OK;
}

STDMETHODIMP CAgeAdjustedMaterial::InitLongitudinalJoint(const CSegmentKey& segmentKey, IMaterials* pMaterials)
{
   m_bIsLongitudinalJoint = true;
   m_SegmentKey = segmentKey;
   m_pMaterials = pMaterials;
   return S_OK;
}

// IMaterial
STDMETHODIMP CAgeAdjustedMaterial::get_E(StageIndexType stageIdx,Float64* E)
{
   IntervalIndexType intervalIdx = (IntervalIndexType)stageIdx;
   if ( IsDeck() )
   {
      *E = m_pMaterials->GetDeckAgeAdjustedEc(intervalIdx);
   }
   else if ( IsSegment() )
   {
      *E = m_pMaterials->GetSegmentAgeAdjustedEc(m_SegmentKey,intervalIdx);
   }
   else if ( IsClosureJoint() )
   {
      *E = m_pMaterials->GetClosureJointAgeAdjustedEc(m_ClosureKey,intervalIdx);
   }
   else if (IsLongitudinalJoint())
   {
      *E = m_pMaterials->GetLongitudinalJointAgeAdjustedEc(intervalIdx);
   }
   else
   {
      ATLASSERT(false);
      *E = 0;
   }
   return S_OK;
}

STDMETHODIMP CAgeAdjustedMaterial::put_E(StageIndexType stageIdx,Float64 E)
{
   ATLASSERT(false);
   return E_NOTIMPL;
}

STDMETHODIMP CAgeAdjustedMaterial::get_Density(StageIndexType stageIdx,Float64* w)
{
   IntervalIndexType intervalIdx = (IntervalIndexType)stageIdx;
   if ( IsDeck() )
   {
      *w = m_pMaterials->GetDeckWeightDensity(intervalIdx);
   }
   else if ( IsSegment() )
   {
      *w = m_pMaterials->GetSegmentWeightDensity(m_SegmentKey,intervalIdx);
   }
   else if ( IsClosureJoint() )
   {
      *w = m_pMaterials->GetClosureJointWeightDensity(m_ClosureKey,intervalIdx);
   }
   else if (IsLongitudinalJoint())
   {
      *w = m_pMaterials->GetLongitudinalJointWeightDensity(intervalIdx);
   }
   else
   {
      ATLASSERT(false);
      *w = 0;
   }
   return S_OK;
}

STDMETHODIMP CAgeAdjustedMaterial::put_Density(StageIndexType stageIdx,Float64 w)
{
   ATLASSERT(false);
   return E_NOTIMPL;
}
