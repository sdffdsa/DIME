//
//
// Derived from Microsoft Sample IME by Jeremy '13,7,17
//
//



#pragma once

#include "File.h"
#include "DictionaryParser.h"
#include "TSFDayiBaseStructure.h"

class CDictionaryResult;

//////////////////////////////////////////////////////////////////////
//
// CDictionarySearch declaration.
//
//////////////////////////////////////////////////////////////////////

class CDictionarySearch : CDictionaryParser
{
public:
    CDictionarySearch(LCID locale, _In_ CFile *pFile, _In_ CStringRange *pSearchKeyCode);
    virtual ~CDictionarySearch();

    BOOL FindPhrase(_Out_ CDictionaryResult **ppdret);
    BOOL FindPhraseForWildcard(_Out_ CDictionaryResult **ppdret);
	BOOL FindConvertedString(CDictionaryResult **ppdret);
    BOOL FindConvertedStringForWildcard(CDictionaryResult **ppdret);
	BOOL ParseConfig(); 

    CStringRange* _pSearchKeyCode;

    DWORD_PTR _charIndex;      // in character. Always point start of line in dictionary file.

private:
    BOOL FindWorker(BOOL isTextSearch, _Out_ CDictionaryResult **ppdret, BOOL isWildcardSearch, _In_opt_ BOOL parseConfig = FALSE);

    DWORD_PTR GetBufferInWCharLength()
    {
        return (_pFile->GetFileSize() / sizeof(WCHAR)) - _charIndex;     // in char count as a returned length.
    }

    const WCHAR* GetBufferInWChar()
    {
        return _pFile->GetReadBufferPointer() + _charIndex;
    }

    CFile* _pFile;
	enum SEARCH_MODE searchMode;
};

//////////////////////////////////////////////////////////////////////
//
// CDictionaryResult declaration.
//
//////////////////////////////////////////////////////////////////////

class CDictionaryResult
{
public:
    CDictionaryResult() { }
    virtual ~CDictionaryResult() { }

    CDictionaryResult& operator=(CDictionaryResult& dret)
    {
        _FindKeyCode = dret._FindKeyCode;
        _FindPhraseList = dret._FindPhraseList;
        return *this;
    }

    CStringRange _SearchKeyCode;
    CStringRange _FindKeyCode;
    CTSFDayiArray<CStringRange> _FindPhraseList;
};

enum SEARCH_MODE
{
	SEARCH_NONE,
	SEARCH_MAPPING,
	SEARCH_MAPPING_WILRDCARD,
	SEARCH_TEXT,
	SEARCH_TEXT_WILRDCARD,
	SEARCH_TEXT_TTS_PHRASE,
	SEARCH_RADICAL,
	SEARCH_CONFIG,
	SEARCH_CONTROLKEY
};
enum CONTROLKEY_TYPE
{
	NOT_CONTROLKEY,
	CIN_CONTROLKEY,
	TTS_CONTROLKEY
};