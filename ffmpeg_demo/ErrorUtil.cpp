#include "ErrorUtil.h"
#include <iostream>

extern "C" {

#include <libavutil/error.h>

}

void ErrorUtil::printError(int error) {

    char buffer[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(error, buffer, sizeof(buffer));

    std::cout << __LINE__ << ":" << buffer << std::endl;

}