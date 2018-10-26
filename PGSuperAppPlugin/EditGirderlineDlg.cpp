// EditGirderlineDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "EditGirderlineDlg.h"
#include <LRFD\StrandPool.h>
#include <EAF\EAFDisplayUnits.h>


#include <PgsExt\BridgeDescription2.h>
#include <IFace\Project.h>

// CEditGirderlineDlg dialog


void DDX_Strand(CDataExchange* pDX,UINT nIDC,const matPsStrand** ppStrand)
{
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   CComboBox* pList = (CComboBox*)pDX->m_pDlgWnd->GetDlgItem( nIDC );

   if (pDX->m_bSaveAndValidate)
   {
      // strand material
      int curSel = pList->GetCurSel();
      Int32 key = (Int32)pList->GetItemData( curSel );
      *ppStrand = pPool->GetStrand( key );
   }
   else
   {
      Int32 target_key = pPool->GetStrandKey(*ppStrand );
      int cStrands = pList->GetCount();
      for ( int i = 0; i < cStrands; i++ )
      {
         Int32 key = (Int32)pList->GetItemData( i );
         if ( key == target_key )
         {
            pList->SetCurSel(i);
            break;
         }
      }
   }
}

void DDX_PTData(CDataExchange* pDX,INT nIDC,CPTData* ptData)
{
   CDuctGrid* pGrid = (CDuctGrid*)pDX->m_pDlgWnd->GetDlgItem(nIDC);
   if ( pDX->m_bSaveAndValidate )
   {
      *ptData = pGrid->GetPTData();
   }
   else
   {
      pGrid->SetPTData(*ptData);
   }
}

IMPLEMENT_DYNAMIC(CEditGirderlineDlg, CDialog)

CEditGirderlineDlg::CEditGirderlineDlg(const CSplicedGirderData* pGirder,CWnd* pParent /*=NULL*/)
	: CDialog(CEditGirderlineDlg::IDD, pParent)
{
   m_Girder = *pGirder;
   m_GirderKey = pGirder->GetGirderKey();
   m_GirderID = pGirder->GetID();

   const CTimelineManager* pTimelineMgr = pGirder->GetGirderGroup()->GetBridgeDescription()->GetTimelineManager();

   DuctIndexType nDucts = pGirder->GetPostTensioning()->GetDuctCount();
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      EventIndexType eventIdx = pTimelineMgr->GetStressTendonEventIndex(m_GirderKey,ductIdx);
      m_TendonStressingEvent.push_back(eventIdx);
   }

   IndexType nClosurePours = pGirder->GetClosurePourCount();
   for ( IndexType idx = 0; idx < nClosurePours; idx++ )
   {
      const CPrecastSegmentData* pSegment = pGirder->GetSegment(idx);
      SegmentIDType segID = pSegment->GetID();
      EventIndexType eventIdx = pTimelineMgr->GetCastClosurePourEventIndex(segID);
      m_CastClosureEvent.push_back(eventIdx);
   }
}

CEditGirderlineDlg::~CEditGirderlineDlg()
{
}

void CEditGirderlineDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   std::_tstring strGirderName = m_Girder.GetGirderName();
   DDX_CBStringExactCase(pDX,IDC_GIRDER_NAME,strGirderName);
   if ( pDX->m_bSaveAndValidate )
      m_Girder.SetGirderName(strGirderName.c_str());

   DDX_Strand(pDX,IDC_STRAND,&m_Girder.GetPostTensioning()->pStrand);

   DDV_GXGridWnd(pDX,&m_DuctGrid);
   DDX_PTData(pDX,IDC_DUCT_GRID,m_Girder.GetPostTensioning());

   DDX_Check_Bool(pDX,IDC_CHECKBOX,m_bCopyToAll);

   Float64 conditionFactor;
   pgsTypes::ConditionFactorType conditionFactorType;
   if ( pDX->m_bSaveAndValidate )
   {
      // data coming out of dialog
      DDX_CBEnum(pDX, IDC_CONDITION_FACTOR_TYPE, conditionFactorType);
      DDX_Text(pDX,   IDC_CONDITION_FACTOR,     conditionFactor);

      m_Girder.SetConditionFactor(conditionFactor);
      m_Girder.SetConditionFactorType(conditionFactorType);
   }
   else
   {
      // data going into of dialog
      conditionFactor     = m_Girder.GetConditionFactor();
      conditionFactorType = m_Girder.GetConditionFactorType();

      DDX_CBEnum(pDX, IDC_CONDITION_FACTOR_TYPE, conditionFactorType);
      DDX_Text(pDX,   IDC_CONDITION_FACTOR,     conditionFactor);
   }
}


BEGIN_MESSAGE_MAP(CEditGirderlineDlg, CDialog)
   ON_BN_CLICKED(IDC_ADD, &CEditGirderlineDlg::OnAddDuct)
   ON_BN_CLICKED(IDC_DELETE, &CEditGirderlineDlg::OnDeleteDuct)
   ON_CBN_SELCHANGE(IDC_GRADE, &CEditGirderlineDlg::OnStrandChanged)
   ON_CBN_SELCHANGE(IDC_TYPE, &CEditGirderlineDlg::OnStrandChanged)
   ON_CBN_SELCHANGE(IDC_STRAND_SIZE, &CEditGirderlineDlg::OnStrandSizeChanged)
   ON_CBN_SELCHANGE(IDC_CONDITION_FACTOR_TYPE, &CEditGirderlineDlg::OnConditionFactorTypeChanged)
   ON_BN_CLICKED(IDHELP, &CEditGirderlineDlg::OnHelp)
END_MESSAGE_MAP()


// CEditGirderlineDlg message handlers

BOOL CEditGirderlineDlg::OnInitDialog()
{
   m_bCopyToAll = false;

   // initialize girder girder
	m_GirderGrid.SubclassDlgItem(IDC_GIRDER_GRID, this);
   m_GirderGrid.CustomInit();

   // initialize duct grid
	m_DuctGrid.SubclassDlgItem(IDC_DUCT_GRID, this);
   m_DuctGrid.CustomInit(&m_Girder);

   // subclass the schematic drawing of the tendons
   m_DrawTendons.SubclassDlgItem(IDC_TENDONS,this);
   m_DrawTendons.CustomInit(this);

   FillStrandList(IDC_STRAND);

   FillGirderComboBox();

   // Initialize the condition factor combo box
   CComboBox* pcbConditionFactor = (CComboBox*)GetDlgItem(IDC_CONDITION_FACTOR_TYPE);
   pcbConditionFactor->AddString(_T("Good or Satisfactory (Structure condition rating 6 or higher)"));
   pcbConditionFactor->AddString(_T("Fair (Structure condition rating of 5)"));
   pcbConditionFactor->AddString(_T("Poor (Structure condition rating 4 or lower)"));
   pcbConditionFactor->AddString(_T("Other"));
   pcbConditionFactor->SetCurSel(0);

   CDialog::OnInitDialog();

   OnConditionFactorTypeChanged();

   CString strCaption;
   strCaption.Format(_T("Group %d Girder %s"),LABEL_GROUP(m_GirderKey.groupIndex),LABEL_GIRDER(m_GirderKey.girderIndex));
   SetWindowText(strCaption);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditGirderlineDlg::OnConditionFactorTypeChanged()
{
   CEdit* pEdit = (CEdit*)GetDlgItem(IDC_CONDITION_FACTOR);
   CComboBox* pcbConditionFactor = (CComboBox*)GetDlgItem(IDC_CONDITION_FACTOR_TYPE);

   int idx = pcbConditionFactor->GetCurSel();
   switch(idx)
   {
   case 0:
      pEdit->EnableWindow(FALSE);
      pEdit->SetWindowText(_T("1.00"));
      break;
   case 1:
      pEdit->EnableWindow(FALSE);
      pEdit->SetWindowText(_T("0.95"));
      break;
   case 2:
      pEdit->EnableWindow(FALSE);
      pEdit->SetWindowText(_T("0.85"));
      break;
   case 3:
      pEdit->EnableWindow(TRUE);
      break;
   }
}

void CEditGirderlineDlg::OnAddDuct()
{
   EventIndexType eventIdx = 0;
   if ( m_TendonStressingEvent.size() != 0 )
      eventIdx = m_TendonStressingEvent.back();

   m_TendonStressingEvent.push_back(eventIdx);
   m_DuctGrid.AddDuct(eventIdx);
   m_DrawTendons.Invalidate();
   m_DrawTendons.UpdateWindow();
}

void CEditGirderlineDlg::OnDeleteDuct()
{
   m_TendonStressingEvent.pop_back();
   m_DuctGrid.DeleteDuct();
   m_DrawTendons.Invalidate();
   m_DrawTendons.UpdateWindow();
}

void CEditGirderlineDlg::FillGirderComboBox()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   std::_tstring strGirderFamilyName = pBridgeDesc->GetGirderFamilyName();

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GIRDER_NAME);

   GET_IFACE2( pBroker, ILibraryNames, pLibNames );
   std::vector<std::_tstring> names;
   std::vector<std::_tstring>::iterator iter;
   
   pLibNames->EnumGirderNames(strGirderFamilyName.c_str(), &names );
   for ( iter = names.begin(); iter < names.end(); iter++ )
   {
      std::_tstring& name = *iter;

      pCB->AddString( name.c_str() );
   }
}

void CEditGirderlineDlg::FillStrandList(UINT nIDC)
{
   CComboBox* pList = (CComboBox*)GetDlgItem(nIDC);
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   // capture the current selection, if any
   int cur_sel = pList->GetCurSel();
   matPsStrand::Size cur_size = matPsStrand::D1270;
   if ( cur_sel != CB_ERR )
   {
      Int32 cur_key = (Int32)pList->GetItemData( cur_sel );
      const matPsStrand* pCurStrand = pPool->GetStrand( cur_key );
      cur_size = pCurStrand->GetSize();
   }

   pList->ResetContent();

   int sel_count = 0;  // Keep count of the number of strings added to the combo box
   int new_cur_sel = -1; // This will be in index of the string we want to select.
   for ( int i = 0; i < 2; i++ )
   {
      matPsStrand::Grade grade = (i == 0 ? matPsStrand::Gr1725 : matPsStrand::Gr1860);
      for ( int j = 0; j < 2; j++ )
      {
         matPsStrand::Type type = (j == 0 ? matPsStrand::LowRelaxation : matPsStrand::StressRelieved);

         lrfdStrandIter iter(grade,type);

         for ( iter.Begin(); iter; iter.Next() )
         {
            const matPsStrand* pStrand = iter.GetCurrentStrand();
            int idx = pList->AddString( pStrand->GetName().c_str() );

            if ( idx != CB_ERR )
            { 
               // if there wasn't an error adding the size, add a data item
               Int32 key;
               key = pPool->GetStrandKey( pStrand );

               if ( pList->SetItemData( idx, key ) == CB_ERR )
               {
                  // if there was an error adding the data item, remove the size
                  idx = pList->DeleteString( idx );
                  ASSERT( idx != CB_ERR ); // make sure it got removed.
               }
               else
               {
                  // data item added successfully.
                  if ( pStrand->GetSize() == cur_size )
                  {
                     // We just found the one we want to select.
                     new_cur_sel = sel_count;
                  }
               }
            }

            sel_count++;
         }
      }
   }

   // Attempt to re-select the strand.
   if ( 0 <= new_cur_sel )
      pList->SetCurSel( new_cur_sel );
   else
      pList->SetCurSel( pList->GetCount()-1 );
}

void CEditGirderlineDlg::FillStrandList(CComboBox* pList,matPsStrand::Grade grade,matPsStrand::Type  type)
{
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   bool bUnitsUS = IS_US_UNITS(pDisplayUnits);

   pList->ResetContent();

   lrfdStrandIter iter( grade, type );
   int sel_count = 0;  // Keep count of the number of strings added to the combo box
   for ( iter.Begin(); iter; iter.Next() )
   {
      const matPsStrand* pStrand = iter.GetCurrentStrand();
      std::_tstring size = matPsStrand::GetSize( pStrand->GetSize(), bUnitsUS );
      int idx = pList->AddString( size.c_str() );

      if ( idx != CB_ERR )
      { 
         // if there wasn't an error adding the size, add a data item
         Int32 key;
         key = pPool->GetStrandKey( pStrand );

         if ( pList->SetItemData( idx, key ) == CB_ERR )
         {
            // if there was an error adding the data item, remove the size
            idx = pList->DeleteString( idx );
            ASSERT( idx != CB_ERR ); // make sure it got removed.
         }
      }

      sel_count++;
   }
}

const matPsStrand* CEditGirderlineDlg::GetStrand()
{
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_STRAND );
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   int cursel = pList->GetCurSel();
   Int32 key = (Int32)pList->GetItemData(cursel);
   return pPool->GetStrand(key);
}

const CSplicedGirderData* CEditGirderlineDlg::GetGirder()
{
   m_Girder.SetPostTensioning( m_DuctGrid.GetPTData() );
   return &m_Girder;
}

const CGirderKey& CEditGirderlineDlg::GetGirderKey()
{
   return m_GirderKey;
}

void CEditGirderlineDlg::OnStrandSizeChanged()
{
   m_DuctGrid.OnStrandChanged();
}

void CEditGirderlineDlg::OnStrandChanged()
{
   CComboBox* pcbGrade = (CComboBox*)GetDlgItem(IDC_GRADE);
   CComboBox* pcbType  = (CComboBox*)GetDlgItem(IDC_TYPE);
   CComboBox* pList    = (CComboBox*)GetDlgItem(IDC_STRAND_SIZE);

   int cursel = pcbGrade->GetCurSel();
   matPsStrand::Grade grade = (matPsStrand::Grade)pcbGrade->GetItemData(cursel);

   cursel = pcbType->GetCurSel();
   matPsStrand::Type type = (matPsStrand::Type)pcbType->GetItemData(cursel);

   cursel = pList->GetCurSel();
   Uint32 key = (Uint32)pList->GetItemData(cursel);
   matPsStrand::Size size = lrfdStrandPool::GetInstance()->GetStrand(key)->GetSize();

   FillStrandList(pList,grade,type);

   int nItems = pList->GetCount();
   for ( int i = 0; i < nItems; i++ )
   {
      Uint32 key = (Uint32)pList->GetItemData(i);
      if (size == lrfdStrandPool::GetInstance()->GetStrand(key)->GetSize() )
      {
         pList->SetCurSel(i);
         break;
      }
   }

   m_DuctGrid.OnStrandChanged();
}

void CEditGirderlineDlg::OnDuctChanged()
{
   m_DrawTendons.Invalidate();
   m_DrawTendons.UpdateWindow();
}


int CEditGirderlineDlg::GetDuctCount()
{
   return m_DuctGrid.GetRowCount();
}

void CEditGirderlineDlg::OnHelp()
{
#pragma Reminder("IMPLEMENT")
   AfxMessageBox(_T("Add Help topic"));
}
