//
//  Copyright (c) 2017-present, ViroMedia, Inc.
//  All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining
//  a copy of this software and associated documentation files (the
//  "Software"), to deal in the Software without restriction, including
//  without limitation the rights to use, copy, modify, merge, publish,
//  distribute, sublicense, and/or sell copies of the Software, and to
//  permit persons to whom the Software is furnished to do so, subject to
//  the following conditions:
//
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

package com.viro.core.internal;

import android.content.Context;
import android.net.Uri;
import android.util.Log;
import android.view.Surface;

import com.google.android.exoplayer2.C;
import com.google.android.exoplayer2.DefaultLoadControl;
import com.google.android.exoplayer2.ExoPlaybackException;
import com.google.android.exoplayer2.ExoPlayer;
import com.google.android.exoplayer2.ExoPlayerFactory;
import com.google.android.exoplayer2.Format;
import com.google.android.exoplayer2.PlaybackParameters;
import com.google.android.exoplayer2.Player;
import com.google.android.exoplayer2.SimpleExoPlayer;
import com.google.android.exoplayer2.Timeline;
import com.google.android.exoplayer2.audio.AudioRendererEventListener;
import com.google.android.exoplayer2.decoder.DecoderCounters;
import com.google.android.exoplayer2.extractor.DefaultExtractorsFactory;
import com.google.android.exoplayer2.extractor.ExtractorsFactory;
import com.google.android.exoplayer2.source.ExtractorMediaSource;
import com.google.android.exoplayer2.source.MediaSource;
import com.google.android.exoplayer2.source.TrackGroupArray;
import com.google.android.exoplayer2.source.dash.DashMediaSource;
import com.google.android.exoplayer2.source.dash.DefaultDashChunkSource;
import com.google.android.exoplayer2.source.hls.HlsMediaSource;
import com.google.android.exoplayer2.source.smoothstreaming.DefaultSsChunkSource;
import com.google.android.exoplayer2.source.smoothstreaming.SsMediaSource;
import com.google.android.exoplayer2.trackselection.AdaptiveTrackSelection;
import com.google.android.exoplayer2.trackselection.DefaultTrackSelector;
import com.google.android.exoplayer2.trackselection.TrackSelection;
import com.google.android.exoplayer2.trackselection.TrackSelectionArray;
import com.google.android.exoplayer2.upstream.DataSource;
import com.google.android.exoplayer2.upstream.DefaultBandwidthMeter;
import com.google.android.exoplayer2.upstream.DefaultDataSourceFactory;
import com.google.android.exoplayer2.upstream.RawResourceDataSource;
import com.google.android.exoplayer2.util.Util;

/**
 * Wraps the Android ExoPlayer and can be controlled via JNI.
 */
public class AVPlayer {

    private static final String TAG = "Viro";

    /**
     * These states mimic the underlying stats in the Android
     * MediaPlayer. We need to ensure we don't violate any state
     * in the Android MediaPlayer, else it becomes invalid.
     */
    private enum State {
        IDLE,
        PREPARED,
        PAUSED,
        STARTED,
    }

    private SimpleExoPlayer mExoPlayer;
    private float mVolume;
    private long mNativeReference;
    private boolean mLoop;
    private State mState;
    private boolean mMute;
    private int mPrevExoPlayerState = -1;
    private boolean mWasBuffering = false;

    public AVPlayer(long nativeReference, Context context) {
        mVolume = 1.0f;
        mNativeReference = nativeReference;
        mLoop = false;
        mState = State.IDLE;
        mMute = false;

        TrackSelection.Factory trackSelectionFactory = new AdaptiveTrackSelection.Factory(new DefaultBandwidthMeter());
        DefaultTrackSelector trackSelector = new DefaultTrackSelector(trackSelectionFactory);
        mExoPlayer = ExoPlayerFactory.newSimpleInstance(context, trackSelector);

        mExoPlayer.addAudioDebugListener(new AudioRendererEventListener() {
            @Override
            public void onAudioEnabled(DecoderCounters counters) {
            }
            @Override
            public void onAudioSessionId(int audioSessionId) {
            }
            @Override
            public void onAudioDecoderInitialized(String decoderName, long initializedTimestampMs, long initializationDurationMs) {
                Log.i(TAG, "AVPlayer audio decoder initialized " + decoderName);
            }
            @Override
            public void onAudioInputFormatChanged(Format format) {
                Log.i(TAG, "AVPlayer audio input format changed to " + format);
            }

            @Override
            public void onAudioSinkUnderrun(int bufferSize, long bufferSizeMs, long elapsedSinceLastFeedMs) {
            }

            @Override
            public void onAudioDisabled(DecoderCounters counters) {
            }
        });
        mExoPlayer.addListener(new Player.EventListener() {

            @Override
            public void onTimelineChanged(Timeline timeline, Object manifest, int reason) {

            }

            @Override
            public void onTracksChanged(TrackGroupArray trackGroups, TrackSelectionArray trackSelections) {
            }
            @Override
            public void onLoadingChanged(boolean isLoading) {
            }

            @Override
            public void onPlayerStateChanged(boolean playWhenReady, int playbackState) {
                // this function sometimes gets called back w/ the same playbackState.
                if (mPrevExoPlayerState == playbackState) {
                    return;
                }
                mPrevExoPlayerState = playbackState;
                switch (playbackState) {
                    case Player.STATE_BUFFERING:
                        if (!mWasBuffering) {
                            nativeWillBuffer(mNativeReference);
                            mWasBuffering = true;
                        }
                        break;
                    case Player.STATE_READY:
                        if (mWasBuffering) {
                            nativeDidBuffer(mNativeReference);
                            mWasBuffering = false;
                        }
                        break;
                    case Player.STATE_ENDED:
                        if (mLoop) {
                            mExoPlayer.seekToDefaultPosition();
                        }
                        nativeOnFinished(mNativeReference);
                        break;
                }
            }

            @Override
            public void onRepeatModeChanged(int repeatMode) {

            }

            @Override
            public void onShuffleModeEnabledChanged(boolean shuffleModeEnabled) {

            }

            @Override
            public void onPlayerError(ExoPlaybackException error) {
                Log.w(TAG, "AVPlayer encountered error [" + error + "]", error);

                String message = null;
                if (error.type == ExoPlaybackException.TYPE_RENDERER) {
                    message = error.getRendererException().getLocalizedMessage();
                }
                else if (error.type == ExoPlaybackException.TYPE_SOURCE) {
                    message = error.getSourceException().getLocalizedMessage();
                }
                else if (error.type == ExoPlaybackException.TYPE_UNEXPECTED) {
                    message = error.getUnexpectedException().getLocalizedMessage();
                }
                nativeOnError(mNativeReference, message);
            }

            @Override
            public void onPositionDiscontinuity(int reason) {

            }

            @Override
            public void onPlaybackParametersChanged(PlaybackParameters playbackParameters) {

            }

            @Override
            public void onSeekProcessed() {

            }

        });
    }

    public boolean setDataSourceURL(String resourceOrURL, final Context context) {
        try {
            reset();

            Uri uri = Uri.parse(resourceOrURL);
            DataSource.Factory dataSourceFactory;
            ExtractorsFactory extractorsFactory = new DefaultExtractorsFactory();
            if (resourceOrURL.startsWith("res")) {
                // the uri we get is in the form res:/#######, so we want the path
                // which is `/#######`, and the id is the path minus the first char
                int id = Integer.parseInt(uri.getPath().substring(1));
                uri = RawResourceDataSource.buildRawResourceUri(id);
                dataSourceFactory = new DataSource.Factory() {
                    @Override
                    public DataSource createDataSource() {
                        return new RawResourceDataSource(context, null);
                    }
                };
            } else {
                dataSourceFactory = new DefaultDataSourceFactory(context,
                        Util.getUserAgent(context, "ViroAVPlayer"), new DefaultBandwidthMeter());
            }
            Log.i(TAG, "AVPlayer setting URL to [" + uri + "]");

            MediaSource mediaSource = buildMediaSource(uri, dataSourceFactory, extractorsFactory);

            mExoPlayer.prepare(mediaSource);
            mExoPlayer.seekToDefaultPosition();
            mState = State.PREPARED;

            Log.i(TAG, "AVPlayer prepared for playback");
            nativeOnPrepared(mNativeReference);

            return true;
        }catch(Exception e) {
            Log.w(TAG, "AVPlayer failed to load video at URL [" + resourceOrURL + "]", e);
            reset();

            return false;
        }
    }

    private MediaSource buildMediaSource(Uri uri, DataSource.Factory mediaDataSourceFactory,
                                         ExtractorsFactory extractorsFactory) {
        int type = inferContentType(uri);
        switch (type) {
            case C.TYPE_SS:
                return new SsMediaSource(uri, mediaDataSourceFactory,
                        new DefaultSsChunkSource.Factory(mediaDataSourceFactory), null, null);
            case C.TYPE_DASH:
                return new DashMediaSource(uri, mediaDataSourceFactory,
                        new DefaultDashChunkSource.Factory(mediaDataSourceFactory), null, null);
            case C.TYPE_HLS:
                return new HlsMediaSource(uri, mediaDataSourceFactory, null, null);
            default:
                // Return an ExtraMediaSource as default.
                return new ExtractorMediaSource(uri, mediaDataSourceFactory, extractorsFactory,
                        null, null);
        }
    }

    private int inferContentType(Uri uri) {
        String path = uri.getPath();
        return path == null ? C.TYPE_OTHER : inferContentType(path);
    }

    private int inferContentType(String fileName) {
        fileName = Util.toLowerInvariant(fileName);
        if (fileName.endsWith(".mpd")) {
            return C.TYPE_DASH;
        } else if (fileName.endsWith(".m3u8")) {
            return C.TYPE_HLS;
        } else if (fileName.endsWith(".ism") || fileName.endsWith(".isml")
                || fileName.endsWith(".ism/manifest") || fileName.endsWith(".isml/manifest")) {
            return C.TYPE_SS;
        } else {
            return C.TYPE_OTHER;
        }
    }

    public void setVideoSink(Surface videoSink) {
        mExoPlayer.setVideoSurface(videoSink);
    }

    public void reset() {
        mExoPlayer.stop();
        mExoPlayer.seekToDefaultPosition();
        mState = State.IDLE;

        Log.i(TAG, "AVPlayer reset");
    }

    public void destroy() {
        reset();
        mExoPlayer.release();

        Log.i(TAG, "AVPlayer destroyed");
    }

    public void play() {
        if (mState == State.PREPARED || mState == State.PAUSED) {
            mExoPlayer.setPlayWhenReady(true);
            mState = State.STARTED;
        }
        else {
            Log.w(TAG, "AVPlayer could not play video in " + mState.toString() + " state");
        }
    }

    public void pause() {
        if (mState == State.STARTED) {
            mExoPlayer.setPlayWhenReady(false);
            mState = State.PAUSED;
        }
        else {
            Log.w(TAG, "AVPlayer could not pause video in " + mState.toString() + " state");
        }
    }

    public boolean isPaused() {
        return mState != State.STARTED;
    }

    public void setLoop(boolean loop) {
        mLoop = loop;
        if (mExoPlayer.getPlaybackState() == ExoPlayer.STATE_ENDED){
            mExoPlayer.seekToDefaultPosition();
        }
    }

    public void setVolume(float volume) {
        mVolume = volume;
        if (!mMute){
            mExoPlayer.setVolume(mVolume);
        }
    }

    public void setMuted(boolean muted) {
        mMute = muted;
        if (muted) {
            mExoPlayer.setVolume(0);
        }
        else {
            mExoPlayer.setVolume(mVolume);
        }
    }

    public void seekToTime(float seconds) {
        if (mState == State.IDLE) {
            Log.w(TAG, "AVPlayer could not seek while in IDLE state");
            return;
        }

        mExoPlayer.seekTo((long) (seconds * 1000));
    }

    public float getCurrentTimeInSeconds() {
        if (mState == State.IDLE) {
            Log.w(TAG, "AVPlayer could not get current time in IDLE state");
            return 0;
        }

        return mExoPlayer.getCurrentPosition() / 1000.0f;
    }

    public float getVideoDurationInSeconds() {
        if (mState == State.IDLE) {
            Log.w(TAG, "AVPlayer could not get video duration in IDLE state");
            return 0;
        } else if (mExoPlayer.getDuration() == C.TIME_UNSET) {
            return 0;
        }

        return mExoPlayer.getDuration() / 1000.0f;
    }

    /**
     * Native Callbacks
     */
    private native void nativeOnPrepared(long ref);
    private native void nativeOnFinished(long ref);
    private native void nativeWillBuffer(long ref);
    private native void nativeDidBuffer(long ref);
    private native void nativeOnError(long ref, String error);
}

