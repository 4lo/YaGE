#include "input_dlg.h"
#include "../yage.h"

namespace yage {
namespace dialog {

gboolean InputDlg::msg_entry_on_key(GtkWidget *widget, GdkEvent *event, InputDlg *source)
{
  if (event->key.hardware_keycode == 36 ||
      event->key.hardware_keycode == 76) {
    // Enter pressed
    gtk_dialog_response(source->gtk_dialog_, GTK_RESPONSE_OK);
    return true;
  }
  return false;
}

void InputDlg::exec_destroy(InputDlg *this_)
{
  if (this_->gtk_dialog_) gtk_widget_destroy(GTK_WIDGET(this_->gtk_dialog_));
}

void InputDlg::exec_set_message(InputDlg *this_, const char *text)
{
  gtk_label_set_markup(this_->gtk_label_, text);
}

void InputDlg::exec_create(InputDlg *this_,
                           const char *title,
                           GtkWindow *parent)
{
  this_->gtk_dialog_ = GTK_DIALOG(gtk_dialog_new_with_buttons(
      title, parent, GTK_DIALOG_MODAL,
      "_OK", GTK_RESPONSE_OK,
      "_Cancel", GTK_RESPONSE_CANCEL,
      nullptr));

  gtk_dialog_set_default_response(this_->gtk_dialog_, GTK_RESPONSE_OK);

  this_->gtk_entry_ = GTK_ENTRY(gtk_entry_new());
  this_->gtk_label_ = GTK_LABEL(gtk_label_new(nullptr));

  g_signal_connect(this_->gtk_entry_, "key-press-event",
                   G_CALLBACK(msg_entry_on_key), this_);

  GtkContainer *container =
      GTK_CONTAINER(gtk_dialog_get_content_area(this_->gtk_dialog_));

  gtk_container_set_border_width(container, 10);
  gtk_container_add(container, GTK_WIDGET(this_->gtk_label_));
  gtk_container_add(container, GTK_WIDGET(this_->gtk_entry_));
  gtk_widget_show_all(GTK_WIDGET(container));
}

void InputDlg::exec_show(InputDlg *this_, std::string &yage_str, bool &ret)
{
  gint run_ret = gtk_dialog_run(GTK_DIALOG(this_->gtk_dialog_));
  if (run_ret == GTK_RESPONSE_OK) {
    const char *gtk_str = gtk_entry_get_text(this_->gtk_entry_);
    yage_str = gtk_str;
    ret = true;
  } else {
    ret = false;
  }
}

bool InputDlg::show(std::string &str)
{
  bool ret;
  runner_call(exec_show, this, &str, &ret);
  return ret;
}

void InputDlg::set_message(const char *text) {
  runner_call(exec_set_message, this, const_cast<char *>(text));
}

InputDlg::InputDlg(const char *title)
{
  runner_call(exec_create, this, const_cast<char *>(title), nullptr);
}

InputDlg::InputDlg(const char *title, Window &window)
{
  runner_call(exec_create, this, const_cast<char *>(title),
              window.pro_get_gtk_window());
}

InputDlg::~InputDlg()
{
  runner_call(exec_destroy, this);
}

}  // namespace dialog
}  // namespace yage