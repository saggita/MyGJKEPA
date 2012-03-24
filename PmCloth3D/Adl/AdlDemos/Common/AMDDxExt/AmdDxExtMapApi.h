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
* @file  amddxextmapapi.h
* @brief
*    AMD D3D Extended Mapping Extension API include file.
*    This is the main include file for apps using the Extended Map extension.
***************************************************************************************************
*/

#ifndef _AMDDXEXTMAPAPI_H_
#define _AMDDXEXTMAPAPI_H_

#include "AmdDxExtApi.h"
#include "AmdDxExtIface.h"
#include "AmdDxExtMap.h"

// Extended map extension ID passed to IAmdDxExt::GetExtInterface()
const unsigned int AmdDxExtMapID = 4;

/**
***************************************************************************************************
* @brief Abstract thread trace extension interface class
*
***************************************************************************************************
*/
class IAmdDxExtMap : public IAmdDxExtInterface
{
public:
    // Setting resources mappable means that resources created after this flag is set will
    // be mappable later via extension.
    // Setting newResourcesMappable = false means that resources will not be mappable.
    // Setting too many resources mappable could have negative effects, so not all resources
    // should be made mappable.
    virtual HRESULT SetOverrideAccessFlags(UINT overrideAccessFlags) = 0;
    virtual VOID    Map(ID3D11Resource *pResource, 
                        UINT Subresource, 
                        D3D11_MAP MapType,
                        UINT MapFlags,
                        D3D11_MAPPED_SUBRESOURCE *pMappedResource) = 0;
    virtual VOID Unmap(ID3D11Resource *pResource, UINT Subresource) = 0;
};

#endif // _AMDDXEXTMAPAPI_H_
