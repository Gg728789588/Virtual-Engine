#pragma once
#ifdef DLL_EXPORT
#define DLL_CLASS __declspec(dllexport)
#else
#define DLL_CLASS __declspec(dllimport)
#endif