///////////////////////////////////////////////////////////////////////////////
// Tests for ResIL API 2, multi-threaded
// Copyright Björn Ganster
// Development startet in 2014
///////////////////////////////////////////////////////////////////////////////

// Prevent stupid MSVC warnings for using basic proven C functions
#ifdef _MSC_VER
#define _SCL_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#endif

#include <Windows.h>
#include <tchar.h>
#include <direct.h>

#include "../include/IL/il2.h"

#include "vector"
using namespace std;

///////////////////////////////////////////////////////////////////////////////

#define PathCharMod "%S"

#ifdef _WIN32
#define PATH_SEPARATOR L"\\"
#else
#define PATH_SEPARATOR "/"
#endif

int errors;

///////////////////////////////////////////////////////////////////////////////

//const TCHAR* sourceDir = L"D:\\testIL\\";
const TCHAR* sourceDir = L"c:\\testIL\\";

//const TCHAR* targetDir = L"D:\\testIL\\decoded\\";
const TCHAR* targetDir = L"c:\\testIL\\decoded\\";

const TCHAR* encodedPath = L"D:\\TestIL\\encoded";

void printOffset(void* a, void* b, char* msg)
{
	size_t s = ((BYTE*)b)-((BYTE*)a);
	printf("%s: %i\n", msg, s);
}

inline void testHeap()
{
   #ifdef _DEBUG
	void* p = malloc(1024);
	free(p);
   #endif
}

///////////////////////////////////////////////////////////////////////////////

void test_ilLoad(wchar_t* sourceFN, wchar_t* targetFN)
{
	testHeap();
	ILimage* handle = il2GenImage();
	testHeap();
	//printf("Loading " PathCharMod "\n", sourceFN);
	il2ResetRead(handle);
	ILenum sourceType = il2DetermineType(sourceFN);
	if (!il2Load(handle, sourceType, sourceFN)) {
		printf("test_ilLoad: Failed to load %S\n", sourceFN);
		++errors;
		return;
	}
	testHeap();
	//ilConvertImage(IL_BGR, IL_UNSIGNED_BYTE);
	//ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
	//iluScale(150, 150, 1);
	testHeap();
	DeleteFile(targetFN);
	//printf("Saving " PathCharMod "\n", targetFN);
	if (!il2SaveImage(handle, targetFN)) {
		printf("test_ilLoad: Failed to save " PathCharMod "\n", targetFN);
		++errors;
	}
	testHeap();
	il2DeleteImage(handle);
}

///////////////////////////////////////////////////////////////////////////////

void test_ilLoadF(wchar_t* sourceFN, wchar_t* targetFN)
{
	testHeap();
	ILimage* image = il2GenImage();
	testHeap();
	//printf("Loading " PathCharMod "\n", sourceFN);
	ILboolean loaded = false;

	//FILE* f = _wfopen(sourceFN, L"rb");
	FILE * f;
	char buf[10];
	_wfopen_s(&f, sourceFN, L"rb");
	fread(buf, 1, 10, f);
	fseek(f, 0, IL_SEEK_SET);
	if (f != NULL) {
		loaded = il2LoadF(image, IL_TYPE_UNKNOWN, f);
		if(!loaded) {
			printf("test_ilLoadF: Failed to load %S\n", sourceFN);
			++errors;
		}
	} else {
		printf("test_ilLoadF: Failed to open %S\n", sourceFN);
		++errors;
	}

	fclose(f);
	testHeap();
	//ilConvertImage(IL_BGR, IL_UNSIGNED_BYTE);
	//ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
	//iluScale(150, 150, 1);
	testHeap();
	DeleteFile(targetFN);
	//printf("Saving " PathCharMod "\n", targetFN);

	if (loaded)
		if (!il2SaveImage(image, targetFN)) {
			printf ("test_ilLoadF: Failed to save %S\n", targetFN);
			++errors;
		}

	testHeap();
	il2DeleteImage(image);
}

///////////////////////////////////////////////////////////////////////////////

void test_ilLoadImage(wchar_t* sourceFN, wchar_t* targetFN)
{
	testHeap();
	ILimage* handle = il2GenImage();
	testHeap();
	//printf("Loading " PathCharMod "\n", sourceFN);
	if (!il2LoadImage(handle, sourceFN)) {
		printf("test_ilLoadImage: Failed to load " PathCharMod "\n", sourceFN);
		++errors;
		return;
	}
	//ilConvertImage(IL_BGR, IL_UNSIGNED_BYTE);
	//ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
	//iluScale(150, 150, 1);
	testHeap();
	DeleteFile(targetFN);
	//printf("Saving " PathCharMod "\n", targetFN);
	if (!il2SaveImage(handle, targetFN)) {
		printf("test_ilLoadImage: Failed to save " PathCharMod "\n", targetFN);
		++errors;
	}
	testHeap();
	il2DeleteImage(handle);
}

///////////////////////////////////////////////////////////////////////////////

void test_ilLoadL(wchar_t* sourceFN, wchar_t* targetFN)
{
	testHeap();
	ILimage* handle = il2GenImage();
	testHeap();
	//printf("Loading " PathCharMod "\n", sourceFN);
	FILE* f = _wfopen(sourceFN, L"rb");
	ILboolean loaded = IL_FALSE;

	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		auto size = ftell(f);
		if (size > 0) {
			fseek(f, 0, SEEK_SET);
			char* lump = new char[size];
			size_t read = fread(lump, 1, size, f);
			if (read == size) {
				loaded = il2LoadL(handle, IL_TYPE_UNKNOWN, lump, read);
				if (!loaded) {
					printf("test_ilLoadL: Failed to load " PathCharMod "\n", sourceFN);
					++errors;
				}
			} else {
				printf("test_ilLoadL: Failed to read " PathCharMod "\n", sourceFN);
				++errors;
			}
			delete lump;
		}
		fclose(f);
	} else {
		printf("test_ilLoadL: Failed to load %S\n", sourceFN);
		++errors;
	}

	testHeap();
	//ilConvertImage(IL_BGR, IL_UNSIGNED_BYTE);
	//ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
	//iluScale(150, 150, 1);
	testHeap();
	DeleteFile(targetFN);
	//printf("Saving " PathCharMod "\n", targetFN);
	if (loaded)
		if (!il2SaveImage(handle, targetFN)) {
			printf("test_ilLoadL: Failed to save " PathCharMod "\n", targetFN);
			++errors;
		}
	testHeap();
	il2DeleteImage(handle);
}

///////////////////////////////////////////////////////////////////////////////
// test ilLoadFuncs

// todo: this data must be stored in a struct, and il2SetRead needs to allow
// to use user-defined structs
/*BYTE* myData;
ILint64 myDataSize, myReadPos;

ILHANDLE  ILAPIENTRY myOpenProc (ILconst_string)
{
	myReadPos = 0;
	return NULL;
}

void ILAPIENTRY myCloseProc(SIO* io)
{
}

ILboolean ILAPIENTRY myEofProc(SIO* io)
{
	if (myReadPos >= myDataSize)
		return true;
	else
		return false;
}

ILint ILAPIENTRY myGetcProc(SIO* io)
{
	if (myReadPos < myDataSize) {
		BYTE retVal = myData[myReadPos];
		++myReadPos;
		return retVal;
	} else 
		return 0;
}

ILint64 ILAPIENTRY myReadProc(SIO* io, void* buf, ILuint size, ILuint count)
{
	size_t toRead = size * count;
	if (myReadPos >= myDataSize) {
		count = 0;
		toRead = 0;
	} else {
		if (myReadPos + toRead >= myDataSize) {
			count = ((myDataSize-myReadPos) / size); // round down to size units
			toRead = count * size;
		}
	}

	if (toRead > 0) {
		memcpy(buf, &myData[myReadPos], toRead);
		myReadPos += toRead;
	}

	return count;
}

ILint64 ILAPIENTRY mySeekProc(SIO* io, ILint64 offset, ILuint mode)
{
	switch(mode) {
	case SEEK_SET:
		myReadPos = offset;
		break;
	case SEEK_CUR:
		myReadPos += offset;
		break;
	case SEEK_END:
		myReadPos = myDataSize + offset;
		break;
	}

	return 0; // success
}

ILint64 ILAPIENTRY myTellProc(SIO* io)
{
	return myReadPos;
}*/

void test_ilLoadFuncs(wchar_t* sourceFN, wchar_t* targetFN)
{
	/*testHeap();
	ILimage* handle = il2GenImage();
	testHeap();
	//printf("Loading " PathCharMod "\n", sourceFN);
	FILE* f = _wfopen(sourceFN, L"rb");
	ILboolean loaded = false;

	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		myDataSize = ftell(f);
		if (myDataSize > 0) {
			fseek(f, 0, SEEK_SET);
			myData = new BYTE[myDataSize];
			size_t read = fread(myData, 1, myDataSize, f);
			myReadPos = 0;
			if (read == myDataSize) {
				//loaded = ilLoadL(IL_TYPE_UNKNOWN, lump, read);
				il2SetRead(handle, myOpenProc, myCloseProc, myEofProc, myGetcProc, myReadProc, mySeekProc, myTellProc);
				loaded = il2LoadFuncs(handle, IL_TYPE_UNKNOWN);
				if (!loaded) {
					printf("test_ilLoadFuncs: Failed to load " PathCharMod "\n", sourceFN);
					++errors;
				}
			} else {
				printf("test_ilLoadFuncs: Failed to read " PathCharMod "\n", sourceFN);
				++errors;
			}
			delete myData;
		}
		fclose(f);
	} else {
		printf("test_ilLoadFuncs: Failed to open %S\n", sourceFN);
		++errors;
	}

	testHeap();
	//ilConvertImage(IL_BGR, IL_UNSIGNED_BYTE);
	//ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
	//iluScale(150, 150, 1);
	testHeap();
	DeleteFile(targetFN);
	//printf("Saving " PathCharMod "\n", targetFN);
	if (loaded)
		if(!il2SaveImage(handle, targetFN)) {
			printf("Failed to save " PathCharMod " after ilLoadFuncs\n", targetFN);
			++errors;
		}
	testHeap();
	il2CloseImage(handle);*/
}

///////////////////////////////////////////////////////////////////////////////

ILboolean testDetermineTypeFromContent(TCHAR* fn)
{
/*	testHeap();
	IlImageExtern* image = il2GenImage();
	testHeap();
	//printf("Loading " PathCharMod "\n", sourceFN);
	FILE* f = _wfopen(fn, L"rb");
	bool loaded = false;
	ILenum type = IL_TYPE_UNKNOWN;

	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		myDataSize = ftell(f);
		if (myDataSize > 0) {
			fseek(f, 0, SEEK_SET);
			myData = new BYTE[myDataSize];
			size_t read = fread(myData, 1, myDataSize, f);
			myReadPos = 0;
			if (read == myDataSize) {
				//loaded = ilLoadL(IL_TYPE_UNKNOWN, lump, read);
				il2SetRead(image, myOpenProc, myCloseProc, myEofProc, myGetcProc, myReadProc, mySeekProc, myTellProc);
				type = il2DetermineTypeFuncs(image);
				if (type == IL_TYPE_UNKNOWN) {
					printf("testDetermineTypeFromContent: Failed to determine type of " PathCharMod "\n", fn);
					++errors;
				}
			} else {
				printf("testDetermineTypeFromContent: Failed to read " PathCharMod "\n", fn);
				++errors;
			}
			delete myData;
		}
		fclose(f);
	} else {
		printf("testDetermineTypeFromContent: Failed to open %S\n", fn);
		++errors;
	}

	testHeap();
	il2CloseImage(image);

	if (type == IL_TYPE_UNKNOWN) {
		++errors;
		return IL_FALSE;
	} else*/
		return IL_TRUE;
}

///////////////////////////////////////////////////////////////////////////////

void testLoaders(TCHAR* fn)
{
	TCHAR sourceFN[MAX_PATH];
	TCHAR targetFN[MAX_PATH];

	_tcscpy(sourceFN, sourceDir);
	_tcscat(sourceFN, fn);

	_tcscpy(targetFN, targetDir);
	_tcscat(targetFN, fn);
	_tcscat(targetFN, L".ilLoadImage.bmp");
	test_ilLoadImage(sourceFN, targetFN);

	_tcscpy(targetFN, targetDir);
	_tcscat(targetFN, fn);
	_tcscat(targetFN, L".ilLoad.bmp");
	test_ilLoad(sourceFN, targetFN);

	// ilLoadF, ilLoadL and ilLoadFuncs can work only if the type of the file can be determined
	// correctly from the contents without knowing the extension, so test that before running
	// other checks
	if (testDetermineTypeFromContent(sourceFN)) {
		_tcscpy(targetFN, targetDir);
		_tcscat(targetFN, fn);
		_tcscat(targetFN, L".ilLoadF.bmp");
		test_ilLoadF(sourceFN, targetFN);

		_tcscpy(targetFN, targetDir);
		_tcscat(targetFN, fn);
		_tcscat(targetFN, L".ilLoadL.bmp");
		test_ilLoadL(sourceFN, targetFN);

		_tcscpy(targetFN, targetDir);
		_tcscat(targetFN, fn);
		_tcscat(targetFN, L".ilLoadFuncs.bmp");
		test_ilLoadFuncs(sourceFN, targetFN);
	} else {
		// Error message printed in testDetermineTypeFromContent
		//printf("Type check failed for " PathCharMod "\n", fn);
	}
}

///////////////////////////////////////////////////////////////////////////////

FILE * writeFile;

ILint ILAPIENTRY myPutc  (ILubyte b, SIO* io)
{
	return fputc(b, writeFile);
}

ILint64 ILAPIENTRY myWrite (const void* data, ILuint count, ILuint size, SIO*)
{
	return fwrite(data, count, size, writeFile);
}

ILint64  ILAPIENTRY mySeek (SIO* io, ILint64 offset, ILuint origin)
{
	return fseek(writeFile, offset, origin);
}

ILint64 ILAPIENTRY myTell (SIO* io)
{
	return ftell(writeFile);
}

void testSavers2(ILimage* handle, ILenum type, const TCHAR* targetName, const TCHAR* targetExt)
{
	TCHAR targetFN[MAX_PATH];

	// Test ilSave
	_tcscpy(targetFN, targetName);
	_tcscat(targetFN, L".ilSave.");
	_tcscat(targetFN, targetExt);
	DWORD t1 = GetTickCount();
	if (!il2Save(handle, type, targetFN)) {
		printf("testSavers2: Failed to save " PathCharMod " using ilSave\n", targetFN);
		++errors;
	}
	DWORD t2 = GetTickCount();
	printf(PathCharMod " using ilSave: %i ms\n", targetFN, t2-t1);

	// Test ilSaveF
	_tcscpy(targetFN, targetName);
	_tcscat(targetFN, L".ilSaveF.");
	_tcscat(targetFN, targetExt);
	FILE* file = _wfopen(targetFN, L"wb");
	if (!il2SaveF(handle, type, file)) {
		printf("testSavers2: Failed to save " PathCharMod " using ilSaveF\n", targetFN);
		++errors;
	}
	fclose(file);

	// Test ilSaveL
	_tcscpy(targetFN, targetName);
	_tcscat(targetFN, L"ilSaveL.");
	_tcscat(targetFN, targetExt);
	size_t lumpSize = il2DetermineSize(handle, type);
	BYTE* lump = new BYTE[lumpSize];
	auto writtenToLump = il2SaveL(handle, type, lump, lumpSize);
	if (writtenToLump > 0) {
		FILE* file = _wfopen(targetFN, L"wb");
		size_t writtenToFile = fwrite(lump, 1, lumpSize, file);
		if (writtenToLump != writtenToFile) {
			printf("testSavers2: Failed to write " PathCharMod " after ilSaveL\n", targetFN);
			++errors;
		}
		fclose(file);
	} else {
		printf("testSavers2: Failed to save " PathCharMod " using ilSaveL\n", targetFN);
		++errors;
	}
	delete lump;

	// Test ilSaveFuncs
	wcscpy(targetFN, targetName);
	wcscat(targetFN, L".ilSaveFuncs.");
	wcscat(targetFN, targetExt);
	writeFile = _wfopen(targetFN, L"wb");
	if (writeFile != NULL) {
		il2SetWrite(handle, NULL, NULL, myPutc, mySeek, myTell, myWrite);
		if (!il2SaveFuncs(handle, type))
			printf("testSavers2: Failed to save " PathCharMod " using ilSave\n", targetFN);
		fclose(writeFile);
	} else
		printf("testSavers2: Failed to open " PathCharMod " for writing\n", targetFN);
}

///////////////////////////////////////////////////////////////////////////////

void testSavers(const TCHAR* sourceFN, const TCHAR* targetFN)
{
	testHeap();
	ILimage* handle = il2GenImage();
	testHeap();
	if (!il2LoadImage(handle, sourceFN)) {
		printf("Failed to load %S using ilLoadImage\n", sourceFN);
		++errors;
		return;
	}
	testHeap();

	// gif, ico: no save support...
	// todo: psd, pcx, tga, tif
	testSavers2(handle, IL_BMP, targetFN, L"bmp");
	testSavers2(handle, IL_JPG, targetFN, L"jpg");
	testSavers2(handle, IL_PNG, targetFN, L"png");
	testSavers2(handle, IL_PSD, targetFN, L"psd");
	testSavers2(handle, IL_PCX, targetFN, L"pcx");
	testSavers2(handle, IL_TGA, targetFN, L"tga");
	testSavers2(handle, IL_TIF, targetFN, L"tif");
	testSavers2(handle, IL_TGA, targetFN, L"tga");
	testSavers2(handle, IL_PCX, targetFN, L"pcx");
	//testSavers2(handle, IL_PNM, targetFN, L"pnm");
	testSavers2(handle, IL_SGI, targetFN, L"sgi");
	testSavers2(handle, IL_WBMP, targetFN, L"wbmp");
	testSavers2(handle, IL_MNG, targetFN, L"mng");
	//testSavers2(handle, IL_VTF, targetFN, L"vtf");

	testHeap();
	il2DeleteImage(handle);
}

///////////////////////////////////////////////////////////////////////////////

DWORD WINAPI runThread(void* param)
{
	vector <TCHAR*>& threadFiles = *(vector <TCHAR*>*) (param);

	// Test ilLoad* function family
	// Each file in a given folder is loaded using the ilLoad* functions and saved as a bmp
	for (size_t i = 0; i < threadFiles.size(); ++i) {
		TCHAR* fn = threadFiles[i];
		DWORD t1 = GetTickCount();
		printf("Decoding " PathCharMod "...", fn);
		testLoaders(fn);
		DWORD t2 = GetTickCount();
		printf (PathCharMod "done, %i ms\n", fn, t2-t1);
	}
	threadFiles.clear();

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

int wmain (int argc, TCHAR** argv)
{
	const int threads = 4;
	il2Init();
	errors = 0;
	_wmkdir(targetDir);

	//testLoaders(L"IMG_5502.mng"); // memory leak?

	// Compile job lists for the threads
	vector < vector <TCHAR*> > files;
	files.resize(threads);
	TCHAR searchMask[MAX_PATH];
	_tcscpy(searchMask, sourceDir);
	_tcscat(searchMask, L"*");
	WIN32_FIND_DATAW data;
	HANDLE h = FindFirstFile(searchMask, &data);
	int count = 0;

	if (h != INVALID_HANDLE_VALUE) {
		do {
			if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				auto len = _tcslen(data.cFileName);
				TCHAR* fn = new TCHAR[len];
				_tcscpy(fn, data.cFileName);
				size_t thread = count % threads;
				files[thread].push_back(fn);
				++count;
			}
		} while (FindNextFile(h, &data));
		FindClose(h);
	}

	// Start threads
	for (size_t j = 1; j < threads; ++j) {
		vector <TCHAR*>& threadFiles = files[j];
		//runThread(threadFiles);
		CreateThread(NULL, 0, runThread, (void*) (&threadFiles), 0, NULL);
	}

	vector <TCHAR*>& threadFiles = files[0];
	runThread(&threadFiles);

	// Wait for threads to finish
	bool finished = false;
	do {
		finished = false;
		for (size_t j = 1; j < threads; ++j) {
			vector <TCHAR*>& threadFiles = files[j];
			if (threadFiles.size() > 0)
				finished = true;
		}
	} while (!finished);

	// Test ilSave* function family
	// A single image is loaded and saved using the ilSave* function family
	_wmkdir(L"D:\\TestIL\\encoded");
	testSavers(L"D:\\TestIL\\Don't Panic 24.bmp", L"D:\\TestIL\\encoded\\Don't Panic 24");

 	printf("%i errors\n", errors);
}
