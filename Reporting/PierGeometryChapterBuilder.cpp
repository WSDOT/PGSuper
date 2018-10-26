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

#include "StdAfx.h"
#include <Reporting\PierGeometryChapterBuilder.h>
#include <Reporting\BridgeDescChapterBuilder.h>

#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>

#include <PgsExt\BridgeDescription.h>

#include <WBFLCogo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CPierGeometryChapterBuilder
****************************************************************************/


void pier_geometry(IBroker*pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CPierGeometryChapterBuilder::CPierGeometryChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CPierGeometryChapterBuilder::GetName() const
{
   return TEXT("Pier Geometry");
}

rptChapter* CPierGeometryChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CBrokerReportSpecification* pSpec = dynamic_cast<CBrokerReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   pier_geometry(pBroker,pChapter,pDisplayUnits);

   return pChapter;
}


CChapterBuilder* CPierGeometryChapterBuilder::Clone() const
{
   return new CPierGeometryChapterBuilder;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
void pier_geometry(IBroker*pBroker,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker, IBridge,      pBridge ); 
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker, IRoadway, pAlignment);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   INIT_UV_PROTOTYPE( rptLengthUnitValue, offset, pDisplayUnits->GetAlignmentLengthUnit(), false );

   CComPtr<IAngleDisplayUnitFormatter> angle_formatter;
   angle_formatter.CoCreateInstance(CLSID_AngleDisplayUnitFormatter);
   angle_formatter->put_Signed(VARIANT_FALSE);

   CComPtr<IDirectionDisplayUnitFormatter> direction_formatter;
   direction_formatter.CoCreateInstance(CLSID_DirectionDisplayUnitFormatter);
   direction_formatter->put_BearingFormat(VARIANT_TRUE);

   rptParagraph* pPara = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pPara;
   *pPara << "Pier Layout" << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(7,"");
   *pPara << pTable << rptNewLine;

   pTable->SetNumberOfHeaderRows(2);

   pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   pTable->SetRowSpan(0,0,2);
   pTable->SetRowSpan(1,0,-1);
   (*pTable)(0,0) << "";

   pTable->SetRowSpan(0,1,2);
   pTable->SetRowSpan(1,1,-1);
   (*pTable)(0,1) << "Station";

   pTable->SetRowSpan(0,2,2);
   pTable->SetRowSpan(1,2,-1);
   (*pTable)(0,2) << "Bearing";

   pTable->SetRowSpan(0,3,2);
   pTable->SetRowSpan(1,3,-1);
   (*pTable)(0,3) << "Skew Angle";

   pTable->SetColumnSpan(0,4,3);
   pTable->SetColumnSpan(0,5,-1);
   pTable->SetColumnSpan(0,6,-1);
   (*pTable)(0,4) << "Alignment Intersection";
   (*pTable)(1,4) << "East" << rptNewLine << "(X)";
   (*pTable)(1,5) << "North" << rptNewLine << "(Y)";
   (*pTable)(1,6) << COLHDR("Elev",rptLengthUnitTag,pDisplayUnits->GetAlignmentLengthUnit());

   const CPierData* pPier = pBridgeDesc->GetPier(0);
   RowIndexType row = pTable->GetNumberOfHeaderRows();
   while ( pPier != NULL )
   {
      PierIndexType pierIdx = pPier->GetPierIndex();

      CComPtr<IDirection> bearing;
      CComPtr<IAngle> skew;

      pBridge->GetPierDirection( pierIdx, &bearing );
      pBridge->GetPierSkew( pierIdx, &skew );

      double skew_value;
      skew->get_Value(&skew_value);

      CComBSTR bstrAngle;
      angle_formatter->Format(skew_value,CComBSTR("°,\',\""),&bstrAngle);

      double bearing_value;
      bearing->get_Value(&bearing_value);

      CComBSTR bstrBearing;
      direction_formatter->Format(bearing_value,CComBSTR("°,\',\""),&bstrBearing);

      if ( pPier->GetPrevSpan() == NULL || pPier->GetNextSpan() == NULL )
         (*pTable)(row,0) << "Abutment " << (Int32)(pierIdx+1);
      else
         (*pTable)(row,0) << "Pier " << (Int32)(pierIdx+1);

      (*pTable)(row,1) << rptRcStation(pPier->GetStation(), &pDisplayUnits->GetStationFormat() );
      (*pTable)(row,2) << RPT_BEARING(OLE2A(bstrBearing));
      (*pTable)(row,3) << RPT_ANGLE(OLE2A(bstrAngle));

      CComPtr<IPoint2d> pntLeft,pntAlignment,pntBridge,pntRight;
      pBridge->GetPierPoints(pierIdx,&pntLeft,&pntAlignment,&pntBridge,&pntRight);

      offset.ShowUnitTag(false);
      double x,y;
      pntAlignment->get_X(&x);
      pntAlignment->get_Y(&y);
      (*pTable)(row,4) << offset.SetValue(x);
      (*pTable)(row,5) << offset.SetValue(y);
      (*pTable)(row,6) << offset.SetValue( pAlignment->GetElevation(pPier->GetStation(),0.0) );

      if ( pPier->GetNextSpan() )
         pPier = pPier->GetNextSpan()->GetNextPier();
      else
         pPier = NULL;

      row++;
   }
}
