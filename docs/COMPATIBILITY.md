# Game compatibility

[Quick start](../README.md#usage) · [Buffering reference](USAGE.md#buffering-and-window-sync-wait)

These upstream compatibility notes retain their original tested versions and scope. See the [fork test record](FORK.md) for monitor-preference testing.

## Sync Wait

In these games, turning off **Window Sync Wait** can disable double-buffer vsync:

- Batman - The Telltale Series (Warehouse patch enables triple buffer, so this option is not needed)
- Pokémon Legends: Arceus
- Pokémon Legends: Z-A
- Pokémon Scarlet
- Pokémon Violet
- Sonic Frontier
- The Legend of Zelda: Tears of the Kingdom (Warehouse patch enables triple buffer, so this option is not needed)
- Xenoblade Chronicles: Definitive Edition
- Xenoblade Chronicles 2
- Xenoblade Chronicles 3
- Xenoblade Chronicles X

## Double-to-triple-buffer forcing

Unreal Engine 5 and 6 games are using on top of 2 front buffers one back buffer, so they are already technically triple buffer and you don't need to enable FPSLocker triple buffer for them.

Games with known problems:

| Title | Tested game version | Reason |
| --- | --- | --- |
| Death end re;Quest | 1.0.1 | Memory exhaustion in 4GB units, crash at boot |
| Kirby and the Forgotten Land | 1.1.0 | Memory exhaustion in 4GB units, crash at boot, we need 26 MB, it gives only 16 MB |
| Stubbs the Zombie in Rebel Without a Pulse | 1.0.3 | Memory exhaustion in 4GB units, changing exefs patch to match expected memory usage boots game with triple buffer, but any change between handheld and docked causes crash |
| Tomodachi Life: Living the Dream | 1.0.0, 1.0.4 | Memory exhaustion in 4GB units, gets to loading screen and after few seconds crash |

### Passed initial tests

| Title | Version | Note |
| --- | --- | --- |
| Air Conflicts: Pacific Carriers | 1.0.2 |  |
| Alan Wake | 1.0.1 |  |
| Amnesia Collection | 1.3.0 |  |
| Beyond Good & Evil | 1.0.1 |  |
| Captain Tsubasa: Rise of New Champions | 1.4.6 |  |
| Card-en-Ciel | 1.9.1 |  |
| Cars 3 | 1.0.2 |  |
| DC Super Hero Girls: Teen Power | 1.0.1 |  |
| Death end re;Quest 2 | 1.0.0 |  |
| Divinity: Original Sin 2 | 1.0.12 |  |
| Don't Starve Together | 1.29.0 |  |
| DRAGON BALL XENOVERSE 2 | 1.25.02 |  |
| Dragon Star VARNIR | 1.0.1 |  |
| Gal*Gun Returns | 1.0.4 |  |
| Gal*Gun: Double Peace | 1.0.2 |  |
| GO VACATION | 1.0.0 |  |
| Kirby's Dream Buffet | 1.0.0a | FPSLocker patch already has triple buffer solution |
| Luigi's Mansion 2 HD | 1.0.0 |  |
| Mary Skelter Finale | 1.0.0 |  |
| Majogami | 1.2.1 |  |
| Metroid Dread | 2.1.0 |  |
| Metroid Prime 4 | 1.1.0 |  |
| Need for Speed: Hot Pursuit | 1.0.3 | Game is already using one back buffer, so there is no need to use triple buffer here. |
| Paper Mario: The Origami King | 1.0.1 |  |
| Paper Mario: The Thousand-Year Door | 1.0.1 |  |
| Portal Knights | 1.7.2 |  |
| Rebel Galaxy: Outlaw | 1.0.1 |  |
| Secret Game: Killer Queen | 1.0.1 |  |
| SENRAN KAGURA Peach Ball | 1.0.2 US |  |
| South Park: The Fractured But Whole | 1.0.5 |  |
| STORY OF SEASONS: A Wonderful Life | 1.0.2 |  |
| SWORD ART ONLINE Alicization Lycoris | 3.0.1 |  |
| SWORD ART ONLINE: Hollow Realization | 1.0.2 |  |
| The Thing: Remastered | 1.1.1 |  |
| The Walking Dead | 1.0.1 |  |
| Tokyo Mirage Sessions | 1.0.0 | FPSLocker patch already has triple buffer solution and is better |
| Trials Rising | 1.0.13 |  |
| TY the Tasmanian Tiger 2 | 1.0.1 |  |
| Unravel Two | 1.0.1 |  |
| Valkyria Chronicles | 1.0.1 |  |
| X-Morph Defense | 1.0.1 |  |
| Xenoblade Chronicles | 1.1.2 |  |
| Xenoblade Chronicles 2 | 2.1.0 |  |
| Xenoblade Chronicles 3 | 2.2.1 |  |
| Xenoblade Chronicles X | 2.0.0 |  |
| Yo-Kai Watch 4++ | 2.2.0 |  |
