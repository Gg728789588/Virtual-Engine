#pragma once
#ifdef DLL_EXPORT
#define DLL_CLASS __declspec(dllexport)
#define DLL_FUNC extern"C" __declspec(dllexport) 
#else
#define DLL_CLASS __declspec(dllimport)
#define DLL_FUNC
#endif