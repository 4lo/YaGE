#ifndef PLAYER_WIN32_H_
#define PLAYER_WIN32_H_
#include "../player.h"

#include <windows.h>

#ifdef __MINGW32__
#include "wmp_mingw.h"
#else
#define CINTERFACE
#define COBJMACROS
#include <wmp.h>
#endif

namespace yage {
namespace platform {

class WinPlayer : public Player {
private:
     static DWORD WINAPI player_worker_(LPVOID lpParameter);

     bool finished_;
     DWORD thread_id_;
     HANDLE thread_handle_;
     IWMPPlayer *player_;
     IWMPControls *control_;
    bool send_message_(WPARAM message);

public:
     WinPlayer(std::string url);
     virtual ~WinPlayer();
     virtual bool play(void);
     virtual void pause(void);
     virtual void stop(void);
     virtual bool is_playing(void);
};

}
}

#endif

