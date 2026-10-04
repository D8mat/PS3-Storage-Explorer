# PS3 Storage Explorer

[English version](README.md)

Homebrew prehliadač úložiska PS3 s aktívnym HEN alebo kompatibilným CFW/Cobra. Zmeria veľkosť jednotlivých hier, nainštalovaných dát a ďalších priečinkov a zoradí ich podľa veľkosti. Herné súbory iba číta, nemá funkciu mazania.

**Aktuálna verzia: 0.1.7** · Title ID: `STOR00001`

![Ikona aplikácie](assets/ICON0.PNG)

## Inštalácia

1. Stiahni [PS3-Storage-Explorer-0.1.7.pkg](dist/PS3-Storage-Explorer-0.1.7.pkg).
2. Cez svoj FTP server na PS3 ho prenes do `/dev_hdd0/packages/`.
3. V XMB otvor **Game → Package Manager → Install Package Files**, vyber interné úložisko/HDD a nainštaluj balík. Názvy položiek závisia od systému. Alternatívou je PKG v koreňovom priečinku FAT32 USB, ktoré PS3 rozpoznáva.
4. Ak systém vyžaduje HEN, aktivuj ho a spusti aplikáciu.

Rovnaké Title ID aktualizuje existujúcu aplikáciu. Inštalácia zapisuje samotnú aplikáciu. Pripravený PKG nevyžaduje kompilátor ani DLL na PC.

[Kontrolný súčet](dist/SHA256SUMS.txt) · [Testovanie cez FTP](docs/TESTING.sk.md) · [Zostavenie](docs/BUILDING.md)

## Funkcie

- Rekurzívne veľkosti so 64-bitovými počítadlami; GiB v zozname a presné bajty pri označenej položke.
- Zoradenie od najväčšej alebo najmenšej položky, stránkovanie a otváranie priečinkov.
- Názvy a ID z `PARAM.SFO` alebo `PS3_GAME/PARAM.SFO`, ak sú dostupné.
- Výber najviac 32 ciest naprieč zariadeniami a spoločné zoradenie výsledkov.
- Vynechanie duplicitných a vnorených vybraných koreňov, aby sa rovnaký podstrom neskenoval dvakrát.
- Priehľadná ikona XMB.

## Ovládanie

Po spustení sa zobrazí **READY**. Select skenuje `/dev_hdd0/game`; trojuholník otvorí ponuku ciest.

| Tlačidlo | Výsledky | Ponuka ciest |
| --- | --- | --- |
| Hore / dole | Výber položky | Výber cesty |
| L1 / R1 | Predošlá / ďalšia strana | Predošlá / ďalšia strana |
| L2 / R2 | — | Predošlé / ďalšie dostupné zariadenie |
| Štvorec | Obrátiť zoradenie | Označiť / odznačiť |
| X | Otvoriť priečinok | Skenovať označené alebo zvýraznenú cestu |
| Kruh | Nadradený priečinok; zo spoločných výsledkov ponuka ciest | Zavrieť ponuku |
| Trojuholník | Otvoriť ponuku ciest | Zavrieť ponuku |
| R3 | — | Zrušiť označenia |
| Select | Zopakovať sken | Obnoviť cesty |
| Start | Návrat do XMB | Návrat do XMB |

Kruh preruší sken a ponechá čiastočné výsledky. Ovládanie sa kontroluje medzi operáciami so súbormi; blokujúca operácia môže reakciu oddialiť. Označenia platia počas aktuálneho spustenia aplikácie.

## Cesty

Kontrolujú sa `/dev_hdd0`, rozpoznané `/dev_usb000` až `/dev_usb127` a `/dev_sd`, `/dev_ms`, `/dev_cf`.

- PS3: `game`, `GAMES`, `GAMEZ`, `PS3ISO`, `GAMEI`.
- PS1/PS2: `PSXISO`, `PSXGAMES`, `PS2ISO`, `PS2DISC`, `CD`, `DVD`, `ROMS/PSXISO`, `ROMS/PS2ISO`.
- Ostatné: `PSPISO`, `ISO`, `ROMS`, `BDISO`, `DVDISO`, `video`, `packages`, `Packages`, `PKG`.
- Varianty: `GAMES_DUP`, `GAMES_BAD`, `[auto]`, profily `_1` až `_4`, staršie manažéry a korene zariadení.

Presný zoznam je v [include/paths.h](include/paths.h). Podpora cesty znamená meranie, nie schopnosť spustiť každý formát z daného zariadenia.

Vlastné cesty pridáš nahratím [paths.example.txt](dist/paths.example.txt) ako `/dev_hdd0/game/STOR00001/USRDIR/paths.txt`. Najviac 32 absolútnych lokálnych ciest, jedna na riadok, bez koncového lomítka. Prázdne riadky a komentáre `#` sa ignorujú. Použi UTF-8 bez BOM, bez segmentov `.` a `..`. Select v ponuke obnoví zoznam.

## Veľkosti a obmedzenia

Veľkosť je súčet dĺžok súborov, nie fyzicky alokované miesto. **1 GiB = 1 073 741 824 bajtov.** Záloha v `GAMES` a inštalačné dáta v `game` zaberajú samostatné miesto; automaticky sa nespájajú pod jednu hru. Kópie, pevné odkazy a aliasy zariadení sa nededuplikujú.

Časti ISO, BIN/CUE a obaly sú samostatné položky, ak nie sú zoskupené v priečinku. Názvy vnútri ISO sa nečítajú. Limity: 8 192 výsledkov, hĺbka 64, cesta do 1 023 bajtov. Font je ASCII; nepodporované znaky sa zobrazia ako `?`. Chyby, `[?]` a `CANCELLED` môžu znamenať neúplný výsledok.

Nie sú implementované špeciálne ovládače NTFS/exFAT ani sieťový sken cez ps3netsrv. Skenujú sa iba natívne dostupné cesty.

## Overenie a zostavenie

Používateľ otestoval staršiu verziu 0.1.2 na CECHL04 s firmvérom 4.92, hláseným HEN/Cobra a obrazom 1080p: sken, veľkosti, názvy a návrat do XMB fungovali. Nejde o úplné overenie verzie 0.1.7 na konzole. Neskoršie stránkovanie, výber ciest a ikona boli kontrolované na PC; obsah PKG bol po dešifrovaní porovnaný s výstupom zostavenia.

Desktopové testy overujú veľkosti, veľké súbory, neplatné metadáta, zoradenie, prerušenie, prekrývajúce sa cesty, stránkovanie a simulované vykresľovanie. Nenahrádzajú konzolový test.

Python 3 a Zig, GCC alebo Clang:

```sh
python tests/run_tests.py
```

Kompilátor pridaj do PATH alebo nastav `ZIG_EXE` / `CC` na jeho spustiteľný súbor. `CC` neprijíma dodatočné prepínače. SDK, binárne nástroje a podporné podpisovacie dáta nie sú priložené. Postup je v [BUILDING.md](docs/BUILDING.md).

`source` a `include` obsahujú kód, `assets` ikonu, `dist` aktuálny PKG a testovacie dáta, `tools` skripty, `tests` testy, `docs` návody.

Ikonu zmeníš cez `assets/ICON0.PNG` (PNG 320 × 176) a nové zostavenie. Skript `tools/prepare_icon.ps1` ju vytvorí z priloženého priehľadného obrázka.

Referencie: [PSL1GHT](https://github.com/ps3dev/PSL1GHT), [PSDK3v2](https://github.com/Estwald/PSDK3v2), [cesty webMAN MOD](https://github.com/aldostools/webMAN-MOD/wiki/Game-Paths-%26-Covers).
