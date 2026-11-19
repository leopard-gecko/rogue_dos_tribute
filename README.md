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

## この移植版の主な特徴 / Main characteristics of this port

Rogue DOS Tributeでは、ゲームデータと一部のルールをDOS版Rogue 1.48を参考に調整するとともに、現代の環境で利用するための機能を追加しています。

*Rogue DOS Tribute adapts game data and selected rules with reference to DOS Rogue 1.48, while adding features for modern systems.*

- UTF-8による日本語・英語の表示と、プレイ中の表示言語の切り替えに対応。  
  *UTF-8 Japanese/English display with language switching during play.*
- macOS・Linux向けのMakefileによるビルドと、設定用の軽量な `configure` スクリプトを用意。  
  *Makefile-based builds on macOS/Linux with a lightweight `configure` helper.*
- 標準ビルドでカラー表示、識別の巻き物の1種類化、通常プレイ中のカーソル非表示を既定に設定。  
  *Standard builds default to color, a single identify-scroll type, and a hidden cursor during ordinary play.*
- 拡張文字の使用を `extchars` / `noextchars` で切り替え可能。  
  *Extended characters can be enabled or disabled with `extchars` / `noextchars`.*
- 通路・迷路をDOS版風に表示し、通路上のプレイヤーには専用色を使用。  
  *DOS-style corridor and maze-passage rendering, with a dedicated player color in passages.*
- 水薬を怪物に投げたときの特殊効果を追加。  
  *Special effects when throwing potions at monsters.*
- 水薬の種類を調整し、`paralysis` と `thirst quenching` を追加。  
  *A DOS-style potion list, including `paralysis` and `thirst quenching`.*
- 識別の巻き物を1種類にし、`blank paper` と `vorpalize weapon` の巻き物を追加。  
  *One identify-scroll type, plus `blank paper` and `vorpalize weapon` scrolls.*
- Ice Monsterの凍結攻撃（`frost`）とslimeの分裂を追加。  
  *Ice Monster frost attacks and slime splitting.*
- `striking` のつえ・魔法の棒、crossbow、crossbow boltを追加。  
  *The wand/staff of striking, crossbow, and crossbow bolts.*
- 怪物の名前・能力値・フラグ・出現テーブルをDOS版Rogue 1.48を参考に調整。  
  *Monster names, statistics, flags, and appearance tables adjusted toward DOS Rogue 1.48.*
- `PROGRAM` で実行ファイル名を変更すると、既定のセーブ・スコア・ロックファイル名も連動して変更。  
  *Executable naming with `PROGRAM`, which also changes the default save, score, and lock filenames.*
- 日本語・英語の案内書とmanページの内容を、このソースの仕様に合わせて整備。  
  *Japanese and English guides and manual pages updated to reflect this source.*

## 著作権・ライセンス / Copyright and license

原典と既存コードの著作権・ライセンス、およびこの派生版での変更については、[LICENSE.TXT](LICENSE.TXT)をご覧ください。オリジナルのライセンス条項は維持しています。

*See [LICENSE.TXT](LICENSE.TXT) for the original authors' copyright notices and license terms, third-party portions, and project-specific notes. The original license notices remain unchanged.*
