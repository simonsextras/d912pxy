/*
MIT License

Copyright(c) 2020 megai2

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#include "stdafx.h"

using namespace d912pxy;
using namespace d912pxy::extras::ShaderPair;

void InfoStorage::readFData(MemoryBlock& mb)
{
	Info newElem = {};

	wchar_t* cur = mb.c_arr<wchar_t>();

	if (*cur != 0xFEFF)
		return;
	else
		++cur;

	wchar_t* varr[4] = {};
	varr[0] = cur;
	int curVal = 0;
	bool skipNextDelimiter=false;

	while (cur < mb.end<wchar_t>())
	{
		if ((*cur == L'=') || (*cur == L'\n') || (*cur == L'\r'))
		{
			if (skipNextDelimiter)
			{
				++cur;
				varr[curVal] = cur;
				continue;
			}
			*cur = 0;
			++curVal;
			varr[curVal] = cur + 1;
			skipNextDelimiter = true;
		}
		else
			skipNextDelimiter = false;

		if (curVal == 3)
		{
			newElem.drawType =
				(lstrcmpW(varr[1], L"simple") == 0) ? DrawType::simple :
				((lstrcmpW(varr[1], L"pass_start") == 0) ? DrawType::pass_start : DrawType::pass_end);

			newElem.name = d912pxy_helper::strdupw(varr[2]);
			swscanf(&varr[0][0], L"%llX", &newElem.spair);

			if (storage.contains(newElem.spair))
			{
				Info& oldv = storage[newElem.spair];
				if (oldv.name)
					PXY_FREE(oldv.name);
				oldv = newElem;
			}
			else
				storage[newElem.spair] = newElem;

			varr[0] = varr[3];
			curVal = 0;
		}

		++cur;
	}
}

InfoStorage::InfoStorage()
{
}

InfoStorage::~InfoStorage()
{
}

void InfoStorage::Init()
{
	NonCom_Init(L"spair info storage");
	reload();
}

Info& InfoStorage::find(d912pxy_shader_pair_hash_type pair)
{
	Info& ret = storage[pair];
	if (!ret.spair)
		ret.spair = pair;

	return ret;
}

void InfoStorage::reload()
{
	//TODO: move tons of this code to shared/helper
	WIN32_FIND_DATA fdFile;
	HANDLE hFind = NULL;

	wchar_t sPath[2048];
	wsprintf(sPath, L"%s%s/*", d912pxy_helper::GetFilePath(FP_SPAIR_INFO_BASE_PATH)->w, d912pxy_s.config.GetValueRaw(PXY_CFG_EXTRAS_SPAIR_SOURCE));

	LOG_INFO_DTDM("Loading spair info from %s", sPath);

	if ((hFind = FindFirstFile(sPath, &fdFile)) == INVALID_HANDLE_VALUE)
	{
		LOG_INFO_DTDM("No spair data loaded");
		return;
	}

	do
	{
		if (wcscmp(fdFile.cFileName, L".") != 0
			&& wcscmp(fdFile.cFileName, L"..") != 0)
		{
			if (fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

			wchar_t fPath[2048];
			wsprintf(fPath, L"%s%s/%s", d912pxy_helper::GetFilePath(FP_SPAIR_INFO_BASE_PATH)->w, d912pxy_s.config.GetValueRaw(PXY_CFG_EXTRAS_SPAIR_SOURCE), fdFile.cFileName);

			FILE* f = _wfopen(fPath, L"rb+");

			if (!f)
				continue;

			fseek(f, 0, SEEK_END);
			int fsz = ftell(f);
			fseek(f, 0, SEEK_SET);

			if (!fsz)
			{
				fclose(f);
				continue;
			}

			MemoryBlock fdata(fsz);
			fread(fdata.getPtr(), 1, fsz, f);
			readFData(fdata);

			fclose(f);
		}
	} while (FindNextFile(hFind, &fdFile));
}