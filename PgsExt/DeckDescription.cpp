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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\DeckDescription.h>
#include <PgsExt\BridgeDescription.h>
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
   CDeckDescription
****************************************************************************/


CDeckDescription::CDeckDescription()
{
   DeckType               = pgsTypes::sdtCompositeCIP;
   TransverseConnectivity = pgsTypes::atcConnectedAsUnit; // only applicable if girder spacing is adjacent

   GrossDepth       = ::ConvertToSysUnits(  8.5, unitMeasure::Inch );
   Fillet           = ::ConvertToSysUnits( 0.75, unitMeasure::Inch );

   SlabConcreteType     = pgsTypes::Normal;
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
   SlabHasFct           = false;
   SlabFct              = 0.0;

   WearingSurface = pgsTypes::wstSacrificialDepth;
   bInputAsDepthAndDensity = false;
   OverlayWeight  = ::ConvertToSysUnits( 25.0, unitMeasure::PSF );
   OverlayDensity = 0;
   OverlayDepth = 0;
   SacrificialDepth = ::ConvertToSysUnits(  0.5, unitMeasure::Inch );
   PanelDepth       = ::ConvertToSysUnits(  0.0, unitMeasure::Inch );
   PanelSupport     = ::ConvertToSysUnits(  4.0, unitMeasure::Inch );
                         // for horizontal shear capacity)

   OverhangTaper = pgsTypes::TopTopFlange;
   OverhangEdgeDepth = ::ConvertToSysUnits( 7.0, unitMeasure::Inch );

   Condition = pgsTypes::cfGood;
   ConditionFactor = 1.0;

   m_pBridgeDesc = NULL;
}

CDeckDescription::CDeckDescription(const CDeckDescription& rOther)
{
   MakeCopy(rOther);
}

CDeckDescription::~CDeckDescription()
{
}

//======================== OPERATORS  =======================================
CDeckDescription& CDeckDescription::operator= (const CDeckDescription& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CDeckDescription::operator == (const CDeckDescription& rOther) const
{
   if ( DeckType != rOther.DeckType )
      return false;

   if ( TransverseConnectivity != rOther.TransverseConnectivity )
      return false;

   if ( !IsEqual( GrossDepth, rOther.GrossDepth ) )
      return false;

   if ( OverhangTaper != rOther.OverhangTaper )
      return false;

   if ( !IsEqual( OverhangEdgeDepth, rOther.OverhangEdgeDepth ) )
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

   if ( DeckEdgePoints != rOther.DeckEdgePoints )
      return false;

   if ( Condition != rOther.Condition )
      return false;

   if ( !IsEqual(ConditionFactor,rOther.ConditionFactor) )
      return false;

   return true;
}

bool CDeckDescription::operator != (const CDeckDescription& rOther) const
{
   return !operator==( rOther );
}

//======================== OPERATIONS =======================================
HRESULT CDeckDescription::Load(IStructuredLoad* pStrLoad,IProgress* pProgress,pgsTypes::SlabOffsetType* pSlabOffsetType,double* pSlabOffset)
{
   USES_CONVERSION;

  CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit("Deck");
      double version;
      hr = pStrLoad->get_Version(&version);

      CComVariant var;

      var.vt = VT_R8;
      hr = pStrLoad->get_Property("SlabType", &var );
      DeckType = (pgsTypes::SupportedDeckType)(int)var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property("TransverseConnectivity", &var );
      TransverseConnectivity = (pgsTypes::AdjacentTransverseConnectivity)(int)var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property("GrossDepth", &var );
      GrossDepth = var.dblVal;

      if ( version < 2 )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("LeftOverhang", &var );
         double LeftOverhang = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("RightOverhang", &var );
         double RightOverhang = var.dblVal;

         // make this into a deck edge point
         long nGirders  = m_pBridgeDesc->GetSpan(0)->GetGirderCount();
         double spacing = m_pBridgeDesc->GetSpan(0)->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing(0);
         CDeckPoint point;
         point.LeftEdge  = LeftOverhang  + (nGirders-1)*spacing/2;
         point.RightEdge = RightOverhang + (nGirders-1)*spacing/2;
         point.Station = 0;
         point.MeasurementType      = pgsTypes::omtBridge;
         point.LeftTransitionType   = pgsTypes::dptLinear;
         point.RightTransitionType  = pgsTypes::dptLinear;
         DeckEdgePoints.push_back(point);
      }
      else
      {
         var.vt = VT_I4;
         pStrLoad->get_Property("DeckEdgePointCount",&var);
         long nPoints = var.lVal;

         if ( 0 < nPoints )
         {
            pStrLoad->BeginUnit("DeckEdgePoints");
            for ( long i = 0;i < nPoints; i++ )
            {
               CDeckPoint point;
               point.Load(pStrLoad,pProgress);
               DeckEdgePoints.push_back(point);
            }
            pStrLoad->EndUnit();
         }
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property("OverhangEdgeDepth",&var);
      OverhangEdgeDepth = var.dblVal;

      var.Clear();
      var.vt = VT_I4;
      hr = pStrLoad->get_Property("OverhangTaperType",&var);
      OverhangTaper = (pgsTypes::DeckOverhangTaper)(var.lVal);

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property("Fillet", &var );
      Fillet = var.dblVal;

      if ( version < 4 )
      {
         // slab offset was moved up to the bridge level with version 4 of this data block
         // if this version is less than 4, read the slab offset and pass it upstream with
         // the supplied pointer. Slab offset is for the entire bridge
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("SlabOffset", &var );
         *pSlabOffset = var.dblVal;
         *pSlabOffsetType = pgsTypes::sotBridge;
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property("PanelDepth",&var );
      PanelDepth = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property("PanelSupport",&var );
      PanelSupport = var.dblVal;

      var.Clear();
      var.vt = VT_I4;
      hr = pStrLoad->get_Property("WearingSurfaceType",&var);
      WearingSurface = (pgsTypes::WearingSurfaceType)(var.lVal);

      if ( version < 3.0 )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("OverlayDepth", &var );
         OverlayDepth = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("OverlayDensity", &var );
         OverlayDensity = var.dblVal;

         Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();
         OverlayWeight = OverlayDepth*OverlayDensity*g;

         bInputAsDepthAndDensity = true;
      }
      else
      {
         var.Clear();
         var.vt = VT_BOOL;
         pStrLoad->get_Property("InputAsDepthAndDensity",&var);
         bInputAsDepthAndDensity = (var.boolVal == VARIANT_TRUE ? true : false);

         if ( bInputAsDepthAndDensity )
         {
            var.Clear();
            var.vt = VT_R8;
            hr = pStrLoad->get_Property("OverlayDepth", &var );
            OverlayDepth = var.dblVal;

            var.Clear();
            var.vt = VT_R8;
            hr = pStrLoad->get_Property("OverlayDensity", &var );
            OverlayDensity = var.dblVal;

            Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();
            OverlayWeight = OverlayDepth*OverlayDensity*g;
         }
         else
         {
            var.Clear();
            var.vt = VT_R8;
            pStrLoad->get_Property("OverlayWeight",&var); // added in version 3.0
            OverlayWeight = var.dblVal;
         }

     }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property("SacrificialDepth", &var );
      SacrificialDepth = var.dblVal;

      // there was a bug in the PGSuper interface that allowed the sacrifical depth to
      // be greater than the gross/cast depth of the slab. Obviously this is incorrect.
      // If this is encountered in the input, fix it.
      if ( GrossDepth <= SacrificialDepth )
         SacrificialDepth = GrossDepth/2;

      if ( version < 6 )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("SlabFc", &var );
         SlabFc = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("SlabWeightDensity", &var );
         SlabWeightDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("SlabStrengthDensity", &var );
         SlabStrengthDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("SlabMaxAggregateSize", &var );
         SlabMaxAggregateSize = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("SlabK1", &var );
         SlabEcK1 = var.dblVal;

         var.Clear();
         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property("SlabUserEc",&var);
         SlabUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("SlabEc", &var );
         SlabEc = var.dblVal;
      }
      else
      {
         hr = pStrLoad->BeginUnit("SlabConcrete");

         var.Clear();
         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property("Type",&var);
         SlabConcreteType = (pgsTypes::ConcreteType)matConcrete::GetTypeFromName(OLE2A(var.bstrVal));

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("Fc", &var );
         SlabFc = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("WeightDensity", &var );
         SlabWeightDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("StrengthDensity", &var );
         SlabStrengthDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("MaxAggregateSize", &var );
         SlabMaxAggregateSize = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("EcK1", &var );
         SlabEcK1 = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("EcK2", &var );
         SlabEcK2 = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("CreepK1", &var );
         SlabCreepK1 = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("CreepK2", &var );
         SlabCreepK2 = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("ShrinkageK1", &var );
         SlabShrinkageK1 = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("ShrinkageK2", &var );
         SlabShrinkageK2 = var.dblVal;

         var.Clear();
         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property("UserEc",&var);
         SlabUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

         if ( SlabUserEc )
         {
            var.Clear();
            var.vt = VT_R8;
            hr = pStrLoad->get_Property("Ec", &var );
            SlabEc = var.dblVal;
         }

         if ( SlabConcreteType != pgsTypes::Normal )
         {
            var.Clear();
            var.vt = VT_BOOL;
            hr = pStrLoad->get_Property("HasFct",&var);
            SlabHasFct = (var.boolVal == VARIANT_TRUE ? true : false);

            if ( SlabHasFct )
            {
               var.Clear();
               var.vt = VT_R8;
               hr = pStrLoad->get_Property("Fct", &var );
               SlabFct = var.dblVal;
            }
         }

         pStrLoad->EndUnit(); // SlabConcrete
      } 

      if ( 4 < version )
      {
         pStrLoad->BeginUnit("Condition");
         var.vt = VT_I4;
         pStrLoad->get_Property("ConditionFactorType",&var);
         Condition = (pgsTypes::ConditionFactorType)(var.lVal);

         var.vt = VT_R8;
         pStrLoad->get_Property("ConditionFactor",&var);
         ConditionFactor = var.dblVal;
      
         pStrLoad->EndUnit();
      }

      DeckRebarData.Load(pStrLoad,pProgress);

      // in some 2.1 Beta versions we stored deck edge points for NoDeck and CompositeOverlay decks
      // these decks should have any points so clear them out right now.
      if ( IsConstantWidthDeck(DeckType) )
         DeckEdgePoints.clear();

      hr = pStrLoad->EndUnit();
   }
   catch(...)
   {
      ATLASSERT(0);
   }

   return hr;
}

HRESULT CDeckDescription::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit("Deck",6.0);

   pStrSave->put_Property("SlabType",         CComVariant(DeckType));
   pStrSave->put_Property("TransverseConnectivity", CComVariant(TransverseConnectivity)); // added for version 14.0
   pStrSave->put_Property("GrossDepth",       CComVariant(GrossDepth));

   pStrSave->put_Property("DeckEdgePointCount",CComVariant((long)DeckEdgePoints.size()));

   if ( 0 < DeckEdgePoints.size() )
   {
      pStrSave->BeginUnit("DeckEdgePoints",1.0);
      std::vector<CDeckPoint>::iterator iter;
      for ( iter = DeckEdgePoints.begin(); iter != DeckEdgePoints.end(); iter++ )
      {
         CDeckPoint& point = *iter;
         point.Save(pStrSave,pProgress);
      }
      pStrSave->EndUnit();
   }

   pStrSave->put_Property("OverhangEdgeDepth",CComVariant(OverhangEdgeDepth));
   pStrSave->put_Property("OverhangTaperType",CComVariant(OverhangTaper));
   pStrSave->put_Property("Fillet",           CComVariant(Fillet));
   //pStrSave->put_Property("SlabOffset",       CComVariant(SlabOffset)); // removed in version 4 (moved up to bridge level)
   pStrSave->put_Property("PanelDepth",       CComVariant(PanelDepth));
   pStrSave->put_Property("PanelSupport",     CComVariant(PanelSupport));
   pStrSave->put_Property("WearingSurfaceType", CComVariant(WearingSurface));

   pStrSave->put_Property("InputAsDepthAndDensity",CComVariant(bInputAsDepthAndDensity)); // added in version 3.0
   if ( bInputAsDepthAndDensity )
   {
      pStrSave->put_Property("OverlayDepth",     CComVariant(OverlayDepth));
      pStrSave->put_Property("OverlayDensity",   CComVariant(OverlayDensity));
   }
   else
   {
      pStrSave->put_Property("OverlayWeight",CComVariant(OverlayWeight)); // added in version 3.0
   }

   pStrSave->put_Property("SacrificialDepth", CComVariant(SacrificialDepth));

   // Added in version 6
   // new parameters are Unit, SlabConcreteType, SlabHasFct, and SlabFct
   pStrSave->BeginUnit("SlabConcrete",1.0);

      pStrSave->put_Property("Type",CComVariant( matConcrete::GetTypeName((matConcrete::Type)SlabConcreteType,false).c_str() ));
      pStrSave->put_Property("Fc",               CComVariant(SlabFc));
      pStrSave->put_Property("WeightDensity",    CComVariant(SlabWeightDensity));
      pStrSave->put_Property("StrengthDensity",  CComVariant(SlabStrengthDensity));
      pStrSave->put_Property("MaxAggregateSize", CComVariant(SlabMaxAggregateSize));
      pStrSave->put_Property("EcK1",             CComVariant(SlabEcK1));
      pStrSave->put_Property("EcK2",             CComVariant(SlabEcK2));
      pStrSave->put_Property("CreepK1",          CComVariant(SlabCreepK1)); 
      pStrSave->put_Property("CreepK2",          CComVariant(SlabCreepK2));
      pStrSave->put_Property("ShrinkageK1",      CComVariant(SlabShrinkageK1)); 
      pStrSave->put_Property("ShrinkageK2",      CComVariant(SlabShrinkageK2));
      pStrSave->put_Property("UserEc", CComVariant(SlabUserEc));

      if ( SlabUserEc )
         pStrSave->put_Property("Ec",     CComVariant(SlabEc));

      if ( SlabConcreteType != pgsTypes::Normal )
      {
         pStrSave->put_Property("HasFct",CComVariant(SlabHasFct));
         
         if ( SlabHasFct )
            pStrSave->put_Property("Fct",CComVariant(SlabFct));
      }

   pStrSave->EndUnit();

   // Added in version 5
   pStrSave->BeginUnit("Condition",1.0);
   pStrSave->put_Property("ConditionFactorType",CComVariant(Condition));
   pStrSave->put_Property("ConditionFactor",CComVariant(ConditionFactor));
   pStrSave->EndUnit(); // Condition

   DeckRebarData.Save(pStrSave,pProgress);

   pStrSave->EndUnit();

   return hr;
}


void CDeckDescription::SetBridgeDescription(const CBridgeDescription* pBridge)
{
   m_pBridgeDesc = pBridge;
}

const CBridgeDescription* CDeckDescription::GetBridgeDescription() const
{
   return m_pBridgeDesc;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CDeckDescription::MakeCopy(const CDeckDescription& rOther)
{
   // Add copy code here...
   DeckType       = rOther.DeckType;
   TransverseConnectivity = rOther.TransverseConnectivity;

   GrossDepth       = rOther.GrossDepth;
   OverhangTaper    = rOther.OverhangTaper;
   OverhangEdgeDepth = rOther.OverhangEdgeDepth;
   Fillet           = rOther.Fillet;
	OverlayWeight    = rOther.OverlayWeight;
   OverlayDensity   = rOther.OverlayDensity;
   OverlayDepth     = rOther.OverlayDepth;
   bInputAsDepthAndDensity = rOther.bInputAsDepthAndDensity;
	SacrificialDepth = rOther.SacrificialDepth;
   WearingSurface   = rOther.WearingSurface;

   SlabConcreteType      = rOther.SlabConcreteType;
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
   SlabHasFct            = rOther.SlabHasFct;
   SlabFct               = rOther.SlabFct;

   PanelDepth       = rOther.PanelDepth;
   PanelSupport     = rOther.PanelSupport;

   Condition = rOther.Condition;
   ConditionFactor = rOther.ConditionFactor;

   DeckRebarData = rOther.DeckRebarData;

   DeckEdgePoints = rOther.DeckEdgePoints;
}

void CDeckDescription::MakeAssignment(const CDeckDescription& rOther)
{
   MakeCopy( rOther );
}
