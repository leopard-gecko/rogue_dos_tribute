# Rogue DOS Tribute JEC.092 — Windows（日本語）

この文書では、**Windows 10 / 11（64ビット）** 向けのビルド、起動、環境変数、セーブ・スコアなどの保存先について説明します。ゲームの紹介と各OS向け文書の一覧は[リポジトリのREADME](https://github.com/leopard-gecko/rogue_dos_tribute)、操作方法やアイテムの説明は[「【運命の洞窟】への案内書」](rogue_ja.html)をご覧ください。

English version: [README_windows_en.md](README_windows_en.md)

## 1. 開発環境の準備

ソースからWindows版を作る場合は、**MSYS2 UCRT64** を使用します。<https://www.msys2.org/> からMSYS2をインストールし、スタートメニューの「MSYS2 UCRT64」を開いてください。

```sh
pacman -Syu
```

更新のため再起動が必要と表示されたらUCRT64を開き直して更新を完了し、続いて開発ツールとPDCursesModをインストールします。

```sh
pacman -S --needed make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-binutils mingw-w64-ucrt-x86_64-pdcurses
```

ソースコードを展開したフォルダーへ移動してください。

## 2. ビルド

```sh
make clean
make
```

成功すると **`rogue.exe`** が作成されます。通常のビルドには `make` だけを使用することもできます。

| 目的 | コマンド |
|---|---|
| 通常ビルド | `make` |
| 実行ファイル名を `rogue-dos.exe` にする | `make PROGRAM=rogue-dos`（または `make PROG=rogue-dos`） |
| 日本語を初期言語にする | `make japanese` |
| 英語を初期言語にする | `make english` |
| 通常と同じゲーム設定で、初期言語を英語にする | `make original` |
| ウィザード機能を組み込む | `make wizard` |

通常の `make` では、Windowsの表示言語から初期言語を自動選択します。`make original` は初期言語を英語にするもので、**ゲームルールやカラーなどの既定の設定は変更しません**。ビルド方法を切り替えるときは、`make clean` の後でビルドしてください。

標準ビルドでは、カラー表示、拡張文字の使用、通常プレイ中のカーソル非表示を既定としています。代表的な設定は次のとおりです。

| 設定 | 通常ビルド `make` | `make original` |
|---|---|---|
| 初期言語 | Windowsの表示言語から自動選択 | 英語 |
| `color`（カラー表示） | on | on |
| `extchars`（拡張文字表示） | on | on |
| `cursor`（カーソル表示） | off | off |

DOS版風のキー操作は既定で有効です。起動時に `ROGUEOPTS` で `nodoskeys` を指定すると、別のキー配置を使用できます。オプションの詳細は案内書をご覧ください。

必要に応じてコンパイラやビルド設定を指定できます。

```sh
make CC=gcc
make CFLAGS="-O2"
```

`PROGRAM` と `PROG` を両方指定した場合は、`PROGRAM` が優先されます。実行ファイル名を変更すると、既定のデータファイル名も連動して変更されます。

## 3. 起動と終了

以下は通常の `make` で作成される `rogue.exe` の例です。Windowsバイナリ配布版では、実行ファイル名を `rogue-dos.exe` に読み替えてください。

エクスプローラーで **`rogue.exe` をダブルクリック**するとゲームを開始します。既定のセーブファイルがある場合は、起動時に次の操作を選べます。

| キー | 内容 |
|---|---|
| `R` または `Enter` | 保存した冒険を再開する |
| `N` | 新しい冒険を開始する |
| `F` | 別のセーブファイルを選ぶ |
| `Q` | 終了する |

既定のセーブファイルがなければ、新しいゲームを開始します。セーブファイルを `rogue.exe` にドラッグ＆ドロップして、指定したファイルから再開することもできます。日本語や空白を含むファイルパスに対応しています。

PowerShell、コマンドプロンプト、MSYS2 UCRT64からも起動できます。PowerShellでの例：

```powershell
.\rogue.exe
```

実行時の画面は **80列×24行** を使用します。ダブルクリックで起動した場合は端末ウィンドウのサイズも80×24に調整するよう試みます。既存の端末から起動した場合は、その端末のウィンドウサイズを変更しません。端末の種類や設定によっては、自動調整できない場合があります。

コマンドラインからは、次のオプションも利用できます。

| コマンド | 内容 |
|---|---|
| `rogue.exe -r` | 既定のセーブファイルから再開する |
| `rogue.exe -s` | スコア一覧を表示する |
| `rogue.exe 冒険.save` | 指定したセーブファイルから再開する |

PowerShellでは、実行ファイル名の前に **`.\`** を付けます（例：`.\rogue.exe -s`）。

セーブから再開すると、原則として元のセーブファイルは削除されます。動作確認に使う場合は、事前にコピーを保存してください。日本語や単独の絵文字を入力できますが、複合絵文字の表示は端末やフォントに依存します。

## 4. Windowsでの環境変数の設定

ゲーム起動時の設定は、Windowsの環境変数 **`ROGUEOPTS`** で指定できます。ここではWindowsでの登録方法を説明します。指定できるオプションとその意味については案内書の「オプション」をご覧ください。

**一時的に指定する方法：** 現在開いている端末から起動する場合は、次のように指定します。`language=ja` は設定文字列の一例です。

PowerShell：

```powershell
$env:ROGUEOPTS = 'language=ja'
.\rogue.exe
```

コマンドプロンプト：

```cmd
set "ROGUEOPTS=language=ja"
rogue.exe
```

MSYS2 UCRT64：

```sh
export ROGUEOPTS='language=ja'
./rogue.exe
```

これらの設定は、現在の端末とそこから起動するプログラムに適用されます。別の端末やエクスプローラーからのダブルクリック起動には引き継がれません。

**Windowsに登録して継続して使う方法：**

1. `Win + R` で「ファイル名を指定して実行」を開き、`sysdm.cpl` と入力します。
2. 「詳細設定」タブから「環境変数」を開きます。
3. 「ユーザー環境変数」で「新規」を選び、変数名に `ROGUEOPTS`、変数値に設定文字列（例：`language=ja`）を入力します。既に登録されている場合は「編集」を選びます。
4. 「OK」で閉じ、改めて端末を開いて起動します。反映されない場合はサインアウトしてからサインインし直してください。

不要になった場合は、ユーザー環境変数から `ROGUEOPTS` を削除します。

## 5. セーブ・スコアなどの保存先

既定では、セーブ **`rogue.save`**、スコア **`rogue.scr`**、ロック **`rogue.lck`** の各ファイルをゲームの作業ディレクトリに置きます。保存先は、ソースからの**ビルド時**に `FILEDIR` で指定できます。

MSYS2 UCRT64で専用フォルダーに保存する場合：

```sh
mkdir -p rogue-data
make clean
make FILEDIR=rogue-data
```

Windows形式の絶対パスを `/` 区切りで指定することもできます。

```sh
make clean
make FILEDIR='C:/Games/RogueDOS'
```

通常の `FILEDIR` 指定ではフォルダーを自動作成しないため、事前に作成して書き込み権限を確保してください。次のコマンドで作成することもできます。

```sh
make prepare-filedir FILEDIR='C:/Games/RogueDOS'
```

**Windowsの「保存したゲーム」を使用する場合（個人用ビルド）：**

```sh
make clean
make FILEDIR=saved-games
```

Windowsが管理する現在のユーザーの「保存したゲーム」フォルダーを取得し、その下の `Rogue` フォルダーに保存します。`Rogue` フォルダーが存在しない場合は自動作成します。通常は次の場所ですが、Windows側で保存先を移動している場合にも対応します。

```text
C:/Users/<ユーザー名>/Saved Games/Rogue/
```

この設定では、**ビルドしたユーザーの保存先の絶対パスがEXEに組み込まれます**。別のPCへ配布する実行ファイルには使用せず、通常は `FILEDIR` を指定せずにビルドしてください。

データファイル名は既定で `PROGRAM` と連動します。例えば次のように指定すると、`rogue-dos.exe` に対して `rogue-dos.save`、`rogue-dos.scr`、`rogue-dos.lck` を使用します。

```sh
make PROGRAM=rogue-dos
```

各ファイル名は個別に変更できます。個別指定は `FILEDIR` より優先されます。

```sh
make PROGRAM=rogue-dos SAVEFILE=my.save SCOREFILE=my.scr LOCKFILE=my.lck
```

相対パスの保存先は、起動時の作業ディレクトリを基準とします。`FILEDIR` などの保存先設定を変更するときは `make clean` してから再ビルドしてください。`make clean` は既存のセーブ・スコア・ロックファイルを削除しません。

## 6. 配布ファイルと説明書

ソース一式の配布用アーカイブはMSYS2 UCRT64から作成できます。

```sh
make dist.src
```

`rogue_dos_tribute_jec092-src.tar.gz` が作成されます。

**Windowsバイナリ配布用ZIP** は、次のコマンドで作成できます。

```sh
make clean
make PROGRAM=rogue-dos
make PROGRAM=rogue-dos dist.win
```

`rogue_dos_tribute_jec092_windows.zip` が作成されます。展開すると **`Rogue DOS Tribute`** フォルダーが作成され、その中に **`rogue-dos.exe`** と説明書が入ります。通常の `make` で作成した `rogue.exe` を `make dist.win` で梱包した場合も、ZIP内では `rogue-dos.exe` になります。ただし名前を変更するだけでは、実行ファイルに組み込まれた既定のセーブ・スコア・ロックファイル名は変わりません。配布用には上記の `PROGRAM=rogue-dos` を推奨します。

| ファイル | 用途 |
|---|---|
| `rogue-dos.exe` | Windowsバイナリ配布用ZIPに同梱する実行ファイル |
| `rogue.exe` | 通常の `make` で作成する実行ファイル |
| `rogue-dos.save` / `rogue-dos.scr` / `rogue-dos.lck` | `PROGRAM=rogue-dos` でビルドした場合の既定のデータファイル名 |
| `Makefile` / `Makefile.win` | ソースからのビルド |
| `windows/rogue.ico` / `windows/rogue-icon.rc` | Windows用アイコンリソース |
| `windows/windows-saved-games.ps1` | 「保存したゲーム」の保存先取得・作成 |
| `rogue_ja.html` / `rogue_en.html` | 日本語・英語のHTML案内書 |
| `rogue_ja.txt` / `rogue_en.txt` | 日本語・英語のテキスト案内書 |
| `README_mac_linux_ja.md` / `README_mac_linux_en.md` | macOS / Linux向けREADME |

同梱文書の一覧とゲームの紹介は[リポジトリのREADME](https://github.com/leopard-gecko/rogue_dos_tribute)をご覧ください。

## 7. 著作権・ライセンス

オリジナルRogueおよび既存コードの著作権・ライセンス条件、この派生版での日本語化・機能追加・Windows用アイコンなどの変更内容は、[LICENSE.TXT](LICENSE.TXT) に記載しています。
