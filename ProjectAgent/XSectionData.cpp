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

/****************************************************************************
CLASS
   CXSectionData
****************************************************************************/

#include "stdafx.h"
#include "ProjectAgent.h"
#include "XSectionData.h"
#include <Units\SysUnits.h>
#include <StdIo.h>

#include <IFace\Tools.h>
#include <IFace\Project.h>
#include <PsgLib\ConcreteLibraryEntry.h>

#include <WbflAtlExt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CXSectionData::CXSectionData()
{
   GdrSpacing = ::ConvertToSysUnits( 5.0, unitMeasure::Feet );
   GdrSpacingMeasurement = Normal;
   GdrLineCount = 5;
   Girder = _T("");
   LeftTrafficBarrier = _T("");
   RightTrafficBarrier = _T("");
   GirderOrientation = pgsTypes::Plumb;

   DeckType               = pgsTypes::sdtCompositeCIP;
   TransverseConnectivity = pgsTypes::atcConnectedAsUnit; // only applicable if girder spacing is adjacent

   GrossDepth       = ::ConvertToSysUnits(  8.5, unitMeasure::Inch );
   LeftOverhang     = ::ConvertToSysUnits(  1.5, unitMeasure::Feet );
   RightOverhang    = ::ConvertToSysUnits(  1.5, unitMeasure::Feet );
   SlabOffset       = ::ConvertToSysUnits( 10.0, unitMeasure::Inch );
   Fillet           = ::ConvertToSysUnits( 0.75, unitMeasure::Inch );

   SlabFc               = ::ConvertToSysUnits(4.,unitMeasure::KSI);
   SlabStrengthDensity  = ::ConvertToSysUnits(160.,unitMeasure::LbfPerFeet3);
   SlabWeightDensity    = ::ConvertToSysUnits(160.,unitMeasure::LbfPerFeet3);
   SlabMaxAggregateSize = ::ConvertToSysUnits(0.75,unitMeasure::Inch);
   SlabEcK1             = 1.0;
   SlabEcK2             = 1.0;
   SlabCreepK1          = 1.0;
   SlabCreepK2          = 1.0;
   SlabShrinkageK1      = 1.0;
   SlabShrinkageK2      = 1.0;
   SlabUserEc           = false;
   SlabEc               = ::ConvertToSysUnits(4200.,unitMeasure::KSI);

   WearingSurface = pgsTypes::wstSacrificialDepth;
   OverlayWeight    = ::ConvertToSysUnits( 25.0, unitMeasure::PSF );
   SacrificialDepth = ::ConvertToSysUnits(  0.5, unitMeasure::Inch );
   PanelDepth       = ::ConvertToSysUnits(  0.0, unitMeasure::Inch );
   PanelSupport     = ::ConvertToSysUnits(  4.0, unitMeasure::Inch );
                         // for horizontal shear capacity)

   OverhangTaper = pgsTypes::TopTopFlange;
   OverhangEdgeDepth = ::ConvertToSysUnits( 7.0, unitMeasure::Inch );

   pLeftTrafficBarrierEntry = 0;
   pRightTrafficBarrierEntry = 0;
   pGirderEntry = 0;
}

CXSectionData::CXSectionData(const CXSectionData& rOther)
{
   MakeCopy(rOther);
}

CXSectionData::~CXSectionData()
{
}

//======================== OPERATORS  =======================================
CXSectionData& CXSectionData::operator= (const CXSectionData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CXSectionData::operator == (const CXSectionData& rOther) const
{
   if ( DeckType == rOther.DeckType )
      return false;

   if ( TransverseConnectivity == rOther.TransverseConnectivity )
      return false;

   if ( !IsEqual( GdrSpacing, rOther.GdrSpacing ) )
      return false;

   if ( GdrSpacingMeasurement == rOther.GdrSpacingMeasurement )
      return false;

   if ( GdrLineCount != rOther.GdrLineCount )
      return false;

   if ( Girder != rOther.Girder )
      return false;

   if ( GirderOrientation != rOther.GirderOrientation )
      return false;

   if ( LeftTrafficBarrier != rOther.LeftTrafficBarrier )
      return false;

   if ( RightTrafficBarrier != rOther.RightTrafficBarrier )
      return false;

   if ( !IsEqual( GrossDepth, rOther.GrossDepth ) )
      return false;

   if ( !IsEqual( LeftOverhang, rOther.LeftOverhang ) )
      return false;

   if ( !IsEqual( RightOverhang, rOther.RightOverhang ) )
      return false;

   if ( !IsEqual( SlabOffset, rOther.SlabOffset ) )
      return false;

   if ( !IsEqual( Fillet, rOther.Fillet ) )
      return false;

   if ( SlabFc != rOther.SlabFc )
      return false;

   if ( !IsEqual( SlabWeightDensity, rOther.SlabWeightDensity ) )
      return false;

   if ( SlabStrengthDensity != rOther.SlabStrengthDensity )
      return false;

   if ( SlabMaxAggregateSize != rOther.SlabMaxAggregateSize )
      return false;

   if ( SlabEcK1 != rOther.SlabEcK1 )
      return false;

   if ( SlabEcK2 != rOther.SlabEcK2 )
      return false;

   if ( SlabCreepK1 != rOther.SlabCreepK1 )
      return false;

   if ( SlabCreepK2 != rOther.SlabCreepK2 )
      return false;

   if ( SlabShrinkageK1 != rOther.SlabShrinkageK1 )
      return false;

   if ( SlabShrinkageK2 != rOther.SlabShrinkageK2 )
      return false;

   if ( SlabUserEc != rOther.SlabUserEc )
      return false;

   if ( !IsEqual(SlabEc,rOther.SlabEc) )
      return false;

   if ( !IsEqual( OverlayWeight, rOther.OverlayWeight ) )
      return false;

	if ( !IsEqual( SacrificialDepth, rOther.SacrificialDepth ) )
      return false;

   if ( PanelDepth != rOther.PanelDepth )
      return false;

   if ( PanelSupport != rOther.PanelSupport )
      return false;

   if ( DeckRebarData != rOther.DeckRebarData )
      return false;

   if ( WearingSurface != rOther.WearingSurface )
      return false;

   return true;
}

bool CXSectionData::operator != (const CXSectionData& rOther) const
{
   return !operator==( rOther );
}

//======================== OPERATIONS =======================================
HRESULT CXSectionData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress, ILibrary* pLibrary)
{
   USES_CONVERSION;

  CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("XSectionData"));
      Float64 version;
      hr = pStrLoad->get_Version(&version);

      CComVariant var;

      if ( version >= 3.0 )
      {
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("SlabType"), &var );
         DeckType = (pgsTypes::SupportedDeckType)(var.lVal);
      }

      if ( version >= 12.0 )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("TransverseConnectivity"), &var );
         TransverseConnectivity = (pgsTypes::AdjacentTransverseConnectivity)(int)var.dblVal;
      }

      if ( version < 14.0 )
      {
         // had connectivity saved in version 12, but wrong default
         // use deck type from previous version to determine connectivity
         if ( DeckType==pgsTypes::sdtNone )
         {
            TransverseConnectivity = pgsTypes::atcConnectedRelativeDisplacement;
         }
         else
         {
            TransverseConnectivity = pgsTypes::atcConnectedAsUnit;
         }
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("GdrSpacing"), &var );
      GdrSpacing = var.dblVal;

      if ( 10 < version )
      {
         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("GdrSpacingMeasurement"), &var );
         GdrSpacingMeasurement = (MeasurementType)(var.lVal);
      }

      var.Clear();
      var.vt = VT_I2;
      hr = pStrLoad->get_Property(_T("GdrLineCount"), &var );
      GdrLineCount = var.iVal;

      var.Clear();
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Girder"), &var );
      Girder = OLE2T(var.bstrVal);

      if ( version < 6.1 )
      {
         GirderOrientation = pgsTypes::Plumb;
      }
      else
      {
         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("GirderOrientation"),&var);
         GirderOrientation = (pgsTypes::GirderOrientationType)var.lVal;
      }

      if ( version < 15 )
      {
         var.Clear();
         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("TrafficBarrier"), &var);
         LeftTrafficBarrier = OLE2T(var.bstrVal);
         RightTrafficBarrier = LeftTrafficBarrier;
      }
      else
      {
         var.Clear();
         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("LeftTrafficBarrier"), &var);
         LeftTrafficBarrier = OLE2T(var.bstrVal);

         var.Clear();
         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("RightTrafficBarrier"), &var);
         RightTrafficBarrier = OLE2T(var.bstrVal);
      }

      if (version < 4.0)
      {
         // Name of a library entry that specified the girder concrete used to be stored here.
         // Now concrete information is stored explicitely in the GirderData struct. 
         // We need to put it into a _T("safe") location so it can be retrieved later by GirderData.
         var.Clear();
         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("GdrConcrete"), &var );
         m_strGirderConcreteName = OLE2T(var.bstrVal);
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("GrossDepth"), &var );
      GrossDepth = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      if ( version < 6 )
      {
         hr = pStrLoad->get_Property(_T("Overhang"), &var );
         LeftOverhang = var.dblVal;
         RightOverhang = var.dblVal;
      }
      else
      {
         hr = pStrLoad->get_Property(_T("LeftOverhang"), &var );
         LeftOverhang = var.dblVal;

         hr = pStrLoad->get_Property(_T("RightOverhang"), &var );
         RightOverhang = var.dblVal;
      }

      if ( 10 < version )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("OverhangEdgeDepth"),&var);
         OverhangEdgeDepth = var.dblVal;

         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("OverhangTaperType"),&var);
         OverhangTaper = (pgsTypes::DeckOverhangTaper)(var.lVal);
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Fillet"), &var );
      Fillet = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("SlabOffset"), &var );
      SlabOffset = var.dblVal;

      if ( version >= 3.0 )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("PanelDepth"),&var );
         PanelDepth = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("PanelSupport"),&var );
         PanelSupport = var.dblVal;
      }

      if ( version < 7 )
      {
         WearingSurface = pgsTypes::wstSacrificialDepth;
      }
      else if ( version < 13.0 )
      {
         var.Clear();
         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property(_T("FutureOverlay"),&var);
         WearingSurface = (var.boolVal == VARIANT_TRUE ? pgsTypes::wstFutureOverlay : 
                  (DeckType == pgsTypes::sdtNone ? pgsTypes::wstOverlay : pgsTypes::wstSacrificialDepth));
      }
      else
      {
         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("WearingSurfaceType"),&var);
         WearingSurface = (pgsTypes::WearingSurfaceType)(var.lVal);

         // if deck type is none, wearing surface cannot be sacrificial depth
         // fix it if that happens
         if ( DeckType == pgsTypes::sdtNone && WearingSurface == pgsTypes::wstSacrificialDepth )
            WearingSurface = pgsTypes::wstOverlay;
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("OverlayDepth"), &var );
      OverlayDepth = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("OverlayDensity"), &var );
      OverlayDensity = var.dblVal;
      Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();
      OverlayWeight = OverlayDensity*OverlayDepth*g;

      if ( version < 13 )
      {
         // if we don't have a future overlay, but overlay depth and density is non-zero
         // then we have a regular overlay
         if ( WearingSurface == pgsTypes::wstSacrificialDepth && (!IsZero(OverlayDepth) && !IsZero(OverlayDensity)) )
            WearingSurface = pgsTypes::wstOverlay;
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("SacrificialDepth"), &var );
      SacrificialDepth = var.dblVal;

      if ( 4.0 <= version )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SlabFc"), &var );
         SlabFc = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SlabWeightDensity"), &var );
         SlabWeightDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SlabStrengthDensity"), &var );
         SlabStrengthDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SlabMaxAggregateSize"), &var );
         SlabMaxAggregateSize = var.dblVal;
      }
      else
      {
         // Slab concrete used to be stored in a library entry - need to get it out and 
         // make it local
         var.Clear();
         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("SlabConcrete"), &var );
         std::_tstring slab_concrete = OLE2T(var.bstrVal);

         const ConcreteLibraryEntry* entry = pLibrary->GetConcreteEntry(slab_concrete.c_str());

         SlabFc = entry->GetFc();
         SlabWeightDensity = entry->GetWeightDensity();
         SlabStrengthDensity = entry->GetStrengthDensity();
         SlabMaxAggregateSize = entry->GetAggregateSize();
      }

      if ( 5.0 <= version )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SlabK1"), &var );
         SlabEcK1 = var.dblVal;
      }
      else
      {
         SlabEcK1 = 1.0;
      }

      if ( 8.0 <= version )
      {
         var.Clear();
         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property(_T("SlabUserEc"),&var);
         SlabUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SlabEc"), &var );
         SlabEc = var.dblVal;
      }

      if ( version < 9.0 )
      {
         // this data was removed in unit 9.0... just read the data if it exists
         // and gobble it up.
         var.Clear();
         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("DiaphragmLayout"), &var );
         //DiaphragmLayout = OLE2T(var.bstrVal);
      }

      if ( 10.0 <= version )
      {
         DeckRebarData.Load(pStrLoad,pProgress);
      }

      hr = pStrLoad->EndUnit();
   }
   catch(...)
   {
      ATLASSERT(0);
   }

   return hr;
}

HRESULT CXSectionData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;
   ATLASSERT(false); // should never get here

//   pStrSave->BeginUnit(_T("XSectionData"),15.0);
//
//   pStrSave->put_Property(_T("SlabType"),         CComVariant(DeckType));
//   pStrSave->put_Property(_T("TransverseConnectivity"), CComVariant(TransverseConnectivity)); // added for version 14.0
//   pStrSave->put_Property(_T("GdrSpacing"),       CComVariant(GdrSpacing));
//   pStrSave->put_Property(_T("GdrSpacingMeasurement"), CComVariant(GdrSpacingMeasurement));
//   pStrSave->put_Property(_T("GdrLineCount"),     CComVariant(GdrLineCount));
//   pStrSave->put_Property(_T("Girder"),           CComVariant(Girder.c_str()));
//   pStrSave->put_Property(_T("GirderOrientation"),CComVariant(GirderOrientation));
//   pStrSave->put_Property(_T("LeftTrafficBarrier"),   CComVariant(LeftTrafficBarrier.c_str()));
//   pStrSave->put_Property(_T("RightTrafficBarrier"),   CComVariant(RightTrafficBarrier.c_str()));
//   pStrSave->put_Property(_T("GrossDepth"),       CComVariant(GrossDepth));
//   pStrSave->put_Property(_T("LeftOverhang"),     CComVariant(LeftOverhang));
//   pStrSave->put_Property(_T("RightOverhang"),    CComVariant(RightOverhang));
//   pStrSave->put_Property(_T("OverhangEdgeDepth"),CComVariant(OverhangEdgeDepth));
//   pStrSave->put_Property(_T("OverhangTaperType"),CComVariant(OverhangTaper));
//   pStrSave->put_Property(_T("Fillet"),           CComVariant(Fillet));
//   pStrSave->put_Property(_T("SlabOffset"),       CComVariant(SlabOffset));
//   pStrSave->put_Property(_T("PanelDepth"),       CComVariant(PanelDepth));
//   pStrSave->put_Property(_T("PanelSupport"),     CComVariant(PanelSupport));
////   pStrSave->put_Property(_T("FutureOverlay"),    CComVariant(bFutureOverlay)); // version 7, removed version 13
//   pStrSave->put_Property(_T("WearingSurfaceType"), CComVariant(WearingSurface));
//   pStrSave->put_Property(_T("OverlayDepth"),     CComVariant(OverlayDepth));
//   pStrSave->put_Property(_T("OverlayDensity"),   CComVariant(OverlayDensity));
//   pStrSave->put_Property(_T("SacrificialDepth"), CComVariant(SacrificialDepth));
//
//   // the following block changed for version 4.0
////   pStrSave->put_Property(_T("SlabConcrete"),     CComVariant(SlabConcrete.c_str()));
//   pStrSave->put_Property(_T("SlabFc"),               CComVariant(SlabFc));
//   pStrSave->put_Property(_T("SlabWeightDensity"),    CComVariant(SlabWeightDensity));
//   pStrSave->put_Property(_T("SlabStrengthDensity"),  CComVariant(SlabStrengthDensity));
//   pStrSave->put_Property(_T("SlabMaxAggregateSize"), CComVariant(SlabMaxAggregateSize));
//   // added for verseion 5 
//   pStrSave->put_Property(_T("SlabK1"),               CComVariant(SlabK1));
//   // added version 8
//   pStrSave->put_Property(_T("SlabUserEc"), CComVariant(SlabUserEc));
//   pStrSave->put_Property(_T("SlabEc"),     CComVariant(SlabEc));
//
//   // Removed when unit number when to 9.0
////   pStrSave->put_Property(_T("DiaphragmLayout"),  CComVariant(DiaphragmLayout.c_str()));
//
//   // added when unit number when to 10
//   DeckRebarData.Save(pStrSave,pProgress);
//
//   pStrSave->EndUnit();

   return hr;
}
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CXSectionData::MakeCopy(const CXSectionData& rOther)
{
   // Add copy code here...
   DeckType       = rOther.DeckType;
   TransverseConnectivity = rOther.TransverseConnectivity;
   GdrSpacing        = rOther.GdrSpacing;
   GdrSpacingMeasurement = rOther.GdrSpacingMeasurement;
   GdrLineCount      = rOther.GdrLineCount;
   Girder            = rOther.Girder;
   GirderOrientation = rOther.GirderOrientation;
   LeftTrafficBarrier  = rOther.LeftTrafficBarrier;
   RightTrafficBarrier = rOther.RightTrafficBarrier;

   GrossDepth       = rOther.GrossDepth;
   LeftOverhang     = rOther.LeftOverhang;
   RightOverhang    = rOther.RightOverhang;
   OverhangTaper    = rOther.OverhangTaper;
   OverhangEdgeDepth = rOther.OverhangEdgeDepth;
	SlabOffset       = rOther.SlabOffset;
   Fillet           = rOther.Fillet;
	OverlayWeight    = rOther.OverlayWeight;
	SacrificialDepth = rOther.SacrificialDepth;
   WearingSurface   = rOther.WearingSurface;

   SlabFc                = rOther.SlabFc;
   SlabWeightDensity     = rOther.SlabWeightDensity;
   SlabStrengthDensity   = rOther.SlabStrengthDensity;
   SlabMaxAggregateSize  = rOther.SlabMaxAggregateSize;
   SlabEcK1              = rOther.SlabEcK1;
   SlabEcK2              = rOther.SlabEcK2;
   SlabCreepK1           = rOther.SlabCreepK1;
   SlabCreepK2           = rOther.SlabCreepK2;
   SlabShrinkageK1       = rOther.SlabShrinkageK1;
   SlabShrinkageK2       = rOther.SlabShrinkageK2;
   SlabUserEc            = rOther.SlabUserEc;
   SlabEc                = rOther.SlabEc;

   PanelDepth       = rOther.PanelDepth;
   PanelSupport     = rOther.PanelSupport;

   DeckRebarData = rOther.DeckRebarData;

   pLeftTrafficBarrierEntry = rOther.pLeftTrafficBarrierEntry;
   pRightTrafficBarrierEntry = rOther.pRightTrafficBarrierEntry;
   pGirderEntry         = rOther.pGirderEntry;
}

void CXSectionData::MakeAssignment(const CXSectionData& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
