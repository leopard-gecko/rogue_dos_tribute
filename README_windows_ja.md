# Rogue DOS Tribute JEC.091 — Windows版README

このソースツリーは、Windows 10 / 11（64ビット）用の実行ファイル `rogue.exe` をビルドします。
macOS / Linuxでのビルド・インストール手順は `README_mac_linux_ja.md` にまとめています。

Rogue DOS Tributeは、Rogue 5.4系（Rogue 5.4.5 JECの移植基盤を含む）の
ソースコードを出発点に、DOS版Rogue 1.48を参考として、
怪物・アイテム・ゲームルール・キー操作・演出を再構成した独立した派生版です。
オリジナルのDOS版Rogue 1.48のソースをそのまま移植したものではありません。
日本語・英語の切り替え、カラー表示、macOS・Linux・Windowsへの対応を含みます。

## ビルド

WindowsではMSYS2の **UCRT64** シェルを使用します。MSYS2をインストールし、
UCRT64シェルで次の開発用パッケージをインストールしてください。

    pacman -S --needed make mingw-w64-ucrt-x86_64-gcc \
        mingw-w64-ucrt-x86_64-binutils mingw-w64-ucrt-x86_64-pdcurses

通常は、ソースツリーのディレクトリで以下だけでビルドできます。

    make

標準の実行ファイルは次の場所に作成されます。

    ./rogue.exe

起動するには次を実行します。

    ./rogue.exe

標準ビルドでは、カラー表示などを有効にした次のオプション既定値を使用します。

    color
    extchars
    nocursor

必要に応じて、コンパイラなどをmakeのコマンドラインから指定できます。

    make CC=gcc
    make CFLAGS="-O2"

ビルドしたオブジェクトを削除して作り直すには、以下を使用します。

    make clean
    make

## 言語選択

ゲームの表示言語は、起動時には環境変数 `ROGUEOPTS` の `language` で指定できます。
UCRT64シェルでの例：

    ROGUEOPTS=language=ja ./rogue.exe
    ROGUEOPTS=language=en ./rogue.exe
    ROGUEOPTS=language=auto ./rogue.exe

`language=ja` は日本語、`language=en` は英語、`language=auto` はOSの
言語設定による自動判定を指定します。`auto` は初期言語の決定にのみ使用し、
ゲーム中のオプションには表示されません。プレイ中はオプション画面の
`language` で、日本語は `J`、英語は `E` を押して切り替えます。
名前や呼び名などの入力文字列はUTF-8として保持されます。

起動時は `ROGUEOPTS` の `language=ja/en/auto` がビルド時の既定値より
優先されます。通常の `make` の初期設定は `auto` で、Windowsでは
ユーザーのUI言語を参照します。

`ROGUEOPTS` は起動時に一度だけ読み込みます。新しいゲームでは
`terse/noterse`、`jump/nojump` を初期設定に使い、セーブからの再開時は
保存済みの `terse`、`jump` の値が優先されます。表示言語は起動時に決まり、
ゲーム中にも変更できます。

OSの言語設定によらず初期言語を固定する場合は、以下を使用します。

    make japanese
    make english
    make original

`make japanese` は日本語、`make english` は英語を初期表示にします。
`make original` は、ゲームルールやカラーなどの既定値を変えず、
初期表示言語を英語にします。

DOS版風のキー操作は既定で有効です。`ROGUEOPTS` に `nodoskeys` を指定すると
Rogue 5.4系を基本とするキー操作に切り替わります。

    ROGUEOPTS=nodoskeys ./rogue.exe

キー操作の詳細は案内書、またはゲーム中の `?` コマンドを参照してください。

## ウィザードモード

ウィザードモードを有効にしてビルドするには、次を使用します。

    make wizard

## プログラム名とデータファイル名

実行ファイル名は、ビルド時に `PROGRAM` で変更できます。`PROG` も別名として
使用でき、同時に指定した場合は `PROGRAM` が優先されます。既定では、
セーブ・スコア・ロックファイル名も実行ファイル名に連動します。

    make PROGRAM=rogue-dos
    make PROG=rogue-dos

この場合、実行ファイルは `rogue-dos.exe`、データファイルは次の名前になります。

    rogue-dos.save
    rogue-dos.scr
    rogue-dos.lck

データファイルの保存先ディレクトリを指定するには `FILEDIR` を使います。

    make PROGRAM=rogue-dos FILEDIR=games

インストールせずに直接起動するときは、指定したディレクトリをあらかじめ
作成してください。日本語を含むファイルパスはWindowsのUnicode APIで扱います。

ファイル名は個別にも指定でき、個別指定は `FILEDIR` より優先されます。

    make PROGRAM=rogue-dos SAVEFILE=my.save SCOREFILE=my.scr LOCKFILE=my.lck

## 起動と画面

エクスプローラーからEXEをダブルクリックすると、専用のコンソールを
80×24文字に自動調整します。既存のcmd、PowerShell、UCRT64などの
ターミナルから起動した場合はウィンドウサイズを変更しません。

標準セーブファイルがある状態でダブルクリックすると、次の操作を選べます。

    R / Enter    セーブした冒険を再開
    N            新しい冒険を開始
    F            別のセーブファイルを選択
    Q            終了

標準セーブがない場合は新規ゲームを開始します。セーブファイルをEXEに
ドラッグ＆ドロップして再開することもできます。

    ./rogue.exe -s           スコア表示
    ./rogue.exe -r           標準の保存先から再開
    ./rogue.exe '冒険.save'  指定ファイルから再開

セーブから再開すると、原則として元のセーブファイルは消費（削除）されます。
検証時にはコピーを保存してください。文字列入力では日本語や単独絵文字が
使用できますが、複合絵文字の表示は端末やフォントに依存します。

## ファイル

標準のセーブファイル：

    $(PROGRAM).save、通常は rogue.save

標準のスコアファイル：

    $(PROGRAM).scr、通常は rogue.scr

標準のロックファイル：

    $(PROGRAM).lck、通常は rogue.lck

## 文書

ソースツリーには以下のユーザー向け文書が含まれています。

    rogue_ja.txt            日本語プレーンテキスト解説書
    rogue_en.txt            英語プレーンテキスト解説書
    rogue_ja.html           日本語HTML解説書
    rogue_en.html           英語HTML解説書
    README_mac_linux_ja.md  macOS / Linux向けREADME（日本語）
    README_mac_linux_en.md  macOS / Linux向けREADME（英語）
    rogue.6.en.in           英語版manページのテンプレート
    rogue.6.ja.in           日本語版manページのテンプレート

ゲームの操作やアイテムの詳細は案内書を参照してください。

## この移植版の主な特徴

Rogue DOS Tributeでは、Rogue 5.4.5 JEC由来の移植・表示基盤を使いながら、
ゲームデータと一部のルールをDOS版Rogue 1.48に近づけています。

- UTF-8の日本語／英語表示と、ゲーム中の表示言語切り替え
- Windows 10 / 11向けのMSYS2 UCRT64ビルドへの対応
- 標準ビルドでカラー表示、識別の巻き物1種類化、通常プレイ中のカーソル非表示を既定にした
- `extchars / noextchars` による拡張文字の切り替え
- DOS版風の通路・迷路表示と通路上のプレイヤー専用色
- 水薬を怪物に投げた際の特殊効果
- DOS版風の水薬表と `paralysis`、`thirst quenching` の追加
- 識別の巻き物の1種類化と `blank paper`、`vorpalize weapon` の追加
- Ice Monsterのfrost攻撃とslimeの分裂
- strikingのつえ／魔法の棒、crossbow、crossbow boltの追加
- 怪物名・能力値・フラグ・出現テーブルのDOS版Rogue 1.48風への調整
- `PROGRAM` 指定に連動した実行ファイル名と標準データファイル名
- このソースの仕様に合わせた日英の案内書・manページ
