# NdlDownload2023

2023-06-13  
国立国会図書館デジタルコレクションのPIDに基づく資料取得。
2022年12月頃の改修に合わせて作成。  
改修前後の解像度の対応は恐らく以下の通り。
| 改修前 | | 改修後 |
----|---- |---- 
OutputFile|=| JSON記述ファイル
JSON記述ファイル|=| 高解像度
一括ダウンロード|=| 通常の解像度

## 経緯
数ある電子資料の公開サイトの中で、国立国会図書館の抱える大きな問題点として、次の二つが挙げられる。  
1. 低画質・大容量  
全コマダウンロードはjpgから作成したpdfであるため、画質に見合わぬ大容量である。  
長期保管用画像はJPEG 2000らしいが、取得できない。([資料](https://www.ndl.go.jp/jp/library/training/remote/pdf/siryo_remote_digi_basic_2019.pdf)74頁)  
2. 余白の占有  
取り込み範囲と資料の外枠との間に大きな余白がある。

欲しいのは以下の条件を満たすpdf形式のファイルである。
1. 可能な限り高画質
2. 低容量
3. 余白裁ち落とし済み

(1)の条件のためには全コマダウンロードは利用できない。jpg画像を各個取得する必要がある。  
(2)はTIFF group4形式によって実現する。  
(3)は外枠を画像処理によって検出することで実現する。

## 実現方法
1. ファイル取得  
⇒ WinAPI → このプログラム
2. 裁ち落とし、画像補正、見開き分割  
⇒ OpenCV →[ImaginisConvertens](https://github.com/BithreenGirlen/ImaginisConvertens)
3. 二値化、Multi-TIFF作成  
⇒ .Net →[Tiff](https://github.com/BithreenGirlen/Tiff)
4. 文字認識、pdf変換  
⇒ Tesseract

## 成果例
『近代日本文學大系 第十二巻 黄表紙集』  
[原資料](https://dl.ndl.go.jp/pid/1883416)  
[手直し製本](https://cloud.mail.ru/public/bCEY/irWJWE9nS)

全コマダウンロードで作成されるpdfの151MBだが、手直したものは43.8 MB。  
外枠の余白もなく、頁毎に分割されており、不満なく読める状態だろう。

## 使い方

資料のPIDを入力するだけ。
