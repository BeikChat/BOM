/*
 * profile_analyzer.h - public C ABI of profile_analyzer.dll
 *
 * The library parses a steel profile designation (e.g. "IPE200", "HEA160",
 * "L150*5", "RHS100x50x4") and returns the profile type, the nominal height
 * and the weight of one metre.
 *
 * ABI contract:
 *   - plain C functions, no exceptions cross the boundary;
 *   - calling convention: stdcall;
 *   - exported names are undecorated on both x86 and x64 (see .def file);
 *   - strings are UTF-16, NUL-terminated;
 *   - the caller owns all buffers. profileType must hold at least
 *     PA_PROFILE_TYPE_BUFFER_SIZE wide characters (including the terminator).
 */
#ifndef PROFILE_ANALYZER_H
#define PROFILE_ANALYZER_H

#include <wchar.h>

#if defined(_WIN32)
    #define PA_CALL __stdcall
    #if defined(PROFILE_ANALYZER_BUILD)
        #define PA_API
    #else
        #define PA_API __declspec(dllimport)
    #endif
#else
    #define PA_CALL
    #define PA_API __attribute__((visibility("default")))
#endif

/* Size (in wchar_t, including the terminating NUL) of the profileType output buffer. */
#define PA_PROFILE_TYPE_BUFFER_SIZE 32

/* Upper bound for the length of the input profile name (in wchar_t, without NUL). */
#define PA_PROFILE_NAME_MAX_LENGTH 128

/* ---- Result codes ------------------------------------------------------ */
#define PA_OK 0                     // weight taken from the reference table
#define PA_E_INVALID_ARGUMENT (-1)  // a NULL pointer was passed
#define PA_E_EMPTY_NAME (-2)        // name is empty or whitespace only
#define PA_E_INVALID_FORMAT (-3)    // name cannot be parsed / wrong number of dimensions
#define PA_E_UNKNOWN_TYPE (-4)      // profile family is unsupported
#define PA_E_OUT_OF_RANGE (-5)      // dimensions are outside the supported range
#define PA_E_INTERNAL (-100)        // unexpected internal error

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Analyzes a profile designation.
 *   profileName    [in]  NUL-terminated UTF-16 string, e.g. L"IPE200".
 *   nominalHeight  [out] nominal height in mm.
 *   weightPerMeter [out] weight of 1 m in kg.
 *   profileType    [out] buffer of PA_PROFILE_TYPE_BUFFER_SIZE wchar_t, receives e.g.
 * L"IPE".
 *   Returns one of the PA_* codes above.
 */
PA_API int PA_CALL AnalyzeProfile(const wchar_t* profileName, double* nominalHeight,
                                  double* weightPerMeter, wchar_t* profileType);

/* Returns a static, human readable (English) description of a result code. Never NULL. */
PA_API const wchar_t* PA_CALL GetAnalyzeProfileErrorText(int code);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PROFILE_ANALYZER_H */
