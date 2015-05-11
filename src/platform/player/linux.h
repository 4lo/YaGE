#ifndef LINUX_H_BQTYW1ZH
#define LINUX_H_BQTYW1ZH
#include <string>
#include <gst/gst.h>
#include "../player.h"

namespace yage {
namespace platform {

class LinuxPlayer : public Player {
private:
  GstElement *pipeline_;
  GstBus *bus_;

  static gboolean bus_call(GstBus *bus, GstMessage *msg, void *user_data);

public:
	LinuxPlayer(const std::string &url);
	virtual ~LinuxPlayer();
	virtual bool play(void);
	virtual void pause(void);
	virtual void stop(void);
	virtual bool is_playing(void);
};

}
}

#endif /* end of include guard: LINUX_H_BQTYW1ZH */
