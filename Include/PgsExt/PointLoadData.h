///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
// PointLoadData.h: interface for the CPointLoadData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_POINTLOADDATA_H__9A3E66DC_E7F7_494B_A4FC_CE1C68668647__INCLUDED_)
#define AFX_POINTLOADDATA_H__9A3E66DC_E7F7_494B_A4FC_CE1C68668647__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

struct IStructuredSave;
struct IStructuredLoad;


#include <PgsExt\PgsExtExp.h>

// namespace to contain user load enums
namespace UserLoads
{
   enum Stage{BridgeSite1, BridgeSite2, BridgeSite3};
   enum LoadCase{DC, DW, LL_IM};
   enum DistributedLoadType {Uniform, Trapezoidal};

   static Int32 GetNumStages() // only for DC and DW
   {
      return 2;
   }

   static Stage GetStage(Int32 stagenum)
   {
      switch(stagenum)
      {
      case BridgeSite1:
         return BridgeSite1;
         break;
      case BridgeSite2:
         return BridgeSite2;
         break;
      case BridgeSite3:
         return BridgeSite3;
         break;
      default:
         CHECK(0);
         return BridgeSite1;
      }
   }

   static std::_tstring GetStageName(Int32 stagenum)
   {
      switch(stagenum)
      {
      case BridgeSite1:
         return std::_tstring(_T("Bridge Site 1"));
         break;
      case BridgeSite2:
         return std::_tstring(_T("Bridge Site 2"));
         break;
      case BridgeSite3:
         return std::_tstring(_T("Bridge Site 3"));
         break;
      default:
         CHECK(0);
         return std::_tstring(_T("Error"));
      }
   }

   static Int32 GetNumLoadCases()
   {
      return 3;
   }

   static LoadCase GetLoadCase(Int32 load_casenum)
   {
      switch(load_casenum)
      {
      case DC:
         return DC;
         break;
      case DW:
         return DW;
         break;
      case LL_IM:
         return LL_IM;
         break;
      default:
         CHECK(0);
         return DC;
      }
   }

   static std::_tstring GetLoadCaseName(Int32 load_casenum)
   {
      switch(load_casenum)
      {
      case DC:
         return std::_tstring(_T("DC"));
         break;
      case DW:
         return std::_tstring(_T("DW"));
         break;
      case LL_IM:
         return std::_tstring(_T("LL + IM"));
         break;
      default:
         CHECK(0);
         return std::_tstring(_T("Error"));
      }
   }

   static int GetNumDistributedLoadTypes()
   {
      return 2;
   }

   static DistributedLoadType GetDistributedLoadType(Int32 num)
   {
      switch(num)
      {
         case Uniform:
            return Uniform;
            break;
         case Trapezoidal:
            return Trapezoidal;
            break;
         default:
            CHECK(0);
            return Trapezoidal;
      }
   }

   static std::_tstring GetDistributedLoadTypeName(Int32 num)
   {
      switch(num)
      {
         case Uniform:
            return std::_tstring(_T("Uniform over entire span"));
            break;
         case Trapezoidal:
            return std::_tstring(_T("Trapezoidal"));
            break;
         default:
            CHECK(0);
            return std::_tstring(_T("Error"));
      }
   }

};

class PGSEXTCLASS CPointLoadData  
{
public:

	CPointLoadData();
	virtual ~CPointLoadData();

   HRESULT Save(IStructuredSave* pSave);
   HRESULT Load(IStructuredLoad* pSave);

   bool operator == (const CPointLoadData& rOther) const;
   bool operator != (const CPointLoadData& rOther) const;

   // properties
   UserLoads::Stage               m_Stage;
   UserLoads::LoadCase            m_LoadCase;

   SpanIndexType    m_Span;      // set to AllSpans if all
   GirderIndexType    m_Girder;    // set to AllGirders if all
   Float64  m_Location;   // cannot be negative
   bool     m_Fractional;
   Float64  m_Magnitude;
   std::_tstring m_Description;
};

#endif // !defined(AFX_POINTLOADDATA_H__9A3E66DC_E7F7_494B_A4FC_CE1C68668647__INCLUDED_)
