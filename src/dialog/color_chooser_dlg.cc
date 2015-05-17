#include "color_chooser_dlg.h"
#include "../yage.h"

namespace yage {
namespace dialog {

ColorChooserDlg::ColorChooserDlg(const char *title)
{
  runner_call(exec_create, this, const_cast<char *>(title), nullptr);
}

ColorChooserDlg::ColorChooserDlg(const char *title, Window &window)
{
  runner_call(exec_create, this, const_cast<char *>(title),
              window.pro_get_gtk_window());
}

ColorChooserDlg::~ColorChooserDlg()
{
  runner_call(exec_destroy, this);
}

void ColorChooserDlg::exec_destroy(ColorChooserDlg *this_)
{
  if (this_->gtk_dialog_) {
    gtk_widget_destroy(GTK_WIDGET(this_->gtk_dialog_));
    this_->gtk_dialog_ = nullptr;
  }
}

void ColorChooserDlg::exec_create(ColorChooserDlg *this_,
                                      const char *title,
                                      GtkWindow *parent)
{
  this_->gtk_dialog_ = GTK_COLOR_CHOOSER(
      gtk_color_chooser_dialog_new(title, parent));
  gtk_color_chooser_set_use_alpha(this_->gtk_dialog_, TRUE);
}

void ColorChooserDlg::exec_show(ColorChooserDlg *this_,
                                yage::draw::Color &yage_color,
                                bool &ret)
{
  gint run_ret = gtk_dialog_run(GTK_DIALOG(this_->gtk_dialog_));
  if (run_ret == GTK_RESPONSE_OK) {
    ret = true;

    GdkRGBA gdk_color;
    gtk_color_chooser_get_rgba(this_->gtk_dialog_, &gdk_color);
    yage_color.set_color(gdk_color.red, gdk_color.green, gdk_color.blue, gdk_color.alpha);
  } else {
    ret = false;
  }
}

bool ColorChooserDlg::show(yage::draw::Color &color)
{
  bool ret;
  runner_call(exec_show, this, &color, &ret);
  return ret;
}

}  // namespace dialog
}  // namespace yage
