#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include "yage.h"
#include <cstring>
#include <cstdarg>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include "../dialog/color_chooser_dlg.h"
#include "../dialog/font_chooser_dlg.h"
#include "../dialog/file_chooser_dlg.h"
#include "../dialog/message_dlg.h"
#include "../dialog/input_dlg.h"
#include "../audio/player.h"
#include "../draw/animation.h"
#include "../window/timer.h"
#include "../util/time.h"

#ifdef _WIN32
#define strdup(x) _strdup(x)
#endif

#ifdef __MINGW32__ // `vsnprintf` that MinGW provides uses MSVCRT library, which is not C99 compatible
#include "../util/snprintf.h"
#endif

using namespace yage;
namespace yage {
namespace api {
namespace yage {

  yage_window *g_window = NULL;
  draw::Paint *g_paint = NULL;
  draw::Color g_fill_color;
  draw::Color g_border_color;
  bool g_auto_update;

  inline draw::Color convert_color(const yage_color &color) {
    return draw::Color(color.r, color.g, color.b, color.a);
  }

  inline void prepare_color(bool is_fill, bool is_border) {
    if(is_border) {
      g_paint->style = Paint::draw_style_stroke;
      g_paint->set_line_color(g_border_color);
    }
    if(is_fill) {
      g_paint->style = Paint::draw_style_fill;
      g_paint->set_fill_color(g_fill_color);
      if(is_border) g_paint->style = Paint::draw_style_stroke_fill;
    }
  }

  void force_update(yage_window *window) {
    window->canvas_bg->clear_all(*g_paint);
    window->canvas_bg->draw_canvas(*g_window->canvas, draw::Point(0, 0));
    window->canvas_bg->draw_canvas(*g_window->canvas_btn, draw::Point(0, 0));
    window->window->update();
  }

  void force_update(int x, int y, int width, int height, yage_window *window) {
    window->canvas_bg->clear_all(*g_paint);
    window->canvas_bg->draw_canvas(*g_window->canvas, draw::Point(0, 0));
    window->canvas_bg->draw_canvas(*g_window->canvas_btn, draw::Point(0, 0));

    cairo_rectangle_int_t rect;
    rect.x      = x;
    rect.y      = y;
    rect.width  = width;
    rect.height = height;
    cairo_region_t *region = cairo_region_create_rectangle(&rect);
    window->window->update(region);
  }

  inline void draw_circle(draw::Canvas &canvas, double x, double y, double r) {
    draw::Circle circle;
    circle.center = draw::Point(x, y);
    circle.radius = r;
    canvas.draw_circle(circle, *g_paint);
    update();
  }

  inline void draw_ellipse(draw::Canvas &canvas, double x, double y,
                           double radius_x, double radius_y) {
    draw::Ellipse ellipse;
    ellipse.center = draw::Point(x, y);
    ellipse.xradius = radius_x;
    ellipse.yradius = radius_y;
    canvas.draw_ellipse(ellipse, *g_paint);
    update();
  }

  inline void draw_rectangle(draw::Canvas &canvas,
                             double x1, double y1, double x2, double y2) {
    draw::Rect rect;
    rect.first = draw::Point(x1, y1);
    rect.second = draw::Point(x2, y2);
    canvas.draw_rect(rect, *g_paint);
    update();
  }

  inline void draw_sector(draw::Canvas &canvas,
                          double x, double y, double r,
                          double angle_begin, double angle_end) {
    draw::EllipticSector sector;
    sector.center.x = x;
    sector.center.y = y;
    sector.xradius = sector.yradius = r;
    sector.startangle = angle_begin;
    sector.endangle = angle_end;
    canvas.draw_elliptical_sector(sector, *g_paint);
    update();
  }

  inline void draw_poly(draw::Canvas &canvas,
                        int num_points, const double *poly_points) {
    Poly poly;
    for (int i = 0; i < num_points; i++) {
        Point point(poly_points[2 * i], poly_points[2 * i + 1]);
        poly.vertex.push_back(point);
    }
    canvas.draw_poly(poly, *g_paint);
    update();
  }
}  // namespace yage
}  // namespace api
}  // namespace yage

using namespace yage::api::yage;

extern "C" {

void yage_sleep(double second) {
  gulong micor_seconds = static_cast<gulong>(second * G_USEC_PER_SEC);
  g_usleep(micor_seconds);
}

void yage_draw_set_auto_update(int mode) {
  g_auto_update = mode ? true : false;
}

void yage_draw_update() {
  force_update();
}

void yage_draw_update_area(double x, double y, double width, double height) {
  force_update(static_cast<int>(x),     static_cast<int>(y),
               static_cast<int>(width), static_cast<int>(height),
               g_window);
}

struct yage_color yage_color_from_string_utf8(const char *color_str) {
  std::string s = color_str;
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);
  if (s[0] == '#' && (s.length() == 7 || s.length() == 9))
  {
    int color_arr[4];
    color_arr[3] = 255;
    for(unsigned int i = 1; i < s.length(); i+=2)
    {
      std::istringstream stream(s.substr(i,2));
      stream >> std::hex >> color_arr[ (i - 1) / 2 ];
    }
    struct yage_color color_struct = {1.0 * color_arr[0] / 255,
                                      1.0 * color_arr[1] / 255,
                                      1.0 * color_arr[2] / 255,
                                      1.0 * color_arr[3] / 255 };
    return color_struct;

  }
  static const struct mapper {
    const char* name;
    yage_color color;
  } map[] = {
    {"black",         {0.0,    0.0,    0.0,    1}},
    {"blue",          {0.0,    0.0,    0.6588, 1}},
    {"green",         {0.0,    0.6588, 0.0,    1}},
    {"cyan",          {0.0,    0.6588, 0.6588, 1}},
    {"red",           {0.6588, 0.0,    0.0,    1}},
    {"magenta",       {0.6588, 0.0,    0.6588, 1}},
    {"brown",         {0.6588, 0.6588, 0.0,    1}},
    {"light gray",    {0.6588, 0.6588, 0.6588, 1}},
    {"dark gray",     {0.3294, 0.3294, 0.3294, 1}},
    {"light blue",    {0.3294, 0.3294, 0.9882, 1}},
    {"light green",   {0.3294, 0.9882, 0.3294, 1}},
    {"light cyan",    {0.3294, 0.9882, 0.9882, 1}},
    {"light red",     {0.9882, 0.3294, 0.3294, 1}},
    {"light magenta", {0.9882, 0.3294, 0.9882, 1}},
    {"yellow",        {0.9882, 0.9882, 0.3294, 1}},
    {"white",         {0.9882, 0.9882, 0.9882, 1}},
    {NULL,            {0.9882, 0.9882, 0.9882, 1}},
  };

  for (const mapper *p = map; p->name; ++p) {
    if (p->name == s) return p->color;
  }

  return map[0].color;
}

struct yage_color yage_color_from_string(const char *color_str) {
  char *utf_8_color_str = yage::util::ansi_to_utf_8(color_str);
  struct yage_color ret = yage_color_from_string_utf8(utf_8_color_str);
  yage::util::free_string(utf_8_color_str);
  return ret;
}

#ifdef _WIN32
struct yage_color yage_color_from_stringW(const wchar_t *color_str) {
  char *utf_8_color_str = yage::util::utf_16_to_utf_8(color_str);
  struct yage_color ret = yage_color_from_string_utf8(utf_8_color_str);
  yage::util::free_string(utf_8_color_str);
  return ret;
}
#endif //_WIN32

struct yage_window *yage_window_create(int width, int height) {
  yage_window *window = new yage_window;

  window->window     = new window::Window(width, height);
  window->canvas     = new draw::Canvas(width, height);
  window->canvas_bg  = new draw::Canvas(width, height);
  window->canvas_btn = new draw::Canvas(width, height);

  window->canvas_bg->clear_all(*g_paint);
  window->window->set_canvas(*window->canvas_bg);
  window->window->set_title(version_string);
  window->window->show();

  return window;
}

void yage_window_delete(struct yage_window *new_default_window) {
  delete g_window->window;
  delete g_window->canvas;
  delete g_window->canvas_bg;
  delete g_window->canvas_btn;
  g_window = new_default_window;
}

void yage_window_set_default(struct yage_window *default_window) {
  g_window = default_window;
}

void yage_window_set_title_utf8(const char *title) {
  g_window->window->set_title(title ? title : version_string);
}

void yage_window_set_title(const char *title) {
  char *utf_8_title = yage::util::ansi_to_utf_8(title);
  yage_window_set_title_utf8(utf_8_title);
  yage::util::free_string(utf_8_title);
}

#ifdef _WIN32
void yage_window_set_titleW(const wchar_t *title) {
  char *utf_8_title = yage::util::utf_16_to_utf_8(title);
  yage_window_set_title_utf8(utf_8_title);
  yage::util::free_string(utf_8_title);
}
#endif

void yage_window_set_icon(struct yage_canvas *canvas) {
  cairo_surface_t *suf = get_canvas_int(canvas)->pro_get_cairo_surface();
  int width, height;
  get_canvas_int(canvas)->get_size(width, height);
  int sidelen = std::min(width, height);
  GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(suf, 0, 0, sidelen, sidelen);
  gtk_window_set_icon(g_window->window->pro_get_gtk_window(), pixbuf);
}

struct yage_canvas *yage_window_get_canvas(struct yage_window *window) {
  if (!window) {
    window = g_window;
  }
  return get_canvas_ext(window->canvas_bg);
}

struct yage_window *yage_init(int width, int height) {
  if (g_paint) return NULL;  // already initialized

  g_fill_color   = draw::Color(0.5, 0.5, 0.5, 1);
  g_border_color = draw::Color(0, 0, 0, 1);

  g_paint        = new draw::Paint;
  g_paint->set_background_color(Color(1, 1, 1, 1));

  srand((unsigned) time(NULL));
  g_auto_update = true;

  g_window = yage_window_create(width, height);
  return g_window;
}


void yage_quit(void) {
  window::quit();
}

struct yage_animation *yage_animation_load_image_utf8(const char *path, struct yage_window *window, double x, double y) {
  if (window == NULL) {
    window = g_window;
  }
  return reinterpret_cast<yage_animation *>(new yage::draw::Animation(*window->window, x, y, path));
}

struct yage_animation *yage_animation_load_image(const char *path, struct yage_window *window, double x, double y) {
  char *utf_8_path = yage::util::ansi_to_utf_8(path);
  struct yage_animation *ret = yage_animation_load_image_utf8(utf_8_path, window, x, y);
  free(utf_8_path);
  return ret;
}

#ifdef _WIN32
struct yage_animation *yage_animation_load_imageW(const wchar_t *path, struct yage_window *window, double x, double y) {
  char *utf_8_path = yage::util::utf_16_to_utf_8(path);
  struct yage_animation *ret = yage_animation_load_image_utf8(utf_8_path, window, x, y);
  free(utf_8_path);
  return ret;
}
#endif

struct yage_canvas *yage_canvas_create(int width, int height) {
  return get_canvas_ext(new draw::Canvas(width, height));
}

struct yage_canvas *yage_canvas_load_image_utf8(const char *path) {
  return get_canvas_ext(new draw::Canvas(path));
}

struct yage_canvas *yage_canvas_from_canvas(struct yage_canvas *canvas,
                                            double x1, double y1,
                                            double x2, double y2) {
  Canvas *ret = new Canvas(abs(static_cast<int>(x2 - x1)),
                           abs(static_cast<int>(y2 - y1)));
  cairo_surface_t *sur = get_canvas_int(canvas)->pro_get_cairo_surface();
  cairo_t *brush = ret->pro_get_brush();
  cairo_save(brush);

  // poisonous cairo API sucks
  cairo_set_source_surface(brush, sur, -std::min(x1, x2), -std::min(y1, y2));
  cairo_paint(brush);

  cairo_restore(brush);
  return get_canvas_ext(ret);
}

struct yage_canvas *yage_canvas_load_image(const char *path) {
  char *utf_8_path = yage::util::ansi_to_utf_8(path);
  struct yage_canvas *ret = yage_canvas_load_image_utf8(utf_8_path);
  yage::util::free_string(utf_8_path);
  return ret;
}

#ifdef _WIN32
struct yage_canvas *yage_canvas_load_imageW(const wchar_t *path) {
  char *utf_8_path = yage::util::utf_16_to_utf_8(path);
  struct yage_canvas *ret = yage_canvas_load_image_utf8(utf_8_path);
  yage::util::free_string(utf_8_path);
  return ret;
}
#endif // _WIN32

void yage_canvas_delete(struct yage_canvas *canvas) {
  delete get_canvas_int(canvas);
}

void yage_canvas_get_size(struct yage_canvas *canvas,
                          int *width, int *height) {
  get_canvas_int(canvas)->get_size(*width, *height);
}

void yage_canvas_clear(struct yage_canvas *canvas) {
  draw::Paint transparent;
  transparent.set_background_color(draw::Color(0, 0, 0, 0));
  get_canvas_int(canvas)->clear_all(transparent);
  update();
}

void yage_clear(void) {
  yage_canvas_clear(get_canvas_ext());
}

void yage_canvas_draw_pixel(struct yage_canvas *canvas,
                            double x, double y, struct yage_color color) {
  g_paint->style = Paint::draw_style_fill;
  g_paint->set_fill_color(convert_color(color));
  draw_rectangle(*get_canvas_int(canvas),
                 x - 0.5, y - 0.5,
                 x + 0.5, y + 0.5);
}

void yage_draw_pixel(double x, double y, struct yage_color color) {
  yage_canvas_draw_pixel(get_canvas_ext(), x, y, color);
}

void yage_canvas_draw_canvas(struct yage_canvas *dst, struct yage_canvas *src,
                             double x, double y,
                             double xscale, double yscale) {
  if (xscale == 0 || yscale == 0) return;
  draw::Paint paint;
  paint.set_scale(xscale, yscale);

  get_canvas_int(dst)->draw_canvas(*get_canvas_int(src),
                                   draw::Point(x / xscale, y / yscale),
                                   paint);
  update();
}

void yage_draw_canvas(struct yage_canvas *canvas,
                      double x, double y, double xscale, double yscale) {
  return yage_canvas_draw_canvas(get_canvas_ext(), canvas,
                                 x, y, xscale, yscale);
}

void yage_set_font_utf8(const char *family, int size, int bold, int italic) {
  if (family)       g_paint->font.set_font_family(family);
  if (size > 0)     g_paint->font.set_size(size);
  if (bold >= 0)    g_paint->font.set_bold_status(bold ? true : false);
  if (italic >= 0)  g_paint->font.set_italic_status(italic ? true : false);
}

void yage_set_font(const char *family, int size, int bold, int italic) {
  char *utf_8_family = yage::util::ansi_to_utf_8(family);
  yage_set_font_utf8(utf_8_family, size, bold, italic);
  yage::util::free_string(utf_8_family);
}

#ifdef _WIN32
void yage_set_fontW(const wchar_t *family, int size, int bold, int italic) {
  char *utf_8_family = yage::util::utf_16_to_utf_8(family);
  yage_set_font_utf8(utf_8_family, size, bold, italic);
  yage::util::free_string(utf_8_family);
}
#endif //_WIN32

void yage_set_font_color(struct yage_color font_color) {
  g_paint->set_font_color(convert_color(font_color));
}

void yage_set_fill_color(struct yage_color fill_color) {
  g_fill_color = convert_color(fill_color);
}

void yage_set_background_color(struct yage_color background_color) {
  g_paint->set_background_color(convert_color(background_color));
  g_window->canvas_bg->clear_all(*g_paint);
  update();
}

void yage_set_border_color(struct yage_color border_color) {
  g_border_color = convert_color(border_color);
}

void yage_set_border_thickness(double thickness) {
  g_paint->line_width = thickness;
}

void yage_canvas_circle(struct yage_canvas *canvas,
                        double x, double y, double r) {
  prepare_color(true, true);
  draw_circle(*get_canvas_int(canvas), x, y, r);
}

void yage_circle(double x, double y, double r) {
  prepare_color(true, true);
  draw_circle(*g_window->canvas, x, y, r);
}

void yage_canvas_circle_fill(struct yage_canvas *canvas,
                             double x, double y, double r) {
  prepare_color(true, false);
  draw_circle(*get_canvas_int(canvas), x, y, r);
}

void yage_circle_fill(double x, double y, double r) {
  prepare_color(true, false);
  draw_circle(*g_window->canvas, x, y, r);
}

void yage_canvas_circle_border(struct yage_canvas *canvas,
                               double x, double y, double r) {
  prepare_color(false, true);
  draw_circle(*get_canvas_int(canvas), x, y, r);
}

void yage_circle_border(double x, double y, double r) {
  prepare_color(false, true);
  draw_circle(*g_window->canvas, x, y, r);
}

void yage_canvas_ellipse(struct yage_canvas *canvas,
                         double x, double y,
                         double radius_x, double radius_y) {
  prepare_color(true, true);
  draw_ellipse(*get_canvas_int(canvas), x, y, radius_x, radius_y);
}

void yage_ellipse(double x, double y, double radius_x, double radius_y) {
  prepare_color(true, true);
  draw_ellipse(*g_window->canvas, x, y, radius_x, radius_y);
}

void yage_canvas_ellipse_fill(struct yage_canvas *canvas,
                              double x, double y,
                              double radius_x, double radius_y) {
  prepare_color(true, false);
  draw_ellipse(*get_canvas_int(canvas), x, y, radius_x, radius_y);
}

void yage_ellipse_fill(double x, double y,
                       double radius_x, double radius_y) {
  prepare_color(true, false);
  draw_ellipse(*g_window->canvas, x, y, radius_x, radius_y);
}

void yage_canvas_ellipse_border(struct yage_canvas *canvas,
                                double x, double y,
                                double radius_x, double radius_y) {
  prepare_color(false, true);
  draw_ellipse(*get_canvas_int(canvas), x, y, radius_x, radius_y);
}

void yage_ellipse_border(double x, double y,
                         double radius_x, double radius_y) {
  prepare_color(false, true);
  draw_ellipse(*g_window->canvas, x, y, radius_x, radius_y);
}

void yage_canvas_rectangle(struct yage_canvas *canvas,
                           double x1, double y1, double x2, double y2) {
  prepare_color(true, true);
  draw_rectangle(*get_canvas_int(canvas), x1, y1, x2, y2);
}

void yage_rectangle(double x1, double y1, double x2, double y2) {
  prepare_color(true, true);
  draw_rectangle(*g_window->canvas, x1, y1, x2, y2);
}

void yage_canvas_rectangle_fill(struct yage_canvas *canvas,
                                double x1, double y1, double x2, double y2) {
  prepare_color(true, false);
  draw_rectangle(*get_canvas_int(canvas), x1, y1, x2, y2);
}

void yage_rectangle_fill(double x1, double y1, double x2, double y2) {
  prepare_color(true, false);
  draw_rectangle(*g_window->canvas, x1, y1, x2, y2);
}

void yage_canvas_rectangle_border(struct yage_canvas *canvas,
                                  double x1, double y1, double x2, double y2) {
  prepare_color(false, true);
  draw_rectangle(*get_canvas_int(canvas), x1, y1, x2, y2);
}

void yage_rectangle_border(double x1, double y1, double x2, double y2) {
  prepare_color(false, true);
  draw_rectangle(*g_window->canvas, x1, y1, x2, y2);
}

void yage_canvas_poly(struct yage_canvas *canvas,
                      int num_points, const double *poly_points) {
  prepare_color(true, true);
  draw_poly(*get_canvas_int(canvas), num_points, poly_points);
}

void yage_poly(int num_points, const double *poly_points) {
  prepare_color(true, true);
  draw_poly(*g_window->canvas, num_points, poly_points);
}

void yage_canvas_poly_fill(struct yage_canvas *canvas,
                           int num_points, const double *poly_points) {
  prepare_color(true, false);
  draw_poly(*get_canvas_int(canvas), num_points, poly_points);
}

void yage_poly_fill(int num_points, const double *poly_points) {
  prepare_color(true, false);
  draw_poly(*g_window->canvas, num_points, poly_points);
}

void yage_canvas_poly_border(struct yage_canvas *canvas,
                             int num_points, const double *poly_points) {
  prepare_color(false, true);
  draw_poly(*get_canvas_int(canvas), num_points, poly_points);
}

void yage_poly_border(int num_points, const double *poly_points) {
  prepare_color(false, true);
  draw_poly(*g_window->canvas, num_points, poly_points);
}

void yage_canvas_sector(struct yage_canvas *canvas,
                        double x, double y, double r,
                        double angle_begin, double angle_end) {
  prepare_color(true, true);
  draw_sector(*get_canvas_int(canvas), x, y, r, angle_begin, angle_end);
}

void yage_sector(double x, double y, double r,
                 double angle_begin, double angle_end) {
  prepare_color(true, true);
  draw_sector(*g_window->canvas, x, y, r, angle_begin, angle_end);
}

void yage_canvas_sector_fill(struct yage_canvas *canvas,
                             double x, double y, double r,
                             double angle_begin, double angle_end) {
  prepare_color(true, false);
  draw_sector(*get_canvas_int(canvas), x, y, r, angle_begin, angle_end);
}

void yage_sector_fill(double x, double y, double r,
                      double angle_begin, double angle_end) {
  prepare_color(true, false);
  draw_sector(*g_window->canvas, x, y, r, angle_begin, angle_end);
}

void yage_canvas_sector_border(struct yage_canvas *canvas,
                               double x, double y, double r,
                               double angle_begin, double angle_end) {
  prepare_color(false, true);
  draw_sector(*get_canvas_int(canvas), x, y, r, angle_begin, angle_end);
}

void yage_sector_border(double x, double y, double r,
                        double angle_begin, double angle_end) {
  prepare_color(false, true);
  draw_sector(*g_window->canvas, x, y, r, angle_begin, angle_end);
}

void yage_canvas_arc_border(struct yage_canvas *canvas,
                            double x, double y, double r,
                            double angle_begin, double angle_end) {
  prepare_color(true, true);

  draw::EllipticArc arc;
  arc.center = draw::Point(x, y);
  arc.xradius = arc.yradius = r;
  arc.startangle = angle_begin;
  arc.endangle = angle_end;
  get_canvas_int(canvas)->draw_elliptical_arc(arc, *g_paint);
  update();
}

void yage_arc_border(double x, double y, double r,
                     double angle_begin, double angle_end) {
  yage_canvas_arc_border(get_canvas_ext(),
                         x, y, r, angle_begin, angle_end);
}

void yage_canvas_line_border(struct yage_canvas *canvas,
                             double x1, double y1, double x2, double y2) {
  prepare_color(true, true);
  draw::Line line(draw::Point(x1, y1), draw::Point(x2, y2));
  g_window->canvas->draw_line(line, *g_paint);
  update();
}

void yage_line_border(double x1, double y1, double x2, double y2) {
  yage_canvas_line_border(get_canvas_ext(), x1, y1, x2, y2);
}

void yage_canvas_printf(struct yage_canvas *canvas,
                        double x, double y, const char *format, ...) {
  char buf[kMaxTextBuffer];

  va_list args;
  va_start(args, format);
#ifdef _MSC_VER
#if _MSC_VER > 1300
  vsnprintf_s(buf, _countof(buf), sizeof(buf), format, args);
#else
  _vsnprintf(buf, sizeof(buf), format, args);
#endif
#else
  vsnprintf(buf, sizeof(buf), format, args);
#endif
  va_end(args);

  char *utf_8_buf = yage::util::ansi_to_utf_8(buf);
  yage::draw::Text text(utf_8_buf);
  yage::util::free_string(utf_8_buf);
  text.position = draw::Point(x, y);
  get_canvas_int(canvas)->draw_text(text, *g_paint);
  update();
}

#ifdef _WIN32
void yage_canvas_printfW(struct yage_canvas *canvas,
                        double x, double y, const wchar_t *format, ...) {
  wchar_t buf[kMaxTextBuffer];

  va_list args;
  va_start(args, format);
#ifdef _MSC_VER
#if _MSC_VER > 1300
  _vsnwprintf_s(buf, _countof(buf), sizeof(buf), format, args);
#else
  _vsnwprintf(buf, sizeof(buf), format, args);
#endif
#else
  _vsnwprintf(buf, sizeof(buf), format, args);
#endif
  va_end(args);

  char *utf_8_buf = yage::util::utf_16_to_utf_8(buf);
  yage::draw::Text text(utf_8_buf);
  yage::util::free_string(utf_8_buf);
  text.position = draw::Point(x, y);
  get_canvas_int(canvas)->draw_text(text, *g_paint);
  update();
}
#endif

void yage_printf(double x, double y, const char *format, ...) {
  char buf[kMaxTextBuffer];

  va_list args;
  va_start(args, format);
#ifdef _MSC_VER
#if _MSC_VER > 1300
  vsnprintf_s(buf, _countof(buf), sizeof(buf), format, args);
#else
  _vsnprintf(buf, sizeof(buf), format, args);
#endif
#else
  vsnprintf(buf, sizeof(buf), format, args);
#endif
  va_end(args);

  char *utf_8_buf = yage::util::ansi_to_utf_8(buf);
  yage::draw::Text text(utf_8_buf);
  yage::util::free_string(utf_8_buf);
  text.position = draw::Point(x, y);
  g_window->canvas->draw_text(text, *g_paint);
  update();
}

#ifdef _WIN32
void yage_printfW(double x, double y, const wchar_t *format, ...) {
  wchar_t buf[kMaxTextBuffer];

  va_list args;
  va_start(args, format);
#ifdef _MSC_VER
#if _MSC_VER > 1300
  _vsnwprintf_s(buf, _countof(buf), sizeof(buf), format, args);
#else
  _vsnwprintf(buf, sizeof(buf), format, args);
#endif
#else
  _vsnwprintf(buf, sizeof(buf), format, args);
#endif
  va_end(args);

  char *utf_8_buf = yage::util::utf_16_to_utf_8(buf);
  yage::draw::Text text(utf_8_buf);
  yage::util::free_string(utf_8_buf);
  text.position = draw::Point(x, y);
  g_window->canvas->draw_text(text, *g_paint);
  update();
}
#endif // _WIN32

struct yage_player *yage_player_load_music(const char *path) {
  return reinterpret_cast<struct yage_player *>(yage::audio::Player::create_player(path));
}

void yage_player_play(struct yage_player *player) {
  if (player) {
    reinterpret_cast<yage::audio::Player *>(player)->play();
  }
}

void yage_player_pause(struct yage_player *player) {
  if (player) {
    reinterpret_cast<yage::audio::Player *>(player)->pause();
  }
}
void yage_player_stop(struct yage_player *player) {
  if (player) {
    reinterpret_cast<yage::audio::Player *>(player)->stop();
  }
}

uint8_t yage_player_get_status(struct yage_player *player) {
  if (player) {
    return static_cast<uint8_t>(reinterpret_cast<yage::audio::Player *>(player)->is_playing());
  }
  return 0;
}

void yage_player_delete(struct yage_player *player) {
  if (player) {
    delete reinterpret_cast<yage::audio::Player *>(player);
  }
}

int yage_get_message(struct yage_message *msg, int wait_ms) {
  guint64 timeout = wait_ms < 0 ? G_MAXUINT64 : wait_ms * 1000;
  int ret = window::poll(*reinterpret_cast<window::Message *>(msg), timeout);
  return ret;
}

int  yage_get_key(void) {
  window::Message msg;
  while (window::poll(msg)) {
    if (msg.type != msg.type_kbd) continue;
    return msg.kbd.keyval;
  }
  return -1;
}

int  yage_dlg_font_utf8(const char *title) {
  if (!title) title = "Choose Font";
  dialog::FontChooserDlg font_dlg(title, *g_window->window);
  return font_dlg.show(g_paint->font);
}

int yage_dlg_font(const char *title) {
  char *utf_8_title = yage::util::ansi_to_utf_8(title);
  int ret = yage_dlg_font_utf8(utf_8_title);
  yage::util::free_string(utf_8_title);
  return ret;
}

#ifdef _WIN32
int yage_dlg_fontW(const wchar_t *title) {
  char *utf_8_title = yage::util::utf_16_to_utf_8(title);
  int ret = yage_dlg_font_utf8(utf_8_title);
  yage::util::free_string(utf_8_title);
  return ret;
}
#endif

int  yage_dlg_color_utf8(const char *title, struct yage_color *color) {
  if (!title) title = "Choose Color";
  dialog::ColorChooserDlg font_dlg(title, *g_window->window);
  draw::Color internal_coler;

  int ret = font_dlg.show(internal_coler);
  color->r = internal_coler.r;
  color->g = internal_coler.g;
  color->b = internal_coler.b;
  color->a = internal_coler.a;
  return ret;
}

int yage_dlg_color(const char *title, struct yage_color *color) {
  char *utf_8_title = yage::util::ansi_to_utf_8(title);
  int ret = yage_dlg_color_utf8(utf_8_title, color);
  yage::util::free_string(utf_8_title);
  return ret;
}

#ifdef _WIN32
int yage_dlg_colorW(const wchar_t *title, struct yage_color *color) {
  char *utf_8_title = yage::util::utf_16_to_utf_8(title);
  int ret = yage_dlg_color_utf8(utf_8_title, color);
  yage::util::free_string(utf_8_title);
  return ret;
}
#endif

char *yage_dlg_file_save_utf8(const char *title) {
  if (!title) title = "Save File";
  dialog::FileChooserDlg fc_dlg(dialog::FileChooserDlg::action_type_save,
                                title, *g_window->window);
  std::string path;
  if (!fc_dlg.show(path)) return NULL;
  return strdup(path.c_str());
}

char *yage_dlg_file_save(const char *title) {
  char *utf_8_title = yage::util::ansi_to_utf_8(title);
  char *utf_8_ret = yage_dlg_file_save_utf8(utf_8_title);
  yage::util::free_string(utf_8_title);
  char *ret = yage::util::utf_8_to_ansi(utf_8_ret);
  if (ret != utf_8_ret) {
    free(utf_8_ret);
  }
  return ret;
}

#ifdef _WIN32
wchar_t *yage_dlg_file_saveW(const wchar_t *title) {
  char *utf_8_title = yage::util::utf_16_to_utf_8(title);
  char *utf_8_ret = yage_dlg_file_save_utf8(utf_8_title);
  yage::util::free_string(utf_8_title);
  wchar_t *ret = yage::util::utf_8_to_utf_16(utf_8_ret);
  free(utf_8_ret);
  return ret;
}
#endif

char *yage_dlg_file_open_utf8(const char *title) {
  if (!title) title = "Open File";
  dialog::FileChooserDlg fc_dlg(dialog::FileChooserDlg::action_type_open,
                                title,
                                *g_window->window);
  std::string path;
  if (!fc_dlg.show(path)) return NULL;
  return strdup(path.c_str());
}

char *yage_dlg_file_open(const char *title) {
  char *utf_8_title = yage::util::ansi_to_utf_8(title);
  char *utf_8_ret = yage_dlg_file_open_utf8(utf_8_title);
  yage::util::free_string(utf_8_title);
  char *ret = yage::util::utf_8_to_ansi(utf_8_ret);
  if (ret != utf_8_ret) {
    free(utf_8_ret);
  }
  return ret;
}

#ifdef _WIN32
wchar_t *yage_dlg_file_openW(const wchar_t *title) {
  char *utf_8_title = yage::util::utf_16_to_utf_8(title);
  char *utf_8_ret = yage_dlg_file_open_utf8(utf_8_title);
  yage::util::free_string(utf_8_title);
  wchar_t *ret = yage::util::utf_8_to_utf_16(utf_8_ret);
  free(utf_8_ret);
  return ret;
}
#endif

void yage_dlg_message_utf8(const char *title, const char *message) {
  using dialog::MessageDlg;
  MessageDlg msg_dlg(MessageDlg::button_type_ok,
                     MessageDlg::icon_type_none,
                     *g_window->window);
  if (title) msg_dlg.set_title(title);
  if (message) msg_dlg.set_message(message);
  msg_dlg.show();
}

void yage_dlg_message(const char *title, const char *message) {
  char *utf_8_title = yage::util::ansi_to_utf_8(title);
  char *utf_8_message = yage::util::ansi_to_utf_8(message);
  yage_dlg_message_utf8(utf_8_title, utf_8_message);
  yage::util::free_string(utf_8_title);
  yage::util::free_string(utf_8_message);
}

#ifdef _WIN32
void yage_dlg_messageW(const wchar_t *title, const wchar_t *message) {
  char *utf_8_title = yage::util::utf_16_to_utf_8(title);
  char *utf_8_message = yage::util::utf_16_to_utf_8(message);
  yage_dlg_message_utf8(utf_8_title, utf_8_message);
  yage::util::free_string(utf_8_title);
  yage::util::free_string(utf_8_message);
}
#endif

yage_dlg_result_type
yage_dlg_question_utf8(const char *title,
                  const char *message,
                  yage_dlg_icon_type icon,
                  yage_dlg_button_type button) {
  using dialog::MessageDlg;
  MessageDlg msg_dlg(static_cast<dialog::MessageDlg::button_type>(button),
                     static_cast<dialog::MessageDlg::icon_type>(icon),
                     *g_window->window);
  if (title) msg_dlg.set_title(title);
  if (message) msg_dlg.set_message(message);

  return static_cast<yage_dlg_result_type>(msg_dlg.show());
}

yage_dlg_result_type
yage_dlg_question(const char *title, const char *message,
                  yage_dlg_icon_type icon, yage_dlg_button_type button) {
  char *utf_8_title = yage::util::ansi_to_utf_8(title);
  char *utf_8_message = yage::util::ansi_to_utf_8(message);
  yage_dlg_result_type ret = yage_dlg_question_utf8(utf_8_title,
                                                    utf_8_message,
                                                    icon,
                                                    button);
  yage::util::free_string(utf_8_title);
  yage::util::free_string(utf_8_message);
  return ret;
}

#ifdef _WIN32
yage_dlg_result_type
yage_dlg_questionW(const wchar_t *title, const wchar_t *message,
                   yage_dlg_icon_type icon, yage_dlg_button_type button) {
  char *utf_8_title = yage::util::utf_16_to_utf_8(title);
  char *utf_8_message = yage::util::utf_16_to_utf_8(message);
  yage_dlg_result_type ret = yage_dlg_question_utf8(utf_8_title,
                                                    utf_8_message,
                                                    icon,
                                                    button);
  yage::util::free_string(utf_8_title);
  yage::util::free_string(utf_8_message);
  return ret;
}
#endif

int yage_input_int_utf8(const char *title, const char *message) {
  if (!title) title = "Input An Integer";
  dialog::InputDlg dlg(title, *g_window->window);
  if (message) dlg.set_message(message);

  std::string str;
  if (dlg.show(str)) {
  int ret;
  std::stringstream stream(str);
    if (stream >> ret) {
      return ret;
    }
  }

  return INT_MIN;
}

int yage_input_int(const char *title, const char *message) {
  char *utf_8_title = yage::util::ansi_to_utf_8(title);
  char *utf_8_message = yage::util::ansi_to_utf_8(message);
  int ret = yage_input_int_utf8(utf_8_title, utf_8_message);
  yage::util::free_string(utf_8_title);
  yage::util::free_string(utf_8_message);
  return ret;
}

#ifdef _WIN32
int yage_input_intW(const wchar_t *title, const wchar_t *message) {
  char *utf_8_title = yage::util::utf_16_to_utf_8(title);
  char *utf_8_message = yage::util::utf_16_to_utf_8(message);
  int ret = yage_input_int_utf8(utf_8_title, utf_8_message);
  yage::util::free_string(utf_8_title);
  yage::util::free_string(utf_8_message);
  return ret;
}
#endif

double yage_input_double_utf8(const char *title, const char *message) {
  if (!title) title = "Input A Float Number";
  dialog::InputDlg dlg(title, *g_window->window);
  if (message) dlg.set_message(message);

  std::string str;
  if (dlg.show(str)) {
    double ret;
    std::stringstream stream(str);
    if (stream >> ret) {
      return ret;
    }
  }

  return NAN;
}

double yage_input_double(const char *title, const char *message) {
  char *utf_8_title = yage::util::ansi_to_utf_8(title);
  char *utf_8_message = yage::util::ansi_to_utf_8(message);
  double ret = yage_input_double_utf8(utf_8_title, utf_8_message);
  yage::util::free_string(utf_8_title);
  yage::util::free_string(utf_8_message);
  return ret;
}

#ifdef _WIN32
double yage_input_doubleW(const wchar_t *title, const wchar_t *message) {
  char *utf_8_title = yage::util::utf_16_to_utf_8(title);
  char *utf_8_message = yage::util::utf_16_to_utf_8(message);
  double ret = yage_input_double_utf8(utf_8_title, utf_8_message);
  yage::util::free_string(utf_8_title);
  yage::util::free_string(utf_8_message);
  return ret;
}
#endif

char *yage_input_line_utf8(const char *title, const char *message) {
  if (!title) title = "Input Some Text";
  dialog::InputDlg dlg(title, *g_window->window);
  if (message) dlg.set_message(message);

  std::string str;
  if (!dlg.show(str)) return NULL;
  return strdup(str.c_str());
}

char *yage_input_line(const char *title, const char *message) {
  char *utf_8_title = yage::util::ansi_to_utf_8(title);
  char *utf_8_message = yage::util::ansi_to_utf_8(message);
  char *utf_8_ret = yage_input_line_utf8(utf_8_title, utf_8_message);
  yage::util::free_string(utf_8_title);
  yage::util::free_string(utf_8_message);
  char *ret = yage::util::utf_8_to_ansi(utf_8_ret);
  yage::util::free_string(utf_8_ret);
  return ret;
}

#ifdef _WIN32
wchar_t *yage_input_lineW(const wchar_t *title, const wchar_t *message) {
  char *utf_8_title = yage::util::utf_16_to_utf_8(title);
  char *utf_8_message = yage::util::utf_16_to_utf_8(message);
  char *utf_8_ret = yage_input_line_utf8(utf_8_title, utf_8_message);
  yage::util::free_string(utf_8_title);
  yage::util::free_string(utf_8_message);
  wchar_t *ret = yage::util::utf_8_to_utf_16(utf_8_ret);
  yage::util::free_string(utf_8_ret);
  return ret;
}
#endif

#if defined(_MSC_VER) && _MSC_VER < 1800
#pragma optimize("", off) // Disable optimization of `yage_input_scanf` function. Enable optimization will cause program crash.
#endif

int yage_input_scanf(const char *title, const char *message,
                     const char *format, ...) {
  char *real_title;
  if (title) {
    real_title = const_cast<char *>(title);
  } else {
    real_title = const_cast<char *>("Input Some Text");
  }
  char *utf_8_title = yage::util::ansi_to_utf_8(real_title);
  char *utf_8_message = yage::util::ansi_to_utf_8(message);
  dialog::InputDlg dlg(utf_8_title, *g_window->window);
  if (utf_8_message) dlg.set_message(utf_8_message);
  yage::util::free_string(utf_8_title);
  yage::util::free_string(utf_8_message);

  std::string str;
  bool ret = dlg.show(str);
  if (!ret) return -1;

  char *ansi_str = yage::util::utf_8_to_ansi(str.c_str());

  int ret_val;
#if defined(_MSC_VER) && _MSC_VER < 1800
  /*
   * The struct of stack till now:
   *
   *     [High]
   *     [Caller's ebp]
   *     caller_local_vars
   *     ...(size unknown)
   *     format
   *     message
   *     title
   *     return_address(by `call xxx`)
   *     saved_caller_ebp(by `push ebp`)
   *     local_vars
   *     [esp]
   *     [Low]
   *
   * To emulate `vsscanf` by `sscanf`, we need to know every variables pushed
   * on stack. The push sequence for cdecl is reversed: ..., format, message,
   * title, from highest address to lowest address.
   *
   * The definition of `sscanf` is:
   *     int sscanf(const char *string, const char *format, ...);
   *
   * We will form a stack to call `sscanf`:
   *
   *     [High]
   *     [Caller's ebp]                 -> args_high
   *     caller_local_vars
   *     ...(size unknown)
   *     format
   *     message                        -> args_low
   *     title
   *     return_address(by `call xxx`)
   *     saved_caller_ebp(by `push ebp`)
   *     local_vars
   *     copied_caller_local_vars       copied but not used
   *     copied_...                     \
   *     copied_format                  | used by sscanf
   *     string                         /
   *     [Low]
   */

  message = ansi_str;
  char *args_low = (char *)&message;
  char *args_high = (char *)*((void **)&title - 2);
  size_t args_size = args_high - args_low;

  char *orign_esp;
  _asm {
    mov orign_esp, esp      // backup original ebp
    sub esp, args_size      // allocate space for storing new stack
  }

  // copy stack
  memcpy(orign_esp - args_size, args_low, args_size);

  _asm {
    call dword ptr [sscanf]
    mov ret_val, eax        // save return value
    mov esp, orign_esp      // restore esp
  }

#else
  va_list args;
  va_start(args, format);
  ret_val = vsscanf(ansi_str, format, args);
  va_end(args);
#endif
  return ret_val;
}

int yage_random_interval(int begin, int end) {
  return (rand() % (end-begin)) +begin;
}

double yage_random_double() {
  return 1.0 * rand() / RAND_MAX;
}

uint32_t yage_timer_now() {
  return util::time::get_timestamp();
}

struct yage_timer *yage_timer_create(double seconds) {
  return reinterpret_cast<struct yage_timer *>(new yage::window::Timer(seconds));
}

void yage_timer_pause(struct yage_timer *timer) {
  reinterpret_cast<yage::window::Timer *>(timer)->pause();
}

void yage_timer_resume(struct yage_timer *timer) {
  reinterpret_cast<yage::window::Timer *>(timer)->resume();
}

void yage_timer_stop(struct yage_timer *timer) {
  reinterpret_cast<yage::window::Timer *>(timer)->stop();
}

double yage_timer_get_time_elapsed(struct yage_timer *timer) {
  return reinterpret_cast<yage::window::Timer *>(timer)->get_time_elapsed();
}

void yage_timer_delete(struct yage_timer *timer) {
  delete reinterpret_cast<yage::window::Timer *>(timer);
}

}  // extern "C"
