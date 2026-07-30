#include "XBaseBridge.h"

#include <XBase/Capabilities.h>
#include <XBase/Core.h>

namespace XBaseBridge {

bool IsWorldReady() {
    return XBase::Core::IsWorldReady();
}

void Init() {
    XBase::Core::Init(XBase::Core::DomainBit(XBase::Core::Domain::Player));
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

bool HasCapability(XBase::Capability capability) {
    return XBase::HasCapability(capability);
}

bool HasCapability(XBase::FeatureCapability capability) {
    return XBase::HasCapability(capability);
}

} // namespace XBaseBridge