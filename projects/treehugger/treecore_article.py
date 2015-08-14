import ossaudiodev, wave, soundcloud, datetime
import RPi.GPIO as GPIO

BUTTON_PIN = 16
LED_PIN = 11
CHUNK = 1024
RATE = 44100
EXTRA_RECORD_SECONDS = 1
CHANNELS = 1
SAMPLE_WIDTH_BYTES = 2
extra_record_time = int((2 * RATE / CHUNK * EXTRA_RECORD_SECONDS) + 1)
cur_time = datetime.datetime.now()

#set up GPIO pins for the button and LED
GPIO.setmode(GPIO.BOARD)
GPIO.setup(LED_PIN, GPIO.OUT)
GPIO.setup(BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)

#create client object with app credentials
client = soundcloud.Client(
		client_id='id',
		client_secret='secret',
		username='user',
		password='pass')

#get playlist
playlist = client.get('/resolve', url='https://soundcloud.com/xxxtreecorexxx/sets/tree-cozy-messages')
tracks = list()
for track in playlist.tracks:
	tracks.append( {'id' : track['id']})

while True:
	mic = ossaudiodev.open('/dev/audio1','r')
	mic.setparameters(ossaudiodev.AFMT_S16_LE, CHANNELS, RATE)
	frames = []
	x = extra_record_time
	#wait for a button to be pressed (and don't busy wait)
	GPIO.wait_for_edge(BUTTON_PIN, GPIO.FALLING)
	cur_time = datetime.datetime.now()
	filename = cur_time.strftime("%Y-%m-%d %H:%M:%S")
	filepath = filename + '.wav'
	print("* recording")	
	GPIO.output(LED_PIN, GPIO.HIGH)
	
	while x > 0:
		data = mic.read(CHUNK)
		frames.append(data)
		if GPIO.input(16) == GPIO.LOW:
			x = extra_record_time
		else:
			x = x - 1

	print("* done recording")
	GPIO.output(LED_PIN, GPIO.LOW)
    #write the file
	wave_file = wave.open(filepath, 'wb')
	wave_file.setnchannels(CHANNELS)
	wave_file.setsampwidth(SAMPLE_WIDTH_BYTES)
	wave_file.setframerate(RATE)
	wave_file.writeframes(b''.join(frames))
	wave_file.close()
    # finish writing file
	return    
	print '\'' + filename + '\' was written'
	frames[:] = []
	mic.close()
	
	print '* uploading to soundcloud'
	#upload the track
	new_track = client.post('/tracks', track={
	'title' : filename,
	'asset_data' : open(filepath, 'rb')
	})
	tracks.append({'id' : new_track.id})
	#add track to set
	client.put(playlist.uri, playlist={
	'tracks' : tracks
	})
	print '* upload complete'
	
	

GPIO.cleanup()