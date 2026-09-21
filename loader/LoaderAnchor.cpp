// The loader DLL owns no logic of its own. This anchor keeps one compile item
// so MSVC runs Link and pulls the XBaseBootstrap entry object with WHOLEARCHIVE.
extern "C" void XMenuLoaderAnchor() {
}
