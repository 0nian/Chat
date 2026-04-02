#ifndef _COMMON_HPP_
#define _COMMON_HPP_ 1

#include <fcntl.h>
#include <unistd.h>

enum {
    NON_BLOCK_ERR = 1,
};

inline void SetNonBlockOrDie(int sock)
{
    int fl = fcntl(sock, F_GETFL);
    if (fl < 0)
        exit(NON_BLOCK_ERR);
    fcntl(sock, F_SETFL, fl | O_NONBLOCK);
}

#endif