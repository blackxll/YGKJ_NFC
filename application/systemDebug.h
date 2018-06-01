/*******************************************************************************
*
* Copyright 2012-2015 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __SYSTEM_DEBUG_H__
#define __SYSTEM_DEBUG_H__

#if _DEBUG
void DBG_Assert(uint8 byAssert, uint32 dwAssertCode, uint32 dwParameter);
void DBG_Warning(uint8 byWarn, uint32 dwAssertCode, uint32 dwParameter);
#else
#define DBG_Assert(...)
#define DBG_Warning(...)
#endif

#endif
