/////////////////////////////////////////////////////////////////////////
//// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
//// Copyright © 1999-2023  Washington State Department of Transportation
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

    INIT_UV_PROTOTYPE(rptAngleUnitValue, Reaction, pDisplayUnits->GetRadAngleUnit(), false);



    ColumnIndexType nCols = 9;


    GET_IFACE2(pBroker, IBearingDesignParameters, pBearingDesignParameters);
    DESIGNPROPERTIES details;
    pBearingDesignParameters->GetBearingDesignProperties(&details);


    CString label = _T("Design Properties");
    rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols, label);

    p_table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
    p_table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

    ColumnIndexType col = 0;
    p_table->SetNumberOfHeaderRows(2);
    p_table->SetRowSpan(0, col, 2);
    (*p_table)(0, col++) << _T("");
    p_table->SetRowSpan(0, col, 2);
    (*p_table)(0, col++) << _T("Fy");
    p_table->SetRowSpan(0, col, 2);
    (*p_table)(0, col++) << _T("Fth");
    p_table->SetColumnSpan(0,col, 2);
    (*p_table)(0, col) << _T("50 Hardness");
    (*p_table)(1, col++) << _T("Gmin");
    (*p_table)(1, col++) << _T("Gmax");
    p_table->SetColumnSpan(0, col, 2);
    (*p_table)(0, col) << _T("60 Hardness");
    (*p_table)(1, col++) << _T("Gmin");
    (*p_table)(1, col++) << _T("Gmax");
    p_table->SetColumnSpan(0, col, 2);
    (*p_table)(0, col) << _T("70 Hardness");
    (*p_table)(1, col++) << _T("Gmin");
    (*p_table)(1, col++) << _T("Gmax");


    col = 0;
    (*p_table)(2, col++) << _T("Design Value");
    (*p_table)(2, col++) << Reaction.SetValue(details.Fy);
    (*p_table)(2, col++) << Reaction.SetValue(details.Fth);
    (*p_table)(2, col++) << Reaction.SetValue(details.Gmin50);
    (*p_table)(2, col++) << Reaction.SetValue(details.Gmax50);
    (*p_table)(2, col++) << Reaction.SetValue(details.Gmin60);
    (*p_table)(2, col++) << Reaction.SetValue(details.Gmax60);
    (*p_table)(2, col++) << Reaction.SetValue(details.Gmin70);
    (*p_table)(2, col++) << Reaction.SetValue(details.Gmax70);


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
