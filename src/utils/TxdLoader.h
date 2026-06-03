#pragma once
#include <string>
#include <vector>
#include <d3d9.h>
#include "imgui/imgui.h"

namespace TxdLoader {
    struct TextureInfo {
        std::string name;
        IDirect3DTexture9* texture;
    };

    // Load a .txd file and extract all its textures
    // Returns a list of TextureInfo containing the texture name and D3D9 texture pointer
    std::vector<TextureInfo> LoadTxd(const std::string& filePath);
}
