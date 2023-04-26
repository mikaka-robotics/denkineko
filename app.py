from gradio_client import Client
import base64
import argparse
import json
import webrtcvad
import os
import pyaudio
import wave
import time
import threading
from threading import Lock
import serial
import ctypes
import simpleaudio as sa

# 引数の解析を定義する
parser = argparse.ArgumentParser(
    description='Denki Neko Controller', add_help=True)
parser.add_argument(
    '--gradio_url', help='gradio demo url (like https://xxxxxxxx.gradio.live/)')
args = parser.parse_args()
gradio_url = args.gradio_url


class Robot():
    _instance = None
    _lock = Lock()

    def __new__(cls, gradio_url=None):
        # シングルトン
        with cls._lock:
            if cls._instance is None:
                cls._instance = super().__new__(cls)
                cls._instance.serial = None  # シリアル通信のコネクター
                cls._instance.background_read_thread = None
                cls._instance.is_on_switch = False
                cls._instance.is_stop = False
                cls._instance.history = []
                cls._instance.play_obj = None
                cls._instance.is_recording = False
                cls._instance.status = 'ready'
                cls._instance.action = ''
                cls._instance.cureent_th_id = 0
                cls._instance.gradio_url = gradio_url
                if cls._instance.gradio_url is not None:
                    cls._instance.client = Client(cls._instance.gradio_url)
                cls._instance.vad = webrtcvad.Vad(mode=3)

        return cls._instance

    def init(self):
        self.connect()
        self.stop()
        self.history = []
        robot.status = 'sleep'

    def connect(self):
        # シルアルポートに接続する
        if self.serial:
            return True
        for device in os.listdir('/dev'):
            if "tty.usb" in device or "ttyUSB" in device:  # raspiの場合は ttyUSB
                print("connect to " + device)
                try:
                    self.serial = serial.Serial(
                        "/dev/" + device, 9600, timeout=0.10)
                    return True
                except:
                    self.serial = None
                    return False
        return False

    def send_msg(self, msg):
        msg = msg + '\n'
        print("send:" + msg)
        self.serial.write(msg.encode())

    def make_talk_thread(self):
        self.stop()
        time.sleep(0.1)
        self.cureent_th_id += 1
        self.background_read_thread = threading.Thread(name='talk_thread',
                                                       target=self.talk_thread, args=(self.cureent_th_id,))
        self.background_read_thread .setDaemon(True)
        self.background_read_thread .start()
        return

    def stop(self):
        self.status = 'ready'
        self.recording = False
        if self.play_obj is not None:
            self.play_obj.stop()
        for thread_id, thread in threading._active.items():
            if 'talk_thread' in thread.name:
                res = ctypes.pythonapi.PyThreadState_SetAsyncExc(
                    ctypes.c_long(thread_id), ctypes.py_object(SystemExit))
        return

    def is_playing(self):
        if self.play_obj is None:
            return False
        return self.play_obj.is_playing()

    def record(self):
        # PyAudioの設定
        FORMAT = pyaudio.paInt16
        CHANNELS = 1
        RATE = 48000
        CHUNK_DURATION_MS = 30
        PADDING_DURATION_MS = 1500
        CHUNK_SIZE = int(RATE * CHUNK_DURATION_MS / 1000)
        PADDING_SIZE = int(RATE * PADDING_DURATION_MS / 1000)
        START_OFFSET = int(PADDING_SIZE / CHUNK_SIZE)

        audio = pyaudio.PyAudio()
        stream = audio.open(format=FORMAT, channels=CHANNELS,
                            rate=RATE, input=True, frames_per_buffer=CHUNK_SIZE)

        # 録音中のフラグ
        recording = False

        # 録音した音声を格納するリスト
        frames = []
        active_flag = []

        # PyAudioストリームの開始
        stream.start_stream()

        start_time = time.time()

        while 1:
            frame = stream.read(CHUNK_SIZE)
            active = self.vad.is_speech(frame, RATE)
            active_flag.append(active)

            if len(active_flag) > 20:
                active_flag.pop(0)

            frames.append(frame)

            if not recording and sum(active_flag) > 10:
                print('Recording started...')
                self.status = 'recording'
                recording = True
            if not recording and len(frames) > 30:
                frames.pop(0)
            if recording and sum(active_flag) == 0:
                print('Recording stopped...')
                break

            if (time.time() - start_time > 10) and not recording:  # timeout
                break

        if not recording:
            return None

        stream.stop_stream()
        stream.close()
        audio.terminate()

        # 録音された音声を保存する
        wavefile = wave.open('recording.wav', 'wb')
        wavefile.setnchannels(CHANNELS)
        wavefile.setsampwidth(audio.get_sample_size(FORMAT))
        wavefile.setframerate(RATE)
        wavefile.writeframes(b''.join(frames))
        wavefile.close()
        return 'recording.wav'

    def gradio_api_call(self, send_wav_file):
        chat_history_text = json.dumps(self.history, ensure_ascii=False)

        result = self.client.predict(
            send_wav_file,
            chat_history_text,
            api_name="/myChatGPT"
        )

        recognized_text = result[1]
        print(recognized_text)

        response_text = result[2]
        print(response_text)

        chat_history_text = result[3]
        self.history = self.parse_chat_history_text(chat_history_text)

        self.action = result[4]

        received_audio_data = result[5]
        decoded_audio = base64.b64decode(received_audio_data)

        output_filename = "output.wav"

        # デコードされたデータをファイルに書き込む
        with open(output_filename, "wb") as f:
            f.write(decoded_audio)

        return output_filename,

    def talk_thread(self, th_id):
        self.status = 'waiting'
        input_sound_filename = self.record()

        if th_id != self.cureent_th_id:  # 録音中に別の応答が始まっていて場合中断
            return

        if input_sound_filename is None:
            self.status = 'ready'
            return

        self.status = 'thinking'
        if self.gradio_url is not None:
            output_sound_filename = self.gradio_api_call(input_sound_filename)
        else:
            # 対話システムAPIのURLが指定されていません
            output_sound_filename = "syokika.wav"

        self.status = 'speach'
        wave_obj = sa.WaveObject.from_wave_file(output_sound_filename)
        self.play_obj = wave_obj.play()

    def parse_chat_history_text(self, chat_history_text):
        try:
            chat_history_text = chat_history_text.replace("'", "\"")
            chat_history = json.loads(chat_history_text)
            return chat_history
        except:
            return []


def manual_input_thread(robot):
    while True:
        user_input = input("> ")
        robot.send_msg(user_input)


# 対話アプリケーション層
if __name__ == '__main__':
    robot = Robot(gradio_url)
    robot.init()

    manual_input_th = threading.Thread(name='manual_input_thread',
                                       target=manual_input_thread, args=(robot,))
    manual_input_th.setDaemon(True)
    manual_input_th.start()

    # init
    last_on_switch_time = time.time() - 10
    last_robot_status = 'sleep'

    robot.send_msg('home,sleep')

    while True:
        data = robot.serial.readline().strip().decode('utf-8')  # 一行分のデータを受信
        if data == 'touch':
            print('switch on')
            last_on_switch_time = time.time()
            robot.stop()
            robot.make_talk_thread()
            last_robot_status = ''

        if last_robot_status != robot.status:  # statusに変化があったら
            print(robot.status)
            if robot.status == 'ready':
                robot.send_msg('neck70,ready,blue')
            elif robot.status == 'waiting':
                robot.send_msg('ready,red')
            elif robot.status == 'recording':
                robot.send_msg('ready,orange')
            elif robot.status == 'thinking':
                robot.send_msg('rainbow')
            elif robot.status == 'speach':
                robot.send_msg('white,lipsync')
            last_robot_status = robot.status

        if robot.action != "":
            robot.send_msg(robot.action)
            robot.action = ""

        if robot.status == 'speach' and robot.is_playing() is False:
            robot.status = 'ready'
            robot.send_msg('ready,neck70,blue')

        time.sleep(0.01)
