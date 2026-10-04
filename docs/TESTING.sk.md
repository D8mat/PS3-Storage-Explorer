# Testovanie 0.1.7 na PS3 cez FTP

Nainštaluj PKG podľa README. Najprv použi malé dáta so známou veľkosťou.

1. Na PC rozbaľ `dist/USB-TEST.zip`. Napriek pôvodnému názvu funguje aj cez FTP.
2. Celý priečinok `STORAGE_TEST` nahraj do `/dev_hdd0/STORAGE_TEST`. Vytvor aj prázdny priečinok `C_EMPTY`.
3. Do `/dev_hdd0/game/STOR00001/USRDIR/paths.txt` pridaj riadok `/dev_hdd0/STORAGE_TEST`; zachovaj existujúce vlastné cesty. Použi UTF-8 bez BOM.
4. Spusti aplikáciu. Over READY a verziu 0.1.7, tlačidlom Start návrat do XMB a aplikáciu spusti znova.
5. Trojuholník otvorí cesty, Select obnoví zoznam. Vyber testovaciu cestu a stlač X, pričom ostatné cesty nechaj neoznačené.

| Položka | Bajty | Súbory |
| --- | ---: | ---: |
| B_BIG | 3 145 728 | 2 |
| A_SMALL | 1 048 576 | 1 |
| C_EMPTY | 0 | 0 |

Spolu **4 194 304 bajtov**. Presné bajty čítaj v detaile označenej položky; GiB sú zaokrúhlené. Štvorec obráti poradie. V B_BIG má `two_mib.bin` 2 097 152 bajtov a NESTED 1 048 576 bajtov. Kruh sa vráti vyššie.

Potom otestuj knižnicu:

- Štvorcom označ dve existujúce cesty a stlač X. Výsledky majú byť spoločne zoradené. Pri rovnakých názvoch skontroluj úplnú cestu.
- L1/R1 a šípkami prejdi viac strán výsledkov.
- Select zopakuje sken. Kruh počas skenu ho preruší a môže ponechať čiastočné výsledky.
- Start vráti aplikáciu do XMB. Blokujúca operácia so súbormi môže reakciu oddialiť.

Pri zamrznutí zaznamenaj poslednú cestu/stav, vybrané korene, firmvér, homebrew prostredie a rozlíšenie. Skús Start alebo PS menu; ak konzola nereaguje, použi tlačidlo napájania a pri reštarte nechaj dokončiť prípadnú kontrolu úložiska. Zasekávajúci sa sken neopakuj bez preverenia príčiny.

Starší skener používateľ overil na CECHL04/4.92/1080p. Verzia 0.1.7 ešte potrebuje úplné overenie na konzole; desktopové testy kompatibilitu hardvéru nepotvrdzujú.
