/*
***************************************************************************************************
*
*  Trade secret of Advanced Micro Devices, Inc.
*  Copyright (c) 2010 Advanced Micro Devices, Inc. (unpublished)
*
*  All rights reserved.  This notice is intended as a precaution against inadvertent publication and
*  does not imply publication or any waiver of confidentiality.  The year included in the foregoing
*  notice is the year of creation of the work.
*
***************************************************************************************************
*/

/**
***************************************************************************************************
* @file  amddxextmap.h
* @brief
*    AMD D3D Map Extension API include file.
*    This is the main include file for apps using extended map extension.
***************************************************************************************************
*/

#ifndef _AMDDXEXTMAP_H_
#define _AMDDXEXTMAP_H_

enum AmdExtMapMapType
{
    AmdExtMapRead,
    AmdExtMapWrite,
    AmdExtMapReadWrite,
    AmdExtMapWriteDiscard,
    AmdExtMapWriteNoOverwrite
};

struct AmdDxExtMappedSubresource
{
    VOID *pData;
    UINT RowPitch;
    UINT DepthPitch;
};

struct AmdDxExtMapMapParams
{
    UINT                      subresource;
    UINT                      mapType;
    UINT                      mapFlags;
    AmdDxExtMappedSubresource *pMappedSubresource;
};

struct AmdDxExtMapUnmapParams
{
    UINT       subresource;
};
#endif // _AMDDXEXTMAP_H_
