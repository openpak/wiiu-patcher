#pragma once

// Signed redirect list: load the SD cache into the module at boot, then fetch a fresh signed
// list in the background once the network is up. A bad or missing list changes nothing: the
// module keeps the cached list, or the bundled one.
void RedirectList_Start();
void RedirectList_Stop();
