#ifndef MY_SIGNAL_H_
#define MY_SIGNAL_H_

/* volatile */
extern bool mySignalOK;

void prepareSignals();
void restoreSignals();

#endif

