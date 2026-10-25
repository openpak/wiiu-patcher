# Protocol: wiiu-patcher

Fork of Pretendo's Inkay (GPL/AGPL as upstream). The same in-memory patches, pointed at
OpenPak: every `*.pretendo.cc` name became the same-length `*.openpak.org` name, and
`data/ca.pem` is the OpenPak Network CA, so the console trusts the leafs Traefik serves for
those names. No DNS setup and no public DNS record: the module resolves every `*.openpak.org` name to the OpenPak box itself (`common/inkay_config.h`, override with `-DOPENPAK_SERVER_IP`). Copy `Inkay-openpak.wms` to `sd:/wiiu/environments/aroma/modules/` and the console goes to
OpenPak; remove it and it goes back. Releases: `vX.Y.Z` tags.
