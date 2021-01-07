//
//  VROAudioPlayerAndroid.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 11/17/16.
//  Copyright © 2016 Viro Media. All rights reserved.
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

#include "VROAudioPlayerAndroid.h"

VROAudioPlayerAndroid::VROAudioPlayerAndroid(std::string fileName) :
    _fileName(fileName) {
    _player = new VROAVPlayer();
}

VROAudioPlayerAndroid::VROAudioPlayerAndroid(std::shared_ptr<VROSoundData> data) {
    _player = new VROAVPlayer();
    _data = data;
}

VROAudioPlayerAndroid::~VROAudioPlayerAndroid() {
    delete (_player);
}

void VROAudioPlayerAndroid::setLoop(bool loop) {
    _player->setLoop(loop);
}

void VROAudioPlayerAndroid::play() {
    _player->play();
}

void VROAudioPlayerAndroid::pause() {
    _player->pause();
}

void VROAudioPlayerAndroid::setVolume(float volume) {
    _player->setVolume(volume);
}

void VROAudioPlayerAndroid::setMuted(bool muted) {
    _player->setMuted(muted);
}

void VROAudioPlayerAndroid::seekToTime(float seconds) {
    // this is just a generic duration from the Android MediaPlayer
    int totalDuration = _player->getVideoDurationInSeconds();
    if (seconds > totalDuration) {
        seconds = totalDuration;
    } else if (seconds < 0) {
        seconds = 0;
    }
    _player->seekToTime(seconds);
}

void VROAudioPlayerAndroid::setDelegate(std::shared_ptr<VROSoundDelegateInternal> delegate) {
    _delegate = delegate;
    std::shared_ptr<VROAVPlayerDelegate> avDelegate =
            std::dynamic_pointer_cast<VROAVPlayerDelegate>(shared_from_this());
    _player->setDelegate(avDelegate);
}

void VROAudioPlayerAndroid::setup() {
    if (_data) {
        _data->setDelegate(shared_from_this());
    }
    if (!_fileName.empty()) {
        _player->setDataSourceURL(_fileName.c_str());
    }
}

#pragma mark - VROAVPlayerDelegate

void VROAudioPlayerAndroid::willBuffer() {
    // no-op
}

void VROAudioPlayerAndroid::didBuffer() {
    // no-op
}

void VROAudioPlayerAndroid::onPrepared() {
    if (_delegate) {
        _delegate->soundIsReady();
    }
}

void VROAudioPlayerAndroid::onFinished() {
    if (_delegate) {
        _delegate->soundDidFinish();
    }
}

void VROAudioPlayerAndroid::onError(std::string error) {
    if (_delegate) {
        _delegate->soundDidFail(error);
    }
}

#pragma mark - VROSoundDataDelegate

void VROAudioPlayerAndroid::dataIsReady() {
    if (_player) {
        _player->setDataSourceURL(_data->getLocalFilePath().c_str());
    }
}

void VROAudioPlayerAndroid::dataError(std::string error) {
    if (_delegate) {
        _delegate->soundDidFail(error);
    }
}