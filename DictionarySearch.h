//
//
// Derived from Microsoft Sample IME by Jeremy '13,7,17
//
//
#ifndef DICTIONARYSEARCH_H
#define DICTIONARYSEARCH_H


#pragma once

#include "File.h"
#include "DictionaryParser.h"
#include "BaseStructure.h"

class CDictionaryResult;

//////////////////////////////////////////////////////////////////////
//
// CDictionarySearch declaration.
//
//////////////////////////////////////////////////////////////////////

class CDIME;
class CDictionarySearch : CDictionaryParser
{
public:
    CDictionarySearch(LCID locale, _In_ CFile *pFile, _In_ CStringRange *pSearchKeyCode, _In_ WCHAR keywordDelimiter);
    virtual ~CDictionarySearch();

    BOOL FindPhrase(_Out_ CDictionaryResult **ppdret);
    BOOL FindPhraseForWildcard(_Out_ CDictionaryResult **ppdret);
	BOOL FindConvertedString(CDictionaryResult **ppdret);
    BOOL FindConvertedStringForWildcard(CDictionaryResult **ppdret);
	BOOL ParseConfig(IME_MODE imeMode, _In_opt_ _T_RacialMap* pRadicalMap); 
	VOID SetSearchSection(SEARCH_SECTION searchSection) { _searchSection =searchSection;}

    CStringRange* _pSearchKeyCode;

    DWORD_PTR _charIndex;      // in character. Always point start of line in dictionary file.

private:
	SEARCH_SECTION _searchSection;
	BOOL FindWorker(BOOL isTextSearch, _Out_ CDictionaryResult **ppdret, BOOL isWildcardSearch, _In_opt_ BOOL parseConfig = FALSE, _In_opt_ _T_RacialMap* pRadicalMap = nullptr);

    DWORD_PTR GetBufferInWCharLength()
    {
        return (_pFile->GetFileSize() / sizeof(WCHAR)) - _charIndex;     // in char count as a returned length.
    }

    const WCHAR* GetBufferInWChar(BOOL *fileReloaded)
    {
        return _pFile->GetReadBufferPointer(fileReloaded) + _charIndex;
    }

    CFile* _pFile;
	enum SEARCH_MODE _searchMode;
	CDIME *_pTextService;
	IME_MODE _imeMode;

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
    CDIMEArray<CStringRange> _FindPhraseList;
};

enum SEARCH_MODE
{
	SEARCH_NONE,
	SEARCH_MAPPING,
	SEARCH_TEXT,
	SEARCH_PHRASE,
	SEARCH_RADICAL,
	SEARCH_CONFIG,
	SEARCH_CONTROLKEY,
	SEARCH_SYMBOL, 
	SEARCH_PRHASE_FROM_KEYSTROKE
};
enum CONTROLKEY_TYPE
{
	NOT_CONTROLKEY,
	CIN_CONTROLKEY,
	TTS_CONTROLKEY
};
#endif