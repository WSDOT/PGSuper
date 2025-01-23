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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\SegmentPTData.h>
#include <LRFD\StrandPool.h>
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSegmentDuctData::CSegmentDuctData()
{
   m_pPTData = nullptr;
   m_pSegment = nullptr;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, ILibraryNames, pLibNames);
   std::vector<std::_tstring> vNames;
   pLibNames->EnumDuctNames(&vNames);
   Name = vNames.front();

   pDuctLibEntry = nullptr;

   nStrands = 0;
   bPjCalc = true;
   Pj = 0.0;
   LastUserPj = 0.0;

   DuctGeometryType = Parabolic;
   JackingEnd = pgsTypes::jeStart;

   DuctPoint[Left].first = WBFL::Units::ConvertToSysUnits(2.0, WBFL::Units::Measure::Inch);
   DuctPoint[Left].second = pgsTypes::TopFace;

   DuctPoint[Middle].first = WBFL::Units::ConvertToSysUnits(2.0, WBFL::Units::Measure::Inch);
   DuctPoint[Middle].second = pgsTypes::BottomFace;

   DuctPoint[Right].first = WBFL::Units::ConvertToSysUnits(2.0, WBFL::Units::Measure::Inch);
   DuctPoint[Right].second = pgsTypes::TopFace;
}

CSegmentDuctData::CSegmentDuctData(const CPrecastSegmentData* pSegment)
{
   m_pPTData = nullptr;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, ILibraryNames, pLibNames);
   std::vector<std::_tstring> vNames;
   pLibNames->EnumDuctNames(&vNames);
   Name = vNames.front();

   pDuctLibEntry = nullptr;

   nStrands = 0;
   bPjCalc = true;
   Pj = 0.0;
   LastUserPj = 0.0;

   DuctGeometryType = Parabolic;
   JackingEnd = pgsTypes::jeStart;

   DuctPoint[Left].first = WBFL::Units::ConvertToSysUnits(2.0, WBFL::Units::Measure::Inch);
   DuctPoint[Left].second = pgsTypes::TopFace;

   DuctPoint[Middle].first = WBFL::Units::ConvertToSysUnits(2.0, WBFL::Units::Measure::Inch);
   DuctPoint[Middle].second = pgsTypes::BottomFace;

   DuctPoint[Right].first = WBFL::Units::ConvertToSysUnits(2.0, WBFL::Units::Measure::Inch);
   DuctPoint[Right].second = pgsTypes::TopFace;

   Init(pSegment);
}

void CSegmentDuctData::SetSegment(const CPrecastSegmentData* pSegment)
{
   m_pSegment = pSegment;
}

const CPrecastSegmentData* CSegmentDuctData::GetSegment() const
{
   return m_pSegment;
}

void CSegmentDuctData::Init(const CPrecastSegmentData* pSegment)
{
}

bool CSegmentDuctData::operator==(const CSegmentDuctData& rOther) const
{
   if (Name != rOther.Name)
   {
      return false;
   }

   if (nStrands != rOther.nStrands)
   {
      return false;
   }

   if (bPjCalc != rOther.bPjCalc)
   {
      return false;
   }

   if (!IsEqual(Pj, rOther.Pj))
   {
      return false;
   }

   if (DuctGeometryType != rOther.DuctGeometryType)
   {
      return false;
   }

   if (JackingEnd != rOther.JackingEnd)
   {
      return false;
   }

   if (!IsEqual(DuctPoint[Left].first, rOther.DuctPoint[Left].first))
   {
      return false;
   }

   if (DuctPoint[Left].second != rOther.DuctPoint[Left].second)
   {
      return false;
   }

   if (!IsEqual(DuctPoint[Right].first, rOther.DuctPoint[Right].first))
   {
      return false;
   }

   if (DuctPoint[Right].second != rOther.DuctPoint[Right].second)
   {
      return false;
   }

   if (DuctGeometryType != rOther.DuctGeometryType)
   {
      return false;
   }

   if (DuctGeometryType == Parabolic)
   {
      if (!IsEqual(DuctPoint[Middle].first, rOther.DuctPoint[Middle].first))
      {
         return false;
      }

      if (DuctPoint[Middle].second != rOther.DuctPoint[Middle].second)
      {
         return false;
      }
   }

   return true;
}

HRESULT CSegmentDuctData::Load(IStructuredLoad* pStrLoad, IProgress* pProgress)
{
   USES_CONVERSION;
   CHRException hr;
   CComVariant var;
   hr = pStrLoad->BeginUnit(_T("Duct"));

   Float64 version;
   pStrLoad->get_Version(&version);

   var.vt = VT_BSTR;
   hr = pStrLoad->get_Property(_T("Name"), &var);
   Name = OLE2T(var.bstrVal);

   var.vt = VT_INDEX;
   hr = pStrLoad->get_Property(_T("NumStrands"), &var);
   nStrands = VARIANT2INDEX(var);

   var.vt = VT_BOOL;
   hr = pStrLoad->get_Property(_T("CalcPj"), &var);
   bPjCalc = (var.boolVal == VARIANT_TRUE ? true : false);

   var.vt = VT_R8;
   hr = pStrLoad->get_Property(_T("Pj"), &var);
   Pj = var.dblVal;
   LastUserPj = Pj;

   if(1 < version)
   {
      // added in version 2
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("LastUserPj"), &var);
      LastUserPj = var.dblVal;
   }

   var.vt = VT_BSTR;
   hr = pStrLoad->get_Property(_T("StressingEnd"), &var);
   if (CComBSTR(_T("Left")) == CComBSTR(var.bstrVal))
   {
      JackingEnd = pgsTypes::jeStart;
   }
   else if (CComBSTR(_T("Right")) == CComBSTR(var.bstrVal))
   {
      JackingEnd = pgsTypes::jeEnd;
   }
   else
   {
      JackingEnd = pgsTypes::jeBoth;
   }

   var.vt = VT_BSTR;
   hr = pStrLoad->get_Property(_T("DuctShape"), &var);
   if (CComBSTR(_T("Linear")) == CComBSTR(var.bstrVal))
   {
      DuctGeometryType = Linear;
   }
   else
   {
      ATLASSERT(CComBSTR(_T("Parabolic")) == CComBSTR(var.bstrVal));
      DuctGeometryType = Parabolic;
   }

   if (DuctGeometryType == Linear)
   {
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Y_Left"), &var);
      DuctPoint[Left].first = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Y_Left_Datum"), &var);
      DuctPoint[Left].second = (CComBSTR(var.bstrVal) == CComBSTR(_T("Top")) ? pgsTypes::TopFace : pgsTypes::BottomFace);

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Y_Right"), &var);
      DuctPoint[Right].first = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Y_Right_Datum"), &var);
      DuctPoint[Right].second = (CComBSTR(var.bstrVal) == CComBSTR(_T("Top")) ? pgsTypes::TopFace : pgsTypes::BottomFace);
   }
   else
   {
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Y_Left"), &var);
      DuctPoint[Left].first = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Y_Left_Datum"), &var);
      DuctPoint[Left].second = (CComBSTR(var.bstrVal) == CComBSTR(_T("Top")) ? pgsTypes::TopFace : pgsTypes::BottomFace);

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Y_Middle"), &var);
      DuctPoint[Middle].first = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Y_Middle_Datum"), &var);
      DuctPoint[Middle].second = (CComBSTR(var.bstrVal) == CComBSTR(_T("Top")) ? pgsTypes::TopFace : pgsTypes::BottomFace);

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Y_Right"), &var);
      DuctPoint[Right].first = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Y_Right_Datum"), &var);
      DuctPoint[Right].second = (CComBSTR(var.bstrVal) == CComBSTR(_T("Top")) ? pgsTypes::TopFace : pgsTypes::BottomFace);
   }

   hr = pStrLoad->EndUnit();
   return S_OK;
}

HRESULT CSegmentDuctData::Save(IStructuredSave* pStrSave, IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("Duct"), 2.0);
   pStrSave->put_Property(_T("Name"), CComVariant(Name.c_str()));
   pStrSave->put_Property(_T("NumStrands"), CComVariant(nStrands));
   pStrSave->put_Property(_T("CalcPj"), CComVariant(bPjCalc ? VARIANT_TRUE : VARIANT_FALSE));
   pStrSave->put_Property(_T("Pj"), CComVariant(Pj));
   pStrSave->put_Property(_T("LastUserPj"), CComVariant(LastUserPj)); // added in version 2
   pStrSave->put_Property(_T("StressingEnd"), CComVariant(JackingEnd == pgsTypes::jeStart ? _T("Left") : (JackingEnd == pgsTypes::jeEnd ? _T("Right") : _T("Both"))));
   pStrSave->put_Property(_T("DuctShape"), (CComVariant(DuctGeometryType == Linear ? _T("Linear") : _T("Parabolic"))));
   if (DuctGeometryType == Linear)
   {
      pStrSave->put_Property(_T("Y_Left"), CComVariant(DuctPoint[Left].first));
      pStrSave->put_Property(_T("Y_Left_Datum"), CComVariant(DuctPoint[Left].second == pgsTypes::TopFace ? _T("Top") : _T("Bottom")));
      pStrSave->put_Property(_T("Y_Right"), CComVariant(DuctPoint[Right].first));
      pStrSave->put_Property(_T("Y_Right_Datum"), CComVariant(DuctPoint[Right].second == pgsTypes::TopFace ? _T("Top") : _T("Bottom")));
   }
   else
   {
      pStrSave->put_Property(_T("Y_Left"), CComVariant(DuctPoint[Left].first));
      pStrSave->put_Property(_T("Y_Left_Datum"), CComVariant(DuctPoint[Left].second == pgsTypes::TopFace ? _T("Top") : _T("Bottom")));
      pStrSave->put_Property(_T("Y_Middle"), CComVariant(DuctPoint[Middle].first));
      pStrSave->put_Property(_T("Y_Middle_Datum"), CComVariant(DuctPoint[Middle].second == pgsTypes::TopFace ? _T("Top") : _T("Bottom")));
      pStrSave->put_Property(_T("Y_Right"), CComVariant(DuctPoint[Right].first));
      pStrSave->put_Property(_T("Y_Right_Datum"), CComVariant(DuctPoint[Right].second == pgsTypes::TopFace ? _T("Top") : _T("Bottom")));
   }
   pStrSave->EndUnit();
   return S_OK;
}

void CSegmentDuctData::MakeCopy(const CSegmentDuctData& rOther)
{
   Name = rOther.Name;
   pDuctLibEntry = rOther.pDuctLibEntry;

   nStrands = rOther.nStrands;
   bPjCalc = rOther.bPjCalc;
   Pj = rOther.Pj;
   LastUserPj = rOther.LastUserPj;

   DuctGeometryType = rOther.DuctGeometryType;
   DuctPoint = rOther.DuctPoint;

   JackingEnd = rOther.JackingEnd;
}

void CSegmentDuctData::MakeAssignment(const CSegmentDuctData& rOther)
{
   MakeCopy(rOther);
}

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

CSegmentPTData::CSegmentPTData()
{
   m_pSegment = nullptr;
   m_pStrand = WBFL::LRFD::StrandPool::GetInstance()->GetStrand(WBFL::Materials::PsStrand::Grade::Gr1860, 
                                                        WBFL::Materials::PsStrand::Type::LowRelaxation, 
                                                        WBFL::Materials::PsStrand::Coating::None, 
                                                        WBFL::Materials::PsStrand::Size::D1524);

   DuctType = pgsTypes::dtMetal;
   InstallationType = pgsTypes::sitPush;
   InstallationEvent = pgsTypes::sptetRelease;
}

CSegmentPTData::CSegmentPTData(const CSegmentPTData& other)
{
   m_pSegment = nullptr;
   MakeCopy(other);
}

CSegmentPTData::~CSegmentPTData()
{
}

CSegmentPTData& CSegmentPTData::operator = (const CSegmentPTData& rOther)
{
   if (this != &rOther)
   {
      MakeAssignment(rOther);
   }
   return *this;
}

bool CSegmentPTData::operator==(const CSegmentPTData& rOther) const
{
   if (DuctType != rOther.DuctType)
   {
      return false;
   }

   if (InstallationType != rOther.InstallationType)
   {
      return false;
   }
   
   if (InstallationEvent != rOther.InstallationEvent)
   {
      return false;
   }

   if (m_pStrand != rOther.m_pStrand)
   {
      return false;
   }

   if (m_Ducts != rOther.m_Ducts)
   {
      return false;
   }

   return true;
}

bool CSegmentPTData::operator!=(const CSegmentPTData& rOther) const
{
   return !operator==(rOther);
}

void CSegmentPTData::SetSegment(CPrecastSegmentData* pSegment)
{
   m_pSegment = pSegment;
   for (auto& duct : m_Ducts)
   {
      duct.SetSegment(pSegment);
   }
}

CPrecastSegmentData* CSegmentPTData::GetSegment()
{
   return m_pSegment;
}

const CPrecastSegmentData* CSegmentPTData::GetSegment() const
{
   return m_pSegment;
}

void CSegmentPTData::AddDuct(CSegmentDuctData& duct)
{
   duct.m_pPTData = this;
   duct.Init(m_pSegment);
   m_Ducts.push_back(duct);
}

DuctIndexType CSegmentPTData::GetDuctCount() const
{
   return m_Ducts.size();
}

CSegmentDuctData* CSegmentPTData::GetDuct(DuctIndexType ductIdx)
{
   return &m_Ducts[ductIdx];
}

const CSegmentDuctData* CSegmentPTData::GetDuct(DuctIndexType ductIdx) const
{
   return &m_Ducts[ductIdx];
}

void CSegmentPTData::SetDucts(const std::vector<CSegmentDuctData>& ducts)
{
   m_Ducts = ducts;
}

const std::vector<CSegmentDuctData>& CSegmentPTData::GetDucts() const
{
   return m_Ducts;
}

void CSegmentPTData::RemoveDuct(DuctIndexType ductIdx)
{
   m_Ducts.erase(m_Ducts.begin() + ductIdx);
}

void CSegmentPTData::RemoveDucts()
{
   m_Ducts.clear();
}

StrandIndexType CSegmentPTData::GetStrandCount(DuctIndexType ductIdx) const
{
   return m_Ducts[ductIdx].nStrands;
}

Float64 CSegmentPTData::GetPjack(DuctIndexType ductIdx) const
{
   if (m_Ducts[ductIdx].bPjCalc)
   {
      return m_Ducts[ductIdx].Pj;
   }
   else
   {
      return m_Ducts[ductIdx].LastUserPj;
   }
}

HRESULT CSegmentPTData::Load(IStructuredLoad* pStrLoad, IProgress* pProgress)
{
   ATLASSERT(m_pSegment != nullptr);
   CHRException hr;
   CComVariant var;

   hr = pStrLoad->BeginUnit(_T("SegmentPTData"));

   var.vt = VT_I4;
   hr = pStrLoad->get_Property(_T("DuctType"), &var);
   DuctType = (pgsTypes::DuctType)(var.lVal);

   hr = pStrLoad->get_Property(_T("InstallationType"), &var);
   InstallationType = (pgsTypes::StrandInstallationType)(var.lVal);

   hr = pStrLoad->get_Property(_T("InstallationEvent"), &var);
   InstallationEvent = (pgsTypes::SegmentPTEventType)(var.lVal);

   hr = pStrLoad->get_Property(_T("TendonMaterialKey"), &var);
   const auto* pPool = WBFL::LRFD::StrandPool::GetInstance();
   Int32 key = var.lVal;
   m_pStrand = pPool->GetStrand(key);

   var.vt = VT_INDEX;
   hr = pStrLoad->get_Property(_T("DuctCount"), &var);
   DuctIndexType nDucts = VARIANT2INDEX(var);
   m_Ducts.clear();
   for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
   {
      CSegmentDuctData duct(m_pSegment);
      hr = duct.Load(pStrLoad, pProgress);
      m_Ducts.push_back(duct);
   }


   hr = pStrLoad->EndUnit();
   return S_OK;
}

HRESULT CSegmentPTData::Save(IStructuredSave* pStrSave, IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("SegmentPTData"), 1.0);
   pStrSave->put_Property(_T("DuctType"), CComVariant(DuctType));
   pStrSave->put_Property(_T("InstallationType"), CComVariant(InstallationType));
   pStrSave->put_Property(_T("InstallationEvent"), CComVariant(InstallationEvent));
   const auto* pPool = WBFL::LRFD::StrandPool::GetInstance();

   auto key = pPool->GetStrandKey(m_pStrand);
   pStrSave->put_Property(_T("TendonMaterialKey"), CComVariant(key));

   pStrSave->put_Property(_T("DuctCount"), CComVariant(m_Ducts.size()));
   for (auto& duct : m_Ducts)
   {
      duct.Save(pStrSave, pProgress);
   }

   pStrSave->EndUnit();
   return S_OK;
}

void CSegmentPTData::MakeCopy(const CSegmentPTData& rOther)
{
   DuctType = rOther.DuctType;
   InstallationType = rOther.InstallationType;
   InstallationEvent = rOther.InstallationEvent;

   m_Ducts = rOther.m_Ducts;

   m_pStrand = rOther.m_pStrand;
}

void CSegmentPTData::MakeAssignment(const CSegmentPTData& rOther)
{
   MakeCopy(rOther);
}

#if defined _DEBUG
void CSegmentPTData::AssertValid()
{

}
#endif
