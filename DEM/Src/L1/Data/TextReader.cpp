#include "TextReader.h"

namespace Data
{

//!!!TEST different cases, especially last line reading!
bool CTextReader::ReadLine(char* pOutValue, DWORD MaxLen)
{
	const int READ_BLOCK_SIZE = 64;

	if (Stream.IsEOF() || MaxLen < 2) FAIL;

	DWORD StartPos = Stream.GetPosition();

	char* pCurr = pOutValue;
	char* pEnd = pOutValue + MaxLen - 1;
	do
	{
		int BytesToRead = pEnd - pCurr;
		if (BytesToRead > READ_BLOCK_SIZE) BytesToRead = READ_BLOCK_SIZE;
		BytesToRead = Stream.Read(pCurr, BytesToRead);

		while (BytesToRead > 0)
		{
			if (*pCurr == '\n' || *pCurr == '\r')
			{
				char Pair = (*pCurr == '\n') ? '\r' : '\n';
				*pCurr = 0;
				++pCurr;
				DWORD SeekTo = StartPos + (pCurr - pOutValue);
				if (*pCurr == Pair)
					++SeekTo;
				Stream.Seek(SeekTo, SSO_BEGIN);
				OK;
			}
			++pCurr;
			--BytesToRead;
		}
	}
	while (pCurr != pEnd && !Stream.IsEOF());

	*pEnd = 0;

	OK; // Else EOF is reached
}
//---------------------------------------------------------------------

} //namespace Data