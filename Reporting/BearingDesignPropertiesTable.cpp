/////////////////////////////////////////////////////////////////////////
//// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
//// Copyright © 1999-2024  Washington State Department of Transportation
////                        Bridge and Structures Office
////
//// This program is free software; you can redistribute it and/or modify
//// it under the terms of the Alternate Route Open Source License as 
//// published by the Washington State Department of Transportation, 
//// Bridge and Structures Office.
////
//// This program is distributed in the hope that it will be useful, but 
//// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
//// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
//// the Alternate Route Open Source License for more details.
////
//// You should have received a copy of the Alternate Route Open Source 
//// License along with this program; if not, write to the Washington 
//// State Department of Transportation, Bridge and Structures Office, 
//// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
//// Bridge_Support@wsdot.wa.gov
/////////////////////////////////////////////////////////////////////////
//
#include "StdAfx.h"
#include <Reporting\BearingDesignPropertiesTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>
#include <IFace\BearingDesignParameters.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///****************************************************************************
//CLASS
//   CBearingReactiontionTable
//****************************************************************************/
//
//
//////////////////////////// PUBLIC     ///////////////////////////////////////
//
////======================== LIFECYCLE  =======================================
CBearingDesignPropertiesTable::CBearingDesignPropertiesTable()
{
}

CBearingDesignPropertiesTable::CBearingDesignPropertiesTable(const CBearingDesignPropertiesTable& rOther)
{
    MakeCopy(rOther);
}

CBearingDesignPropertiesTable::~CBearingDesignPropertiesTable()
{
}

//======================== OPERATORS  =======================================
CBearingDesignPropertiesTable& CBearingDesignPropertiesTable::operator= (const CBearingDesignPropertiesTable& rOther)
{
    if (this != &rOther)
    {
        MakeAssignment(rOther);
    }

    return *this;
}









//======================== OPERATIONS =======================================
rptRcTable* CBearingDesignPropertiesTable::BuildBearingDesignPropertiesTable(IBroker* pBroker, const CGirderKey& girderKey, pgsTypes::AnalysisType analysisType,
    bool bIncludeImpact, bool bIncludeLLDF, bool bDesign, bool bUserLoads, bool bIndicateControllingLoad, IEAFDisplayUnits* pDisplayUnits, bool bDetail) const
{

    INIT_UV_PROTOTYPE(rptStressUnitValue, Reaction, pDisplayUnits->GetStressUnit(), false);



    ColumnIndexType nCols = 3;


    GET_IFACE2(pBroker, IBearingDesignParameters, pBearingDesignParameters);
    DESIGNPROPERTIES details;
    pBearingDesignParameters->GetBearingDesignProperties(&details);


    CString label = _T("Bearing Design Properties");
    rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, label);

    p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
    p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

    ColumnIndexType col = 0;
    p_table->SetNumberOfHeaderRows(2);
    p_table->SetRowSpan(0, col, 2);
    (*p_table)(0, col++) << _T("");
    p_table->SetRowSpan(0, col, 2);
    (*p_table)(0, col++) << COLHDR(Sub2(_T("F"), _T("y")),
        rptStressUnitTag, pDisplayUnits->GetStressUnit());
    p_table->SetRowSpan(0, col, 2);
    (*p_table)(0, col++) << COLHDR(Sub2(_T("F"), _T("th")),
        rptStressUnitTag, pDisplayUnits->GetStressUnit());


    col = 0;
    (*p_table)(2, col++) << _T("Design Value");
    (*p_table)(2, col++) << Reaction.SetValue(details.Fy);
    (*p_table)(2, col++) << Reaction.SetValue(details.Fth);


    return p_table;

}



////======================== ACCESS     =======================================
////======================== INQUIRY    =======================================
//
//////////////////////////// PROTECTED  ///////////////////////////////////////
//
////======================== LIFECYCLE  =======================================
////======================== OPERATORS  =======================================
////======================== OPERATIONS =======================================
void CBearingDesignPropertiesTable::MakeCopy(const CBearingDesignPropertiesTable& rOther)
{
    // Add copy code here...
}

void CBearingDesignPropertiesTable::MakeAssignment(const CBearingDesignPropertiesTable& rOther)
{
    MakeCopy(rOther);
}
//
////======================== ACCESS     =======================================
////======================== INQUIRY    =======================================
//
//////////////////////////// PRIVATE    ///////////////////////////////////////
//
////======================== LIFECYCLE  =======================================
////======================== OPERATORS  =======================================
////======================== OPERATIONS =======================================
////======================== ACCESS     =======================================
////======================== INQUERY    =======================================
