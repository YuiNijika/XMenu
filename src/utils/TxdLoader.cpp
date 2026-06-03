#include "TxdLoader.h"
#include "utils/Log.h"
#include "plugin.h"
#include <filesystem>

#ifdef RW
#include <rw/rwcore.h>

struct RwD3D9Raster {
    union {
        IDirect3DTexture9* texture;
        IDirect3DSurface9* surface;
    };
    unsigned char* palette;
    unsigned char alpha;
    unsigned char cubeTextureFlags;
    unsigned char textureFlags;
    unsigned char lockedLevel;
    IDirect3DSurface9* lockedSurface;
    D3DLOCKED_RECT lockedRect;
    D3DFORMAT format;
    IDirect3DSwapChain9* swapChain;
    HWND* hwnd;
};

struct RwRasterEx : public RwRaster {
    RwD3D9Raster* m_pRenderResource;
};

static IDirect3DTexture9* GetTextureFromRaster(RwTexture* pTexture) {
    if (!pTexture || !pTexture->raster) return nullptr;
    RwRasterEx* raster = reinterpret_cast<RwRasterEx*>(&pTexture->raster->parent);
    if (!raster->m_pRenderResource) return nullptr;
    return raster->m_pRenderResource->texture;
}

static RwTexDictionary* CallLoadTexDictionary(const char* filename) {
#ifdef GTASA
    return plugin::CallAndReturnDynGlobal<RwTexDictionary*, const char*>(0x5B3860, filename);
#elif GTAVC
    // TODO: Need the VC address for LoadTexDictionary
    return nullptr;
#elif GTA3
    // TODO: Need the III address for LoadTexDictionary
    return nullptr;
#else
    return nullptr;
#endif
}
#endif

namespace TxdLoader {
    std::vector<TextureInfo> LoadTxd(const std::string& filePath) {
        std::vector<TextureInfo> textures;

        if (!std::filesystem::exists(filePath)) {
            Log::Error("TxdLoader: File does not exist");
            return textures;
        }

#ifdef RW
        RwTexDictionary* pRwTexDictionary = CallLoadTexDictionary(filePath.c_str());
        if (pRwTexDictionary) {
            RwLinkList* pRLL = (RwLinkList*)pRwTexDictionary->texturesInDict.link.next;
            RwTexDictionary* pEndDic;
            do {
                pEndDic = (RwTexDictionary*)pRLL->link.next;
                RwTexture* pTex = (RwTexture*)&pRLL[-1];

                if (pTex && pTex->name) {
                    TextureInfo info;
                    info.name = pTex->name;
                    info.texture = GetTextureFromRaster(pTex);
                    if (info.texture) {
                        textures.push_back(info);
                    }
                }
                
                pRLL = (RwLinkList*)pEndDic;
            } while (pEndDic != (RwTexDictionary*)&pRwTexDictionary->texturesInDict);
        } else {
            Log::Error("TxdLoader: Failed to parse TXD");
        }
#else
        Log::Error("TxdLoader: RW not defined, cannot load TXD");
#endif

        return textures;
    }
}
