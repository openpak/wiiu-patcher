# Wii U update path (card #127)

Rule: everything a game or system update needs goes to Nintendo; everything else goes to OpenPak.
Evidence: nn-soap capture (`openpak-soap-capture`, 2026-11-04/05) of a Terraria update on EU 5.5.5E.

| Host | Used by an update | Where it goes | How |
|---|---|---|---|
| tagaya.wup.shop / tagaya-wup.cdn | yes, version list | Nintendo | URL patch absent; `COMPILED_NEVER` |
| nus.wup.shop (NetUpdateSOAP, GetSystemTitleHash) | yes, system titles | Nintendo | URL patch absent; `COMPILED_NEVER` |
| ccs.wup.shop / ccs.cdn.wup.shop | yes, TMD and contents | Nintendo | `.shop.nintendo.net`, `.cdn.nintendo.net` never-set; our ECS hands out these URLs |
| nus.cdn.wup.shop | yes, system contents | Nintendo | same |
| idbe-wup.cdn (icondata) | yes, the update dialog fetches the icon; a 404 is 105-1185 (CURL_RESPONSE_CODE_404) | Nintendo | idbe-wup URL patches removed (v0.6.3); `.cdn.nintendo.net` never-set |
| ecs.wup.shop | yes, one call: GetAccountStatus | OpenPak | URL patch kept. Our answer carries the stock Nintendo ccs/nus ServiceURLs, which is all the update uses. No other ECS action appeared in any update capture. Open risk: Region/Country in the answer are fixed (USA/US) |
| ias.wup.shop | no (shop sign-in) | OpenPak | patch kept; not called by an update |
| pushmore.wup.shop / pushmo | no (background download notices; 404 here is harmless) | OpenPak | patch kept |
| pls, npts, npvk, nppl, bst | no | OpenPak | patch kept |
| idbe-ctr.cdn | no (3DS titles) | OpenPak | patch kept, 3DS is out of scope |

Not verified: Pretendo's Inkay source and the WiiUBrew error list were not consulted; 105-1185 comes from the owner.
