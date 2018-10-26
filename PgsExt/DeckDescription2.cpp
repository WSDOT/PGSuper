///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <PgsExt\DeckDescription2.h>
#include <PgsExt\BridgeDescription2.h>
#include <Units\SysUnits.h>
#include <StdIo.h>

#include <IFace\Tools.h>
#include <IFace\Project.h>

#include <WbflAtlExt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDeckDescription2
****************************************************************************/


CDeckDescription2::CDeckDescription2()
{
   DeckType               = pgsTypes::sdtCompositeCIP;
   TransverseConnectivity = pgsTypes::atcConnectedAsUnit; // only applicable if girder spacing is adjacent

   GrossDepth       = ::ConvertToSysUnits(  8.5, unitMeasure::Inch );
   Fillet           = ::ConvertToSysUnits( 0.75, unitMeasure::Inch );

   Concrete.Type             = pgsTypes::Normal;
   Concrete.bHasInitial      = false;
   Concrete.Fci              = 0; // not using
   Concrete.Eci              = 0; // not using
   Concrete.bUserEci         = false; // not using
   Concrete.Fc               = ::ConvertToSysUnits(4.,unitMeasure::KSI);
   Concrete.StrengthDensity  = ::ConvertToSysUnits(160.,unitMeasure::LbfPerFeet3);
   Concrete.WeightDensity    = ::ConvertToSysUnits(160.,unitMeasure::LbfPerFeet3);
   Concrete.MaxAggregateSize = ::ConvertToSysUnits(0.75,unitMeasure::Inch);
   Concrete.EcK1             = 1.0;
   Concrete.EcK2             = 1.0;
   Concrete.CreepK1          = 1.0;
   Concrete.CreepK2          = 1.0;
   Concrete.ShrinkageK1      = 1.0;
   Concrete.ShrinkageK2      = 1.0;
   Concrete.bUserEc          = false;
   Concrete.Ec               = ::ConvertToSysUnits(4200.,unitMeasure::KSI);
   Concrete.bHasFct          = false;
   Concrete.Fct              = 0.0;

   Concrete.bACIUserParameters = false;
   Concrete.A                  = ::ConvertToSysUnits(4.0,unitMeasure::Day);
   Concrete.B                  = 0.85;
   Concrete.CureMethod         = pgsTypes::Moist;
   Concrete.CementType         = pgsTypes::TypeI;

   WearingSurface = pgsTypes::wstSacrificialDepth;
   bInputAsDepthAndDensity = false;
   OverlayWeight  = ::ConvertToSysUnits( 25.0, unitMeasure::PSF );
   OverlayDensity = 0;
   OverlayDepth = 0;
   SacrificialDepth = ::ConvertToSysUnits(  0.5, unitMeasure::Inch );
   PanelDepth       = ::ConvertToSysUnits(  0.0, unitMeasure::Inch );
   PanelSupport     = ::ConvertToSysUnits(  4.0, unitMeasure::Inch );
                         // for horizontal shear capacity)

   OverhangTaper = pgsTypes::dotTopTopFlange;
   OverhangEdgeDepth = ::ConvertToSysUnits( 7.0, unitMeasure::Inch );

   Condition = pgsTypes::cfGood;
   ConditionFactor = 1.0;

   m_pBridgeDesc = NULL;
}

CDeckDescription2::CDeckDescription2(const CDeckDescription2& rOther)
{
   MakeCopy(rOther,true/*copy data only*/);
}

CDeckDescription2::~CDeckDescription2()
{
}

CDeckDescription2& CDeckDescription2::operator= (const CDeckDescription2& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CDeckDescription2::CopyDeckData(const CDeckDescription2* pDeck)
{
   MakeCopy(*pDeck,true/*copy data only*/);
}

bool CDeckDescription2::operator == (const CDeckDescription2& rOther) const
{
   if ( DeckType != rOther.DeckType )
   {
      return false;
   }

   if ( TransverseConnectivity != rOther.TransverseConnectivity )
   {
      return false;
   }

   if ( !IsEqual( GrossDepth, rOther.GrossDepth ) )
   {
      return false;
   }

   if ( OverhangTaper != rOther.OverhangTaper )
   {
      return false;
   }

   if ( !IsEqual( OverhangEdgeDepth, rOther.OverhangEdgeDepth ) )
   {
      return false;
   }

   if ( !IsEqual( Fillet, rOther.Fillet ) )
   {
      return false;
   }

   if ( Concrete != rOther.Concrete )
   {
      return false;
   }

   if ( !IsEqual( OverlayWeight, rOther.OverlayWeight ) )
   {
      return false;
   }

	if ( !IsEqual( SacrificialDepth, rOther.SacrificialDepth ) )
   {
      return false;
   }

   if ( PanelDepth != rOther.PanelDepth )
   {
      return false;
   }

   if ( PanelSupport != rOther.PanelSupport )
   {
      return false;
   }

   if ( DeckRebarData != rOther.DeckRebarData )
   {
      return false;
   }

   if ( WearingSurface != rOther.WearingSurface )
   {
      return false;
   }

   if ( DeckEdgePoints != rOther.DeckEdgePoints )
   {
      return false;
   }

   if ( Condition != rOther.Condition )
   {
      return false;
   }

   if ( !IsEqual(ConditionFactor,rOther.ConditionFactor) )
   {
      return false;
   }

   return true;
}

bool CDeckDescription2::operator != (const CDeckDescription2& rOther) const
{
   return !operator==( rOther );
}

// this global and free function are used to clean up neg moment rebar data
// during load. See information at bottom of Load method
PierIndexType g_NumPiers2;
bool MaxPierIdx2(CDeckRebarData::NegMomentRebarData& rebarData)
{
   return g_NumPiers2 <= rebarData.PierIdx;
}

HRESULT CDeckDescription2::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

  CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("Deck"));
      Float64 version;
      hr = pStrLoad->get_Version(&version);

      CComVariant var;

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("SlabType"), &var );
      DeckType = (pgsTypes::SupportedDeckType)(int)var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("TransverseConnectivity"), &var );
      TransverseConnectivity = (pgsTypes::AdjacentTransverseConnectivity)(int)var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("GrossDepth"), &var );
      GrossDepth = var.dblVal;

      var.vt = VT_INDEX;
      pStrLoad->get_Property(_T("DeckEdgePointCount"),&var);
      CollectionIndexType nPoints = VARIANT2INDEX(var);

      if ( 0 < nPoints )
      {
         pStrLoad->BeginUnit(_T("DeckEdgePoints"));
         for ( CollectionIndexType i = 0;i < nPoints; i++ )
         {
            CDeckPoint point;
            point.Load(pStrLoad,pProgress);
            DeckEdgePoints.push_back(point);
         }
         pStrLoad->EndUnit();
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("OverhangEdgeDepth"),&var);
      OverhangEdgeDepth = var.dblVal;

      var.Clear();
      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("OverhangTaperType"),&var);
      OverhangTaper = (pgsTypes::DeckOverhangTaper)(var.lVal);

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Fillet"), &var );
      Fillet = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("PanelDepth"),&var );
      PanelDepth = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("PanelSupport"),&var );
      PanelSupport = var.dblVal;

      var.Clear();
      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("WearingSurfaceType"),&var);
      WearingSurface = (pgsTypes::WearingSurfaceType)(var.lVal);

      var.Clear();
      var.vt = VT_BOOL;
      pStrLoad->get_Property(_T("InputAsDepthAndDensity"),&var);
      bInputAsDepthAndDensity = (var.boolVal == VARIANT_TRUE ? true : false);

      if ( bInputAsDepthAndDensity )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("OverlayDepth"), &var );
         OverlayDepth = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("OverlayDensity"), &var );
         OverlayDensity = var.dblVal;

         Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();
         OverlayWeight = OverlayDepth*OverlayDensity*g;
      }
      else
      {
         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("OverlayWeight"),&var); // added in version 3.0
         OverlayWeight = var.dblVal;
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("SacrificialDepth"), &var );
      SacrificialDepth = var.dblVal;

      // there was a bug in the PGSuper interface that allowed the sacrifical depth to
      // be greater than the gross/cast depth of the slab. Obviously this is incorrect.
      // If this is encountered in the input, fix it.
      if ( DeckType != pgsTypes::sdtNone && GrossDepth <= SacrificialDepth )
      {
         SacrificialDepth = GrossDepth/2;
      }

      Concrete.Load(pStrLoad,pProgress);

      pStrLoad->BeginUnit(_T("Condition"));
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("ConditionFactorType"),&var);
      Condition = (pgsTypes::ConditionFactorType)(var.lVal);

      var.vt = VT_R8;
      pStrLoad->get_Property(_T("ConditionFactor"),&var);
      ConditionFactor = var.dblVal;
   
      pStrLoad->EndUnit();

      DeckRebarData.Load(pStrLoad,pProgress);

      // in some 2.1 Beta versions we stored deck edge points for NoDeck and CompositeOverlay decks
      // these decks should have any points so clear them out right now.
      if ( IsConstantWidthDeck(DeckType) )
      {
         DeckEdgePoints.clear();
      }

      hr = pStrLoad->EndUnit();
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   // Not sure how this happened, but at least on regression test file (UserSelect.pgs) has negative
   // moment rebar data for a pier that doesn't exist. If it happened for that file, it is likely
   // to have happened for other files as well. When this data is encountered in other places in 
   // the software it causes a crash. This code block removes deck rebar data at piers that 
   // don't exist.
   g_NumPiers2 = m_pBridgeDesc->GetPierCount();
   std::vector<CDeckRebarData::NegMomentRebarData>::iterator new_end = std::remove_if(DeckRebarData.NegMomentRebar.begin(),DeckRebarData.NegMomentRebar.end(),MaxPierIdx2);
   DeckRebarData.NegMomentRebar.erase(new_end,DeckRebarData.NegMomentRebar.end());

   return hr;
}

HRESULT CDeckDescription2::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("Deck"),1.0);

   pStrSave->put_Property(_T("SlabType"),         CComVariant(DeckType));
   pStrSave->put_Property(_T("TransverseConnectivity"), CComVariant(TransverseConnectivity)); // added for version 14.0
   pStrSave->put_Property(_T("GrossDepth"),       CComVariant(GrossDepth));

   pStrSave->put_Property(_T("DeckEdgePointCount"),CComVariant((long)DeckEdgePoints.size()));

   if ( 0 < DeckEdgePoints.size() )
   {
      pStrSave->BeginUnit(_T("DeckEdgePoints"),1.0);
      std::vector<CDeckPoint>::iterator iter(DeckEdgePoints.begin());
      std::vector<CDeckPoint>::iterator end(DeckEdgePoints.end());
      for ( ; iter != end; iter++ )
      {
         CDeckPoint& point = *iter;
         point.Save(pStrSave,pProgress);
      }
      pStrSave->EndUnit();
   }

   pStrSave->put_Property(_T("OverhangEdgeDepth"),CComVariant(OverhangEdgeDepth));
   pStrSave->put_Property(_T("OverhangTaperType"),CComVariant(OverhangTaper));
   pStrSave->put_Property(_T("Fillet"),           CComVariant(Fillet));
   pStrSave->put_Property(_T("PanelDepth"),       CComVariant(PanelDepth));
   pStrSave->put_Property(_T("PanelSupport"),     CComVariant(PanelSupport));
   pStrSave->put_Property(_T("WearingSurfaceType"), CComVariant(WearingSurface));

   pStrSave->put_Property(_T("InputAsDepthAndDensity"),CComVariant(bInputAsDepthAndDensity));
   if ( bInputAsDepthAndDensity )
   {
      pStrSave->put_Property(_T("OverlayDepth"),     CComVariant(OverlayDepth));
      pStrSave->put_Property(_T("OverlayDensity"),   CComVariant(OverlayDensity));
   }
   else
   {
      pStrSave->put_Property(_T("OverlayWeight"),CComVariant(OverlayWeight));
   }

   pStrSave->put_Property(_T("SacrificialDepth"), CComVariant(SacrificialDepth));

   Concrete.Save(pStrSave,pProgress);

   pStrSave->BeginUnit(_T("Condition"),1.0);
   pStrSave->put_Property(_T("ConditionFactorType"),CComVariant(Condition));
   pStrSave->put_Property(_T("ConditionFactor"),CComVariant(ConditionFactor));
   pStrSave->EndUnit(); // Condition

   DeckRebarData.Save(pStrSave,pProgress);

   pStrSave->EndUnit();

   return hr;
}


void CDeckDescription2::SetBridgeDescription(const CBridgeDescription2* pBridge)
{
   m_pBridgeDesc = pBridge;
}

const CBridgeDescription2* CDeckDescription2::GetBridgeDescription() const
{
   return m_pBridgeDesc;
}

void CDeckDescription2::MakeCopy(const CDeckDescription2& rOther,bool bCopyDataOnly)
{
   if ( !bCopyDataOnly )
   {
      // If deck had an ID or Index, it would be copied here
   }

   DeckType                = rOther.DeckType;
   TransverseConnectivity  = rOther.TransverseConnectivity;

   GrossDepth              = rOther.GrossDepth;
   OverhangTaper           = rOther.OverhangTaper;
   OverhangEdgeDepth       = rOther.OverhangEdgeDepth;
   Fillet                  = rOther.Fillet;
	OverlayWeight           = rOther.OverlayWeight;
   OverlayDensity          = rOther.OverlayDensity;
   OverlayDepth            = rOther.OverlayDepth;
   bInputAsDepthAndDensity = rOther.bInputAsDepthAndDensity;
	SacrificialDepth        = rOther.SacrificialDepth;
   WearingSurface          = rOther.WearingSurface;

   Concrete                = rOther.Concrete;

   PanelDepth              = rOther.PanelDepth;
   PanelSupport            = rOther.PanelSupport;

   Condition               = rOther.Condition;
   ConditionFactor         = rOther.ConditionFactor;

   DeckRebarData           = rOther.DeckRebarData;
   DeckEdgePoints          = rOther.DeckEdgePoints;
}

void CDeckDescription2::MakeAssignment(const CDeckDescription2& rOther)
{
   MakeCopy( rOther, false /*copy everything*/ );
}
