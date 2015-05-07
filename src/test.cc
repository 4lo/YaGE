#include "core/window.h"
#include "draw/canvas.h"
#include "draw/circle.h"
#include "draw/color.h"
#include <cstdlib>
using namespace yage::core;

#define P_PROP(type, name, format) fprintf(stderr, #name"="#format", ", msg.type.name)
#define P_NAME(type) \
  case msg.type:\
fprintf(stderr, #type ": ")

#define P_STOP() \
  fprintf(stderr, "\n"); \
break

void test_message()
{
  Window w;
  w.show();

  Message msg;
  while (Window::poll(msg)) {
    switch (msg.type) {
      P_NAME(type_nop);
      P_STOP();

      P_NAME(type_kbd);
      P_PROP(kbd, is_press, %d);
      P_PROP(kbd, keyval, %u);
      P_PROP(kbd, keycode, %u);
      P_PROP(kbd, is_modkey, %u);
      P_PROP(kbd, modkey_alt, %u);
      P_PROP(kbd, modkey_ctrl, %u);
      P_PROP(kbd, modkey_shift, %u);
      P_STOP();

      P_NAME(type_mouse);
      P_PROP(mouse, x, %.1lf);
      P_PROP(mouse, y, %.1lf);
      P_PROP(mouse, type, %d);
      P_PROP(mouse, is_left, %u);
      P_PROP(mouse, is_right, %u);
      P_PROP(mouse, is_middle, %u);
      P_PROP(mouse, modkey_alt, %u);
      P_PROP(mouse, modkey_ctrl, %u);
      P_PROP(mouse, modkey_shift, %u);
      P_STOP();

      P_NAME(type_window);
      P_PROP(window, type, %d);
      P_STOP();

      default:
      break;
    }
  }
  fprintf(stderr, "test_message: Bye!\n");
}

void test_window_count(void)
{
  Window w;
  w.show();
  w.set_title("Window count test");
  char buf[20];
  int n = 0;

  Message msg;
  while (Window::poll(msg)) {
    if (msg.type != msg.type_mouse) continue;
    if (msg.mouse.type != msg.mouse.type_press) continue;
    if (msg.mouse.is_left) {
      Window *new_win = new Window;
      new_win->show();
      snprintf(buf, sizeof(buf), "Window %d", ++n);
      new_win->set_title(buf);
    }
    if (msg.mouse.is_right) msg.source->destroy();
  }
  fprintf(stderr, "test_window_count: Bye!\n");
}

void test_draw(void)
{
  Window w;
  w.show();

  Message msg;
  while (Window::poll(msg)) {
    if (msg.type != msg.type_mouse) continue;
    if (msg.mouse.type != msg.mouse.type_move) continue;
    if (!msg.mouse.is_left) continue;
    yage::draw::Canvas& canvas = w.pro_get_canvas();
    using yage::draw::Circle;
    using yage::draw::Point;
    using yage::draw::Color;
    Circle *circle = new Circle();
    Point *point = new Point(300, 200);
    circle->set_center(*point);
    Color *color = new Color(1, 0, 0, 1);
    circle->set_bgcolor(*color);
    Color *color1 = new Color(0, 1, 0, 1);
    circle->set_fgcolor(*color1);
    circle->set_radius(100);
    canvas.draw_circle(*circle);
  }
}

void test_resize()
{
  Window w;
  bool resize = false;
  w.show();
  int x = 300, y = 300;

  Message msg;
  while (Window::poll(msg)) {
    if (msg.type != msg.type_kbd) continue;
    if (!msg.kbd.is_press) continue;

    switch (msg.kbd.keyval) {
      case 't':
        resize = !resize;
        w.set_resizable(resize);
        fprintf(stderr, "Allow resize: %d\n", resize);
        break;

      case 'x':
        x -= 100;
        w.set_size(x, y);
        break;

      case 'X':
        x += 100;
        w.set_size(x, y);
        break;

      case 'y':
        y -= 100;
        w.set_size(x, y);
        break;

      case 'Y':
        y += 100;
        w.set_size(x, y);
        break;

      case 'q':
        fprintf(stderr, "x = %d, y = %d, ", x, y);
        w.get_size(x, y);
        fprintf(stderr, "nx = %d, ny = %d\n", x, y);
        break;

      case '!':
        w.hide();
        sleep(1);
        w.show();
    }
  }
}

void test_fix_size()
{
  Window w;
  w.show();
  Message msg;

  Window *t = nullptr;
  while (Window::poll(msg)) {
    if (msg.type != msg.type_kbd) continue;
    if (!msg.kbd.is_press) continue;

    switch (msg.kbd.keyval) {
      case 'a':
        if (t) delete t;
        t = new Window;
        t->show();
        fprintf(stderr, "show\n");
        break;

      case 'b':
        if (t) delete t;
        t = new Window;
        t->show();
        t->set_size(100, 200);
        fprintf(stderr, "show, set_size\n");
        break;

      case 'c':
        if (t) delete t;
        t = new Window;
        t->set_resizable(true);
        t->set_size(100, 200);
        t->show();
        fprintf(stderr, "set_res, set_size, show\n");
        break;

      case 'd':
        if (t) delete t;
        t = new Window;
        t->set_size(100, 200);
        t->set_resizable(true);
        t->show();
        fprintf(stderr, "set_size, set_res, show\n");
        break;

      case 'e':
        if (t) delete t;
        t = new Window;
        t->set_size(100, 200);
        t->show();
        t->set_resizable(true);
        fprintf(stderr, "set_size, show, set_res\n");
        break;

      case 'f':
        if (t) delete t;
        t = new Window;
        t->set_resizable(true);
        t->show();
        t->set_size(100, 200);
        fprintf(stderr, "set_res, show, set_size\n");
        break;

      case 'g':
        if (t) delete t;
        t = new Window;
        t->show();
        t->set_resizable(true);
        t->set_size(100, 200);
        fprintf(stderr, "show, set_res, set_size\n");
        break;

      case 'h':
        if (t) delete t;
        t = new Window;
        t->show();
        t->set_size(100, 200);
        t->set_resizable(true);
        fprintf(stderr, "show, set_size, set_res\n");
        break;

    }
  }
}

int main(int argc, char *argv[])
{
  Window::init(test_draw);
  //test_fix_size();
  return 0;
}
