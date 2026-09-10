#pragma once

#include "help.h"
#include "machine.h"
#include "timing.h"
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Text_Editor.H>

namespace vm8 {
enum class Action {
  New, Open, Save, SaveAs, Export, Quit, Find, Assemble, Run, Pause,
  Step, Reset, Breakpoint, ClearBreakpoints, Demo, Popcount, Help, Apply, ApplyClock, ClearLog
};

class App : public Fl_Double_Window {
public:
  App();
  ~App() override;
  int handle(int event) override;
  void resize(int x, int y, int width, int height) override;
  void act(Action action);
  int smoke_test(const std::string &directory);

private:
  Machine machine_;
  HelpWindow help_;
  ExecutionTimer execution_timer_;
  std::uint32_t clock_hz_ = DEFAULT_CLOCK_HZ;
  Fl_Text_Buffer source_, styles_, log_, state_, timing_, memory_, binary_;
  Fl_Menu_Bar *menu_;
  Fl_Box *brand_, *subtitle_, *editor_title_, *cpu_title_, *log_title_, *status_, *active_clock_;
  Fl_Text_Editor *editor_;
  Fl_Text_Display *log_display_, *state_display_, *timing_display_, *memory_display_, *binary_display_;
  Fl_Input *input_, *clock_input_;
  Fl_Check_Button *trace_;
  Fl_Choice *speed_;
  Fl_Button *apply_, *apply_clock_, *clear_log_;
  Fl_Tabs *tabs_;
  Fl_Group *memory_page_, *binary_page_;
  std::array<Fl_Button *, 7> controls_{};
  bool loading_ = false, dirty_ = false, running_ = false;
  bool breakpoint_stop_ = false, skip_breakpoint_ = false;
  unsigned run_count_ = 0;
  size_t error_line_ = 0;
  std::string path_, status_text_;

  void layout();
  void refresh();
  void color_source();
  void changed();
  void set_status(const std::string &text);
  void append_log(const std::string &text);
  void set_source(const std::string &text, const std::string &path, bool modified);
  bool get_input(uint8_t &value);
  void apply_clock();
  bool assemble();
  bool ensure_assembled();
  bool save(bool choose_path);
  bool save_to(const std::string &path);
  bool read_from(const std::string &path);
  bool may_replace();
  void open();
  void export_binary();
  void run();
  void pause(const std::string &reason);
  void tick();
  void step();
  void toggle_breakpoint();
  void reveal_line(size_t line);
  static void timer(void *context);
  static void action_callback(Fl_Widget *widget, void *data);
};

void configure_theme();
}
