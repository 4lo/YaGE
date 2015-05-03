#include "window.h"
#include <glib.h>
#include "../platform/unix.h"

namespace {
using namespace yage::core;

void window_on_key(Message &msg, const GdkEvent *evt)
{
	msg.kbd.is_press = evt->key.type == GDK_KEY_PRESS;
	msg.kbd.keyval = evt->key.keyval;
	msg.kbd.keycode = evt->key.hardware_keycode;

	msg.kbd.is_modkey = evt->key.is_modifier;
	msg.kbd.modkey_shift = evt->key.state & GDK_SHIFT_MASK;
	msg.kbd.modkey_ctrl = evt->key.state & GDK_CONTROL_MASK;
	msg.kbd.modkey_alt = evt->key.state & GDK_MOD1_MASK;
}

void window_on_motion(Message &msg, const GdkEvent *evt)
{
	msg.mouse.x = evt->motion.x;
	msg.mouse.y = evt->motion.y;

	msg.mouse.is_left = evt->motion.state & GDK_BUTTON1_MASK;
	msg.mouse.is_right = evt->motion.state & GDK_BUTTON3_MASK;
	msg.mouse.is_middle = evt->motion.state & GDK_BUTTON2_MASK;

	msg.mouse.modkey_shift = evt->motion.state & GDK_SHIFT_MASK;
	msg.mouse.modkey_ctrl = evt->motion.state & GDK_CONTROL_MASK;
	msg.mouse.modkey_alt = evt->motion.state & GDK_MOD1_MASK;
}


void window_on_button(Message &msg, const GdkEvent *evt)
{
	msg.mouse.x = evt->button.x;
	msg.mouse.y = evt->button.y;

	msg.mouse.is_left = evt->button.button == 1;
	msg.mouse.is_right = evt->button.button == 3;
	msg.mouse.is_middle = evt->button.button == 2;

	msg.mouse.modkey_shift = evt->button.state & GDK_SHIFT_MASK;
	msg.mouse.modkey_ctrl = evt->button.state & GDK_CONTROL_MASK;
	msg.mouse.modkey_alt = evt->button.state & GDK_MOD1_MASK;
}

gpointer event_thread(gpointer data)
{
	gtk_main();
	return nullptr;
}

} /* empty namespace */

namespace yage {
namespace core {

GAsyncQueue *Window::msg_queue_ = nullptr;

size_t Window::window_num_ = 0;

void Window::init()
{
#ifdef USE_X11
	yage::platform::enable_x11_thread_support();
#endif
	gtk_init(NULL, NULL);
	msg_queue_ = g_async_queue_new();
	g_thread_new("YaGE GTK event handler", event_thread, nullptr);
}

Window::Window()
{
	GtkWidget *widget_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_events(widget_window, gtk_widget_get_events(widget_window)
			| GDK_BUTTON_PRESS_MASK
			| GDK_POINTER_MOTION_MASK);

	g_signal_connect(widget_window, "event", G_CALLBACK(event_on_window), this);
	g_signal_connect(widget_window, "destroy", G_CALLBACK(evt_window_destroy), this);

	widget_draw_ = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(widget_window), widget_draw_);

	++Window::window_num_;
	fprintf(stderr, "New win %zu\n", Window::window_num_);
}

Window::~Window()
{
	destroy();
}

void Window::show()
{
	gtk_widget_show_all(gtk_widget_get_toplevel(widget_draw_));
}

void Window::hide()
{
	gtk_widget_hide(gtk_widget_get_toplevel(widget_draw_));
}

void Window::destroy()
{
	if (widget_draw_) gtk_widget_destroy(gtk_widget_get_toplevel(widget_draw_));
}

bool Window::is_valid()
{
	return widget_draw_ != nullptr;
}

void Window::evt_window_destroy(GtkWidget *widget, Window *window)
{
	Message &msg = *(new Message);
	msg.source = window;
	msg.type = msg.type_window;
	msg.window.type = msg.window.type_destroy;

	window->widget_draw_ = nullptr;
	--Window::window_num_;
	g_async_queue_push(Window::msg_queue_, &msg);
}

gboolean Window::event_on_window(GtkWidget *widget, GdkEvent *evt, Window *window)
{
	Message &msg = *(new Message);
	msg.source = window;
	msg.type = msg.type_nop;
	bool ret = false;

	switch (evt->type) {
		case GDK_KEY_PRESS:
		case GDK_KEY_RELEASE:
			msg.type = Message::type_kbd;
			window_on_key(msg, evt);
			break;

		// GDK_MOTION_NOTIFY
		// GDK_BUTTON_RELEASE
		// GDK_BUTTON_PRESS:
		//
		// if we let GTK to process the event, there will be
		// the same event come immediately.
		case GDK_MOTION_NOTIFY:
			msg.type = Message::type_mouse;
			msg.mouse.type = msg.mouse.type_move;
			window_on_motion(msg, evt);
			ret = true;
			break;

		case GDK_BUTTON_RELEASE:
			msg.type = Message::type_mouse;
			msg.mouse.type = msg.mouse.type_release;
			window_on_button(msg, evt);
			ret = true;
			break;

		case GDK_BUTTON_PRESS:
			msg.type = Message::type_mouse;
			msg.mouse.type = msg.mouse.type_press;
			window_on_button(msg, evt);
			ret = true;
			break;

		case GDK_ENTER_NOTIFY:
			msg.type = Message::type_window;
			msg.window.type = msg.window.type_enter;
			break;

		case GDK_LEAVE_NOTIFY:
			msg.type = Message::type_window;
			msg.window.type = msg.window.type_leave;
			break;

		default:
			fprintf(stderr, "Unhandled event %d\n", evt->type);
			break;
	}

	if (msg.type != msg.type_nop) {
		g_async_queue_push(Window::msg_queue_, &msg);
	}
	return ret;
}

/**
 * Get a message from pool.
 *
 * @param block: If no message availiable, whether block until new message comes.
 * @param [out] msg: The arrived message will be written here
 * @return Returns false when there are no window left, thus no messages can be
 * 	generated. Returns true when new message can arrive in the future.
 */
bool Window::poll(Message &msg, bool block)
{
	if (Window::window_num_ == 0) return false;
	gpointer pmsg = block ? g_async_queue_pop(Window::msg_queue_)
			      : g_async_queue_try_pop(Window::msg_queue_);
	if (pmsg == nullptr) {
		msg.type = msg.type_nop;
	} else {
		msg = *reinterpret_cast<Message *>(pmsg);
		delete reinterpret_cast<Message *>(pmsg);
	}
	return true;
}

} /* core */
} /* yage */

