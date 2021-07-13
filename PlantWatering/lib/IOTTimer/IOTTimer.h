#ifndef _IOTTimer_H_
#define _IOTTimer_H_

class IOTTimer {

    unsigned int _timerStart;
    unsigned int _timerTarget;

  public:

    void startTimer(unsigned int msec) {
      _timerStart = millis();
      _timerTarget = msec;
    }

    bool isTimerReady() {
      return ((millis() - _timerStart) > _timerTarget);
    }
};
#endif 
