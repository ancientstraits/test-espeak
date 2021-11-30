#include <stdio.h>
#include <ctype.h>
#include <espeak-ng/speak_lib.h>


FILE* outwav = NULL;

// Write 4 bytes to a file, least significant first
static void wav_4byte(int value) {
	for (int ix = 0; ix < 4; ix++) {
		fputc(value & 0xff, outwav);
		value = value >> 8;
	}
}

static int wav_open(const char* path, int rate) {
	static unsigned char wave_hdr[44] = {
		'R',  'I',  'F',  'F',
		0x24, 0xf0, 0xff, 0x7f,
		'W',  'A',  'V',  'E',
		'f',  'm',  't',  ' ',
		0x10, 0, 0, 0, 1, 0, 1, 0,  9, 0x3d, 0, 0, 0x12, 0x7a, 0, 0,
		2, 0, 0x10, 0,
		'd', 'a', 't', 'a',  0x00, 0xf0, 0xff, 0x7f
	};

	if (path == NULL)
		return 0;
	
	while (isspace(*path)) path++;

	outwav = fopen(path, "wb");
	if (outwav == NULL) {
		perror(path);
		return 0;
	}

	fwrite(wave_hdr, 1, 24, outwav);
	wav_4byte(rate);
	wav_4byte(rate * 2);
	fwrite(&wave_hdr[32], 1, 12, outwav);

	return 1;
}

static void wav_close() {
	fflush(outwav);
	unsigned long size = ftell(outwav);

	if (fseek(outwav, 4, SEEK_SET) != -1)
		wav_4byte(size - 8);
	else
		perror("Failed to write filesize");
	
	if (fseek(outwav, 40, SEEK_SET) != -1)
		wav_4byte(size - 44);
	else
		perror("Failed to write wav data size");
	
	fclose(outwav);
	outwav = NULL;
}

static int samplerate;
unsigned int samples_total = 0;
unsigned int samples_split = 0;
unsigned int samples_split_seconds = 0;
unsigned int wavefile_count = 0;
char fname[210];
static int SynthCallback(short *wav, int numsamples, espeak_EVENT *events) {
	if (wav == NULL)
		return 0;	
	for (; events->type != 0; events++) {
		switch(events->type) {
		case espeakEVENT_SAMPLERATE:
			samplerate = events->id.number;
			samples_split = samplerate * samples_split_seconds;
			break;
		case espeakEVENT_SENTENCE:
			if ((samples_split > 0) && (samples_total > samples_split)) {
				wav_close();
				samples_total = 0;
				wavefile_count++;
			}
			break;
		default:
			break;
		}
	}
	if (outwav == NULL) {
		if (samples_split > 0) {
			// THIS NEVER HAPPENS
			sprintf(fname, "%s_%.2d%s", "out.wav", wavefile_count + 1, ".wav");
			if (!wav_open(fname, samplerate))
				return 1;
		} else if (!wav_open("out.wav", samplerate))
			return 1;
	}

	if (numsamples > 0) {
		samples_total += numsamples;
		fwrite(wav, numsamples*2, 1, outwav);
	}
	return 0;
}

int main(int argc, char* argv[]) {
	char voicename[] = "English"; // Set voice by its name
	char text[] = "Hello world!";
	int buflength = 0, options = espeakINITIALIZE_DONT_EXIT;
	unsigned int position = 0, position_type = 0, end_position = 0, flags = espeakCHARS_AUTO;

	espeak_AUDIO_OUTPUT output = AUDIO_OUTPUT_SYNCHRONOUS;
	char *path = NULL;
	void* user_data;
	unsigned int *identifier;

	espeak_Initialize(output, buflength, path, options);
	espeak_SetSynthCallback(SynthCallback);

	espeak_SetVoiceByName(voicename);

	espeak_Synth(text, buflength, position, position_type, end_position, flags, identifier, user_data);

	if (outwav != NULL)
		wav_close();

	return 0;
}