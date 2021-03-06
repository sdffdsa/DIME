//
//
// Derived from Microsoft Sample IME by Jeremy '13,7,17
//
//
//#define DEBUG_PRINT

#include "Private.h"
#include "Globals.h"
#include "DIME.h"
#include "CompositionProcessorEngine.h"
#include "UIPresenter.h"
#include "GetTextExtentEditSession.h"

//+---------------------------------------------------------------------------
//
// ITfCompositionSink::OnCompositionTerminated
//
// Callback for ITfCompositionSink.  The system calls this method whenever
// someone other than this service ends a composition.
//----------------------------------------------------------------------------

STDAPI CDIME::OnCompositionTerminated(TfEditCookie ecWrite, _In_ ITfComposition *pComposition)
{
	debugPrint(L"CDIME::OnCompositionTerminated()");
	HRESULT hr = S_OK;
    ITfContext* pContext = _pContext;
   
	// Clear dummy composition
	_RemoveDummyCompositionForComposing(ecWrite, pComposition);
	
    // Clear display attribute and end composition, _EndComposition will release composition for us
     if (pContext)
    {
        pContext->AddRef();
	}
	
	_EndComposition(pContext);
	_DeleteCandidateList(TRUE, pContext);
	
	if (pContext)
	{
		pContext->Release();
        pContext = nullptr;
    }
    return hr;
}


//+---------------------------------------------------------------------------
//
// _IsComposing
//
//----------------------------------------------------------------------------

BOOL CDIME::_IsComposing()
{
    return _pComposition != nullptr;
}

//+---------------------------------------------------------------------------
//
// _SetComposition
//
//----------------------------------------------------------------------------

void CDIME::_SetComposition(_In_ ITfComposition *pComposition)
{
    _pComposition = pComposition;
}

//+---------------------------------------------------------------------------
//
// _AddComposingAndChar
//
//----------------------------------------------------------------------------

HRESULT CDIME::_AddComposingAndChar(TfEditCookie ec, _In_ ITfContext *pContext, _In_ CStringRange *pstrAddString)
{
	debugPrint(L"CDIME::_AddComposingAndChar()");
    HRESULT hr = S_OK;

    ULONG fetched = 0;
    TF_SELECTION tfSelection;

    if (pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &fetched) != S_OK || fetched == 0)
        return S_FALSE;

    //
    // make range start to selection
    //
    ITfRange* pAheadSelection = nullptr;
    hr = pContext->GetStart(ec, &pAheadSelection);
    if (SUCCEEDED(hr) && pAheadSelection)
    {
        hr = pAheadSelection->ShiftEndToRange(ec, tfSelection.range, TF_ANCHOR_START);
        if (SUCCEEDED(hr))
        {
            ITfRange* pRange = nullptr;
            BOOL exist_composing = _FindComposingRange(ec, pContext, pAheadSelection, &pRange);

            _SetInputString(ec, pContext, pRange, pstrAddString, exist_composing);

            if (pRange)
            {
                pRange->Release();
            }
        }
    }

    tfSelection.range->Release();

    if (pAheadSelection)
    {
        pAheadSelection->Release();
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _AddCharAndFinalize
//
//----------------------------------------------------------------------------

HRESULT CDIME::_AddCharAndFinalize(TfEditCookie ec, _In_ ITfContext *pContext, _In_ CStringRange *pstrAddString)
{
	debugPrint(L"CDIME::_AddCharAndFinalize()");
    HRESULT hr = E_FAIL;

    ULONG fetched = 0;
    TF_SELECTION tfSelection;

	if ((hr = pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &fetched)) != S_OK || fetched != 1 || tfSelection.range==nullptr || pstrAddString ==nullptr)
        return hr;

    // we use SetText here instead of InsertTextAtSelection because we've already started a composition
    // we don't want the app to adjust the insertion point inside our composition
    hr = tfSelection.range->SetText(ec, 0, pstrAddString->Get(), (LONG)pstrAddString->GetLength());
    if (hr == S_OK)
    {
        // update the selection, we'll make it an insertion point just past
        // the inserted text.
        tfSelection.range->Collapse(ec, TF_ANCHOR_END);
		if(pContext)
			pContext->SetSelection(ec, 1, &tfSelection);
    }

    tfSelection.range->Release();

    return hr;
}

//+---------------------------------------------------------------------------
//
// _FindComposingRange
//
//----------------------------------------------------------------------------

BOOL CDIME::_FindComposingRange(TfEditCookie ec, _In_ ITfContext *pContext, _In_ ITfRange *pSelection, _Outptr_result_maybenull_ ITfRange **ppRange)
{
	debugPrint(L"CDIME::_FindComposingRange()");

    if (pContext == nullptr || ppRange == nullptr)
    {
        return FALSE;
    }

    *ppRange = nullptr;

    // find GUID_PROP_COMPOSING
    ITfProperty* pPropComp = nullptr;
    IEnumTfRanges* enumComp = nullptr;

    HRESULT hr = pContext->GetProperty(GUID_PROP_COMPOSING, &pPropComp);
    if (FAILED(hr) || pPropComp == nullptr)
    {
        return FALSE;
    }

    hr = pPropComp->EnumRanges(ec, &enumComp, pSelection);
    if (FAILED(hr) || enumComp == nullptr)
    {
        pPropComp->Release();
        return FALSE;
    }

    BOOL isCompExist = FALSE;
    VARIANT var;
    ULONG  fetched = 0;

    while (enumComp->Next(1, ppRange, &fetched) == S_OK && fetched == 1)
    {
        hr = pPropComp->GetValue(ec, *ppRange, &var);
        if (hr == S_OK)
        {
            if (var.vt == VT_I4 && var.lVal != 0)
            {
                isCompExist = TRUE;
                break;
            }
        }
        if(*ppRange)
			(*ppRange)->Release();
        *ppRange = nullptr;
    }

    pPropComp->Release();
    enumComp->Release();

    return isCompExist;
}

//+---------------------------------------------------------------------------
//
// _SetInputString
//
//----------------------------------------------------------------------------

HRESULT CDIME::_SetInputString(TfEditCookie ec, _In_ ITfContext *pContext, _Out_opt_ ITfRange *pRange, _In_ CStringRange *pstrAddString, BOOL exist_composing)
{

    ITfRange* pRangeInsert = nullptr;
    if (!exist_composing)
    {
        _InsertAtSelection(ec, pContext, pstrAddString, &pRangeInsert);
        if (pRangeInsert == nullptr)
        {
            return S_OK;
        }
        pRange = pRangeInsert;
    }
    if (pRange != nullptr)
    {
        pRange->SetText(ec, 0, pstrAddString->Get(), (LONG)pstrAddString->GetLength());
    }

    _SetCompositionLanguage(ec, pContext);

	_SetCompositionDisplayAttributes(ec, pContext, _gaDisplayAttributeConverted);// _gaDisplayAttributeInput);

    // update the selection, we'll make it an insertion point just past
    // the inserted text.
    ITfRange* pSelection = nullptr;
    TF_SELECTION sel;

    if ((pRange != nullptr) && (pRange->Clone(&pSelection) == S_OK))
    {
        pSelection->Collapse(ec, TF_ANCHOR_END);

        sel.range = pSelection;
        sel.style.ase = TF_AE_NONE;
        sel.style.fInterimChar = FALSE;
        pContext->SetSelection(ec, 1, &sel);
        pSelection->Release();
    }

    if (pRangeInsert)
    {
        pRangeInsert->Release();
    }


    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _InsertAtSelection
//
//----------------------------------------------------------------------------

HRESULT CDIME::_InsertAtSelection(TfEditCookie ec, _In_ ITfContext *pContext, _In_ CStringRange *pstrAddString, _Outptr_ ITfRange **ppCompRange)
{
    ITfRange* rangeInsert = nullptr;
    ITfInsertAtSelection* pias = nullptr;
    HRESULT hr = S_OK;

    if (ppCompRange == nullptr || pContext == nullptr || pstrAddString == nullptr)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppCompRange = nullptr;

    hr = pContext->QueryInterface(IID_ITfInsertAtSelection, (void **)&pias);
    if (FAILED(hr) || pias == nullptr)
    {
        goto Exit;
    }

    hr = pias->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, pstrAddString->Get(), (LONG)pstrAddString->GetLength(), &rangeInsert);

    if ( FAILED(hr) || rangeInsert == nullptr)
    {
        rangeInsert = nullptr;
        pias->Release();
        goto Exit;
    }

    *ppCompRange = rangeInsert;
    pias->Release();
    hr = S_OK;

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
// _RemoveDummyCompositionForComposing
//
//----------------------------------------------------------------------------

HRESULT CDIME::_RemoveDummyCompositionForComposing
	(TfEditCookie ec, _In_ ITfComposition *pComposition)
{
	debugPrint(L"CDIME::_RemoveDummyCompositionForComposing()\n");
    HRESULT hr = S_OK;

    ITfRange* pRange = nullptr;
    
    if (pComposition)
    {
        hr = pComposition->GetRange(&pRange);
        if (SUCCEEDED(hr) && pRange)
        {
            pRange->SetText(ec, 0, nullptr, 0);
            pRange->Release();
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// _SetCompositionLanguage
//
//----------------------------------------------------------------------------

BOOL CDIME::_SetCompositionLanguage(TfEditCookie ec, _In_ ITfContext *pContext)
{
    HRESULT hr = S_OK;
    BOOL ret = TRUE;

	if (pContext == nullptr || _pComposition == nullptr)
    {
		ret = FALSE;
        goto Exit;
    }

    ITfRange* pRangeComposition = nullptr;
    ITfProperty* pLanguageProperty = nullptr;

    // we need a range and the context it lives in
    hr = _pComposition->GetRange(&pRangeComposition);
    if (FAILED(hr) || pRangeComposition == nullptr)
    {
        ret = FALSE;
        goto Exit;
    }

    // get our the language property
    hr = pContext->GetProperty(GUID_PROP_LANGID, &pLanguageProperty);
    if (FAILED(hr) || pLanguageProperty == nullptr)
    {
        ret = FALSE;
        goto Exit;
    }

    VARIANT var;
    var.vt = VT_I4;   // we're going to set DWORD
    var.lVal = _langid; 

    hr = pLanguageProperty->SetValue(ec, pRangeComposition, &var);
    if (FAILED(hr) || pRangeComposition == nullptr)
    {
        ret = FALSE;
        goto Exit;
    }

    pLanguageProperty->Release();
    pRangeComposition->Release();

Exit:
    return ret;
}


//+---------------------------------------------------------------------------
//
// CProbeComposistionEditSession
//
//----------------------------------------------------------------------------

class CProbeComposistionEditSession : public CEditSessionBase
{
public:
    CProbeComposistionEditSession(_In_ CDIME *pTextService, _In_ ITfContext *pContext) : CEditSessionBase(pTextService, pContext)
    {
    }

    // ITfEditSession
    STDMETHODIMP DoEditSession(TfEditCookie ec);
};

//+---------------------------------------------------------------------------
//
// ITfEditSession::DoEditSession
//
//----------------------------------------------------------------------------

STDAPI CProbeComposistionEditSession::DoEditSession(TfEditCookie ec)
{
	debugPrint(L"CProbeComposistionEditSession::DoEditSession()\n");
	if(_pTextService) 
		_pTextService->_ProbeCompositionRangeNotification(ec, _pContext);
	
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
//
// CDIME class
//
//////////////////////////////////////////////////////////////////////+---------------------------------------------------------------------------
//
// _ProbeComposition
//
// starts a new (std::nothrow) pProbeComposistionEditSession at the selection of the current 
// focus context to get correct caret position.
//----------------------------------------------------------------------------

void CDIME::_ProbeComposition(_In_ ITfContext *pContext)
{
	debugPrint(L"CDIME::_ProbeComposition() pContext = %x\n", pContext);


	CProbeComposistionEditSession* pProbeComposistionEditSession = new (std::nothrow) CProbeComposistionEditSession(this, pContext);

	if (pProbeComposistionEditSession && pContext)
	{
		
		HRESULT hrES = S_OK, hr = S_OK;
		hr = pContext->RequestEditSession(_tfClientId, pProbeComposistionEditSession, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hrES);
		
		debugPrint(L"CDIME::_ProbeComposition() RequestEdisession HRESULT = %x, return HRESULT  = %x\n", hrES, hr );

		pProbeComposistionEditSession->Release();
	}
	
   
}

HRESULT CDIME::_ProbeCompositionRangeNotification(_In_ TfEditCookie ec, _In_ ITfContext *pContext)
{
	debugPrint(L"CDIME::_ProbeCompositionRangeNotification(), \n");


	HRESULT hr = S_OK;
	if(!_IsComposing())
		_StartComposition(pContext);
	
	hr = E_FAIL;
    ULONG fetched = 0;
    TF_SELECTION tfSelection;

    if ( pContext == nullptr || (hr = pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &fetched)) != S_OK || fetched != 1 ||tfSelection.range==nullptr)
	{
		_TerminateComposition(ec,pContext);
        return hr;
	}
	tfSelection.range->Release();
   
	
	ITfRange *pRange;
	ITfContextView* pContextView;
	ITfDocumentMgr* pDocumgr;
	if (_pComposition&&SUCCEEDED(_pComposition->GetRange(&pRange)) && pRange)
	{
		if(pContext && SUCCEEDED(pContext->GetActiveView(&pContextView)) && pContextView)
		{
			if(pContext && SUCCEEDED( pContext->GetDocumentMgr(&pDocumgr)) && pDocumgr && _pThreadMgr &&_pUIPresenter)
			{
				ITfDocumentMgr* pFocusDocuMgr;
				_pThreadMgr->GetFocus(&pFocusDocuMgr);
				if(pFocusDocuMgr == pDocumgr)
				{
					_pUIPresenter->_StartLayout(pContext, ec, pRange);
					_pUIPresenter->_MoveUIWindowsToTextExt();
				}
				else 
					_pUIPresenter->ClearNotify();

				pDocumgr->Release();
			}
		pContextView->Release();
		}
		pRange->Release();
	}


    
	_TerminateComposition(ec,pContext);
	return hr;
}
