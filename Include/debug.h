/* $Id: debug.h $ */
/** @file
 * debug.h
 */
#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <Library/MemLogLib.h>

#if 0
#define KEXT_PATCH_DEBUG
#endif
#if 0
#define KEXT_INJECT_DEBUG
#endif

#if 1
#define BOOT_DEBUG
#endif

#ifndef BOOT_DEBUG
#define DBG(...)
#else
#define BOOT_LOG L"EFI\\bareboot\\boot.log"
#define DBG(...) MemLog(TRUE, 0, __VA_ARGS__)
#endif

#endif
