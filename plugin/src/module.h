/*  Copyright 2024 Pretendo Network contributors <pretendo.network>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstdint>

enum class InkayStatus {
    Uninitialized, ///< The module isn't initialized
    Nintendo,      ///< The module is initialized but hasn't applied any patches
    Pretendo,      ///< The module is initialized and has applied the Pretendo patches

    Error = -1     ///< Failed to retrieve the module status
};

void Inkay_Initialize(bool apply_patches, bool show_startup_toast);
void Inkay_Finalize();
InkayStatus Inkay_GetStatus();
void Inkay_SetPluginRunning();

// The network-wide signed redirect list lives in the module (it verifies and applies it).
// Returns the accepted list version, or a negative value when the module refused it.
int32_t Inkay_SetRedirectList(const char *envelope, uint32_t len);
void Inkay_GetRedirectInfo(uint32_t *names, int32_t *version);
