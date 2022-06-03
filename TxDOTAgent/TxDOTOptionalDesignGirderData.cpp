///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include "StdAfx.h"
#include "resource.h"
#include "TxDOTOptionalDesignGirderData.h"
#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignUtilities.h"

#include <Units\SysUnits.h>
#include <Lrfd\StrandPool.h>
#include <WbflAtlExt.h>
#include <limits>
#include <GeomModel\IShape.h>
#include <WBFLSections.h>
#include <WBFLGenericBridge.h>

#include <IFace\BeamFactory.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************/
// free functions


/****************************************************************************
CLASS
   CTxDOTOptionalDesignGirderData
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTxDOTOptionalDesignGirderData::CTxDOTOptionalDesignGirderData(CTxDOTOptionalDesignData* pParent):
m_pParent(pParent)
{
   ASSERT(pParent!=nullptr);

   ResetData();
}

CTxDOTOptionalDesignGirderData::CTxDOTOptionalDesignGirderData(const CTxDOTOptionalDesignGirderData& rOther)
{
   MakeCopy(rOther);
}

CTxDOTOptionalDesignGirderData::~CTxDOTOptionalDesignGirderData()
{
}

///////////////////////////////////////////////////////
// Resets all data to default values
void CTxDOTOptionalDesignGirderData::ResetData()
{
   m_Grade = matPsStrand::Gr1860;
   m_Type  = matPsStrand::LowRelaxation;
   m_Size  = matPsStrand::D1270;
   m_Coating = matPsStrand::None;

   m_Fci = Float64_Inf;
   m_Fc  = Float64_Inf;

   ResetStrandNoData();
}

void CTxDOTOptionalDesignGirderData::ResetStrandNoData()
{
   m_StrandFillType = sfStandard;

   // Data for standard fill
   // ========================
   m_NumStrands = 0;
   m_StrandTo = Float64_Inf;

   // Data for non-standard fill
   // ==========================
   m_StrandRowsAtCL.clear();
   m_StrandRowsAtEnds.clear();

   // Data for direct manip fill
   // ===========================
   m_DirectFilledStraightStrands.clear();
   m_DirectFilledStraightDebond.clear();
}

CString CTxDOTOptionalDesignGirderData::GetGirderEntryName()
{
   ASSERT(m_pParent);
   return m_pParent->GetGirderEntryName();
}

//======================== OPERATORS  =======================================
CTxDOTOptionalDesignGirderData& CTxDOTOptionalDesignGirderData::operator= (const CTxDOTOptionalDesignGirderData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//bool CTxDOTOptionalDesignGirderData::operator==(const CTxDOTOptionalDesignGirderData& rOther) const
//{
//   ASSERT(0);
//   return false;
//}
//
//
//bool CTxDOTOptionalDesignGirderData::operator!=(const CTxDOTOptionalDesignGirderData& rOther) const
//{
//   return !operator==(rOther);
//}

//======================== OPERATIONS =======================================
HRESULT CTxDOTOptionalDesignGirderData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("TxDOTOptionalGirderData"),3.0);

   pStrSave->put_Property(_T("StrandFillType"),         CComVariant(m_StrandFillType));

   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   const matPsStrand* pStrand = pPool->GetStrand(m_Grade, m_Type, m_Coating, m_Size);
   Int64 key=0;
   if (pStrand!=nullptr)
   {
      key = pPool->GetStrandKey(pStrand);
   }
   else
   {
      ASSERT(0);
   }

   pStrSave->put_Property(_T("StrandMaterialKey"),         CComVariant(key)); // starting with version 3, key contains a coating bit

   pStrSave->put_Property(_T("Fci"), CComVariant(m_Fci));
   pStrSave->put_Property(_T("Fc"), CComVariant(m_Fc));

   if (m_StrandFillType==sfStandard)
   {
      // Data for standard fill
      // ======================
      pStrSave->BeginUnit(_T("StandardFillData"),1.0);

      pStrSave->put_Property(_T("NumStrands"), CComVariant(m_NumStrands));
      pStrSave->put_Property(_T("StrandTo"), CComVariant(m_StrandTo));
      pStrSave->EndUnit(); //StandardFillData
   }
   else if (m_StrandFillType==sfHarpedRows)
   {
      // Data for non-standard fill
      // ==========================
      pStrSave->BeginUnit(_T("NonstandardFillData"),2.0);

      RowIndexType rowcount = m_StrandRowsAtCL.size();
      pStrSave->put_Property(_T("StrandRowsAtCLCount"),CComVariant(rowcount));
      for (StrandRowIterator it = m_StrandRowsAtCL.begin(); it!=m_StrandRowsAtCL.end(); it++)
      {
         pStrSave->BeginUnit(_T("StrandRowAtCL"),1.0);
         const StrandRow& row = *it;
         pStrSave->put_Property(_T("RowElev"), CComVariant(row.RowElev));
         pStrSave->put_Property(_T("StrandsInRow"), CComVariant(row.StrandsInRow));
         pStrSave->EndUnit();
      }

      rowcount = m_StrandRowsAtEnds.size();
      pStrSave->put_Property(_T("StrandRowsAtEndsCount"),CComVariant(rowcount));
      for (StrandRowIterator it = m_StrandRowsAtEnds.begin(); it!=m_StrandRowsAtEnds.end(); it++)
      {
         pStrSave->BeginUnit(_T("StrandRowAtEnds"),1.0);
         const StrandRow& row = *it;
         pStrSave->put_Property(_T("RowElev"), CComVariant(row.RowElev));
         pStrSave->put_Property(_T("StrandsInRow"), CComVariant(row.StrandsInRow));
         pStrSave->EndUnit();
      }

      pStrSave->EndUnit(); // NonstandardFillData
   }
   else if (m_StrandFillType==sfDirectFill)
   {
      pStrSave->BeginUnit(_T("DirectSelectStrandFill"), 1.0);
      pStrSave->BeginUnit(_T("StraightStrands"), 1.0);
      pStrSave->put_Property(_T("NumStraightFill"), CComVariant( StrandIndexType(m_DirectFilledStraightStrands.size())));
      for (CDirectStrandFillCollection::const_iterator its=m_DirectFilledStraightStrands.begin(); its!=m_DirectFilledStraightStrands.end(); its++)
      {
         its->Save(pStrSave);
      }
      pStrSave->EndUnit(); // StraightStrands

      pStrSave->BeginUnit(_T("StraightStrandDebonding"),1.0);
      CollectionIndexType nDebondInfo = m_DirectFilledStraightDebond.size();
      pStrSave->put_Property(_T("DebondInfoCount"),CComVariant(nDebondInfo));
      std::vector<CDebondData>::iterator debond_iter;
      for ( debond_iter = m_DirectFilledStraightDebond.begin(); debond_iter != m_DirectFilledStraightDebond.end(); debond_iter++ )
      {
         const CDebondData& debond_info = *debond_iter;
         debond_info.Save(pStrSave,pProgress);
      }
      pStrSave->EndUnit(); // StraightStrandDebonding
      pStrSave->EndUnit(); // DirectSelectStrandFill
   }
   else
   {
      ATLASSERT(false);
   }

   pStrSave->EndUnit();

   return hr;
}

HRESULT CTxDOTOptionalDesignGirderData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

   CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("TxDOTOptionalGirderData"));
      Float64 version;
      pStrLoad->get_Version(&version);

      CComVariant var;

      if (version<2.0)
      {
         // old version was either standard or non
         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property(_T("StandardStrandFill"), &var );
         if( var.boolVal!=VARIANT_FALSE )
         {
            m_StrandFillType = sfStandard;
         }
         else
         {
            m_StrandFillType = sfHarpedRows; // assume for now - may need to make direct fill downstream
         }
      }
      else
      {
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("StrandFillType"), &var );
         m_StrandFillType = (StrandFillType)var.lVal;
      }

      var.Clear();
      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("StrandMaterialKey"), &var );
      Int32 key = var.lVal;
      if ( version < 3 )
      {
         // prior to version 3, strand key did not contain a coating type bit.
         // the assumption was the strand was uncoated. Add the bit for uncoated strand here.
         key |= matPsStrand::None;
      }

      lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
      const matPsStrand* pStrand = pPool->GetStrand(key);
      if (pStrand!=nullptr)
      {
         m_Grade = pStrand->GetGrade();
         m_Type = pStrand->GetType();
         m_Size = pStrand->GetSize();
         m_Coating = pStrand->GetCoating();
      }
      else
      {
         ASSERT(0); // problemo
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Fci"), &var );
      m_Fci = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Fc"), &var );
      m_Fc = var.dblVal;

      if (m_StrandFillType==sfStandard)
      {
         hr = pStrLoad->BeginUnit(_T("StandardFillData"));

         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("NumStrands"), &var );
         m_NumStrands = var.lVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("StrandTo"), &var );
         m_StrandTo = var.dblVal;

         hr = pStrLoad->EndUnit(); // end BridgeInputData
      }
      else if (m_StrandFillType==sfHarpedRows)
      {
         hr = pStrLoad->BeginUnit(_T("NonstandardFillData"));

         bool IsVersion1AndStraight = false; // Version 1.0 used rows to define straight strands
                                             // We will take the easy way out and define identical row tables
         if (version<2.0)
         {
            var.Clear();
            var.vt = VT_BOOL;
            hr = pStrLoad->get_Property(_T("UseDepressedStrands"), &var );
            if(var.boolVal==VARIANT_FALSE)
            {
               // Old version, and previous assumption about harped strands was incorrect.
               IsVersion1AndStraight = true;
            }
         }

         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("StrandRowsAtCLCount"), &var );
         Int32 rowcount = var.lVal;

         for (Int32 ir=0; ir<rowcount; ir++)
         {
            hr = pStrLoad->BeginUnit(_T("StrandRowAtCL"));

            StrandRow row;

            var.Clear();
            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("RowElev"), &var );
            row.RowElev = var.dblVal;

            var.Clear();
            var.vt = VT_I4;
            hr = pStrLoad->get_Property(_T("StrandsInRow"), &var );
            row.StrandsInRow = var.lVal;

            m_StrandRowsAtCL.insert(row);

            hr = pStrLoad->EndUnit();
         }

         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("StrandRowsAtEndsCount"), &var );
         rowcount = var.lVal;

         if (IsVersion1AndStraight)
         {
            ATLASSERT(rowcount==0); // old nondepressed had no data

            m_StrandRowsAtEnds = m_StrandRowsAtCL; // rows same at ends and middle - e.g., straight strands
         }
         else
         {
            for (Int32 ir=0; ir<rowcount; ir++)
            {
               hr = pStrLoad->BeginUnit(_T("StrandRowAtEnds"));

               StrandRow row;

               var.Clear();
               var.vt = VT_R8;
               hr = pStrLoad->get_Property(_T("RowElev"), &var );
               row.RowElev = var.dblVal;

               var.Clear();
               var.vt = VT_I4;
               hr = pStrLoad->get_Property(_T("StrandsInRow"), &var );
               row.StrandsInRow = var.lVal;

               m_StrandRowsAtEnds.insert(row);

               hr = pStrLoad->EndUnit();
            }
         }

         hr = pStrLoad->EndUnit(); // NonstandardFillData

      }
      else if (m_StrandFillType==sfDirectFill)
      {
         hr = pStrLoad->BeginUnit(_T("DirectSelectStrandFill"));
         if (FAILED(hr))
         {
            ATLASSERT(false);
            return hr;
         }

         m_DirectFilledStraightStrands.clear();

         pStrLoad->BeginUnit(_T("StraightStrands"));
         var.Clear();
         var.vt = VT_UI2;
         pStrLoad->get_Property(_T("NumStraightFill"), &var );
         StrandIndexType nums = var.uiVal;
         for (StrandIndexType is=0; is<nums; is++)
         {
            CDirectStrandFillInfo fi;
            fi.Load(pStrLoad);
            m_DirectFilledStraightStrands.AddFill(fi);
         }
         pStrLoad->EndUnit();  

         // debond
         m_DirectFilledStraightDebond.clear();
         pStrLoad->BeginUnit(_T("StraightStrandDebonding"));
         long nDebondInfo;
         var.Clear();
         var.vt = VT_I4;
         pStrLoad->get_Property(_T("DebondInfoCount"),&var);
         nDebondInfo = var.lVal;

         int i = 0;
         for ( i = 0; i < nDebondInfo; i++ )
         {
            CDebondData debond_info;
            debond_info.Load(pStrLoad,pProgress);
            m_DirectFilledStraightDebond.push_back(debond_info);
         }

         pStrLoad->EndUnit(); // StraightStrandDebonding
         pStrLoad->EndUnit();  // DirectSelectStrandFill
      }
      else
      {
         ATLASSERT(false);
      }

      hr = pStrLoad->EndUnit(); // TxDOTOptionalGirderData
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   // One last step - clear out any strand data from fill methods not used
   if (m_StrandFillType != sfStandard)
   {
      m_NumStrands = 0;
      m_StrandTo = 0.0;
   }

   if (m_StrandFillType != sfHarpedRows)
   {
      m_StrandRowsAtCL.clear();
      m_StrandRowsAtEnds.clear();
   }

   if (m_StrandFillType != sfDirectFill)
   {
      m_DirectFilledStraightStrands.clear();
      m_DirectFilledStraightDebond.clear();
   }

   return hr;
}


//======================== ACCESS     =======================================
void CTxDOTOptionalDesignGirderData::SetStrandFillType(StrandFillType val)
{
   if (val != m_StrandFillType)
   {
      m_StrandFillType = val;
      FireChanged(ITxDataObserver::ctGirder);
   }
}

CTxDOTOptionalDesignGirderData::StrandFillType CTxDOTOptionalDesignGirderData::GetStrandFillType() const
{
   return m_StrandFillType;
}

void CTxDOTOptionalDesignGirderData::SetStrandData(matPsStrand::Grade grade,
                   matPsStrand::Type type,
                   matPsStrand::Coating coating,
                   matPsStrand::Size size)
{
   if (m_Grade!=grade || m_Type!=type || m_Coating != coating || m_Size!=size)
   {
      m_Grade = grade;
      m_Type  = type;
      m_Coating = coating;
      m_Size  = size;

      FireChanged(ITxDataObserver::ctGirder);
   }
}

void CTxDOTOptionalDesignGirderData::GetStrandData(matPsStrand::Grade* pgrade,
                   matPsStrand::Type* ptype,
                   matPsStrand::Coating* pcoating,
                   matPsStrand::Size* psize)
{
   *pgrade = m_Grade;
   *ptype  = m_Type;
   *pcoating = m_Coating;
   *psize  = m_Size;
}

void CTxDOTOptionalDesignGirderData::SetFci(Float64 val)
{
   if (val != m_Fci)
   {
      m_Fci = val;
      FireChanged(ITxDataObserver::ctGirder);
   }
}

Float64 CTxDOTOptionalDesignGirderData::GetFci() const
{
   return m_Fci;
}

void CTxDOTOptionalDesignGirderData::SetFc(Float64 val)
{
   if (val != m_Fc)
   {
      m_Fc = val;
      FireChanged(ITxDataObserver::ctGirder);
   }
}

Float64 CTxDOTOptionalDesignGirderData::GetFc() const
{
   return m_Fc;
}

// Data for standard fill
// ========================
StrandIndexType CTxDOTOptionalDesignGirderData::GetStrandCount()
{
   return m_NumStrands;
}

void CTxDOTOptionalDesignGirderData::SetNumStrands(StrandIndexType val)
{
   if (val != m_NumStrands)
   {
      m_NumStrands = val;
      FireChanged(ITxDataObserver::ctGirder);
   }
}


void CTxDOTOptionalDesignGirderData::SetStrandTo(Float64 val)
{
   if (val != m_StrandTo)
   {
      m_StrandTo = val;
      FireChanged(ITxDataObserver::ctGirder);
   }
}

Float64 CTxDOTOptionalDesignGirderData::GetStrandTo() const
{
   return m_StrandTo;
}

// Utilities for standard fill
// ===========================
std::vector<StrandIndexType> CTxDOTOptionalDesignGirderData::ComputeAvailableNumStrands(GirderLibrary* pLib)
{
   std::vector<StrandIndexType> strands;

   // zero strands is always an option
   strands.push_back(0);

   CString girder_name = m_pParent->GetGirderEntryName();
   const GirderLibraryEntry* pGdrEntry = dynamic_cast<const GirderLibraryEntry*>(pLib->GetEntry(girder_name));
   if (pGdrEntry==nullptr)
   {
      CString msg, stmp;
      stmp.LoadString(IDS_GDR_ERROR);
      msg.Format(stmp,girder_name);
      ::AfxMessageBox(msg);
   }
   else
   {
      StrandIndexType maxStraight = pGdrEntry->GetMaxStraightStrands();
      StrandIndexType maxHarped   = pGdrEntry->GetMaxHarpedStrands();

      StrandIndexType maxNum = maxStraight+maxHarped;

      for(StrandIndexType currNum=1; currNum<=maxNum; currNum++)
      {
         StrandIndexType numStraight, numHarped;
         if( pGdrEntry->GetPermStrandDistribution(currNum, &numStraight, &numHarped))
         {
            strands.push_back(currNum);
         }
      }
   }

   return strands;
}

bool CTxDOTOptionalDesignGirderData::ComputeToRange(GirderLibrary* pLib, StrandIndexType ns, Float64* pToLower, Float64* pToUpper)
{

   CString girder_name = m_pParent->GetGirderEntryName();
   const GirderLibraryEntry* pGdrEntry = dynamic_cast<const GirderLibraryEntry*>(pLib->GetEntry(girder_name));
   if (pGdrEntry==nullptr)
   {
      ASSERT(0);
      CString msg, stmp;
      stmp.LoadString(IDS_GDR_ERROR);
      msg.Format(stmp,girder_name);
      ::AfxMessageBox(msg);
   }
   else
   {
      StrandIndexType numStraight, numHarped;
      if( pGdrEntry->GetPermStrandDistribution(ns, &numStraight, &numHarped))
      {
         if (numHarped==0)
         {
            return false; // To value is meaningless
         }

         Float64 height = pGdrEntry->GetBeamHeight(pgsTypes::metStart);

         // Adjustment limits for strand locations at ends
         pgsTypes::FaceType  topFace, bottomFace;
         Float64  topLimit, bottomLimit;
         pGdrEntry->GetEndAdjustmentLimits(&topFace, &topLimit, &bottomFace, &bottomLimit);

         // To max is easy
         *pToUpper = topFace==pgsTypes::BottomFace ? topLimit : height-topLimit;

         // To-min must take height of strand bundle into consideration
         // compute lower and upper bounds of numHarped strands at girder end
         StrandIndexType maxHarpedCoords =  pGdrEntry->GetNumHarpedStrandCoordinates();

         Float64 ymin(Float64_Max), ymax(-Float64_Max);
         StrandIndexType currHarped = 0;
         for (StrandIndexType ih=0; ih<maxHarpedCoords; ih++)
         {
            Float64 xstart, ystart, xhp, yhp, xend, yend;
            pGdrEntry->GetHarpedStrandCoordinates(ih,&xstart,&ystart,&xhp,&yhp,&xend,&yend);

            ymin = Min(ymin,ystart);
            ymax = Max(ymax,ystart);

            currHarped += (xstart>0.0 || xhp>0.0) ? 2 : 1;

            if (currHarped>=numHarped)
               break;
         }

         ASSERT(currHarped>=numHarped);

         Float64 h_bundle = ymax-ymin;

         Float64 bot_loc = bottomFace==pgsTypes::TopFace ? height-bottomLimit : bottomLimit;

         *pToLower = h_bundle + bot_loc;
      }
      else
      {
         ASSERT(0);
         CString msg;
         msg.Format(_T("%d Strands do not fit in this girder. This is a programming error"),ns);
         ::AfxMessageBox(msg);
         return false;
      }
   }

   return true;
}

bool CTxDOTOptionalDesignGirderData::ComputeEccentricities(GirderLibrary* pLib, StrandIndexType ns, Float64 To, Float64* pEccEnds, Float64* pEccCL)
{
   *pEccEnds = 0.0;
   *pEccCL = 0.0;

   if (ns==0)
      return false;

   CString girder_name = m_pParent->GetGirderEntryName();
   const GirderLibraryEntry* pGdrEntry = dynamic_cast<const GirderLibraryEntry*>(pLib->GetEntry(girder_name));
   if (pGdrEntry==nullptr)
   {
      CString msg, stmp;
      stmp.LoadString(IDS_GDR_ERROR);
      msg.Format(stmp,girder_name);
      ::AfxMessageBox(msg);
      return false;
   }
   else
   {
      // first need cg location of outer shape - this requires some work
      CComPtr<IBeamFactory> pFactory;
      pGdrEntry->GetBeamFactory(&pFactory);
      GirderLibraryEntry::Dimensions dimensions = pGdrEntry->GetDimensions();

      CComPtr<IGirderSection> gdrSection;
      pFactory->CreateGirderSection(nullptr,INVALID_ID,dimensions,-1,-1,&gdrSection);

      CComPtr<IShape>  pShape;
      gdrSection.QueryInterface(&pShape);

      CComPtr<IShapeProperties> pShapeProps;
      pShape->get_ShapeProperties(&pShapeProps);

      Float64 ybot,ytop;
      pShapeProps->get_Ybottom(&ybot);
      pShapeProps->get_Ytop(&ytop);

      Float64 height = ybot+ytop;

      // Strands...
      StrandIndexType numStraight, numHarped;
      bool st = pGdrEntry->GetPermStrandDistribution(ns, &numStraight, &numHarped);
      ASSERT(st);
      ASSERT(ns == numStraight+numHarped);

      // compute ecc for straight group (assume non-sloping strands)
      StrandIndexType max_straight_coords = pGdrEntry->GetNumStraightStrandCoordinates();
      StrandIndexType num_straight_strands(0);
      Float64 firstMom(0.0);
      for(StrandIndexType currIdx=0; (currIdx<=max_straight_coords && num_straight_strands<numStraight); currIdx++)
      {
         Float64 Xstart, Ystart, Xend, Yend;
         bool canDebond;
         pGdrEntry->GetStraightStrandCoordinates(currIdx, &Xstart, &Ystart, &Xend, &Yend, &canDebond);
         if (Xstart>0.0)
         {
            num_straight_strands += 2;
            firstMom += 2.0 * Ystart;
         }
         else
         {
            num_straight_strands += 1;
            firstMom += Ystart;
         }

         if (num_straight_strands>=numStraight)
            break;
      }

      ASSERT(num_straight_strands==numStraight); // coerce odd strands? not supported here

      Float64 cg_straight_strands = numStraight>0 ? firstMom/numStraight : 0.0;

      // compute unadjusted (To) ecc for harped group
      StrandIndexType max_harped_coords = pGdrEntry->GetNumHarpedStrandCoordinates();
      Float64 num_harped_strands = 0;
      Float64 ymin(Float64_Max), ymax(-Float64_Max); // y bounds
      Float64 endFirstMom(0.0), hpFirstMom(0.0);
      for(StrandIndexType currIdx=0; currIdx<=max_harped_coords && num_harped_strands<numHarped; currIdx++)
      {
         Float64 Xstart, Ystart, Xhp, Yhp, Xend, Yend;
         pGdrEntry->GetHarpedStrandCoordinates(currIdx, &Xstart, &Ystart, &Xhp, &Yhp, &Xend, &Yend);

         ymin = Min(ymin,Ystart);
         ymax = Max(ymax,Ystart);

         if (Xstart>0.0 || Xhp>0.0)
         {
            num_harped_strands += 2;
            endFirstMom += 2.0 * Ystart;
            hpFirstMom += 2.0 * Yhp;
         }
         else
         {
            num_harped_strands += 1;
            endFirstMom += Ystart;
            hpFirstMom +=  Yhp;
         }

         if (num_harped_strands>=numHarped)
            break;
      }

      ASSERT(num_harped_strands==numHarped); // odd strands? (not supported here)

      Float64 cg_hp_harped_strands = numHarped>0 ? hpFirstMom/numHarped : 0.0;

      Float64 cg_end_harped_strands = numHarped>0 ? endFirstMom/numHarped : 0.0;

      // Adjust end harped strands for To
      if (numHarped > 0)
      {
         Float64 adjust = To - ymax;
         cg_end_harped_strands += adjust;
      }

      // Now can compute composite cg's and eccs'
      Float64 cg_end_comp = (numStraight*cg_straight_strands + numHarped*cg_end_harped_strands)/ns;
      Float64 cg_hp_comp  = (numStraight*cg_straight_strands + numHarped*cg_hp_harped_strands) /ns;

      *pEccEnds = ybot - cg_end_comp;
      *pEccCL   = ybot - cg_hp_comp;
   }

   return true;
}

bool CTxDOTOptionalDesignGirderData::ComputeDirectFillEccentricity(const GirderLibraryEntry* pGdrEntry, Float64* pEcc) const
{
   *pEcc = 0.0;

   if (m_DirectFilledStraightStrands.GetFilledStrandCount() == 0)
      return false;

   if (pGdrEntry==nullptr)
   {
      return false;
   }
   else
   {
      // first need cg location of outer shape - this requires some work
      CComPtr<IBeamFactory> pFactory;
      pGdrEntry->GetBeamFactory(&pFactory);
      GirderLibraryEntry::Dimensions dimensions = pGdrEntry->GetDimensions();

      CComPtr<IGirderSection> gdrSection;
      pFactory->CreateGirderSection(nullptr,INVALID_ID,dimensions,-1,-1,&gdrSection);

      CComPtr<IShape>  pShape;
      gdrSection.QueryInterface(&pShape);

      CComPtr<IShapeProperties> pShapeProps;
      pShape->get_ShapeProperties(&pShapeProps);

      Float64 ybot,ytop;
      pShapeProps->get_Ybottom(&ybot);
      pShapeProps->get_Ytop(&ytop);

      Float64 height = ybot+ytop;

      // Strands...
      StrandIndexType maxStraight = pGdrEntry->GetMaxStraightStrands();
      ASSERT(maxStraight > 0);

      // compute ecc for straight group (assume non-sloping strands)
      StrandIndexType max_straight_coords = pGdrEntry->GetNumStraightStrandCoordinates();
      StrandIndexType num_straight_strands(0);
      Float64 firstMom(0.0);

      for(std::vector<CDirectStrandFillInfo>::const_iterator it=m_DirectFilledStraightStrands.begin(); it!=m_DirectFilledStraightStrands.end(); it++)
      {
         const CDirectStrandFillInfo& fillInfo = *it;

         StrandIndexType strandIdx = fillInfo.permStrandGridIdx;
         ATLASSERT(strandIdx<maxStraight);

         Float64 Xstart, Ystart, Xend, Yend;
         bool canDebond;
         pGdrEntry->GetStraightStrandCoordinates(strandIdx, &Xstart, &Ystart, &Xend, &Yend, &canDebond);
         if (Xstart>0.0)
         {
            num_straight_strands += 2;
            firstMom += 2.0 * Ystart;
         }
         else
         {
            num_straight_strands += 1;
            firstMom += Ystart;
         }
      }

      Float64 cg_straight_strands = num_straight_strands>0 ? firstMom/num_straight_strands : 0.0;

      *pEcc = ybot - cg_straight_strands;
   }

   return true;
}


// Data for non-standard row fill
// ==========================
const CTxDOTOptionalDesignGirderData::StrandRowContainer CTxDOTOptionalDesignGirderData::GetStrandsAtCL() const
{
   return m_StrandRowsAtCL;
}

void CTxDOTOptionalDesignGirderData::SetStrandsAtCL(const CTxDOTOptionalDesignGirderData::StrandRowContainer& container)
{
   if (m_StrandRowsAtCL != container)
   {
      m_StrandRowsAtCL = container;

      FireChanged(ITxDataObserver::ctGirder);
   }
}

const CTxDOTOptionalDesignGirderData::StrandRowContainer CTxDOTOptionalDesignGirderData::GetStrandsAtEnds() const
{
   return m_StrandRowsAtEnds;
}

void CTxDOTOptionalDesignGirderData::SetStrandsAtEnds(const CTxDOTOptionalDesignGirderData::StrandRowContainer& container)
{
   if (m_StrandRowsAtEnds != container)
   {
      m_StrandRowsAtEnds = container;

      FireChanged(ITxDataObserver::ctGirder);
   }
}

CTxDOTOptionalDesignGirderData::AvailableStrandsInRowContainer CTxDOTOptionalDesignGirderData::ComputeAvailableStrandRows(const GirderLibraryEntry* pGdrEntry)
{
   ASSERT(!(pGdrEntry->IsDifferentHarpedGridAtEndsUsed() && pGdrEntry->GetMaxHarpedStrands()>0) ); // we can't do separate grids

   CTxDOTOptionalDesignGirderData::AvailableStrandsInRowContainer available_rows;

   // Cycle through strands in global order and add to our row collection as we go
   StrandIndexType num_global = pGdrEntry->GetPermanentStrandGridSize();

   for (StrandIndexType i_global=0; i_global<num_global; i_global++)
   {
      GirderLibraryEntry::psStrandType strand_type;
      StrandIndexType i_local;
      pGdrEntry->GetGridPositionFromPermStrandGrid(i_global, &strand_type, &i_local);

      // Representative x, y, and num strands
      Float64 x_strand, y_strand;
      StrandIndexType num_strands(0), num_harped(0);
      if (strand_type==GirderLibraryEntry::stStraight)
      {
         Float64  Xend, Yend;
         bool canDebond;
         pGdrEntry->GetStraightStrandCoordinates(i_local, &x_strand, &y_strand, &Xend, &Yend, &canDebond);

         num_strands = x_strand==0.0 ? 1 : 2; // two strands if x>0.0
      }
      else if (strand_type==GirderLibraryEntry::stAdjustable)
      {
         Float64 Xhp, Yhp, Xend, Yend;
         pGdrEntry->GetHarpedStrandCoordinates(i_local, &x_strand, &y_strand, &Xhp, &Yhp, &Xend, &Yend); // all locations assumed same

         num_strands = (x_strand==0.0) ? 1 : 2; // two strands if x>0.0
         num_harped = num_strands;
      }
      else
         ASSERT(0); // txdot adding new stand type?

      // see if row exists - if so, add strands to it - if not, add row
      AvailableStrandsInRow tester(y_strand);
      AvailableStrandsInRowIterator testiter = available_rows.find(tester);
      if (testiter==available_rows.end())
      {
         // row not in collection - add it
         StrandIncrement incr;
         tester.AvailableStrandIncrements.push_back(incr); // zero strands is always an option

         incr.TotalStrands     = num_strands;
         incr.GlobalFill       = i_global;
         tester.AvailableStrandIncrements.push_back(incr);

         tester.MaxHarped = num_harped;

         available_rows.insert(tester);
      }
      else
      {
         // Row exists - add next increment
         AvailableStrandsInRow& rowdata = const_cast<AvailableStrandsInRow&>(*testiter);
         const StrandIncrement& last_inc = rowdata.AvailableStrandIncrements.back();

         StrandIncrement incr;
         incr.TotalStrands     = last_inc.TotalStrands + num_strands;            // next value is last + new available
         incr.GlobalFill       = i_global;

         rowdata.MaxHarped += num_harped;

         rowdata.AvailableStrandIncrements.push_back(incr); 
      }
   }

   return available_rows;
}

// Data for direct manip fill
// ===========================
const CDirectStrandFillCollection& CTxDOTOptionalDesignGirderData::GetDirectFilledStraightStrands() const
{
   return m_DirectFilledStraightStrands;
}

void CTxDOTOptionalDesignGirderData::SetDirectFilledStraightStrands(const CDirectStrandFillCollection& coll)
{
   if (coll != m_DirectFilledStraightStrands)
   {
      m_DirectFilledStraightStrands = coll;
      FireChanged(ITxDataObserver::ctGirder);
   }
}

const std::vector<CDebondData>& CTxDOTOptionalDesignGirderData::GetDirectFilledStraightDebond() const
{
   return m_DirectFilledStraightDebond;
}

void CTxDOTOptionalDesignGirderData::SetDirectFilledStraightDebond(const std::vector<CDebondData>& coll)
{
   if (coll != m_DirectFilledStraightDebond)
   {
      m_DirectFilledStraightDebond = coll;
      FireChanged(ITxDataObserver::ctGirder);
   }
}

// Private data structures for CheckAndBuildStrandRows
struct ShRow
{
   Float64          RowElev;
   StrandIndexType  NumHarpedCL;
   StrandIndexType  NumStraightCL;
   StrandIndexType  NumHarpedEnd;
   StrandIndexType  NumStraightEnd;

   ShRow(): RowElev(-1.0), NumHarpedCL(0), NumStraightCL(0), NumHarpedEnd(0), NumStraightEnd(0)
   {;}

   ShRow(Float64 rowElev):
   RowElev(rowElev), NumHarpedCL(0), NumStraightCL(0), NumHarpedEnd(0), NumStraightEnd(0)
   {;}

   bool operator==(const ShRow& rOther) const 
   { 
      return ::IsEqual(RowElev,rOther.RowElev);
   }

   bool operator<(const ShRow& rOther) const 
   { 
      return RowElev < rOther.RowElev; 
   }
};
typedef std::set<ShRow> ShRowContainer;
typedef ShRowContainer::iterator ShRowIterator;

bool CTxDOTOptionalDesignGirderData::CheckAndBuildStrandRows(const GirderLibraryEntry* pMasterGdrEntry, 
                                                             const StrandRowContainer& rClRows, const StrandRowContainer& rEndRows, 
                                                             CString& rErrMsg,
                                                             GirderLibraryEntry* pCloneGdrEntry)
{
   // Container to track straight and harped strands per row
   ShRowContainer shrows;

   // Get possible strand locations from master
   AvailableStrandsInRowContainer avail_rows  = ComputeAvailableStrandRows(pMasterGdrEntry);

   // Compute number of harped strands at cl and end - make sure they match
   // NOTE:: Assumption here that harped strands are always first strands placed in a row at the C.L.
   // =====
   StrandIndexType tot_nh_cl(0), tot_nh_end(0);
   for(StrandRowConstIterator hit=rClRows.begin(); hit!=rClRows.end(); hit++)
   {
      const StrandRow& sr = *hit;
      Float64 elev = sr.RowElev;
      StrandIndexType no_cl = sr.StrandsInRow;

      StrandIndexType nh(0);
      AvailableStrandsInRowIterator avit = avail_rows.find(elev);
      if (avit!=avail_rows.end())
      {
         // Harped strands are always first strands placed in a row
         nh = Min(no_cl, avit->MaxHarped);

         tot_nh_cl += nh;
      }
      else
      {
         Float64 elev_in = ::ConvertFromSysUnits(elev, unitMeasure::Inch);
         rErrMsg.Format(_T("Non-Standard strand input data at girder centerline specified a strand at %.3f in. from the girder bottom. There are no stands at this location in the library."),elev_in);
         return false;
      }

      // insert row into local collection
      ShRow shrow(elev);
      shrow.NumHarpedCL = nh;
      shrow.NumStraightCL = no_cl - nh;

      shrows.insert(shrow);
   }

   // Compute number of harped/straight for each row at ends
   for(StrandRowConstIterator hit=rEndRows.begin(); hit!=rEndRows.end(); hit++)
   {
      const StrandRow& sr = *hit;
      Float64 elev = sr.RowElev;
      StrandIndexType no_end = sr.StrandsInRow;

      if (no_end<=0)
      {
         ASSERT(0); // should not have input with no strands
         break;
      }

      // Get max harped strands allowed
      StrandIndexType nh_max(0);
      AvailableStrandsInRowIterator avit = avail_rows.find(elev);
      if (avit!=avail_rows.end())
      {
         nh_max = avit->MaxHarped;
      }
      else
      {
         Float64 elev_in = ::ConvertFromSysUnits(elev, unitMeasure::Inch);
         rErrMsg.Format(_T("Non-Standard strand input data at girder ends specified a strand at %.3f in. from the girder bottom. There are no stands at this location in the library."),elev_in);
         return false;
      }

      // Now look at strands at same elev at c.l. If there are more at cl than here, this is a hole not a harped strand
      StrandIndexType no_cl = 0;
      StrandRowConstIterator clit = rClRows.find(elev);
      if (clit!=rClRows.end())
      {
         no_cl = clit->StrandsInRow;
      }

      StrandIndexType nh(0);
      if (no_end >= no_cl)
      {
         // This is a harped location
         nh = Min(no_end,nh_max);
         tot_nh_end += nh;
      }

      // fill our local container
      ShRowIterator shrit = shrows.find(elev);
      if (shrit != shrows.end())
      {
         ShRow& row(const_cast<ShRow&>(*shrit));
         row.NumHarpedEnd = nh;
         row.NumStraightEnd = no_end - nh;
      }
      else
      {
         ShRow shrow(elev);
         shrow.NumHarpedEnd = nh;
         shrow.NumStraightEnd = no_end - nh;

         shrows.insert(shrow);
      }
   }

   if (tot_nh_end != tot_nh_cl)
   {
      rErrMsg.Format(_T("The number of depressed strands at the girder ends and centerline do not match. Ncl=%d, Nends=%d. Cannot continue"),tot_nh_cl,tot_nh_end);
      return false;
   }

   // Next check that the number of straight strands at ends and cl are the same in each row
   for(ShRowIterator shit=shrows.begin(); shit!=shrows.end(); shit++)
   {
      const ShRow& shrow = *shit;

      if (shrow.NumStraightCL != shrow.NumStraightEnd)
      {
         Float64 elev_in = ::ConvertFromSysUnits(shrow.RowElev, unitMeasure::Inch);
         rErrMsg.Format(_T("The number of straight strands in each row at the C.L. and ends must be the same. They are not at row %.3f in. Ncl=%d, Nends=%d."),elev_in, shrow.NumStraightCL, shrow.NumStraightEnd);
         return false;
      }
   }

   // At this point we are mostly done checking, and if we have a clone, time to make the strands
   if (pCloneGdrEntry != nullptr)
   {
      ASSERT(!pMasterGdrEntry->IsDifferentHarpedGridAtEndsUsed() && pMasterGdrEntry->GetMaxHarpedStrands()>0);

      ShRowIterator shit_cl  = shrows.begin();
      ShRowIterator shit_end = shrows.begin();
      while(shit_cl != shrows.end())
      {
         // Get available strands at cl. Use these for both straight and harped strands
         AvailableStrandsInRowIterator avail_cl = avail_rows.find(shit_cl->RowElev);
         ASSERT(avail_cl != avail_rows.end());

         // First fill harped strands in row
         if (shit_cl->NumHarpedCL > 0)
         {
            // Harped strands in this row at the cl - need to match with end harped at this or higher row
            while (shit_end->NumHarpedEnd==0 && shit_end!=shrows.end())
               shit_end++;

            if(shit_end==shrows.end())
            {
               ASSERT(0);
               rErrMsg.Format(_T("Programming error computing harped strands - cannot continue."));
               return false;
            }

            if (shit_cl->NumHarpedCL != shit_end->NumHarpedEnd)
            {
               Float64 elev_in = ::ConvertFromSysUnits(shit_cl->RowElev, unitMeasure::Inch);
               rErrMsg.Format(_T("The number of harped strands at connecting locations must be the same. They are not at CL row %.3f in. Ncl=%d, Nends=%d."),elev_in, shit_cl->NumHarpedCL, shit_end->NumHarpedEnd);
               return false;
            }

            // We have harped strand locations at cl, get at end, add to library
            AvailableStrandsInRowIterator avail_end = avail_rows.find(shit_end->RowElev);
            ASSERT(avail_end != avail_rows.end());

            // Loop to fill harped strands
            StrandIndexType nh_filled=0;
            std::vector<StrandIncrement>::iterator siit_end = const_cast<AvailableStrandsInRow&>(*avail_end).AvailableStrandIncrements.begin();
            for (std::vector<StrandIncrement>::iterator siit_cl = const_cast<AvailableStrandsInRow&>(*avail_cl).AvailableStrandIncrements.begin();
                                                        (siit_cl!= const_cast<AvailableStrandsInRow&>(*avail_cl).AvailableStrandIncrements.end() && siit_end!= const_cast<AvailableStrandsInRow&>(*avail_end).AvailableStrandIncrements.end());
                                                        siit_cl++, siit_end++)
            {
               StrandIncrement& incr_cl  = *siit_cl;
               StrandIncrement& incr_end = *siit_end;

               if (incr_cl.GlobalFill != INVALID_INDEX)
               {
                  // Get coordinates at cl and ends - doesn't matter if they are harped or straight at this point
                  Float64 cl_x, cl_y;
                  GetGlobalStrandCoordinate(pMasterGdrEntry, incr_cl.GlobalFill, &cl_x, &cl_y);
                  ASSERT(IsEqual(cl_y, shit_cl->RowElev));

                  Float64 end_x, end_y;
                  GetGlobalStrandCoordinate(pMasterGdrEntry, incr_end.GlobalFill, &end_x, &end_y);
                  ASSERT(IsEqual(end_y, shit_end->RowElev));

                  // Create strand location and add it to global fill in clone
                  StrandIndexType nhnew = pCloneGdrEntry->AddHarpedStrandCoordinates(end_x, end_y, cl_x, cl_y, end_x, end_y);

                  pCloneGdrEntry->AddStrandToPermStrandGrid(GirderLibraryEntry::stAdjustable, nhnew-1);

                  incr_cl.WasFilled = true;  // unavailable after this point
                  incr_end.WasFilled = true;

                  nh_filled += (end_x>0.0 ? 2 : 1);

                  if (nh_filled >= shit_cl->NumHarpedCL)
                  {
                     ASSERT(nh_filled == shit_cl->NumHarpedCL);
                     break;
                  }
               }
            }
         }

         // Next fill straight strands
         if (shit_cl->NumStraightCL > 0)
         {
            StrandIndexType ns_filled=0;
            for (std::vector<StrandIncrement>::iterator siit_cl = const_cast<AvailableStrandsInRow&>(*avail_cl).AvailableStrandIncrements.begin();
                                                        siit_cl!= const_cast<AvailableStrandsInRow&>(*avail_cl).AvailableStrandIncrements.end();
                                                        siit_cl++)
            {
               StrandIncrement& incr_cl  = *siit_cl;

               if (incr_cl.GlobalFill != INVALID_INDEX && !incr_cl.WasFilled) // make sure we didn't already fill with a harped strand
               {
                  // Get coordinates 
                  Float64 cl_x, cl_y;
                  GetGlobalStrandCoordinate(pMasterGdrEntry, incr_cl.GlobalFill, &cl_x, &cl_y);

                  // Create strand location and add it to global fill in clone
                  StrandIndexType nsnew = pCloneGdrEntry->AddStraightStrandCoordinates(cl_x, cl_y, cl_x, cl_y, false);

                  pCloneGdrEntry->AddStrandToPermStrandGrid(GirderLibraryEntry::stStraight, nsnew-1);

                  incr_cl.WasFilled = true;  // unavailable after this point

                  ns_filled += (cl_x>0.0 ? 2 : 1);

                  if (ns_filled >= shit_cl->NumStraightCL)
                  {
                     ASSERT(ns_filled == shit_cl->NumStraightCL);
                     break;
                  }
               }
            }
         }

         // Increment end if we filled harped strands at cl
         if (shit_cl->NumHarpedCL > 0 && shit_end!=shrows.end())
            shit_end++;

         shit_cl++;
      }
   }

   return true;
}

void CTxDOTOptionalDesignGirderData::GetGlobalStrandCoordinate(const GirderLibraryEntry* pGdrEntry, StrandIndexType globalIdx, Float64* pX, Float64* pY)
{
   GirderLibraryEntry::psStrandType type;
   StrandIndexType local_idx;
   pGdrEntry->GetGridPositionFromPermStrandGrid(globalIdx, &type, &local_idx);

   // ASSUME: prismatic xsection, harped strands defined straight
   if (type==GirderLibraryEntry::stStraight)
   {
      Float64 xend, yend;
      bool cand;
      pGdrEntry->GetStraightStrandCoordinates(local_idx, pX, pY, &xend, &yend, &cand);
   }
   else
   {
      ASSERT(type==GirderLibraryEntry::stAdjustable);

      Float64 xend, yend, xstart, ystart;
      pGdrEntry->GetHarpedStrandCoordinates(local_idx, &xstart, &ystart, pX, pY, &xend, &yend); 
   }
}


//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////
//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CTxDOTOptionalDesignGirderData::MakeCopy(const CTxDOTOptionalDesignGirderData& rOther)
{

   m_pParent = rOther.m_pParent;

   m_StrandFillType = rOther.m_StrandFillType;
   m_Grade = rOther.m_Grade;
   m_Type = rOther.m_Type;
   m_Coating = rOther.m_Coating;
   m_Size = rOther.m_Size;

   m_Fci = rOther.m_Fci;
   m_Fc = rOther.m_Fc;

   m_NumStrands = rOther.m_NumStrands;
   m_StrandTo = rOther.m_StrandTo;

   m_StrandRowsAtCL = rOther.m_StrandRowsAtCL;
   m_StrandRowsAtEnds = rOther.m_StrandRowsAtEnds;

   m_DirectFilledStraightStrands = rOther.m_DirectFilledStraightStrands;
   m_DirectFilledStraightDebond = rOther.m_DirectFilledStraightDebond;
}

void CTxDOTOptionalDesignGirderData::MakeAssignment(const CTxDOTOptionalDesignGirderData& rOther)
{
   MakeCopy( rOther );
}

void CTxDOTOptionalDesignGirderData::FireChanged(ITxDataObserver::ChangeType change)
{
   // notify our parent/observers we changed
   m_pParent->FireChanged(change);
}
