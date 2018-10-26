// BoundaryConditionDlg.cpp : implementation file
//

#include <PgsExt\PgsExtLib.h>
#include "BoundaryConditionDlg.h"
#include <MFCTools\CustomDDX.h>
#include <EAF\EAFUtilities.h>
#include <IFace\Project.h>

#include <PgsExt\PierData2.h>

// CBoundaryConditionDlg dialog

IMPLEMENT_DYNAMIC(CBoundaryConditionDlg, CDialog)

CBoundaryConditionDlg::CBoundaryConditionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBoundaryConditionDlg::IDD, pParent)
{
   m_BoundaryCondition = pgsTypes::ContinuousAfterDeck;
}

CBoundaryConditionDlg::~CBoundaryConditionDlg()
{
}

void CBoundaryConditionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_Control(pDX,IDC_BOUNDARY_CONDITION,m_cbBoundaryCondition);
   DDX_CBItemData(pDX, IDC_BOUNDARY_CONDITION, m_BoundaryCondition);
}


BEGIN_MESSAGE_MAP(CBoundaryConditionDlg, CDialog)
END_MESSAGE_MAP()


// CBoundaryConditionDlg message handlers

BOOL CBoundaryConditionDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   ATLASSERT(pIBridgeDesc->GetPier(m_PierIdx)->IsBoundaryPier());

   std::vector<pgsTypes::PierConnectionType> connections = pIBridgeDesc->GetPierConnectionTypes(m_PierIdx);

   m_cbBoundaryCondition.SetPierType(PIERTYPE_INTERMEDIATE);
   m_cbBoundaryCondition.Initialize(connections);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}
