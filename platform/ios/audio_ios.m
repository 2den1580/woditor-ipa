#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>

#include "host.h"

static AudioUnit g_unit;
static double g_rate = 48000;
static bool g_play, g_started;

static OSStatus render_cb(void* ud, AudioUnitRenderActionFlags* flags, const AudioTimeStamp* ts, UInt32 bus,
                          UInt32 frames, AudioBufferList* io) {
  host_audio_mix((int16_t*)io->mBuffers[0].mData, (int)frames, (int)g_rate);
  return noErr;
}

static void session_setup(void) {
  AVAudioSession* s = AVAudioSession.sharedInstance;

  [s setCategory:AVAudioSessionCategorySoloAmbient error:nil];
  [s setPreferredSampleRate:48000 error:nil];
  [s setPreferredIOBufferDuration:0.010 error:nil];
  [s setActive:YES error:nil];
  g_rate = s.sampleRate > 0 ? s.sampleRate : 48000;
}

static void unit_open(void) {
  AudioComponentDescription d = {kAudioUnitType_Output, kAudioUnitSubType_RemoteIO, kAudioUnitManufacturer_Apple, 0, 0};
  AudioComponent comp = AudioComponentFindNext(NULL, &d);
  if (!comp || AudioComponentInstanceNew(comp, &g_unit) != noErr) {
    host_log("audio: RemoteIO failed to open");
    g_unit = NULL;
    return;
  }
  AudioStreamBasicDescription f = {0};
  f.mSampleRate = g_rate;
  f.mFormatID = kAudioFormatLinearPCM;
  f.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
  f.mChannelsPerFrame = 2;
  f.mBitsPerChannel = 16;
  f.mBytesPerFrame = 4;
  f.mFramesPerPacket = 1;
  f.mBytesPerPacket = 4;
  AudioUnitSetProperty(g_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &f, sizeof f);
  AURenderCallbackStruct cb = {render_cb, NULL};
  AudioUnitSetProperty(g_unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &cb, sizeof cb);
  AudioUnitInitialize(g_unit);
  host_log("audio: %.0f Hz, buffer %.1f ms", g_rate, AVAudioSession.sharedInstance.IOBufferDuration * 1000);
}

static void apply_state(void) {
  if (!g_unit) return;
  if (g_play) {
    [AVAudioSession.sharedInstance setActive:YES error:nil];
    AudioOutputUnitStart(g_unit);
  } else {
    AudioOutputUnitStop(g_unit);
  }
}

void audio_start(bool play) {
  dispatch_sync(dispatch_get_main_queue(), ^{
    session_setup();
    unit_open();
    g_play = play;
    g_started = true;
    apply_state();
    [NSNotificationCenter.defaultCenter
        addObserverForName:AVAudioSessionInterruptionNotification object:nil queue:NSOperationQueue.mainQueue
                usingBlock:^(NSNotification* n) {
                  NSUInteger type = [n.userInfo[AVAudioSessionInterruptionTypeKey] unsignedIntegerValue];
                  if (type == AVAudioSessionInterruptionTypeEnded) apply_state();
                }];
    [NSNotificationCenter.defaultCenter
        addObserverForName:AVAudioSessionMediaServicesWereResetNotification object:nil queue:NSOperationQueue.mainQueue
                usingBlock:^(NSNotification* n) {
                  if (g_unit) {
                    AudioOutputUnitStop(g_unit);
                    AudioComponentInstanceDispose(g_unit);
                    g_unit = NULL;
                  }
                  session_setup();
                  unit_open();
                  apply_state();
                }];
  });
}

void audio_set_paused(bool paused) {
  g_play = !paused;
  if (g_started) apply_state();
}

void audio_tick(void) {}
