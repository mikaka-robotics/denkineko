# 電気ネコ<img height=30px src=https://github.com/mikaka-robotics/denkineko/assets/36753812/7ad4cd7d-8997-4f69-a923-f9299f3f7aea>

- Demo動画<br>
[![demo](http://img.youtube.com/vi/TgMYkIjSbwk/0.jpg)](https://www.youtube.com/watch?v=TgMYkIjSbwk)

### デモの実行方法
事前に[こちらのNotebook](https://colab.research.google.com/drive/1hb-0-26DFwklIStM_WgTVe1c2eLKU9Zp?usp=sharing)でGradio web appを起動して、URL(https://XXXXXXXX.gradio.live/) をコピーしておく

```
git clone https://github.com/mikaka-robotics/denkineko.git
cd denkineko
(Macの場合) brew install portaudio
pip install -r  requirements.txt
python app.py --gradio_url https://XXXXXXXX.gradio.live/
```

上記を起動後、[デモ動画](https://www.youtube.com/watch?v=TgMYkIjSbwk)のように頭のネジに触ることで会話を始められます。  

また、gradio_urlを与えずに`python app.py`のみで起動し、コマンドラインに直接コマンドを打ち込んでロボットを動かしてみることも可能です。(コマンドの詳細は以下)

# 開発者向け

<img width="815" alt="スクリーンショット 2023-05-16 6 19 48" src="https://github.com/mikaka-robotics/denkineko/assets/36753812/5ffbc79d-2635-4f4e-86d9-e2e5e42a340f">


## ロボットとの通信方法の詳細
ロボットとの通信は、USBシリアル通信によって行っています。通信速度は9600bpsです。

ロボットが受け付ける命令は以下です。

|  コマンド名 | 動作 | 
| -------- | -------- | 
| nod    | うなずく nodnodで２回うなずく   | 
| shake_body    | 体を左右に揺らす    | 
| neck数字   | 数字の角度に首を縦に動かす  例: neck80<br>範囲は (下向き  65 ~  90 上向き)    | 
| body数字   | 数字の角度に体を横に動かす    例: body120<br>範囲は (右  0 ~  180 左)    | 
| home    | 首と体をホームポジションに戻す    | 
| ready    | LEDを通常状態にする    | 
| rainbow    | LEDを虹色にする    | 
| lipsync    | LEDをランダムに明滅させる    | 
| sleep    | スリープモードにする(ロボットが寝ます)    | 
| blue    | LEDを青にする    | 
| yellow    | LEDを黄色にする    | 
| red    | LEDを赤にする    | 
| orange    | LEDをオレンジにする    | 
| white    | LEDを白にする    | 
| bright数字    | LEDの明るさを調整する (0~255)<br>例: bright50    | 
| status    | 現在のロボットの状態を送信させる<br>[sleep/ready]    | 

## Arduinoについて
台座部分の基板に[Arduino Leonardo](www.amazon.co.jp/dp/B07C31L2B6)があります。
[こちら](https://github.com/mikaka-robotics/denkineko/blob/main/arduino_sketch/denkineko.ino)に書き込んでいるスケッチを置いています。

## 部品について
仕様している主な部品は以下です。  

|  部品 | 数 | 型番など | 
| -------- | -------- | -------- | 
| サーボモータ    | 2   | www.amazon.co.jp/dp/B09V4PZFCM  | 
| Arduino    | 1   | www.amazon.co.jp/dp/B07C31L2B6 | 
| RGB LED    | 1   | https://akizukidenshi.com/catalog/g/gI-08411/ | 
| 3.3V 3端子レギュレータ   | 1   | https://akizukidenshi.com/catalog/g/gI-14505/| 


## 3Dプリントデータ
[stl_files](https://github.com/mikaka-robotics/denkineko/tree/main/stl_files)フォルダに置いてあります

