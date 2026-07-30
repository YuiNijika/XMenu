#pragma once

#include <XBase/Capabilities.h>
#include <XBase/Core.h>

namespace XBaseBridge {

bool IsWorldReady();
void Init();
void Process();
void SetEnabledDomains(XBase::Core::DomainMask enabledDomains);
void NotifyGameInit();
void Shutdown();
bool HasCapability(XBase::Capability capability);
bool HasCapability(XBase::FeatureCapability capability);

} // namespace XBaseBridge