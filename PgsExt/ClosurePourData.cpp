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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\ClosurePourData.h>
#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\BridgeDescription2.h>

#include <PsgLib\StructuredLoad.h>
#include <PsgLib\StructuredSave.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Float64 gs_DefaultSlabOffset2 = ::ConvertToSysUnits(10.0,unitMeasure::Inch);

CClosurePourData::CClosurePourData()
{
   m_pGirder        = NULL;
   m_pTempSupport   = NULL;
   m_pPier          = NULL;
   m_pLeftSegment   = NULL;
   m_pRightSegment  = NULL;

   m_TempSupportID  = INVALID_ID;
   m_PierID         = INVALID_ID;
   m_Index          = INVALID_INDEX;

   m_SlabOffset = gs_DefaultSlabOffset2;
}

CClosurePourData::CClosurePourData(CSplicedGirderData* pGirder,const CTemporarySupportData* pTempSupport)
{
   m_pGirder        = pGirder;
   m_pTempSupport   = pTempSupport;
   m_pPier          = NULL;
   m_pLeftSegment   = NULL;
   m_pRightSegment  = NULL;

   m_TempSupportID  = INVALID_ID;
   m_PierID         = INVALID_ID;
   m_Index          = INVALID_INDEX;

   m_SlabOffset = gs_DefaultSlabOffset2;
}  

CClosurePourData::CClosurePourData(CSplicedGirderData* pGirder,const CPierData2* pPier)
{
   m_pGirder        = pGirder;
   m_pTempSupport   = NULL;
   m_pPier          = pPier;
   m_pLeftSegment   = NULL;
   m_pRightSegment  = NULL;

   m_TempSupportID  = INVALID_ID;
   m_PierID         = INVALID_ID;
   m_Index          = INVALID_INDEX;

   m_SlabOffset = gs_DefaultSlabOffset2;
}  

CClosurePourData::CClosurePourData(const CClosurePourData& rOther)
{
   m_pGirder        = NULL;
   m_pTempSupport   = NULL;
   m_pPier          = NULL;
   m_pLeftSegment   = NULL;
   m_pRightSegment  = NULL;

   m_TempSupportID  = INVALID_ID;
   m_PierID         = INVALID_ID;
   m_Index          = INVALID_INDEX;

   m_SlabOffset = gs_DefaultSlabOffset2;

   MakeCopy(rOther,true /*copy only data*/);
}

CClosurePourData::~CClosurePourData()
{
}

CClosurePourData& CClosurePourData::operator= (const CClosurePourData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CClosurePourData::CopyClosurePourData(const CClosurePourData* pClosure)
{
   MakeCopy(*pClosure,true/*copy only data*/);
   ResolveReferences();
}

bool CClosurePourData::operator==(const CClosurePourData& rOther) const
{
   if ( !IsEqual(m_SlabOffset,rOther.m_SlabOffset) )
      return false;

   if ( m_Concrete != rOther.m_Concrete )
      return false;

   if ( m_Stirrups != rOther.m_Stirrups)
      return false;

   if ( m_Rebar != rOther.m_Rebar)
      return false;

   return true;
}

bool CClosurePourData::operator!=(const CClosurePourData& rOther) const
{
   return !operator==(rOther);
}

bool CClosurePourData::operator<(const CClosurePourData& rOther) const
{
   Float64 station1, station2;
   if ( m_pTempSupport != NULL )
      station1 = m_pTempSupport->GetStation();
   else
      station1 = m_pPier->GetStation();

   if ( rOther.GetTemporarySupport() != NULL )
      station2 = rOther.GetTemporarySupport()->GetStation();
   else
      station2 = rOther.GetPier()->GetStation();

   return station1 < station2;
}

CollectionIndexType CClosurePourData::GetIndex() const
{
   return m_Index;
}

IDType CClosurePourData::GetID() const
{
   return GetLeftSegment()->GetID();
}

CClosureKey CClosurePourData::GetClosureKey() const
{
   CClosureKey closureKey;
   if ( m_pGirder )
   {
      closureKey.groupIndex   = m_pGirder->GetGirderGroupIndex();
      closureKey.girderIndex  = m_pGirder->GetIndex();
      closureKey.segmentIndex = m_Index;
   }
   return closureKey;
}

void CClosurePourData::SetGirder(CSplicedGirderData* pGirder)
{
   m_pGirder = pGirder;
   ResolveReferences();
}

CSplicedGirderData* CClosurePourData::GetGirder() 
{
   return m_pGirder;
}

const CSplicedGirderData* CClosurePourData::GetGirder() const
{
   return m_pGirder;
}

void CClosurePourData::SetTemporarySupport(const CTemporarySupportData* pTS)
{
   ATLASSERT(m_pPier == NULL);
   m_pTempSupport = pTS;
   m_TempSupportID = INVALID_ID;
}

const CTemporarySupportData* CClosurePourData::GetTemporarySupport() const
{
   ATLASSERT( !(m_pPier == NULL && m_pTempSupport == NULL) ); // can't have both
   return m_pTempSupport;
}

SupportIndexType CClosurePourData::GetTemporarySupportIndex() const
{
   if ( m_pTempSupport )
      return m_pTempSupport->GetIndex();

   return INVALID_INDEX;
}

SupportIDType CClosurePourData::GetTemporarySupportID() const
{
   if ( m_pTempSupport )
      return m_pTempSupport->GetID();

   return INVALID_ID;
}

void CClosurePourData::SetPier(const CPierData2* pPier)
{
   ATLASSERT(m_pTempSupport == NULL);
   m_pPier = pPier;
   m_PierID = INVALID_ID;
}

const CPierData2* CClosurePourData::GetPier() const
{
   ATLASSERT( !(m_pPier == NULL && m_pTempSupport == NULL) ); // can't have both
   return m_pPier;
}

PierIndexType CClosurePourData::GetPierIndex() const
{
   if ( m_pPier )
      return m_pPier->GetIndex();

   return INVALID_INDEX;
}

PierIDType CClosurePourData::GetPierID() const
{
   if ( m_pPier )
      return m_pPier->GetID();

   return INVALID_ID;
}

void CClosurePourData::SetLeftSegment(CPrecastSegmentData* pLeftSegment)
{
   m_pLeftSegment = pLeftSegment;
}

const CPrecastSegmentData* CClosurePourData::GetLeftSegment() const
{
   return m_pLeftSegment;
}

CPrecastSegmentData* CClosurePourData::GetLeftSegment()
{
   return m_pLeftSegment;
}

void CClosurePourData::SetRightSegment(CPrecastSegmentData* pRightSegment)
{
   m_pRightSegment = pRightSegment;
}

const CPrecastSegmentData* CClosurePourData::GetRightSegment() const
{
   return m_pRightSegment;
}

CPrecastSegmentData* CClosurePourData::GetRightSegment()
{
   return m_pRightSegment;
}

void CClosurePourData::SetSlabOffset(Float64 offset)
{
#if defined _DEBUG
   if ( m_pGirder )
   {
      const CGirderGroupData* pGroup = m_pGirder->GetGirderGroup();
      if ( pGroup )
      {
         const CBridgeDescription2* pBridge = pGroup->GetBridgeDescription();
         if ( pBridge )
         {
            // if this assert fires, setting the slab offset now doesn't really change anything
            ATLASSERT(pBridge->GetSlabOffsetType() == pgsTypes::sotSegment);
         }
      }
   }
#endif
   m_SlabOffset = offset;
}

Float64 CClosurePourData::GetSlabOffset() const
{
   pgsTypes::SlabOffsetType offsetType = m_pGirder->GetGirderGroup()->GetBridgeDescription()->GetSlabOffsetType();
   if ( offsetType == pgsTypes::sotBridge )
   {
      return m_pGirder->GetGirderGroup()->GetBridgeDescription()->GetSlabOffset();
   }
   else if ( offsetType == pgsTypes::sotGroup )
   {
#pragma Reminder("UPDATE: if slab offset is different at each end of the group, what is it at a closure pour?")
      // does it need to be interpolated???
      return m_pGirder->GetGirderGroup()->GetSlabOffset(m_pGirder->GetIndex(),pgsTypes::metStart);
   }
   else
   {
      if ( m_pGirder->GetGirderGroup()->GetBridgeDescription()->GetDeckDescription()->DeckType == pgsTypes::sdtNone )
         return 0;

      return m_SlabOffset;
   }

   ATLASSERT(false); // should never get here
}

void CClosurePourData::SetConcrete(const CConcreteMaterial& concrete)
{
   m_Concrete = concrete;
}

const CConcreteMaterial& CClosurePourData::GetConcrete() const
{
   return m_Concrete;
}

CConcreteMaterial& CClosurePourData::GetConcrete()
{
   return m_Concrete;
}

void CClosurePourData::SetStirrups(const CShearData2& stirrups)
{
   m_Stirrups = stirrups;
}

const CShearData2& CClosurePourData::GetStirrups() const
{
   return m_Stirrups;
}

CShearData2& CClosurePourData::GetStirrups()
{
   return m_Stirrups;
}

void CClosurePourData::SetRebar(const CLongitudinalRebarData& rebar)
{
   m_Rebar = rebar;
}

const CLongitudinalRebarData& CClosurePourData::GetRebar() const
{
   return m_Rebar;
}

CLongitudinalRebarData& CClosurePourData::GetRebar()
{
   return m_Rebar;
}

void CClosurePourData::CopyMaterialFrom(const CClosurePourData& rOther)
{
   m_Concrete = rOther.m_Concrete;
}

void CClosurePourData::CopyLongitudinalReinforcementFrom(const CClosurePourData& rOther)
{
   m_Rebar    = rOther.m_Rebar;
}

void CClosurePourData::CopyTransverseReinforcementFrom(const CClosurePourData& rOther)
{
   m_Stirrups = rOther.m_Stirrups;
}

void CClosurePourData::MakeCopy(const CClosurePourData& rOther,bool bCopyDataOnly)
{
   if ( !bCopyDataOnly )
   {
      if ( rOther.m_pTempSupport )
      {
         // other has a temporary support that is resolved
         m_pTempSupport  = NULL; // NULL because this will be resolved later
         m_TempSupportID = rOther.m_pTempSupport->GetID(); // capture ID for later resolution of m_pTempSupport
         m_pPier         = NULL; // No pier
         m_PierID        = INVALID_ID; // No pier
      }
      else if ( rOther.m_TempSupportID != INVALID_ID )
      {
         // other has a temporary support that is not resolved
         m_pTempSupport  = NULL; // NULL because this will be resolved later
         m_TempSupportID = rOther.m_TempSupportID; // capture ID for later resolution of m_pTempSupport
         m_pPier         = NULL; // No pier
         m_PierID        = INVALID_ID; // No pier
      }
      else if ( rOther.m_pPier )
      {
         // other has a pier that is resolved
         m_pTempSupport  = NULL; // No temp support
         m_TempSupportID = INVALID_ID; // No temp support 
         m_pPier         = NULL; // NULL because this will be resolved later
         m_PierID        = rOther.m_pPier->GetID(); // capture index for later resolution of m_pPier
      }
      else
      {
         // other has a pier that is not resolved
         m_pTempSupport  = NULL; // No temp support 
         m_TempSupportID = INVALID_ID; // No temp support 
         m_pPier         = NULL; // NULL because this will be resolved later
         m_PierID        = rOther.m_PierID; // capture index for later resolution of m_pPier
      }
   }

   CopyMaterialFrom(rOther);
   CopyTransverseReinforcementFrom(rOther);
   CopyLongitudinalReinforcementFrom(rOther);

   m_SlabOffset = rOther.m_SlabOffset;
}


void CClosurePourData::MakeAssignment(const CClosurePourData& rOther)
{
   MakeCopy( rOther, false /*copy everything*/ );
}

HRESULT CClosurePourData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("ClosurePour"),1.0);

   if ( m_pGirder->GetGirderGroup()->GetBridgeDescription()->GetSlabOffsetType() == pgsTypes::sotSegment )
   {
      pStrSave->put_Property(_T("SlabOffset"),CComVariant(m_SlabOffset));
   }

   if ( m_pPier )
   {
      pStrSave->put_Property(_T("SupportType"),CComVariant("Pier"));
      pStrSave->put_Property(_T("ID"),CComVariant(m_pPier->GetID()));
   }
   else
   {
      pStrSave->put_Property(_T("SupportType"),CComVariant("TempSupport"));
      pStrSave->put_Property(_T("ID"),CComVariant(m_pTempSupport->GetID()));
   }

   m_Concrete.Save(pStrSave,pProgress);

   CStructuredSave save(pStrSave);
   m_Stirrups.Save(&save);

   m_Rebar.Save(pStrSave,pProgress);
   pStrSave->EndUnit(); // ClosurePour

   return S_OK;
}

HRESULT CClosurePourData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CHRException hr;
   try
   {
      hr = pStrLoad->BeginUnit(_T("ClosurePour"));

      CComVariant var;
      var.vt = VT_R8;

      if ( m_pGirder->GetGirderGroup()->GetBridgeDescription()->GetSlabOffsetType() == pgsTypes::sotSegment )
      {
         hr = pStrLoad->get_Property(_T("SlabOffset"),&var);
         m_SlabOffset = var.dblVal;
      }

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("SupportType"),&var);
      CString strSupportType(var.bstrVal);

      if ( strSupportType == _T("Pier") )
      {
         var.vt = VT_ID;
         hr = pStrLoad->get_Property(_T("ID"),&var);
         PierIDType pierID = VARIANT2ID(var);

         if ( m_pGirder->GetGirderGroup() )
         {
            CGirderGroupData* pGirderGroup = m_pGirder->GetGirderGroup();
            m_pPier = pGirderGroup->GetBridgeDescription()->FindPier(pierID);
            m_PierID = INVALID_ID;
         }
         else
         {
            m_pPier = NULL;
            m_PierID = pierID;
         }
      }
      else
      {
         var.vt = VT_ID;
         hr = pStrLoad->get_Property(_T("ID"),&var);
         SupportIDType id = VARIANT2ID(var);

         if ( m_pGirder->GetGirderGroup() )
         {
            CGirderGroupData* pGirderGroup = m_pGirder->GetGirderGroup();
            m_pTempSupport = pGirderGroup->GetBridgeDescription()->FindTemporarySupport(id);
            m_TempSupportID = INVALID_ID;
         }
         else
         {
            m_pTempSupport = NULL;
            m_TempSupportID = id;
         }
      }
      hr = m_Concrete.Load(pStrLoad,pProgress);

      CStructuredLoad load(pStrLoad);
      hr = m_Stirrups.Load(&load);

      hr = m_Rebar.Load(pStrLoad,pProgress);
      hr = pStrLoad->EndUnit(); // ClosurePour
   }
   catch(...)
   {
      ATLASSERT(false);
   }

   return S_OK;
}

void CClosurePourData::ResolveReferences()
{
   // There are times when a closure pour object is created and there isn't access to its 
   // associated pier or temporary support. The ID of the associated object is stored and now
   // it is time to resolve that reference.

   const CGirderGroupData* pGirderGroup = m_pGirder->GetGirderGroup();
   if ( pGirderGroup == NULL )
      return; // can't resolve it

   const CBridgeDescription2* pBridge = pGirderGroup->GetBridgeDescription();
   if ( pBridge == NULL )
      return; // can't resolve it

   if ( m_TempSupportID != INVALID_ID )
   {
      // referencing a temporary support
      m_pTempSupport = pBridge->FindTemporarySupport(m_TempSupportID);

      m_TempSupportID = INVALID_ID;
      m_PierID = INVALID_INDEX;
      m_pPier = NULL;
   }
   else if (m_PierID != INVALID_ID )
   {
      m_pTempSupport = NULL;
      m_TempSupportID = INVALID_ID;
      m_pPier = pBridge->FindPier(m_PierID);
      m_PierID = INVALID_INDEX;
   }
}

#if defined _DEBUG
void CClosurePourData::AssertValid()
{
   if ( m_PierID == INVALID_ID && m_TempSupportID == INVALID_ID )
   {
      _ASSERT(m_pPier != NULL || m_pTempSupport != NULL);
   }
}
#endif
