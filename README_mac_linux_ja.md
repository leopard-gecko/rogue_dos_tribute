# Rogue DOS Tribute JEC.091 — macOS / Linux（日本語）

この文書では、**macOS、Linux、Raspberry Pi OS** でのビルド、起動、インストール、セーブ・スコアなどの保存先について説明します。ゲームの紹介と各OS向け文書の一覧は[リポジトリのREADME](README.md)、操作方法やアイテムの説明は[「【運命の洞窟】への案内書」](rogue_ja.html)をご覧ください。

English version: [README_mac_linux_en.md](README_mac_linux_en.md)

## 1. 開発環境の準備

ソースコードを展開し、ターミナルでそのディレクトリを開きます。

**macOS：** Appleのコマンドラインツールをインストールします。

```sh
xcode-select --install
```

**Ubuntu / Debian / Raspberry Pi OS：**

```sh
sudo apt update
sudo apt install build-essential pkg-config libncurses-dev
```

**Fedora：** `sudo dnf install gcc make pkgconf-pkg-config ncurses-devel`  
**Arch Linux：** `sudo pacman -S --needed base-devel pkgconf ncurses`  
**Alpine Linux：** `sudo apk add build-base pkgconf ncurses-dev`

通常は同梱の `Makefile` を使用して、そのまま `make` でビルドできます。Linuxで使用環境に合わせた設定を保存する場合は、ビルド前に次のように実行します。

```sh
./configure
```

`configure` は、利用できるcursesライブラリを調べて `config.mk` に設定を保存する軽量なスクリプトです。Autoconf製のスクリプトではなく、`Makefile` や `config.h` を生成しません。macOSでもcurses関連の調整が必要な場合は、`--with-curses-cflags` または `--with-curses-libs` を指定できます。

## 2. ビルド

通常は次のコマンドでビルドします。

```sh
make clean
make
```

成功すると **`rogue`** が作成されます。既にビルド済みのファイルがない場合は `make` だけでも構いません。

| 目的 | コマンド |
|---|---|
| 通常ビルド | `make` |
| 実行ファイル名を `rogue-dos` にする | `make PROGRAM=rogue-dos`（または `make PROG=rogue-dos`） |
| 日本語を初期言語にする | `make japanese` |
| 英語を初期言語にする | `make english` |
| 通常と同じゲーム設定で、初期言語を英語にする | `make original` |
| ウィザード機能を組み込む | `make wizard` |

通常の `make` はOSの言語設定に応じて初期言語を選択します。`make original` は初期言語だけを英語にし、**ゲームルールやカラーなどの既定の設定は変更しません**。いずれもプレイ中に表示言語を切り替えられます。

標準ビルドの主な初期設定は次のとおりです。`on` は有効、`off` は無効を意味します。

| 設定 | 通常ビルド `make` | `make original` |
|---|---|---|
| 初期言語 | OSの設定から自動選択 | 英語 |
| `color`（カラー表示） | on | on |
| `extchars`（拡張文字表示） | on | on |
| `cursor`（カーソル表示） | off | off |

DOS版風のキー操作は既定で有効です。起動時に環境変数 `ROGUEOPTS` で `nodoskeys` を指定すると、別のキー配置に切り替えられます。ゲームオプションの詳細は案内書をご覧ください。

`configure` を使用する場合は、次のような指定もできます。

```sh
./configure --program=rogue-dos
./configure --prefix=/usr --program=rogue-dos
./configure --default-language=ja
./configure --ja-man-style=mac
./configure --with-curses-libs="-lncursesw"
```

`--default-language` は `auto`、`ja`、`en` を指定できます。`PROGRAM` と `PROG` を同時に指定した場合は `PROGRAM` が優先されます。コンパイラやビルド用フラグは、例えば `make CC=clang` や `make CFLAGS="-O2"` のように指定できます。

ビルド方法やファイル名などの設定を変更した場合は、`make clean` してから再ビルドしてください。**`make clean` と `make distclean` は、既存のセーブ・スコア・ロックファイルを削除しません。** `make distclean` はビルド生成物に加えて `config.mk` と `config.log` も削除します。

## 3. 起動

ビルドしたフォルダーで次のように起動します。

```sh
./rogue
```

通常起動では新しいゲームが始まります。ターミナルには **80列×24行以上** を用意してください。

| コマンド | 内容 |
|---|---|
| `./rogue -r` | 既定のセーブファイルから再開する |
| `./rogue -s` | スコア一覧を表示する |
| `./rogue 保存名.save` | 指定したセーブファイルから再開する |

起動時の表示言語やオプションは、環境変数 **`ROGUEOPTS`** でも指定できます。

```sh
ROGUEOPTS=language=ja ./rogue
ROGUEOPTS=language=en ./rogue
ROGUEOPTS=language=auto ./rogue
ROGUEOPTS=nodoskeys ./rogue
```

`language=auto` はOSの言語設定から初期言語を判定します。macOS/Linuxでは `LC_ALL`、`LC_MESSAGES`、`LANG` の順に参照します。`ROGUEOPTS` は起動時に読み込まれ、オプション画面で変更できる項目はゲーム中にも変更できます。セーブから再開するときは、保存済みの `terse` と `jump` の値が起動時の指定より優先されます。

操作方法やオプションについては案内書、またはゲーム中の `?` コマンドをご覧ください。

## 4. インストール（任意）

ビルドしたフォルダーで遊ぶ場合、インストールは不要です。通常は次のコマンドで `/usr/local` 以下に実行ファイル、日英のmanページ、同梱文書をインストールします。

```sh
sudo make install
```

別の場所へインストールする場合は `./configure --prefix=DIR` を指定できます。`--program=NAME` で実行ファイル名を変更できます。日本語manページのインストール先は `--ja-man-style=auto|linux|mac|all|none` で選択できます。

パッケージ作成などのために、一時的なインストール先を指定することもできます。

```sh
make install DESTDIR=/tmp/rogue-stage prefix=/usr
```

アンインストールするときは、インストール時と同じ設定で `make uninstall` を実行してください。`make install` はセーブ・スコア・ロックファイルを作成・インストールしません。ただし `FILEDIR` が指定されている場合に限り、そのディレクトリを必要に応じて作成します。

## 5. セーブ・スコアなどの保存先

既定では、セーブ **`rogue.save`**、スコア **`rogue.scr`**、ロック **`rogue.lck`** の各ファイルをゲームの作業ディレクトリに置きます。実行ファイル名を変更すると、既定のデータファイル名も連動して変わります。

**3種類のデータファイルを専用ディレクトリにまとめる例：**

```sh
mkdir -p "$HOME/.local/share/rogue"
./configure --program=rogue-dos --file-dir="$HOME/.local/share/rogue"
make clean
make
```

この例では実行ファイルが `rogue-dos`、データファイルが `rogue-dos.save`、`rogue-dos.scr`、`rogue-dos.lck` になります。直接実行する場合は、上の例のように保存先を先に作成し、プレイするユーザーの書き込み権限を確保してください。`make install` によって保存先ディレクトリを作成する場合も、所有者とアクセス権限に注意してください。

`configure` を使わず、ビルド時に `FILEDIR` と `PROGRAM`（または `PROG`）を指定することもできます。

```sh
make PROGRAM=rogue-dos FILEDIR="$HOME/.local/share/rogue"
```

個別のファイル名は `SAVEFILE`、`SCOREFILE`、`LOCKFILE` で指定できます。個別指定は `FILEDIR` より優先されます。

```sh
make PROGRAM=rogue-dos SAVEFILE=my.save SCOREFILE=my.scr LOCKFILE=my.lck
./configure --program=rogue-dos --save-file=my.save --score-file=my.scr --lock-file=my.lck
```

`--file-dir='~/Documents'` のように引用符付きで `~/` を指定しても、`configure` は現在のユーザーのホームディレクトリに展開してから `config.mk` に保存します。同じ処理は `--save-file`、`--score-file`、`--lock-file`、`--prefix` などのパス指定にも適用されます。`~ユーザー名` の形式は展開しません。

相対パスを指定した場合は、ゲーム起動時の作業ディレクトリを基準とします。保存先やファイル名を変更した後は再ビルドしてください。

## 6. ファイルの説明と配布

| ファイル | 用途 |
|---|---|
| `rogue` | ビルド後の実行ファイル |
| `rogue.save` / `rogue.scr` / `rogue.lck` | セーブ・スコア・ロックファイル（プレイ時に使用） |
| `configure` / `Makefile` | ビルド・設定・インストール |
| `config.mk` / `config.log` | `configure` が生成する設定とログ |
| `rogue.6.en.in` / `rogue.6.ja.in` | 日英のmanページの原稿 |
| `rogue_ja.html` / `rogue_en.html` | 日本語・英語のHTML案内書 |
| `rogue_ja.txt` / `rogue_en.txt` | 日本語・英語のテキスト案内書 |
| `README_windows_ja.md` / `README_windows_en.md` | Windows向けREADME |
| `*.c` / `*.h` | ソースコード |

ソース配布用アーカイブは次のコマンドで作成できます。

```sh
make dist.src
```

`rogue_dos_tribute_jec091-src.tar.gz` が作成され、ソースコード、ビルド用ファイル、ライセンス、README、案内書、manページの原稿が含まれます。

実行ファイルと生成済みmanページを含むアーカイブは、次のコマンドで作成します。

```sh
make dist
```

`rogue_dos_tribute_jec091.tar.gz` が作成されます。ゲームの紹介と同梱文書へのリンクは[リポジトリのREADME](README.md)をご覧ください。

## 7. 著作権・ライセンス

オリジナルRogueおよび既存コードの著作権・ライセンス条件、この派生版での日本語化・機能追加・Windows用アイコンなどの変更内容は、[LICENSE.TXT](LICENSE.TXT) に記載しています。
