// base Audio Engine and File classes

#pragma once

#include "ManaGlobals.h"
#include "target/TargetOS.h"
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include "base/File.h"
#include "base/IThread.h"

namespace Mana
{
	typedef size_t AudioFileHandle;

	enum class AudioCategory { Sound, Music, Voice };
	enum class AudioLoadType { Static, Streaming };
	enum class AudioFormat { Wav, Ogg, Mod };

	// internal class used by the Audio engine
	class AudioFileBase
	{
	protected:
		AudioFileBase();
		// dtor doesn't stop sounds or destroy audio buffers.
		// The Audio engine handles this.
		virtual ~AudioFileBase();

	public:
		AudioFileHandle audioFileHandle;
		xstring filePath;
		AudioCategory category;
		AudioLoadType loadType;
		AudioFormat format;
		float pan;
		bool isPaused;
		size_t fileSize;				// raw file size
		//WAVEFORMATEXTENSIBLE wfx;
		BYTE* pDataBuffer;				// pcm buffer
		size_t dataBufferSize;			// pcm buffer size
		size_t currentStreamBufIndex;	// 0 to (AudioStreamBufCount - 1)
		size_t currentBufPos;			// 0 to (dataBufferSize - 1)

		bool lastBufferPlaying;

		UINT32 playBegin;               // First sample in this buffer to be played.
		UINT32 playLength;              // Length of the region to be played in samples,
										//  or 0 to play the whole buffer.
		UINT32 loopBegin;               // First sample of the region to be looped.
		UINT32 loopLength;              // Length of the desired loop region in samples,
										//  or 0 to loop the entire buffer.
		UINT32 loopCount;               // Number of times to repeat the loop region,
										//  or XAUDIO2_LOOP_INFINITE to loop forever.

		// TODO: maybe use an interface here?
		void* lib; // openmpt::module,  etc

		//friend class AudioBase;
	};

	class AudioBase;
	extern AudioBase* g_pAudioEngine;

	class AudioBase
	{
	public:
		AudioBase() {}
		// TODO: maybe in debug build, assert if there are still files loaded?
		virtual ~AudioBase() {}

		virtual bool Init() = 0;
		virtual void Uninit() = 0;

		// Returns non-zero for success.
		virtual AudioFileHandle Load(xstring filePath,
			AudioCategory category, AudioFormat format,
			int simultaneousSounds = 1) = 0;
		// stops and destroys all voices and the buffer
		// and removes from m_fileMap
		virtual void Unload(AudioFileHandle audioFileHandle) = 0;
		
		// This version of play doesn't modify looping fields
		virtual bool Play(AudioFileHandle audioFileHandle) = 0;
		// loopBegin and loopLength are in pcm samples.
		virtual bool Play(AudioFileHandle audioFileHandle,
			unsigned loopCount, unsigned loopBegin = 0, unsigned loopLength = 0) = 0;

		virtual void Stop(AudioFileHandle audioFileHandle) = 0;
		void StopAll();

		// Pause only works for files that only have 1 simultaneous sound
		// (doesn't make sense otherwise)
		// Call Play or Resume to continue playing.
		virtual void Pause(AudioFileHandle audioFileHandle) = 0;
		// Call ResumeAll to continue playing all paused voices.
		void PauseAll();

		virtual void Resume(AudioFileHandle audioFileHandle) = 0;
		void ResumeAll();

		virtual float GetVolume(AudioFileHandle audioFileHandle) = 0;
		// get average volume of sounds in the category,
		// since some may have been set individually
		virtual float GetVolume(AudioCategory category) = 0;
		virtual float GetMasterVolume() = 0;

		// allows developers to tweak volumes of individual files
		virtual void SetVolume(AudioFileHandle audioFileHandle, float volume) = 0;
		// allows users to set per-category volumes
		virtual void SetVolume(AudioCategory category, float volume) = 0;
		// allows users to set master volume
		virtual void SetMasterVolume(float& volume) = 0;

		// uses simple linear panning
		// pan of -1.0 is all left, 0 is middle, 1.0 is all right.
		virtual void SetPan(AudioFileHandle audioFileHandle, float pan) = 0;
		virtual float GetPan(AudioFileHandle audioFileHandle) = 0;

		// returns true if at least one voice of this sound is playing
		virtual bool IsPlaying(AudioFileHandle audioFileHandle) = 0;
		virtual bool IsPaused(AudioFileHandle audioFileHandle) = 0;

		std::vector<AudioFileBase*> GetStreamingFiles();

	protected:
		// this does not include simultaneous
		// versions of the same sound
		const AudioFileHandle MAX_SOUNDS_LOADED = 500;

		//IXAudio2* m_pXAudio2 = nullptr;
		//IXAudio2MasteringVoice* m_pMasterVoice = nullptr;
		std::map<AudioFileHandle, AudioFileBase*> m_fileMap;

		AudioFileHandle m_lastAudioFileHandle = 0;
		AudioFileHandle GetNextFreeAudioFileHandle();

		AudioFileBase* GetAudioFile(AudioFileHandle audioFileHandle);
		virtual void ClampVolume(float& volume) = 0;

	private:
		virtual bool Play(AudioFileHandle audioFileHandle, bool updateLoopFields,
			unsigned loopCount, unsigned loopBegin, unsigned loopLength) = 0;
	};
}