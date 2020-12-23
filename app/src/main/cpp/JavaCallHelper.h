//
// Created by admin on 2020/12/23.
//

#ifndef PLAYER_JAVACALLHELPER_H
#define PLAYER_JAVACALLHELPER_H


class JavaCallHelper {

public:
    JavaCallHelper();

    ~JavaCallHelper();

    //回调java
    void onError(int thread, int errorCode);

    void onPrepare(int thread);

private:

};


#endif //PLAYER_JAVACALLHELPER_H
