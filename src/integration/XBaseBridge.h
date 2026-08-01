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
XBase::CapabilitySupport GetCapabilitySupport(XBase::Capability capability);
XBase::CapabilitySupport GetCapabilitySupport(XBase::FeatureCapability capability);
bool HasCapability(XBase::Capability capability);
bool HasCapability(XBase::FeatureCapability capability);
bool IsDomainEnabled(XBase::Core::Domain domain);

} // namespace XBaseBridge