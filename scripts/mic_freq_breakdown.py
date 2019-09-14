import signal
import pyaudio
import math
import numpy as np
import scipy.fftpack
import matplotlib.pyplot as plt

FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 44100
CHUNK = RATE * 2	# 2 second chunks
T = 1.0 / RATE		# Sample spacing

def next_power_2(n):
	i = 2
	while i < n:
		i *= 2
	return i

audio = pyaudio.PyAudio()

stream = audio.open(format=FORMAT, channels=CHANNELS,
	rate=RATE, input=True,
	frames_per_buffer=CHUNK)

def signal_handler(signal, frame):
	stream.stop_stream()
	stream.close()
	audio.terminate()

signal.signal(signal.SIGINT, signal_handler)

data = np.frombuffer(stream.read(CHUNK), dtype=np.int16)
xf = np.linspace(0.0, 1.0/(2.0*T), CHUNK/2)
yf = scipy.fftpack.fft(data)
x = np.linspace(0.0, CHUNK*T, CHUNK)
y = np.sin(50.0 * 2.0*np.pi*x) + 0.5*np.sin(80.0 * 2.0*np.pi*x)
fig, ax = plt.subplots()
ax.plot(xf, 2.0/CHUNK * np.abs(yf[:CHUNK//2]))
plt.show()

signal_handler(None, None)