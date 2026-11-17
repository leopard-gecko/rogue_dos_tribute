# Rogue DOS Tribute JEC.091 — Windows版README

この文書は、Windows 10 / 11（64ビット）でのビルド・起動・配布について説明します。
本作は5.4系のコードを基盤にDOS版Rogue 1.48を参考にして再構成した派生版で、
DOS 1.48の原典ソースを直接移植したものではありません。
ゲームの操作やアイテムについては rogue_ja.html / rogue_ja.txt を参照してください。
macOS / Linuxでのビルドは README_mac_linux_ja.md を参照してください。

## 必要な環境

MSYS2 の UCRT64 シェルを使用します。PDCursesMod の VT バックエンドのみを使用し、
従来の wincon バックエンドへの切り替えには対応していません。

    pacman -S --needed make zip mingw-w64-ucrt-x86_64-gcc \
        mingw-w64-ucrt-x86_64-binutils mingw-w64-ucrt-x86_64-pdcurses

## ビルド

    make clean
    make
    ./rogue.exe

実行ファイル名を変更できます（PROGRAM が PROG より優先されます）。

    make PROGRAM=rogue-dos
    make PROG=rogue-dos

初期表示言語は通常 OS のUI言語に従います。言語の指定には ROGUEOPTS の
language=ja / language=en を使用します。ROGUE_LANG は使用しません。
DOS版風のキー操作が既定です。ROGUEOPTS=nodoskeys でRogue 5.4系のキー操作に切り替わります。

    make japanese             （初期表示：日本語）
    make english              （初期表示：英語）
    make original             （初期表示：英語。ゲーム内容・色設定は通常版と同じ）
    make wizard               （ウィザード機能をビルド）

上記のビルド切り替えは内部の .win.o を作り直します。既存のセーブ、スコア、
ロックファイルを削除しません。独立した実行ファイルを作る場合は
make PROGRAM=名前 を指定してください。標準のファイル名もその名前に連動します。

    make FILEDIR=games

FILEDIR に保存先を指定できます。ディレクトリは実行前に作成してください。
日本語を含むパスもUTF-8の設定とWindows Unicode APIに対応しています。

## 起動と画面

エクスプローラーからEXEをダブルクリックすると、専用のコンソールを
80×24文字に自動調整します。既存のcmd、PowerShell、UCRT64などの
ターミナルから起動した場合はウィンドウサイズを変更しません。

標準セーブファイルがある状態でダブルクリックすると、次の操作を選べます。

    R / Enter    セーブした冒険を再開
    N            新しい冒険を開始
    F            別のセーブファイルを選択
    Q            終了

標準セーブがない場合は新規ゲームを開始します。セーブファイルをEXEへ
ドラッグ＆ドロップして再開することもできます。

    ./rogue.exe -s           スコア表示
    ./rogue.exe -r           標準の保存先から再開
    ./rogue.exe '冒険.save'  指定ファイルから再開

セーブから再開すると、原則として元のセーブファイルは消費（削除）されます。
検証時には必ずコピーを保存してください。ゲーム中の文字列入力では、日本語と
単独絵文字を使用できますが、複合絵文字の表示は端末に依存します。

## Windows用配布ZIP

MSYS2 UCRT64で通常版をビルドした後、次のコマンドで配布ZIPを作成できます。

    make dist.win

「rogue_dos_tribute_jec091_windows-rogue.zip」には、実行ファイル、
Windows用の日英README、日英案内書（HTML・テキスト）、LICENSE.TXTが入ります。
DLLやテスト用ファイルは同梱しません。ZIP作成にはMSYS2のzipが必要です。

    pacman -S --needed zip

外部DLLの依存関係を調べる場合は、必要に応じて次を実行してください。

    ldd rogue.exe

参照先がすべてWindows標準のSystem32のDLLであれば、追加DLLの同梱は通常不要です。
ただし配布前には、MSYS2が入っていないWindows 10/11上でも起動を確認してください。
ソース配布用アーカイブは `make dist.src` で作成できます。
アイコンは実行ファイルに埋め込まれます。

## GitHub Actionsによるビルド確認

GitHubリポジトリの .github/workflows/build.yml は、push・pull request・手動実行で
Linux、macOS、Windows UCRT64の各ビルドを確認します。通常版と初期言語英語の
make original を別々にビルドし、実行ファイルとWindows配布ZIPを成果物として保存します。

Actionsの成果物はビルド確認用です。配布前にはWindows Terminal/ConsoleHostでの
カーテン・墓標・階層移動・日本語セーブ再開を実機確認してください。
Windows版が別PCでも起動することはビルドだけでは保証されません。
Windows標準以外のDLLへの依存がないことも確認してください。
