//#define DEBUG_PRINT

#include "Private.h"
#include "ReverseConversionProvider.h"
#include "DIME.h"


//---------------------------------------------------------------------------------
//ITfReverseConversionMgr::GetReverseConversion()
//  Reverse conversion COM provider for other TSF IM to lookup key sequence from us.
//---------------------------------------------------------------------------------
HRESULT CDIME::GetReverseConversion(_In_ LANGID langid, _In_   REFGUID guidProfile, _In_ DWORD dwflag, _Out_ ITfReverseConversion **ppReverseConversion)
{
	debugPrint(L"CDIME(ITfReverseConversionMgr)::GetReverseConversion() langid = %d, giudProfile = %d, dwflag = %d", langid, guidProfile, dwflag);
	if(_pCompositionProcessorEngine) return E_FAIL;
	IME_MODE imeMode = _pCompositionProcessorEngine->GetImeModeFromGuidProfile(guidProfile);

	if(imeMode == IME_MODE_NONE)
		return E_NOTIMPL;

	if (_pCompositionProcessorEngine == nullptr)
    {
		debugPrint(L"CDIME::_AddTextProcessorEngine() create new CompositionProcessorEngine .");
        _pCompositionProcessorEngine = new (std::nothrow) CCompositionProcessorEngine(this);
    }
	if (_pCompositionProcessorEngine == nullptr)
	{
		debugPrint(L"CDIME(ITfReverseConversionMgr)::GetReverseConversion(); no valid compositionEngine,  return E_NOTIMPL");
		return E_OUTOFMEMORY;
	}
	
	_pCompositionProcessorEngine->SetupDictionaryFile(imeMode);

	if(_pReverseConversion[imeMode] == nullptr)
	{
		_pReverseConversion[imeMode] =  new (std::nothrow) CReverseConversion(_pCompositionProcessorEngine, imeMode);
	}
	

	if(_pReverseConversion[imeMode])
	{
		*ppReverseConversion = _pReverseConversion[imeMode];
		debugPrint(L"CDIME(ITfReverseConversionMgr)::GetReverseConversion(); ppReverseConversion ready,  return S_OK");
		return S_OK;
	}
	else
	{
		*ppReverseConversion = nullptr;
		debugPrint(L"CDIME(ITfReverseConversionMgr)::GetReverseConversion(); no valid ppReverseConversion,  return E_NOTIMPL");
		return E_NOTIMPL;
	}
}


CReverseConversion::CReverseConversion(CCompositionProcessorEngine* pCompositionProcessorEngine, IME_MODE imeMode)
{
	debugPrint(L"CReverseConversion(ITfReverseConversion)::CReverseConversion() constructor");
	_refCount = 1;
	_pReverseConversionList = nullptr;
	_pCompositionProcessorEngine = pCompositionProcessorEngine;
	_imeMode = imeMode;
	_pRadicalMap = _pCompositionProcessorEngine->GetRadicalMap(_imeMode);

	DllAddRef();
}
CReverseConversion::~CReverseConversion()
{
	debugPrint(L"CReverseConversion(ITfReverseConversion)::~CReverseConversion() destructor");
	if(_pReverseConversionList)
	{
		_pReverseConversionList->Release();
		_pReverseConversionList = nullptr;
	}
	if(_pCompositionProcessorEngine)
	{
		_pCompositionProcessorEngine = nullptr;
	}
	DllRelease();
}

//ITfReverseConversion
HRESULT CReverseConversion::DoReverseConversion(_In_ LPCWSTR lpstrToConvert, _Out_ ITfReverseConversionList **ppList)
{

	debugPrint(L"CReverseConversion(ITfReverseConversion)::DoReverseConversion() strint to conver = %s", lpstrToConvert);
	HRESULT hr = S_FALSE;
	if(_pReverseConversionList == nullptr)
	{	
		_pReverseConversionList = new (std::nothrow) CReverseConversionList(_imeMode, _pRadicalMap); 
		if(_pReverseConversionList == nullptr)	
			return S_FALSE;		
	}

	_pReverseConversionList->AddRef();
	*ppList = _pReverseConversionList;
	if(_pCompositionProcessorEngine == nullptr) return E_FAIL;
	CDIMEArray<CCandidateListItem> candidateList;
	hr = _pCompositionProcessorEngine->GetReverConversionResults(_imeMode, lpstrToConvert, &candidateList);
	if(SUCCEEDED(hr) && _pReverseConversionList)
		_pReverseConversionList->SetResultList(&candidateList);
	return hr;
}

STDAPI CReverseConversion::QueryInterface(REFIID riid, _Outptr_ void **ppvObj)
{
	debugPrint(L"CReverseConversion(ITfReverseConversion)::QueryInterface() ");
    if (ppvObj == nullptr)
    {
        return E_INVALIDARG;
    }

    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfReverseConversion))
    {
        *ppvObj = (ITfReverseConversion *)this;
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CReverseConversion::AddRef()
{
	debugPrint(L"CReverseConversion(ITfReverseConversion)::AddRef() _refCount = %d ", _refCount+1);
    return ++_refCount;
}

STDAPI_(ULONG) CReverseConversion::Release()
{
	debugPrint(L"CReverseConversion(ITfReverseConversion)::Release() _refCount = %d ", _refCount-1);
    LONG cr = --_refCount;

    assert(_refCount >= 0);

    if (_refCount == 0)
    {
        delete this;
    }

    return cr;
}

CReverseConversionList::CReverseConversionList(IME_MODE imeMode, _T_RacialMap* pRadicalMap)
{
	debugPrint(L"CReverseConversionList(ITfReverseConversionList)::CReverseConversionList() constructor, imeMode = %d", imeMode);
	_refCount = 1;
	_resultFound = FALSE;
	_resultString = new (std::nothrow) WCHAR[REVERSE_CONV_RESULT_LENGTH];
	_imeMode = imeMode;
	_pRadicalMap = pRadicalMap;

}
CReverseConversionList::~CReverseConversionList()
{
	debugPrint(L"CReverseConversionList(ITfReverseConversionList)::~CReverseConversionList() destructor");
	delete [] _resultString;
}

STDAPI CReverseConversionList::QueryInterface(REFIID riid, _Outptr_ void **ppvObj)
{
	debugPrint(L"CReverseConversionList(ITfReverseConversionList)::QueryInterface() ");
    if (ppvObj == nullptr)
    {
        return E_INVALIDARG;
    }

    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfReverseConversionList))
    {
        *ppvObj = (ITfReverseConversionList *)this;
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CReverseConversionList::AddRef()
{
	debugPrint(L"CReverseConversionList::AddRef() _refCount = %d ", _refCount+1);
    return ++_refCount;
}

STDAPI_(ULONG) CReverseConversionList::Release()
{
	debugPrint(L"CReverseConversionList::Release() _refCount = %d ", _refCount-1);
    LONG cr = --_refCount;

    assert(_refCount >= 0);

    if (_refCount == 0)
    {
        delete this;
    }

    return cr;
}

HRESULT CReverseConversionList::GetLength(__RPC__out UINT *puIndex)
{
	if(_resultFound)
		*puIndex = 1;
	else
		*puIndex = 0;
	debugPrint(L"CReverseConversionList(ITfReverseConversionList)::GetLength() puIndex = %d", *puIndex);
	return S_OK;
}
HRESULT CReverseConversionList::GetString(UINT uIndex, __RPC__deref_out_opt BSTR *pbstr)
{
	debugPrint(L"CReverseConversionList(ITfReverseConversionList)::GetString() uIndex = %d", uIndex);
	
	if(!_resultFound) return S_OK;

	*pbstr = SysAllocString(_resultString);

	if (pbstr == NULL)
		return S_FALSE;
	else
		return S_OK;
}

void CReverseConversionList::SetResultList(CDIMEArray<CCandidateListItem>* pCandidateList)
{
	debugPrint(L"CReverseConversionList(ITfReverseConversionList)::SetResultList()");
	_resultFound = TRUE;
	
	if(pCandidateList == nullptr || pCandidateList->Count() == 0) 
	{
		_resultFound = FALSE;
		return;
	}
	*_resultString = L'\0';
	for (UINT index = 0; index < pCandidateList->Count(); index++)
    {
		if(pCandidateList->GetAt(index)->_FindKeyCode.GetLength() && _pRadicalMap->size())
		{
			if(index)
			{
				assert(wcslen(_resultString) + 2 < REVERSE_CONV_RESULT_LENGTH-1);
				StringCchCat(_resultString, REVERSE_CONV_RESULT_LENGTH, L"|"); 
			}
			for(UINT i=0; i <pCandidateList->GetAt(index)->_FindKeyCode.GetLength(); i++)
			{ // query keyname from keymap
				map<WCHAR, PWCH>::iterator item = 
					_pRadicalMap->find(towupper(*(pCandidateList->GetAt(index)->_FindKeyCode.Get() + i)));
				if(item != _pRadicalMap->end() )
				{
					assert(wcslen(_resultString) + 1 < REVERSE_CONV_RESULT_LENGTH-1);
					StringCchCat(_resultString, REVERSE_CONV_RESULT_LENGTH, item->second); 
				}
			}
		}
		else
		{
			if(index)
			{
				assert(wcslen(_resultString) + 2 < REVERSE_CONV_RESULT_LENGTH-1);
				StringCchCat(_resultString, 256, L", "); 
			}
			assert(wcslen(_resultString) + pCandidateList->GetAt(index)->_FindKeyCode.GetLength() < REVERSE_CONV_RESULT_LENGTH-1);
			StringCchCatN(_resultString, 256, pCandidateList->GetAt(index)->_FindKeyCode.Get(), pCandidateList->GetAt(index)->_FindKeyCode.GetLength());
		}
    }
}


void CDIME::ReleaseReverseConversion()
{
 
	for (UINT i =0 ; i<5 ; i++)
	{
		if(_pReverseConversion[i])
		{
			delete _pReverseConversion[i];
			_pReverseConversion[i] = nullptr;
		}
		if(_pITfReverseConversion[i])
		{
			_pITfReverseConversion[i]->Release();
			_pITfReverseConversion[i] = nullptr;
		}
	}

}
