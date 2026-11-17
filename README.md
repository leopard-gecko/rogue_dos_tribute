# Rogue DOS Tribute — JEC.091

**Rogue DOS Tribute JEC.091**

Rogueは、文字や記号で描かれた迷宮を探索し、怪物と戦いながら「イェンダーの魔除け」を探す古典的なコンピューターRPGです。

*Rogue is a classic dungeon-crawling computer RPG in which players explore a text-based maze, fight monsters, and search for the Amulet of Yendor.*

**Rogue DOS Tribute** は、Rogue 5.4系のソースコード（Rogue 5.4.5 JECの移植基盤を含む）をもとに、**DOS版Rogue 1.48**を参考として、怪物・アイテム・ゲームルール・キー操作・演出を再構成した派生版です。DOS版Rogue 1.48の原典ソースを直接移植したものではありません。日本語・英語の切り替え、カラー表示、現代のmacOS・Linux・Windowsへの対応を追加しています。

*Rogue DOS Tribute is a derivative based on Rogue 5.4-family source code (including the Rogue 5.4.5 JEC porting framework). Its monsters, items, gameplay rules, controls, and presentation have been reworked with reference to DOS Rogue 1.48. It is not a direct port of the original DOS Rogue 1.48 source. It adds Japanese/English switching, color display, and support for modern macOS, Linux, and Windows.*

## 対応環境とREADME / Platforms and documentation

macOS、Linux（Raspberry Pi OSを含む）、Windows 10 / 11に対応しています。ビルド、起動、オプションやセーブファイルの説明は、OS・言語別のREADMEをご覧ください。

*For build instructions, launching, options, and save-file handling, choose the README for your platform and language.*

| 対象OS / Platform | 日本語 | English |
| --- | --- | --- |
| macOS / Linux / Raspberry Pi OS | [README（日本語）](README_mac_linux_ja.md) | [README (English)](README_mac_linux_en.md) |
| Windows 10 / 11 | [README（日本語）](README_windows_ja.md) | [README (English)](README_windows_en.md) |

macOS / Linuxでは通常 `make`、WindowsではMSYS2 UCRT64環境からビルドできます。詳細は各READMEを参照してください。

*On macOS/Linux, the standard build uses `make`. Windows builds use MSYS2 UCRT64; see the platform-specific instructions for details.*

## 【運命の洞窟】への案内書 / A Guide to the Dungeons of Doom

ゲームの操作、画面表示、アイテム、オプションなどは日英の案内書にまとめています。HTML版とプレーンテキスト版を同梱しています。

*The Japanese and English guides describe controls, displays, items, and options. Both HTML and plain-text editions are included.*

| 言語 / Language | HTML | TXT |
| --- | --- | --- |
| 日本語 | [rogue_ja.html](rogue_ja.html) | [rogue_ja.txt](rogue_ja.txt) |
| English | [rogue_en.html](rogue_en.html) | [rogue_en.txt](rogue_en.txt) |

## 著作権・ライセンス / Copyright and license

原典と既存コードの著作権・ライセンス、およびこの派生版での変更については、[LICENSE.TXT](LICENSE.TXT)をご覧ください。オリジナルのライセンス条項は維持しています。

*See [LICENSE.TXT](LICENSE.TXT) for the original authors' copyright notices and license terms, third-party portions, and project-specific notes. The original license notices remain unchanged.*
