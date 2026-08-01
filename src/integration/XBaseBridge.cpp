#include "XBaseBridge.h"

#include <XBase/Capabilities.h>
#include <XBase/Core.h>

namespace XBaseBridge {

bool IsWorldReady() {
    return XBase::Core::IsWorldReady();
}

void Init() {
    XBase::Core::Init(XBase::Core::AllDomains);
}

void Process() {
    XBase::Core::Process();
}

void SetEnabledDomains(XBase::Core::DomainMask enabledDomains) {
    XBase::Core::SetEnabledDomains(enabledDomains);
}

void NotifyGameInit() {
    XBase::Core::NotifyGameInit();
}

void Shutdown() {
    XBase::Core::Shutdown();
}

XBase::CapabilitySupport GetCapabilitySupport(XBase::Capability capability) {
    return XBase::GetCapabilitySupport(capability);
}

XBase::CapabilitySupport GetCapabilitySupport(XBase::FeatureCapability capability) {
    return XBase::GetCapabilitySupport(capability);
}

bool HasCapability(XBase::Capability capability) {
    return XBase::HasCapability(capability);
}

bool HasCapability(XBase::FeatureCapability capability) {
    return XBase::HasCapability(capability);
}

bool IsDomainEnabled(XBase::Core::Domain domain) {
    return XBase::Core::IsDomainEnabled(domain);
}

} // namespace XBaseBridge