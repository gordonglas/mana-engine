#pragma once

#include "mana-globals.h"
#include "target/target-os.h"
#include <xaudio2.h>
#include <string>
#include <map>
#include <vector>

#pragma comment(lib, "xaudio2_9redist.lib")

namespace Mana
{
	typedef size_t AudioFileHandle;

	enum class AudioCategory { Sound, Music, Voice };
	enum class AudioLoadType { Static, Streaming };
	enum class AudioFormat { Wav, Ogg, Mod };

	// internal class used by the Audio engine
	class AudioFile
	{
	public:
		AudioFile();
		// dtor doesn't stop sounds or destroy xaudio2 buffers.
		// The Audio engine handles this.
		~AudioFile();

	private:
		AudioFileHandle audioFileHandle;
		std::wstring filePath;
		AudioCategory category;
		AudioLoadType loadType;
		AudioFormat format;
		float pan;
		bool isPaused;

		WAVEFORMATEXTENSIBLE wfx;
		BYTE* pDataBuffer;
		XAUDIO2_BUFFER buffer;

		std::vector<IXAudio2SourceVoice*> sourceVoices;
		size_t sourceVoicePos;

		friend class Audio;
	};

	// XAudio2.9 wrapper with focus on simple indie games.
	// Can load: static uncompressed wav, streaming ogg vorbis, and various mod formats.
	// Assumes functions are all called on the same (usually main) thread.
	class Audio
	{
	public:
		static const unsigned MAX_LOOP_COUNT = XAUDIO2_MAX_LOOP_COUNT;
		static const unsigned LOOP_INFINITE = XAUDIO2_LOOP_INFINITE;

		bool Init();
		void Uninit();

		// Returns non-zero for success.
		AudioFileHandle Load(std::wstring filePath,
			AudioCategory category, AudioLoadType loadType,
			AudioFormat format, int simultaneousSounds = 1);
		void Unload(AudioFileHandle audioFileHandle);
		
		// This version of play doesn't modify looping fields
		bool Play(AudioFileHandle audioFileHandle);
		// loopBegin and loopLength are in pcm samples.
		bool Play(AudioFileHandle audioFileHandle,
			unsigned loopCount, unsigned loopBegin = 0, unsigned loopLength = 0);

		void Stop(AudioFileHandle audioFileHandle);
		void StopAll();

		// Pause only works for files that only have 1 simultaneous sound
		// (doesn't make sense otherwise)
		// Call Play or Resume to continue playing.
		void Pause(AudioFileHandle audioFileHandle);
		// Call ResumeAll to continue playing all paused voices.
		void PauseAll();

		void Resume(AudioFileHandle audioFileHandle);
		void ResumeAll();

		float GetVolume(AudioFileHandle audioFileHandle);
		// get average volume of sounds in the category,
		// since some may have been set individually
		float GetVolume(AudioCategory category);
		float GetMasterVolume();

		// allows developers to tweak volumes of individual files
		void SetVolume(AudioFileHandle audioFileHandle, float volume);
		// allows users to set per-category volumes
		void SetVolume(AudioCategory category, float volume);
		// allows users to set master volume
		void SetMasterVolume(float& volume);

		// uses simple linear panning
		// pan of -1.0 is all left, 0 is middle, 1.0 is all right.
		void SetPan(AudioFileHandle audioFileHandle, float pan);
		float GetPan(AudioFileHandle audioFileHandle);

		// returns true if at least one voice of this sound is playing
		bool IsPlaying(AudioFileHandle audioFileHandle);
		bool IsPaused(AudioFileHandle audioFileHandle);
	private:
		// this does not include simultaneous
		// versions of the same sound
		const AudioFileHandle MAX_SOUNDS_LOADED = 500;
		// 0 is silent
		const float AUDIO_MIN_VOLUME = 0.0f;
		// 1.0 is no attenuation or gain.
		// Setting it any higher can result in clipping,
		// although clipping can occur at 1.0f as well.
		// See: https://stackoverflow.com/a/40223133
		const float AUDIO_MAX_VOLUME = 1.0f;

		IXAudio2* m_pXAudio2;
		IXAudio2MasteringVoice* m_pMasterVoice;
		std::map<AudioFileHandle, AudioFile*> m_fileMap;

		AudioFileHandle m_lastAudioFileHandle;
		AudioFileHandle GetNextFreeAudioFileHandle();

		AudioFile* GetAudioFile(AudioFileHandle audioFileHandle);
		void ClampVolume(float& volume);

		bool Play(AudioFileHandle audioFileHandle, bool updateLoopFields,
			unsigned loopCount, unsigned loopBegin, unsigned loopLength);
	};
}

// speaker layout macros that correspond to WAVEFORMATEXTENSIBLE format
// used for panning

#ifndef SPEAKER_FRONT_LEFT
	#define SPEAKER_FRONT_LEFT            0x00000001
	#define SPEAKER_FRONT_RIGHT           0x00000002
	#define SPEAKER_FRONT_CENTER          0x00000004
	#define SPEAKER_LOW_FREQUENCY         0x00000008
	#define SPEAKER_BACK_LEFT             0x00000010
	#define SPEAKER_BACK_RIGHT            0x00000020
	#define SPEAKER_FRONT_LEFT_OF_CENTER  0x00000040
	#define SPEAKER_FRONT_RIGHT_OF_CENTER 0x00000080
	#define SPEAKER_BACK_CENTER           0x00000100
	#define SPEAKER_SIDE_LEFT             0x00000200
	#define SPEAKER_SIDE_RIGHT            0x00000400
	#define SPEAKER_TOP_CENTER            0x00000800
	#define SPEAKER_TOP_FRONT_LEFT        0x00001000
	#define SPEAKER_TOP_FRONT_CENTER      0x00002000
	#define SPEAKER_TOP_FRONT_RIGHT       0x00004000
	#define SPEAKER_TOP_BACK_LEFT         0x00008000
	#define SPEAKER_TOP_BACK_CENTER       0x00010000
	#define SPEAKER_TOP_BACK_RIGHT        0x00020000
	#define SPEAKER_RESERVED              0x7FFC0000
	#define SPEAKER_ALL                   0x80000000
	#define _SPEAKER_POSITIONS_
#endif

#ifndef SPEAKER_STEREO
	#define SPEAKER_MONO             (SPEAKER_FRONT_CENTER)
	#define SPEAKER_STEREO           (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT)
	#define SPEAKER_2POINT1          (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY)
	#define SPEAKER_SURROUND         (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER)
	#define SPEAKER_QUAD             (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT)
	#define SPEAKER_4POINT1          (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT)
	#define SPEAKER_5POINT1          (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT)
	#define SPEAKER_7POINT1          (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER)
	#define SPEAKER_5POINT1_SURROUND (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT)
	#define SPEAKER_7POINT1_SURROUND (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT  | SPEAKER_SIDE_RIGHT)
#endif
