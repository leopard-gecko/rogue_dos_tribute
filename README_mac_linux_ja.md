# Rogue DOS Tribute JEC.091

このソースツリーは、単一の実行ファイル "rogue" をビルドします。
Windows 10/11 用のビルド・起動・配布方法は README_windows_ja.md に
まとめています。以下のビルド・インストール手順は macOS / Linux 用です。

Rogue DOS Tributeは、Rogue 5.4系（Rogue 5.4.5 JECの移植基盤を含む）の
ソースコードを出発点に、DOS版Rogue 1.48を参考として、
怪物・アイテム・ゲームルール・キー操作・演出を再構成した独立した派生版です。
オリジナルのDOS版Rogue 1.48のソースをそのまま移植したものではありません。
日本語・英語の切り替え、カラー表示、macOS・Linux・Windowsへの対応を含みます。

## ビルド

このソースツリーは、同梱されている Makefile をそのまま使用できます。
また、Linux などで環境に合わせた設定を先に行いたい場合のために、
軽量な configure スクリプトも用意しています。この configure は
Autoconf で生成されたものではなく、Makefile が読み込む config.mk を
作成するだけです。Makefile や config.h は生成しません。

macOS では、C コンパイラがまだ使えない場合は Xcode command line tools を
インストールしてください。

Linux では、C コンパイラ、make、pkg-config、および ncurses の
ワイド文字対応開発パッケージをインストールしてください。パッケージ名は
ディストリビューションによって異なりますが、代表的な例は以下の通りです。

    Debian / Ubuntu / Raspberry Pi OS: build-essential pkg-config libncurses-dev
    Fedora:                            gcc make pkg-config ncurses-devel
    Arch Linux:                        base-devel pkg-config ncurses
    Alpine Linux:                      build-base pkgconf ncurses-dev

インストールコマンドの例:

    macOS:
        xcode-select --install

    Debian / Ubuntu / Raspberry Pi OS:
        sudo apt update
        sudo apt install build-essential pkg-config libncurses-dev

    Fedora:
        sudo dnf install gcc make pkgconf-pkg-config ncurses-devel

    Arch Linux:
        sudo pacman -S --needed base-devel pkgconf ncurses

    Alpine Linux:
        sudo apk add build-base pkgconf ncurses-dev

通常は以下だけでビルドできます。

    make

configure を使う場合は、以下のように実行します。

    ./configure
    make

configure は、pkg-config、ncursesw-config、-lncursesw、-lncurses、-lcurses
の順に curses / ncurses を検出し、結果を config.mk に保存します。
config.mk は Makefile から自動的に読み込まれます。

標準のビルドでは、次の実行ファイルが作られます。

    ./rogue

標準のビルドでは、カラー表示などを有効にしたオプション既定値を使用します。

    color
    extchars
    nocursor

ローカル環境に合わせた設定は、make のコマンドラインから指定できます。例:

    make CC=clang
    make CFLAGS="-O2" LDFLAGS="..." LIBS="-lncursesw"

configure を使う場合は、代表的には以下のように指定できます。

    ./configure --program=rdt
    ./configure --prefix=/usr --program=rdt
    ./configure --default-language=ja
    ./configure --ja-man-style=mac
    ./configure --with-curses-libs="-lncursesw"

configure で作成された config.mk と config.log を削除するには、以下を
使用します。

    make distclean

## 配布アーカイブ

配布用のターゲットは2種類あります。

    make dist.src

ソース配布用の rogue_dos_tribute_jec091-src.tar.gz を作成します。ソースを公開する
場合はこちらを使用します。ソースファイル、ビルド用ファイル、ライセンス、README、
案内書、HTML案内書、man ページのテンプレートが含まれます。

    make dist

実行ファイルと man ページをビルドしたうえで、バイナリ／文書配布用の
rogue_dos_tribute_jec091.tar.gz を作成します。実行ファイル、ライセンス、README、
案内書、HTML案内書、生成済み man ページが含まれます。

## 言語選択

ゲームの表示言語は、起動時には ROGUEOPTS の language で指定できます。

    ROGUEOPTS=language=ja ./rogue
    ROGUEOPTS=language=en ./rogue
    ROGUEOPTS=language=auto ./rogue

language=ja は日本語、language=en は英語を指定します。language=auto は
起動時の初期言語を OS の言語設定から自動判定します。auto は初期言語を
決めるためだけのモードで、ゲーム中の言語設定には表示されません。
プレイ中はオプション画面の "language" で、日本語は J、英語は E を押して
切り替えます。ユーザーが入力した名前や呼び名は UTF-8 文字列として保持されます。

起動時の優先順位は、ROGUEOPTS の language=ja/en/auto、ビルド時の既定値、
auto の場合の OS 言語判定の順です。通常の make は auto を既定にします。
Unix 系では LC_ALL、LC_MESSAGES、LANG の順に確認し、日本語ロケールなら
日本語、それ以外は英語になります。Windows ではユーザーの UI 言語から判定します。

ROGUEOPTS は起動時に一度だけ読み込みます。新しいゲームでは terse / noterse、
jump / nojump を初期設定に使用しますが、セーブから再開したゲームでは
保存済みの terse と jump の値を優先します。表示言語は従来どおり起動時に
決まり、ゲーム中にも変更できます。

ユーザーのロケールを無視する既定言語のバイナリを作る場合は、
以下を使用します。

    make japanese
    make english
    make original

make english は標準のオプション既定値のまま、英語を既定言語にします。
make original は、Rogue DOS Tribute のゲームルールとカラーなどの既定値は
そのままに、表示言語の初期値を英語にします。
make japanese は標準のオプション既定値のまま、日本語を既定言語にします。
configure では --default-language=auto、ja、en を指定できます。

DOS版に近いキー操作はデフォルトで有効です。実行時に無効にする場合は、
ROGUEOPTS で次のように指定します。

    ROGUEOPTS=nodoskeys ./rogue

DOS版風キー操作では、+ は投げる、- はつえ/魔法の棒、f は何かを見つけるまで進む
接頭辞、g は拾わずに進む接頭辞、F はマクロの登録・変更、Ctrl-F は
マクロ実行、Ctrl-R は最後のメッセージ再表示、Ctrl-T は簡潔表示の切り替え、
Ctrl-L はDOS版風の画面確認メッセージになります。DOS版風キー操作では M と x は
コマンドではありません。nodoskeys では M でマクロを登録・変更し、x で実行します。
F1〜F10の
変換と、矢印キー、Shift+矢印、Ctrl+矢印の代替入力にも対応します。
このモードでは ! と F10 で fake DOS を開き、rogue と入力するか Esc を押すと
ゲームに戻ります。

デフォルトでは doskeys です。ROGUEOPTS=nodoskeys でDOS版風キー操作を
無効にできます。表示言語も ROGUEOPTS の language=ja/en/auto で指定できます。

標準ビルドの既定値は、カラー表示、拡張文字、識別の巻き物を1種類にする
設定を有効にし、通常プレイ中のカーソルを非表示にします。

## ウィザードモード

ウィザードモードを有効にしてビルドする場合は、以下を使用します。

    make wizard

## プログラム名とデータファイル名

実行ファイル名は、ビルド時に PROGRAM で変更できます。Clone III などの
他の移植版と同じように、PROG も別名として使えます。PROGRAM と
PROG を同時に指定した場合は PROGRAM が優先されます。既定では、
セーブファイル、スコアファイル、ロックファイルの名前も PROGRAM に
連動します。それぞれのファイル名は個別に上書きすることもできます。

例:

    make PROGRAM=rdt
    make PROG=rdt

configure を使う場合は、--program を使用します。

    ./configure --program=rdt

この場合、実行ファイルは rdt として作られ、プレイ時のデータファイルは
以下になります。

    rdt.save
    rdt.scr
    rdt.lck

3つのデータファイルを同じディレクトリに置く場合は、FILEDIR または
--file-dir を指定します。FILEDIR が指定されている場合、make install は
指定されたディレクトリを自動的に作成します。

    make PROGRAM=rdt FILEDIR=$HOME/.local/share/rogue
    make PROGRAM=rdt FILEDIR=$HOME/.local/share/rogue install

configure を使う場合は以下のように指定します。

    ./configure --program=rdt --file-dir=$HOME/.local/share/rogue
    make
    make install

インストールせずにビルドディレクトリから直接実行する場合は、
指定したディレクトリを事前に作成しておいてください。

この場合、データファイルは以下になります。

    ~/.local/share/rogue/rdt.save
    ~/.local/share/rogue/rdt.scr
    ~/.local/share/rogue/rdt.lck

ファイル名を個別に指定することもできます。個別指定は FILEDIR より優先されます。

    make PROGRAM=rdt SAVEFILE=my.save SCOREFILE=my.scr LOCKFILE=my.lck
    ./configure --program=rdt --save-file=my.save --score-file=my.scr --lock-file=my.lck

## インストール

インストールは任意です。実行ファイル、英語版／日本語版のマニュアルページ、同梱文書を
/usr/local 以下にインストールするには、以下を使用します。

    sudo make install

パッケージ作成や一時的なインストール先を指定するために、DESTDIR と
prefix を使用できます。例:

    make install DESTDIR=/tmp/rogue-stage prefix=/usr

configure を使う場合は、インストール先を先に config.mk に保存できます。

    ./configure --prefix=/usr --program=rdt
    make
    make install DESTDIR=/tmp/rogue-stage

アンインストールするには、同じインストール先の指定で以下を使用します。

    sudo make uninstall

素の make install では、日本語 man ページを Linux 向けの ja/man6 と、
macOS 向けの ja_JP.UTF-8/man6・ja.UTF-8/man6 にインストールします。
configure を使う場合は、--ja-man-style=auto によりOSに応じた場所が選ばれます。
必要に応じて --ja-man-style=linux、mac、all、none も指定できます。

make install は *.scr、*.lck、*.save ファイルを作成・インストールしません。
ただし FILEDIR が指定されている場合に限り、そのディレクトリだけは
自動的に作成します。*.scr、*.lck、*.save はプレイ時のデータファイルであり、
実行時のセーブファイル／スコアファイル設定に従ってゲームが作成・使用します。

## ファイル

標準のセーブファイル:

    $(PROGRAM).save、通常は rogue.save

標準のスコアファイル:

    $(PROGRAM).scr、通常は rogue.scr

標準のロックファイル:

    $(PROGRAM).lck、通常は rogue.lck

## 文書

ソースツリーには、以下のユーザー向け文書が含まれています。

    rogue_ja.txt     日本語プレーンテキスト解説書
    rogue_en.txt     このソースに合わせて更新した英語プレーンテキスト解説書
    rogue_ja.html    日本語 HTML 解説書
    rogue_en.html    英語 HTML 解説書
    README_windows_ja.md  Windows版ビルド・起動・配布案内（日本語）
    README_windows_en.md  Windows版ビルド・起動・配布案内（英語）
    rogue.6.en.in    英語版 man ページのテンプレート
    rogue.6.ja.in    日本語版 man ページのテンプレート

README_mac_linux_ja.md は、ビルド、パッケージング、実行時の言語切り替えに関する
日本語の説明です。英語版は README_mac_linux_en.md に分けています。

## この移植版の主な特徴

Rogue DOS Tributeでは、Rogue 5.4.5 JEC由来の移植・表示基盤を使いながら、
ゲームデータと一部のルールをDOS版Rogue 1.48に近づけています。

- UTF-8 の日本語／英語表示に対応し、実行時に表示言語を切り替え可能にした
- macOS と一般的な Linux で使いやすい Makefile ベースのビルド方式に整理し、軽量な configure を追加した
- 標準ビルドでカラー表示、識別の巻き物1種類化、通常プレイ中のカーソル非表示を既定にした
- 拡張文字の使用を extchars / noextchars で切り替えられるようにした
- 通路や迷路をDOS版風に表示し、通路上のプレイヤーには専用色を使うようにした
- 水薬を怪物に投げた時の特殊効果を追加した
- 水薬表を DOS版風にし、paralysis と thirst quenching を追加した
- 識別の巻き物を1種類にし、blank paper と vorpalize weapon を追加した
- Ice Monster の frost 攻撃と slime の分裂を追加した
- striking のつえ/魔法の棒、crossbow、crossbow bolt を追加した
- 怪物名、能力値、フラグ、出現テーブルを DOS版 Rogue 1.48 風に調整した
- PROGRAM 指定により、実行ファイル名と標準のセーブ／スコア／ロックファイル名を連動して変更できるようにした
- 日本語版／英語版の解説文書と man ページの内容を、このソースの仕様に合わせて整理した
