/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or imlied.
// See the License for the specific language governing permissions and
// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "RenderTechnique.h"
#include "Structures.fxh"

struct SPrecomputedOpticalDepthTexDim
{
    const int iNumStartPosZenithAngles;
    const int iNumStartPosAzimuthAngles;
    const int iNumDirectionZenithAngles;
    const int iNumDirectionAzimuthAngles;

    SPrecomputedOpticalDepthTexDim() : 
        iNumStartPosZenithAngles(32),
        iNumStartPosAzimuthAngles(64),
        iNumDirectionZenithAngles(32),
        iNumDirectionAzimuthAngles(64)
    {
    }
};

struct SPrecomputedScatteringInParticleTexDim
{
    const int iNumStartPosZenithAngles;
    const int iNumViewDirAzimuthAngles;
    const int iNumViewDirZenithAngles;
    const int iNumDistancesFromCenter;
    const int iNumDensityLevels;
    SPrecomputedScatteringInParticleTexDim() : 
        iNumStartPosZenithAngles(32),
        iNumViewDirAzimuthAngles(64),
        iNumViewDirZenithAngles(32),
        iNumDistancesFromCenter(32),
        iNumDensityLevels(8)
    {
    }
};

class CCloudsController
{
public:
    CCloudsController();
    ~CCloudsController();
    HRESULT OnCreateDevice(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext);
    void OnDestroyDevice();
    void Update( const SGlobalCloudAttribs &NewAttribs,
                 const D3DXVECTOR3 &CameraPos, 
                 const D3DXVECTOR3 &LightDir,
                 ID3D11Device *pDevice,
                 ID3D11DeviceContext *pDeviceContext, 
                 ID3D11Buffer *pcbCameraAttribs, 
                 ID3D11Buffer *pcbLightAttribs, 
                 ID3D11Buffer *pcMediaScatteringParams );
    
    struct SRenderAttribs
    {
        ID3D11Device *pDevice;
        ID3D11DeviceContext *pDeviceContext;
        D3DXMATRIX ViewProjMatr;
        ID3D11Buffer *pcbCameraAttribs; 
        ID3D11Buffer *pcbLightAttribs;
        ID3D11Buffer *pcMediaScatteringParams;
        ID3D11ShaderResourceView *pPrecomputedNetDensitySRV;
        ID3D11ShaderResourceView *pAmbientSkylightSRV;
        ID3D11ShaderResourceView *pDepthBufferSRV;
        ID3D11DepthStencilView *pShadowMapDSV;
        ID3D11ShaderResourceView *pLiSpCloudTransparencySRV;
        ID3D11ShaderResourceView *pLiSpCloudMinMaxDepthSRV;
        D3DXVECTOR3 f3CameraPos;
        D3DXVECTOR3 f3ViewDir;
        int iCascadeIndex;
        float fCurrTime;
        UINT uiLiSpCloudDensityDim;
        bool bLightSpacePass;
        bool bTiledMode;
        const SCameraAttribs *m_pCameraAttribs;
        const SShadowMapAttribs *m_pSMAttribs;
        SRenderAttribs(){memset(this, 0, sizeof(*this));}
    };

    void RenderScreenSpaceDensityAndColor(SRenderAttribs &RenderAttribs);

    void RenderLightSpaceDensity(SRenderAttribs &RenderAttribs);

    void MergeLiSpDensityWithShadowMap(SRenderAttribs &RenderAttribs);

    void CombineWithBackBuffer(ID3D11Device *pDevice, 
                               ID3D11DeviceContext *pDeviceContext, 
                               ID3D11ShaderResourceView *pDepthBufferSRV,
                               ID3D11ShaderResourceView *pBackBufferSRV);

    void OnResize(ID3D11Device *pDevice, 
                  UINT uiWidth, UINT uiHeight);
    ID3D11ShaderResourceView *GetScrSpaceCloudColor(){return m_ptex2DScreenCloudColorSRV;}
    ID3D11ShaderResourceView *GetScrSpaceCloudTransparency(){return m_ptex2DScrSpaceCloudTransparencySRV;}
    ID3D11ShaderResourceView *GetScrSpaceCloudMinMaxDist(){return m_ptex2DScrSpaceDistToCloudSRV;}
    const SGlobalCloudAttribs &GetCloudAttribs(){return m_CloudAttribs;}
    bool IsPSOrderingAvailable(){return m_bPSOrderingAvailable;}

private:
    CCloudsController(const CCloudsController&);
    const CCloudsController& operator =(const CCloudsController&);

    void DefineMacros(class CD3DShaderMacroHelper &Macros);
    void RenderMaxDensityMip(ID3D11Device *pDevice, 
                             ID3D11DeviceContext *pDeviceContext, 
                             ID3D11Texture2D *ptex2DMaxDensityMipMap, 
                             ID3D11Texture2D *ptex2DTmpMaxDensityMipMap, 
                             const D3D11_TEXTURE2D_DESC &MaxCloudDensityMipDesc);

    void RenderFlatClouds(SRenderAttribs &RenderAttribs);
    void RenderParticles(SRenderAttribs &RenderAttribs);

    void PerformTiling(SRenderAttribs &RenderAttribs,
                       ID3D11UnorderedAccessView *pFirstListIndUAV,
                       const D3DXMATRIX &TilingMatrix,
                       UINT uiUAVWidth, UINT uiUAVHeight);

    void ComputeParticleVisibility(ID3D11Device *pDevice,
                                   ID3D11DeviceContext *pDeviceContext, 
                                   ID3D11Buffer *pcbCameraAttribs,
                                   bool bLightSpacePass);

    void SerializeVisibileParticles(ID3D11Device *pDevice,
                                    ID3D11DeviceContext *pDeviceContext,
                                    const std::vector<UINT> &ParticleOrder);

    void PerformTilingInLightSpace(SRenderAttribs &RenderAttribs);

    void ComputeParticleLighting(SRenderAttribs &RenderAttribs);
    
    void CreateLiSpFirstListIndTex(ID3D11Device *pDevice);
    
    HRESULT CreateBufferAndViews(ID3D11Device *pDevice, 
                                 const D3D11_BUFFER_DESC &BuffDesc, 
                                 D3D11_SUBRESOURCE_DATA *pInitData, 
                                 ID3D11Buffer **ppBuffer, 
                                 ID3D11ShaderResourceView **ppSRV = nullptr, 
                                 ID3D11UnorderedAccessView **ppUAV = nullptr, 
                                 UINT UAVFlags = 0);

    HRESULT CreateParticleDataBuffer(ID3D11Device *pDevice);
    
    HRESULT PrecomputParticleDensity(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext);
    
    HRESULT PrecomputeScatteringInParticle(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext);
    HRESULT ComputeExitance(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext);
    
    HRESULT Create3DNoise(ID3D11Device *pDevice);

    static const int sm_iCSThreadGroupSize = 64;
    LPCTSTR m_strEffectPath;
    LPCTSTR m_strPreprocessingEffectPath;

	static const int sm_iTileSize = 16;
    bool m_bPSOrderingAvailable;

    SPrecomputedOpticalDepthTexDim m_PrecomputedOpticalDepthTexDim;
    SPrecomputedScatteringInParticleTexDim m_PrecomputedSctrInParticleLUTDim;

    D3DXVECTOR3 m_f3PrevLightDir;

    UINT m_uiCloudDensityTexWidth, m_uiCloudDensityTexHeight;
    UINT m_uiBackBufferWidth, m_uiBackBufferHeight, m_uiTileTexWidth, m_uiTileTexHeight;

    CComPtr<ID3D11SamplerState> m_psamLinearWrap, m_psamPointWrap, m_psamLinearClamp;

    SGlobalCloudAttribs m_CloudAttribs;
    
    // Auxiliary arrays used to store distance from each particle to camera and 
    // light space Z coordiante
    std::vector<float> m_ParticleDistToCam, m_ParticleLightSpaceZ;
    // Particles sorted for rendering from camera and light
    std::vector<UINT> m_CamSpaceOrder, m_LightSpaceOrder;
    // Packed locations for all cells and particles
    std::vector<UINT> m_PackedCellLocations, m_PackedParticleLocations;
    // Auxiliary array containing attributes for each ring used to calculate particle location
    std::vector<D3DXVECTOR3> m_RingGridAttribs;

    CComPtr<ID3D11Buffer> m_pcbGlobalCloudAttribs;
    
    // Render techniques
    CRenderTechnique m_RenderCloudsTech[2], m_RenderFlatCloudsTech[2], m_CombineWithBBTech, m_RenderCloudDetphToShadowMap;
    CRenderTechnique m_RenderCloudsTiledTech, m_PerformTilingTech;
    CRenderTechnique m_ProcessCloudGridTech, m_ComputeParticleVisibilityTech[2], m_ComputeCloudLightingTech, m_SmoothParticleLightingTech;
    CRenderTechnique m_SerializeVisibleParticlesTech;
    CRenderTechnique m_ProcessParticlesTech;
    CRenderTechnique m_ComputeDispatchArgsTech;
    CRenderTechnique m_ComputeOpticalDepthTech;
    CRenderTechnique m_ApplyParticleLayersTech;
    CRenderTechnique m_ComputeSingleSctrInParticleTech;
    CRenderTechnique m_GatherPrevSctrOrderTech;
    CRenderTechnique m_ComputeScatteringOrderTech;
    CRenderTechnique m_AccumulateInscatteringTech;
    CRenderTechnique m_RenderScatteringLUTSliceTech;
    CRenderTechnique m_ComputeExitanceTech;

    // States
    CComPtr<ID3D11DepthStencilState> m_pdsEnableDepth, m_pdsDisableDepth;
    CComPtr<ID3D11RasterizerState> m_prsSolidFillCullFront, m_prsSolidFillNoCull;
    CComPtr<ID3D11BlendState> m_pbsDefault;
    CComPtr<ID3D11BlendState> m_pbsRT0MulRT1MinRT2Over;

    // 2D cloud density and noise textures
    CComPtr<ID3D11ShaderResourceView> m_ptex2DCloudDensitySRV, m_ptex2DWhiteNoiseSRV;
    // Maximum mip map pyramid
    CComPtr<ID3D11ShaderResourceView> m_ptex2DMaxDensityMipMapSRV;
    // 3D noise texture
    CComPtr<ID3D11ShaderResourceView> m_ptex3DNoiseSRV;

    // SRV and UAV for cloud grid
    CComPtr<ID3D11ShaderResourceView> m_pbufCloudGridSRV;
    CComPtr<ID3D11UnorderedAccessView> m_pbufCloudGridUAV;
    
    // SRV and UAV for particle lattice
    CComPtr<ID3D11UnorderedAccessView> m_pbufCloudParticlesUAV;
    CComPtr<ID3D11ShaderResourceView> m_pbufCloudParticlesSRV;
    
    // SRV & AUV for a buffer containing packed 1-bit particle visibility flags 
    CComPtr<ID3D11ShaderResourceView> m_pbufVisibleParticlesFlagsSRV;
    CComPtr<ID3D11UnorderedAccessView> m_pbufVisibleParticlesFlagsUAV;

    // SRV & AUV for a buffer storing particle lighting information 
    CComPtr<ID3D11ShaderResourceView> m_pbufParticlesLightingSRV, m_pbufAttenuatedSunLightSRV;
    CComPtr<ID3D11UnorderedAccessView> m_pbufParticlesLightingUAV, m_pbufAttenuatedSunLightUAV;
    CComPtr<ID3D11Buffer> m_pbufDefaultAttenuatedSunLight;

    // Buffer containing unordered list of all valid cells
    CComPtr<ID3D11Buffer> m_pbufValidCellsUnorderedList;
    CComPtr<ID3D11UnorderedAccessView> m_pbufValidCellsUnorderedListUAV;
    CComPtr<ID3D11ShaderResourceView> m_pbufValidCellsUnorderedListSRV;
    
    // Buffer containing number of valis cells or particles
    CComPtr<ID3D11Buffer> m_pbufValidCellsCounter;
    CComPtr<ID3D11ShaderResourceView> m_pbufValidCellsCounterSRV;

    // Buffer containing unordered list of all valid particles
    CComPtr<ID3D11Buffer> m_pbufValidParticlesUnorderedList;
    CComPtr<ID3D11UnorderedAccessView> m_pbufValidParticlesUnorderedListUAV;
    CComPtr<ID3D11ShaderResourceView> m_pbufValidParticlesUnorderedListSRV;
    
    // Buffer containing sorted list of ALL particles
    CComPtr<ID3D11Buffer> m_pbufSortedParticlesOrder;
    CComPtr<ID3D11ShaderResourceView> m_pbufSortedParticlesOrderSRV;

    // Buffer containing sorted list of VISIBLE particles only
    CComPtr<ID3D11Buffer> m_pbufSerializedVisibleParticles;
    CComPtr<ID3D11ShaderResourceView> m_pbufSerializedVisibleParticlesSRV;

    // Buffer used to store DispatchIndirect() arguments
    CComPtr<ID3D11Buffer> m_pbufDispatchArgs;
    CComPtr<ID3D11UnorderedAccessView> m_pbufDispatchArgsUAV;

    // SRV for the buffer containing packed cell locations
    CComPtr<ID3D11ShaderResourceView> m_pbufPackedCellLocationsSRV;

    // Cloud color, transparancy and distance buffer for camera space
    CComPtr<ID3D11ShaderResourceView> m_ptex2DScreenCloudColorSRV;
    CComPtr<ID3D11RenderTargetView>   m_ptex2DScreenCloudColorRTV;
    CComPtr<ID3D11ShaderResourceView> m_ptex2DScrSpaceCloudTransparencySRV, m_ptex2DScrSpaceDistToCloudSRV;
    CComPtr<ID3D11RenderTargetView>   m_ptex2DScrSpaceCloudTransparencyRTV, m_ptex2DScrSpaceDistToCloudRTV;

    // Downscaled cloud color, transparancy and distance buffer for camera space
    CComPtr<ID3D11ShaderResourceView> m_ptex2DDownscaledScrCloudColorSRV;
    CComPtr<ID3D11RenderTargetView>   m_ptex2DDownscaledScrCloudColorRTV;
    CComPtr<ID3D11ShaderResourceView> m_ptex2DDownscaledScrCloudTransparencySRV, m_ptex2DDownscaledScrDistToCloudSRV;
    CComPtr<ID3D11RenderTargetView>   m_ptex2DDownscaledScrCloudTransparencyRTV, m_ptex2DDownscaledScrDistToCloudRTV;

    // Buffer used to store particle list after tiling
    CComPtr<ID3D11UnorderedAccessView> m_pbufParticleListsBuffUAV;
    CComPtr<ID3D11ShaderResourceView> m_pbufParticleListsBuffSRV;

    // Buffer storing index of the first particle for each tile in screen space
    CComPtr<ID3D11UnorderedAccessView> m_ptex2DScrFirstListIndUAV;
    CComPtr<ID3D11ShaderResourceView> m_ptex2DScrFirstListIndSRV;

    // Buffer storing index of the first particle for each tile in light space
    CComPtr<ID3D11UnorderedAccessView> m_ptex2DLiSpFirstListIndUAV;
    CComPtr<ID3D11ShaderResourceView> m_ptex2DLiSpFirstListIndSRV;

    CComPtr<ID3D11ShaderResourceView> m_pbufParticleLayersSRV;
    CComPtr<ID3D11UnorderedAccessView> m_pbufParticleLayersUAV;
    CComPtr<ID3D11Buffer> m_pbufClearParticleLayers;

    CComPtr<ID3D11ShaderResourceView> m_ptex3DPrecomputedParticleDensitySRV;
    CComPtr<ID3D11ShaderResourceView> m_ptex3DSingleSctrInParticleLUT_SRV;
    CComPtr<ID3D11ShaderResourceView> m_ptex3DMultipleSctrInParticleLUT_SRV;


    // Inpute layout for streamed out particles
    CComPtr<ID3D11InputLayout> m_pRenderCloudsInputLayout;
};
