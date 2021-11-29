#include <stdio.h>
#include <ctype.h>
#include <espeak-ng/speak_lib.h>


FILE* outwav = NULL;

// Write 4 bytes to a file, least significant first
static void wav_4byte(int value, FILE* fp) {
	for (int ix = 0; ix < 4; ix++) {
		fputc(value & 0xff, fp);
		value = value >> 8;
	}
}

static int wav_open(FILE* fp, const char* path, int rate) {
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

	fp = fopen(path, "wb");
	if (fp == NULL) {
		perror(path);
		return 0;
	}

	fwrite(wave_hdr, 1, 24, fp);
	wav_4byte(rate, fp);
	wav_4byte(rate * 2, fp);
	fwrite(&wave_hdr[32], 1, 12, fp);

	return 1;
}

static void wav_close(FILE* fp) {
	fflush(fp);
	unsigned long size = ftell(fp);

	if (fseek(fp, 4, SEEK_SET) != -1)
		wav_4byte(size - 8, fp);
	else
		perror("Failed to write filesize");
	
	if (fseek(fp, 40, SEEK_SET) != -1)
		wav_4byte(size - 44, fp);
	else
		perror("Failed to write wav data size");
	
	fclose(fp);
	fp = NULL;
}

static int SynthCallback(short *wav, int numsamples, espeak_EVENT *events) {
	if (outwav == NULL && !wav_open(outwav, "out.wav", espeakEVENT_SAMPLERATE))
		return 1;
	if (wav == NULL)
		return 0;	
	if (numsamples > 0)
		fwrite(wav, numsamples*2, 1, outwav);
	for (; events->type != 0; events++) {
		switch(events->type) {
		case espeakEVENT_SAMPLERATE:
			
		}
	}
	return 0;
}

int main(int argc, char* argv[]) {
	char voicename[] = {"English"}; // Set voice by its name
	char text[] = {"Hello world!"};
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
		wav_close(outwav);

	return 0;
}

