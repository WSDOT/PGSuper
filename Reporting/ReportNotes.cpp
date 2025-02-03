#include "StdAfx.h"
#include <Reporting\ReportNotes.h>
#include <Lrfd/BDSManager.h>
#include <IFace\Project.h>
#include <psgLib/PrestressLossCriteria.h>


void ReportNotes::GetSpecificationCompleteInfo(rptParagraph* pPara)
{
	CComPtr<IBroker> pBroker;
	EAFGetBroker(&pBroker);
	GET_IFACE2(pBroker, ILossParameters, pLossParams);
	bool bTimeStep = (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP ? true : false);
	bool bTenthEdition =  (WBFL::LRFD::BDSManager::GetEdition() > WBFL::LRFD::BDSManager::Edition::NinthEdition2020);
	*pPara << WBFL::LRFD::BDSManager::GetSpecificationName() << _T(", ") << WBFL::LRFD::BDSManager::GetEditionAsString();
	if (bTimeStep && bTenthEdition)
	{
		*pPara << rptNewLine;
		*pPara << PT_SPEC_REQUIRED;
	}
}
