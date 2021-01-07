//
//  VROTransaction.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 10/22/15.
//  Copyright © 2015 Viro Media. All rights reserved.
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

#include "VROTransaction.h"
#include "VROTime.h"
#include "VROLog.h"
#include "VROTimingFunctionLinear.h"
#include "VROMath.h"
#include <stack>
#include <vector>
#include <algorithm>

#pragma mark - Transaction Management

static thread_local std::stack<std::shared_ptr<VROTransaction>> openTransactions;
static thread_local std::vector<std::shared_ptr<VROTransaction>> committedTransactions;

std::shared_ptr<VROTransaction> VROTransaction::get() {
    if (openTransactions.empty()) {
        return {};
    }
    else {
        return openTransactions.top();
    }
}

bool VROTransaction::isActive() {
    return !openTransactions.empty() && openTransactions.top()->_durationSeconds > 0;
}

void VROTransaction::begin() {
    std::shared_ptr<VROTransaction> animation = std::shared_ptr<VROTransaction>(new VROTransaction());
    openTransactions.push(animation);
}

void VROTransaction::add(std::shared_ptr<VROTransaction> transaction) {
    openTransactions.push(transaction);
}

std::shared_ptr<VROTransaction> VROTransaction::commit() {
    std::shared_ptr<VROTransaction> animation = get();
    if (!animation) {
        pabort();
    }

    animation->_t = 0;
    animation->_startTimeSeconds = VROTimeCurrentSeconds();

    openTransactions.pop();
    committedTransactions.push_back(animation);
    return animation;
}

void VROTransaction::setAnimationTimeOffset(float timeOffset) {
    std::shared_ptr<VROTransaction> animation = get();
    if (!animation) {
        pabort();
    }

    animation->_offsetTimeSeconds = timeOffset;
}

void VROTransaction::setFinishCallback(std::function<void (bool terminate)> finishCallback) {
    std::shared_ptr<VROTransaction> animation = get();
    if (!animation) {
        pabort();
    }

    animation->_finishCallback = finishCallback;
}

void VROTransaction::setTimingFunction(VROTimingFunctionType timingFunctionType) {
    setTimingFunction(VROTimingFunction::forType(timingFunctionType));
}

void VROTransaction::setTimingFunction(std::unique_ptr<VROTimingFunction> timingFunction) {
    std::shared_ptr<VROTransaction> animation = get();
    if (!animation) {
        pabort();
    }

    animation->_timingFunction = std::move(timingFunction);
}

void VROTransaction::setAnimationDelay(float delaySeconds) {
    std::shared_ptr<VROTransaction> animation = get();
    if (!animation) {
        pabort();
    }

    animation->_delayTimeSeconds = delaySeconds;
}

void VROTransaction::setAnimationDuration(float durationSeconds) {
    std::shared_ptr<VROTransaction> animation = get();
    if (!animation) {
        pabort();
    }

    animation->_durationSeconds = durationSeconds;
}

void VROTransaction::setAnimationSpeed(float speed) {
    std::shared_ptr<VROTransaction> animation = get();
    if (!animation) {
        pabort();
    }

    animation->_speed = speed;
}

void VROTransaction::setAnimationLoop(bool loop) {
    std::shared_ptr<VROTransaction> animation = get();
    if (!animation) {
        pabort();
    }
    animation->_loop = loop;
}

float VROTransaction::getAnimationDuration() {
    std::shared_ptr<VROTransaction> animation = get();
    if (!animation) {
        pabort();
    }

    return animation->_durationSeconds;
}

void VROTransaction::setAnimationSpeed(std::shared_ptr<VROTransaction> transaction, float speed) {
    double currentTime = VROTimeCurrentSeconds();
    float passedTime = currentTime - (transaction->_startTimeSeconds);
    transaction->_startTimeSeconds = currentTime;
    // Increase or decrease passage of time based on speed. 1 is normal speed. 0 freezes animation.
    transaction->_currentSpeedModulatedTime = passedTime * transaction->_speed + transaction->_currentSpeedModulatedTime;
    // switch to new speed.
    transaction->_speed = speed;

}

void VROTransaction::resume(std::shared_ptr<VROTransaction> transaction) {
    if (transaction->_t == 1.0){
        pinfo("WARN: Cannot resume a completed VROTansaction!");
        return;
    } else if (!transaction->_paused){
        pinfo("WARN: Cannot resume an VROTansaction that is not paused!");
        return;
    }

    double currentTime = VROTimeCurrentSeconds();
    transaction->_startTimeSeconds = currentTime - transaction->_processedTimeWhenPaused;
    transaction->_processedTimeWhenPaused = 0;
    transaction->_paused = false;
}

void VROTransaction::pause(std::shared_ptr<VROTransaction> transaction) {
    if (transaction->_t == 1.0){
        pinfo("WARN: Cannot to pause completed VROTansaction!");
        return;
    } else if (transaction->_paused){
        pinfo("WARN: Cannot pause an VROTansaction that is paused!");
        return;
    }

    double currentTime = VROTimeCurrentSeconds();
    transaction->_processedTimeWhenPaused = currentTime - transaction->_startTimeSeconds;
    transaction->_paused = true;
}

void VROTransaction::cancel(std::shared_ptr<VROTransaction> transaction) {
    std::vector<std::shared_ptr<VROTransaction>>::iterator transactionToCancel =  std::find(committedTransactions.begin(), committedTransactions.end(), transaction);
    if (transactionToCancel == committedTransactions.end()){
        pinfo("WARN: Can't cancel terminated/cancelled transaction!");
        return;
    }
    committedTransactions.erase(transactionToCancel);
}

void VROTransaction::terminate(std::shared_ptr<VROTransaction> transaction, bool jumpToEnd){
    std::vector<std::shared_ptr<VROTransaction>>::iterator transactionToTerminate =  std::find(committedTransactions.begin(), committedTransactions.end(), transaction);
    if (transactionToTerminate == committedTransactions.end()){
        pinfo("WARN: Can't terminate terminated transaction!");
        return;
    }

    // If jumpToEnd is true then invoke onTermination to move to the end of anim. Otherwise we terminate animation at current point.
    if (jumpToEnd == true) {
        transaction->onTermination();
    }

    committedTransactions.erase(transactionToTerminate);
}

void VROTransaction::update() {
    double time = VROTimeCurrentSeconds();

    /*
     Copy the vector over, because the committedTransactions vector can be modified
     by finish callbacks during this iteration.
     */
    std::vector<std::shared_ptr<VROTransaction>>::iterator it;
    std::vector<std::shared_ptr<VROTransaction>> runningTransactions = committedTransactions;

    for (it = runningTransactions.begin(); it != runningTransactions.end(); ++it) {
        std::shared_ptr<VROTransaction> transaction = *it;
        float passedTime = time - (transaction->_startTimeSeconds);
        // Increase or decrease passage of time based on speed. 1 is normal speed. 0 freezes animation.
        passedTime = passedTime * transaction->_speed + transaction->_currentSpeedModulatedTime;

        float passedTimeInSeconds = passedTime + transaction->_offsetTimeSeconds;
        if (transaction->_paused || passedTimeInSeconds <= transaction->_delayTimeSeconds){
            continue;
        }

        float percent = (passedTimeInSeconds - transaction->_delayTimeSeconds) / transaction->_durationSeconds;
        if (isinf(percent) || percent > 1.0 - kEpsilon) {

            // if _loop set, reset _t to 0 (done inside processAnimations(percent)
            if (transaction->_loop) {
                if (transaction -> _finishCallback) {
                    transaction -> _finishCallback(false);
                }
                transaction->_startTimeSeconds = VROTimeCurrentSeconds();
                transaction->_currentSpeedModulatedTime = 0;
                transaction->processAnimations(0);
            } else {
                transaction->onTermination();
            }
        }
        else {
            transaction->processAnimations(percent);
        }
    }

    /*
     Remove all completed transactions.
     */
    committedTransactions.erase(std::remove_if(committedTransactions.begin(), committedTransactions.end(),
            [](std::shared_ptr<VROTransaction> candidate) {
                return candidate->_t > 1.0 - kEpsilon;
            }), committedTransactions.end());
}

#pragma mark - Transaction Class

VROTransaction::VROTransaction() :
        _paused(false),
        _t(0),
        _speed(1),
        _offsetTimeSeconds(0),
        _durationSeconds(0),
        _startTimeSeconds(0),
        _delayTimeSeconds(0),
        _currentSpeedModulatedTime(0),
        _loop(false) {
    _timingFunction = std::unique_ptr<VROTimingFunction>(new VROTimingFunctionLinear());
}

void VROTransaction::processAnimations(float t) {
    _t = t;
    float transformedT = _timingFunction->getT(t);

    for (std::shared_ptr<VROAnimation> animation : _animations) {
        animation->processAnimationFrame(transformedT);
    }
}

void VROTransaction::onTermination() {
    _t = 1.0;
    for (std::shared_ptr<VROAnimation> animation : _animations) {
        animation->onTermination();
    }

    if (_finishCallback) {
        _finishCallback(true);
    }
}

