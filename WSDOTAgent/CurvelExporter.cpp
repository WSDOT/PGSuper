///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// CurvelExporter.cpp : Implementation of CCurvelExporter
#include "stdafx.h"
#include "WSDOTAgentImp.h"
#include "CurvelExporter.h"

#include <EAF\EAFAutoProgress.h>
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>

#include <Curvel.h>
#include <MFCTools\Prompts.h>

class CSuperelevationValidator : public CMultiChoiceValidator
{
public:
   virtual BOOL IsValid(const std::vector<int>& options);
   virtual void DisplayValidationErrorMessage();

private:
   CString m_strMessage;
};

BOOL CSuperelevationValidator::IsValid(const std::vector<int>& options)
{
   if ( options.size() != 3 )
   {
      m_strMessage = _T("Please select three consecutive sections");
      return FALSE;
   }
   else
   {
      std::vector<int>::const_iterator iter(options.begin());
      std::vector<int>::const_iterator end(options.end());
      int last = *iter++;
      for ( ; iter != end; iter++ )
      {
         if ( *iter != last+1 )
         {
            m_strMessage = _T("Please select three consecutive sections");
            return FALSE;
         }

         last = *iter;
      }
   }

   return TRUE;
}

void CSuperelevationValidator::DisplayValidationErrorMessage()
{
   AfxMessageBox(m_strMessage,MB_OK | MB_ICONEXCLAMATION);
}

HRESULT CCurvelExporter::FinalConstruct()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   VERIFY(m_Bitmap.LoadBitmap(IDB_CURVEL));
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CCurvelExporter

STDMETHODIMP CCurvelExporter::Init(UINT nCmdID)
{
   return S_OK;
}

STDMETHODIMP CCurvelExporter::GetMenuText(BSTR*  bstrText)
{
   *bstrText = CComBSTR("BEToolbox:Curvel vertical profile model");
   return S_OK;
}

STDMETHODIMP CCurvelExporter::GetBitmapHandle(HBITMAP* phBmp)
{
   *phBmp = m_Bitmap;
   return S_OK;
}

STDMETHODIMP CCurvelExporter::GetCommandHintText(BSTR*  bstrText)
{
   *bstrText = CComBSTR("Export BEToolbox:Curvel model\nTool tip text");
   return S_OK;   
}

#define xTEST_CODE
STDMETHODIMP CCurvelExporter::Export(IBroker* pBroker)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
#pragma Reminder("UPDATE: remove test code after creating example")
   // There are two block of code here, conditionally compiled with the TEST_CODE macro.
   // The TEST_CODE was used to prototype and test the OpenBridgeML Units implementation
   // RAB left it here for an example until such time a simple example is developed
   // to demonstrate OpenBridgeML
#if defined TEST_CODE
	CFileDialog fileDlg(FALSE,_T("curvel"),_T("Example1a.curvel"),OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Curvel File (*.curvel)|*.curvel||"));
	if (fileDlg.DoModal() == IDOK)
	{
      std::auto_ptr<Curvel> curvelXML( CreateCurvelModel() ); // create an empty/default model

      // Provide a <UnitsDeclaration>
      OpenBridgeML::Units::UnitsDeclarationType unitsDeclaration;

      // Provide an <ExtendedUnit>
      OpenBridgeML::Units::ExtendedUnitsType extendedUnits;

      // First, just extend the Standard "Length" unit type by adding a new unit of measure
      // funny unit type... one length will be in number of half-feet
      // so for example if the value is 100 ft, the value in halfFoot would be 50hft.
      // one-half foot = 0.1524 meter
      OpenBridgeML::Units::UnitOfMeasureExType halfFoot(0.1524,OpenBridgeML::Units::UnitSystemEnum::unitsUS,_T("hft"),_T("Length"));
      extendedUnits.UnitOfMeasure().push_back(halfFoot);

      // Now, extend the Standard unit types by adding a new unit type and the units
      // of measure that go along with it. Curvel wont use it, but we can test
      // the creation end here and the interpretation end in Curvel.

      // declare container that will hold all of the extnded unit types
      OpenBridgeML::Units::ExtendedUnitTypeType extendedUnitTypes;

      // create my unit type
      OpenBridgeML::Units::UnitTypeType myUnitType(_T("MyUnitType"),1.0,1.0,1.0,1.0,1.0);

      // create units of measure for my unit type
      OpenBridgeML::Units::UnitOfMeasureType unit1(2.0,OpenBridgeML::Units::UnitSystemEnum::unitsSI,_T("a"));
      OpenBridgeML::Units::UnitOfMeasureType unit2(3.0,OpenBridgeML::Units::UnitSystemEnum::unitsUS,_T("b"));

      // add the units of measure to the definition of my unit type
      myUnitType.UnitOfMeasure().push_back(unit1);
      myUnitType.UnitOfMeasure().push_back(unit2);

      // add my unit type to the collection of extended unit types
      extendedUnitTypes.UnitType().push_back(myUnitType);

      // add the extended unit types to the extended unit type element
      extendedUnits.UnitTypes(extendedUnitTypes);

      // done defining all the units extensions, add them to the units declaration
      unitsDeclaration.ExtendedUnits(extendedUnits);


      // Provide <ConsistentUnits> (use slug,ft,sec,F,deg so that length units are all in feet
      // this is different than the internal PGSuper units and the internal Curvel units)
      OpenBridgeML::Units::ConsistentUnitsType consistentUnits(_T("slug"),_T("ft"),_T("sec"),_T("F"),_T("deg"));
      unitsDeclaration.ConsistentUnits(consistentUnits);

      curvelXML->UnitsDeclaration(unitsDeclaration);

      GET_IFACE2(pBroker,IRoadwayData,pRoadway);
      ProfileData2 profileData = pRoadway->GetProfileData2();
      VerticalCurveDataType& vCurveXML = curvelXML->VerticalCurveData();

      if ( profileData.VertCurves.size() == 0 )
      {
         // No vertical curves in PGSuper/PGSplice... create a zero length
         // curve Curvel
         vCurveXML.g1( profileData.Grade );
         vCurveXML.g2( profileData.Grade );
         vCurveXML.Length(0.0);
         vCurveXML.PVIStation(profileData.Station);
         vCurveXML.PVIElevation(profileData.Elevation);
      }
      else if ( profileData.VertCurves.size() == 1 )
      {
         VertCurveData& vCurve(profileData.VertCurves.front());
         if ( !IsEqual(vCurve.L1,vCurve.L2) && !IsZero(vCurve.L2) )
         {
            AfxMessageBox(_T("Curvel does not support unsymmetric vertical curves"));
            return FALSE;
         }

         vCurveXML.g1( profileData.Grade );
         vCurveXML.g2( vCurve.ExitGrade );

         /////////////////////////////////////////////////
         // Write Curve Length in inch units... OpenBridgeML Units Test
         Float64 length = vCurve.L1 + vCurve.L2;
         length = ::ConvertFromSysUnits(length,unitMeasure::Feet);
         length *= 2.0; // number of half feet (using the silly half-foot unit)
         vCurveXML.Length(length);
         vCurveXML.Length().unit().set(_T("hft"));

         // The consistent units that we want to write into the XML file
         // have length units of feet. Convert the consistent values in PGSuper
         // to units of feet.
         if ( IsEqual(profileData.Station,vCurve.PVIStation) )
         {
            vCurveXML.PVIStation(::ConvertFromSysUnits(profileData.Station,unitMeasure::Feet));
            vCurveXML.PVIElevation(::ConvertFromSysUnits(profileData.Elevation,unitMeasure::Feet));
         }
         else
         {
            // Reference station and PVI of vertical curve are at different locations... compute
            // the PVI elevation
            Float64 pviElevation = profileData.Elevation + profileData.Grade*(vCurve.PVIStation - profileData.Station);
            vCurveXML.PVIStation(::ConvertFromSysUnits(vCurve.PVIStation,unitMeasure::Feet));
            vCurveXML.PVIElevation(::ConvertFromSysUnits(pviElevation,unitMeasure::Feet));
         }
      }
      else
      {
#pragma Reminder("UPDATE: need to have user select the vertical curve")
         AfxMessageBox(_T("Too many vertical curves"));
         return FALSE;
      }

#pragma Reminder("IMPLEMENT: add export for crown slopes")
      CrownSlopeType crownXML;
      CrownSlopeType::SuperelevationProfilePoint_sequence& superelevationPointsXML(crownXML.SuperelevationProfilePoint());

      RoadwaySectionData sectionData = pRoadway->GetRoadwaySectionData();
      std::size_t nSections = sectionData.Superelevations.size();
      if ( nSections <= 3 )
      {
         Float64 crownPointOffset;
         for ( std::size_t i = 0; i < nSections; i++ )
         {
            CrownData2& crown = sectionData.Superelevations[i];
            if ( i == 0 )
            {
               crownPointOffset = crown.CrownPointOffset;
            }
            else
            {
               if ( !IsEqual(crownPointOffset,crown.CrownPointOffset) )
               {
#pragma Reminder("UPDATE: be more forgiving... don't have to error out here")
                  // this has to be a way to make this work
                  AfxMessageBox(_T("The crown point offset must be the same at all sections"));
                  return FALSE;
               }
            }

            Float64 station = ::ConvertFromSysUnits(crown.Station,unitMeasure::Feet);
            superelevationPointsXML.push_back(SuperelevationProfilePointType(station,crown.Left,crown.Right));
         }

         if ( nSections < 3 )
         {
            std::size_t nTempSections = 3 - nSections;
            for ( std::size_t i = 0; i < nTempSections; i++ )
            {
               SuperelevationProfilePointType pp = superelevationPointsXML.back();
               pp.Station(::ConvertFromSysUnits(pp.Station() + ::ConvertToSysUnits(50,unitMeasure::Feet),unitMeasure::Feet));
               superelevationPointsXML.push_back(pp);
            }
         }
         SuperelevationDataType superelevationXML(::ConvertFromSysUnits(crownPointOffset,unitMeasure::Feet),crownXML);
         curvelXML->SuperelevationData(superelevationXML);
      }
      else
      {
#pragma Reminder("UPDATE: be more forgiving... don't have to error out here")
         // this has to be a way to make this work
         AfxMessageBox(_T("There cannot be more than three sections"));
         return FALSE;
      }

		CString strPathName = fileDlg.GetPathName();
      return SaveCurvelModel(strPathName,curvelXML.get());
	}		
#else
   // write some bridge data to a text file
	CFileDialog fileDlg(FALSE,_T("curvel"),_T("PGSuperExport.curvel"),OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Curvel File (*.curvel)|*.curvel||"));
	if (fileDlg.DoModal() == IDOK)
	{
      GET_IFACE2(pBroker,IRoadwayData,pRoadway);
      std::auto_ptr<Curvel> curvelXML( CreateCurvelModel() ); // create a default curvel model

      ///////////////////////////////////////////////////////////////
      // Export vertical profile
      ///////////////////////////////////////////////////////////////
      ProfileData2 profileData = pRoadway->GetProfileData2();
      VerticalCurveDataType& vCurveXML = curvelXML->VerticalCurveData();

      if ( profileData.VertCurves.size() == 0 )
      {
         // No vertical curves in PGSuper/PGSplice... create a zero length
         // curve Curvel
         vCurveXML.g1( profileData.Grade );
         vCurveXML.g2( profileData.Grade );
         vCurveXML.Length(0.0);
         vCurveXML.PVIStation(profileData.Station);
         vCurveXML.PVIElevation(profileData.Elevation);
      }
      else
      {
         int curveIdx = 0;

         // Curvel supports a single vertical curve, select which vertical curve to export to Curvel
         if ( 1 < profileData.VertCurves.size() )
         {
            GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
            CString strList;
            std::vector<VertCurveData>::iterator iter(profileData.VertCurves.begin());
            std::vector<VertCurveData>::iterator end(profileData.VertCurves.end());
            int idx = 1;
            for ( ; iter != end; iter++, idx++ )
            {
               VertCurveData& vCurve(*iter);
               CString str;
               str.Format(_T("Curve %d: PVI Station %s"),idx,::FormatStation(pDisplayUnits->GetStationFormat(),vCurve.PVIStation));
               strList += str + _T("\n");

            }
            curveIdx = AfxChoose(_T("Multiple Vertical Curves"),_T("Curvel is limited to a single vertical curve. Select the vertical curve to export."),strList,0,TRUE);
            if ( curveIdx == -1 )
               return S_FALSE;
         }
         
         VertCurveData& vCurve(profileData.VertCurves[curveIdx]);
         if ( !IsEqual(vCurve.L1,vCurve.L2) && !IsZero(vCurve.L2) )
         {
            AfxMessageBox(_T("Cannot export profile information. Curvel does not support unsymmetric vertical curves"),MB_OK | MB_ICONSTOP);
            return FALSE;
         }

         vCurveXML.g1( profileData.Grade );
         vCurveXML.g2( vCurve.ExitGrade );
         vCurveXML.Length(vCurve.L1 + vCurve.L2);

         if ( IsEqual(profileData.Station,vCurve.PVIStation) )
         {
            vCurveXML.PVIStation(profileData.Station);
            vCurveXML.PVIElevation(profileData.Elevation);
         }
         else
         {
            // Reference station and PVI of vertical curve are at different locations... compute
            // the PVI elevation
            Float64 pviElevation = profileData.Elevation + profileData.Grade*(vCurve.PVIStation - profileData.Station);
            vCurveXML.PVIStation(vCurve.PVIStation);
            vCurveXML.PVIElevation(pviElevation);
         }
      }

      ///////////////////////////////////////////////////////////////
      // Export crown slopes
      ///////////////////////////////////////////////////////////////
      CrownSlopeType crownXML;
      CrownSlopeType::SuperelevationProfilePoint_sequence& superelevationPointsXML(crownXML.SuperelevationProfilePoint());

      RoadwaySectionData sectionData = pRoadway->GetRoadwaySectionData();
      std::size_t nSections = sectionData.Superelevations.size();
      if ( nSections <= 3 )
      {
         Float64 crownPointOffset;
         bool bNotifyCrownPointOffset = false;
         for ( std::size_t i = 0; i < nSections; i++ )
         {
            CrownData2& crown = sectionData.Superelevations[i];
            if ( i == 0 )
            {
               crownPointOffset = crown.CrownPointOffset;
            }
            else
            {
               if ( !IsEqual(crownPointOffset,crown.CrownPointOffset) )
               {
                  bNotifyCrownPointOffset = true;
               }
            }

            superelevationPointsXML.push_back(SuperelevationProfilePointType(crown.Station,crown.Left,crown.Right));
         }

         if ( bNotifyCrownPointOffset )
         {
            GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
            CString strMsg;
            strMsg.Format(_T("Curvel is limited to a single crown point offset. The exported crown point offset is %s."),::FormatOffset(crownPointOffset,pDisplayUnits->GetAlignmentLengthUnit(),false));
            AfxMessageBox(strMsg,MB_OK | MB_ICONINFORMATION);
         }

         if ( nSections < 3 )
         {
            std::size_t nTempSections = 3 - nSections;
            for ( std::size_t i = 0; i < nTempSections; i++ )
            {
               SuperelevationProfilePointType pp = superelevationPointsXML.back();
               pp.Station(pp.Station() + ::ConvertToSysUnits(50,unitMeasure::Feet));
               superelevationPointsXML.push_back(pp);
            }
         }
         SuperelevationDataType superelevationXML(crownPointOffset,crownXML);
         curvelXML->SuperelevationData(superelevationXML);
      }
      else
      {
         CString strOptions;
         GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
         for ( std::size_t i = 0; i < nSections; i++ )
         {
            CrownData2& crown = sectionData.Superelevations[i];
            CString str;
            str.Format(_T("Section %d: Station %s, Left Slope: %s, Right Slope %s"), i+1,
                       ::FormatStation(pDisplayUnits->GetStationFormat(),crown.Station),
                       ::FormatScalar(crown.Left,pDisplayUnits->GetScalarFormat()),
                       ::FormatScalar(crown.Right,pDisplayUnits->GetScalarFormat()));
            strOptions += str + _T("\n");
         }
         std::vector<int> choices;
         choices.push_back(0);
         choices.push_back(1);
         choices.push_back(2);
         CSuperelevationValidator validator;
         choices = AfxMultiChoice(_T("Select Cross Sections"),_T("Select three consecutive cross sections to export to Curvel."),strOptions,&validator,choices,TRUE);
         if ( choices.size() == 0 )
         {
            return S_FALSE;
         }

         ATLASSERT(choices.size()==3);
         std::vector<int>::iterator iter(choices.begin());
         std::vector<int>::iterator end(choices.end());
         Float64 crownPointOffset;
         bool bNotifyCrownPointOffset = false;
         int i = 0;
         for ( ; iter != end; iter++, i++ )
         {
            CrownData2& crown = sectionData.Superelevations[*iter];
            if ( i == 0 )
            {
               crownPointOffset = crown.CrownPointOffset;
            }
            else
            {
               if ( !IsEqual(crownPointOffset,crown.CrownPointOffset) )
               {
                  bNotifyCrownPointOffset = true;
               }
            }

            superelevationPointsXML.push_back(SuperelevationProfilePointType(crown.Station,crown.Left,crown.Right));
            SuperelevationDataType superelevationXML(crownPointOffset,crownXML);
            curvelXML->SuperelevationData(superelevationXML);
         }

         if ( bNotifyCrownPointOffset )
         {
            CString strMsg;
            strMsg.Format(_T("Curvel is limited to a single crown point offset. The exported crown point offset is %s."),::FormatOffset(crownPointOffset,pDisplayUnits->GetAlignmentLengthUnit(),false));
            AfxMessageBox(strMsg,MB_OK | MB_ICONINFORMATION);
         }
      }

      ///////////////////////////////////////////////////////////////
      // Export horizontal curves
      ///////////////////////////////////////////////////////////////

      // Curvel does not model horizontal curves the same way PGSuper does
      // Create a skew line reporting point for each horizontal curve
      // This is the best way to export the horizontal curve information
      AlignmentData2 alignmentData = pRoadway->GetAlignmentData2();
      std::vector<HorzCurveData>::iterator iter(alignmentData.HorzCurves.begin());
      std::vector<HorzCurveData>::iterator end(alignmentData.HorzCurves.end());
      bool bSpiral = false;
      bool bCurves = false;
      SkewLinesType skewLines;
      for ( ; iter != end; iter++ )
      {
         bCurves = true;
         HorzCurveData& hCurve(*iter);
         SkewLineType skewLine(hCurve.PIStation,OffsetType::RadialFromCrownLine,0.0,"0.0 L",hCurve.Radius,0.0);
         if ( !IsZero(hCurve.EntrySpiral) || !IsZero(hCurve.ExitSpiral) )
         {
            bSpiral = true;
         }

         skewLines.SkewLine().push_back(skewLine);
      }

      if ( bCurves )
      {
         curvelXML->SkewLines().set(skewLines);

         CString strMsg(_T("Horizontal curves have been modeled as Skew Line input in Curvel."));
         if ( bSpiral )
         {
            strMsg += _T("\nEntry and Exit Spirals are not modeled in Curvel.");
         }

         AfxMessageBox(strMsg,MB_OK | MB_ICONINFORMATION);
      }

      ///////////////////////////////////////////////////////////////
      // Done creating CurveXML model... save it
      ///////////////////////////////////////////////////////////////
		CString strPathName = fileDlg.GetPathName();
      if ( SaveCurvelModel(strPathName,curvelXML.get()) )
      {
         AfxMessageBox(_T("Export complete"),MB_OK | MB_ICONEXCLAMATION);
         return S_OK;
      }
	}	
#endif // TEST_CODE

   return S_FALSE;
}
