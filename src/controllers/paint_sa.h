#pragma once
#include <string>
#include <unordered_map>
#include "plugin.h"
#include "CVehicle.h"

/*
	Vehicle color & texturring implementation class for GTA: San Andreas

	TODO: Implement for VC & 3 too (maybe)
	Dunno how it'd work with the d3d8to9 wrapper
*/
class PaintMgr
{
public:
    struct PaintData
    {
        struct MatInfo
        {
            bool m_bRecolor = false;
            bool m_bRetexture = false;
            RwRGBA m_nColor = {0, 0, 0, 0};
            RwRGBA m_nOriginalColor = {0, 0, 0, 0};
            RwTexture* m_pTexture = nullptr;
            RwTexture* m_pOriginalTexture = nullptr;
            RpGeometry* m_pGeometry = nullptr;
            RwInt32 m_nOriginalGeometryFlags = 0;
        };
        unsigned char m_nCarColors[4];  // carcols color IDs (primary, secondary, tertiary, quaternary)
        std::string m_nTextureName = ""; // current applied texture name

        std::unordered_map<RpMaterial*, MatInfo> m_nMapInfoList;

        PaintData(CVehicle* pVeh)
        {
            m_nCarColors[0] = pVeh->m_nPrimaryColor;
            m_nCarColors[1] = pVeh->m_nSecondaryColor;
            m_nCarColors[2] = pVeh->m_nTertiaryColor;
            m_nCarColors[3] = pVeh->m_nQuaternaryColor; 
        }

        // Resets applied material colors
        void ResetMatColor(RpMaterial* pMat);

        // Resets applied material textures
        void ResetMatTexture(RpMaterial* pMat);

        // Sets the material color to provided value
        void SetMatColor(RpMaterial* pMat, RpGeometry* pGeo, RwRGBA color);

        // Sets the material to provided texture
        void SetMatTexture(RpMaterial* pMat, RwTexture* pTex);
    };

private:
    plugin::VehicleExtendedData<PaintData> m_VehPaint;

    PaintMgr();
    PaintMgr(const PaintMgr&) = delete;

public:
    static PaintMgr& Get() {
        static PaintMgr instance;
        return instance;
    }

    // Returns internal data structure
    PaintData &GetData(CVehicle* pVeh);

    // Resets applied applied colors
    void ResetColor(CVehicle* pVeh);

    // Resets appllied textures
    void ResetTexture(CVehicle* pVeh);

    // Applies color to vehicle
    void SetColor(CVehicle* pVeh, CRGBA color);

    // Sets vehicle carcol colors
    void SetCarcols(CVehicle *pVeh, unsigned int primary, unsigned int secondary, unsigned int tertiary, unsigned int quaternary, bool reset = true);
};

extern PaintMgr& Paint;